#include "driver/i2c_master.h"
#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

static const char *TAG = "example";


//I2C Port Configuration for display
#define I2C_DISPLAY_BUS_PORT     0
#define I2C_DISPLAY_SDA          5
#define I2C_DISPLAY_SCL          4

//I2C Bus Handler and Configuration for display
i2c_master_bus_handle_t i2c_display_bus = NULL;    
i2c_master_bus_config_t display_bus_config = 
{
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .i2c_port = I2C_DISPLAY_BUS_PORT,
    .sda_io_num = I2C_DISPLAY_SDA,
    .scl_io_num = I2C_DISPLAY_SCL,
    .flags.enable_internal_pullup = true,
};

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_io_i2c_config_t io_config = 
{
    .dev_addr            = 0x3C,
    .scl_speed_hz        = 400000,
    .control_phase_bytes = 1,   // According to SSD1306 datasheet
    .lcd_cmd_bits        = 8,   // According to SSD1306 datasheet
    .lcd_param_bits      = 8,   // According to SSD1306 datasheet
    .dc_bit_offset       = 6,   // According to SSD1306 datasheet
};

// The pixel number in horizontal and vertical
#define LCD_H_RES              128
#define LCD_V_RES              64




//I2C Port Configuration for sensor
#define I2C_SENSOR_BUS_PORT     1
#define I2C_SENSOR_SDA          33
#define I2C_SENSOR_SCL          32

//I2C Bus Handler and Configuration for sensor
i2c_master_bus_handle_t i2c_sensor_bus = NULL;    
i2c_master_bus_config_t sensor_bus_config = 
{
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .i2c_port = I2C_SENSOR_BUS_PORT,
    .sda_io_num = I2C_SENSOR_SDA,
    .scl_io_num = I2C_SENSOR_SCL,
    .flags.enable_internal_pullup = true,
};

//I2C for SMP3011 Pressure Sensor configuration and handler for sensor
i2c_master_dev_handle_t i2c_smp3011_handle = NULL;
i2c_device_config_t i2c_smp3011_config = 
{
    .dev_addr_length  = I2C_ADDR_BIT_LEN_7,         /*!< Select the address length of the slave device. */
    .device_address  = 0x78,                        /*!< I2C device raw address. (The 7/10 bit address without read/write bit) */
    .scl_speed_hz  = 400000,                        /*!< I2C SCL line frequency. */
    .scl_wait_us = 1000000,                         /*!< Timeout value. (unit: us). Please note this value should not be so small that it can handle stretch/disturbance properly. If 0 is set, that means use the default reg value*/
    .flags = 
    {
        .disable_ack_check = 0                       /*!< Disable ACK check. If this is set false, that means ack check is enabled, the transaction will be stopped and API returns error when nack is detected. */
    }
};

/*
    PROTOTYPES
*/
void displayInit();
void smp3011Init();
void smp3011Poll();


/*
    VARIABLES
*/
float pressure = 0;
float temperature = 0;


/**
 * @brief Entry point of the application.
 *
 * This function configures the I2C master mode and scans the bus for devices.
 * The bus is configured to use GPIO 5 for SDA and GPIO 4 for SCL, and the
 * clock speed is set to 100000 Hz. The scan starts from address 1 and goes
 * to address 126 (inclusive). If a device is found at an address, a message
 * is printed to the console with the address of the device.
 */
void app_main() 
{
    //------------------------------------------------
    // I2C Initialization
    //------------------------------------------------
    ESP_LOGI(TAG, "Initialize I2C bus");
    ESP_ERROR_CHECK(i2c_new_master_bus(&sensor_bus_config, &i2c_sensor_bus));
    ESP_ERROR_CHECK(i2c_new_master_bus(&display_bus_config, &i2c_display_bus));
        
    
    //------------------------------------------------
    // SMP3011 Initialization
    //------------------------------------------------    
    smp3011Init();

    //------------------------------------------------
    // LVGL
    //------------------------------------------------
    displayInit();
    
    //------------------------------------------------
    // Create a Label
    //------------------------------------------------
    lvgl_port_lock(portMAX_DELAY);
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);
        
    lv_obj_t *lblPressure = lv_label_create(scr);    
    lv_label_set_text_fmt(lblPressure, "P: %5.1f", pressure);    
    lv_obj_set_width(lblPressure, LCD_H_RES);
    lv_obj_align(lblPressure, LV_ALIGN_TOP_MID, 0, 0);    
    lv_obj_set_y(lblPressure, 0);


    lv_obj_t *lblTemperature = lv_label_create(scr);    
    lv_label_set_text_fmt(lblTemperature, "T: %3.0f", temperature);    
    lv_obj_set_width(lblTemperature, LCD_H_RES);
    lv_obj_align(lblTemperature, LV_ALIGN_TOP_MID, 0, 0);    
    lv_obj_set_y(lblTemperature, 16);

    lvgl_port_unlock();

    while(1)
    {
        smp3011Poll(); 
        vTaskDelay(100/portTICK_PERIOD_MS);

        lvgl_port_lock(portMAX_DELAY);        
        lv_label_set_text_fmt(lblPressure, "P: %5.1f kPa", pressure);    
        lv_label_set_text_fmt(lblTemperature, "T: %3.0f oC", temperature);    
        lvgl_port_unlock();
    }    
}

void smp3011Init()
{
    i2c_master_bus_add_device(i2c_sensor_bus, &i2c_smp3011_config, &i2c_smp3011_handle);    
    
    uint8_t PressSensorCommand = 0xAC;  //Comando para iniciar conversor ADC
    i2c_master_transmit(i2c_smp3011_handle, (uint8_t *)(&PressSensorCommand), 1, 20); 
}

void smp3011Poll()
{
    uint8_t PressSensorBuffer[6];
    i2c_master_receive(i2c_smp3011_handle, (uint8_t *)(&PressSensorBuffer), sizeof(PressSensorBuffer), 20);

    if((PressSensorBuffer[0]&0x20) == 0)   //Bit5 do status está em 0 significa que a conversão está pronta
    {              
        printf("Raw Data: %02X %02X %02X %02X %02X %02X\n", PressSensorBuffer[0], PressSensorBuffer[1], PressSensorBuffer[2], PressSensorBuffer[3], PressSensorBuffer[4], PressSensorBuffer[5]);
        
        uint8_t PressSensorCommand = 0xAC;  //Comando para iniciar conversor ADC
        i2c_master_transmit(i2c_smp3011_handle, (uint8_t *)(&PressSensorCommand), 1, 20);            

        float pressurePercentage = (((uint32_t)PressSensorBuffer[1]<<16)|((uint32_t)PressSensorBuffer[2]<<8)|((uint32_t)PressSensorBuffer[3]));        
        pressurePercentage = (pressurePercentage / 16777215.0f);        
        pressurePercentage -= 0.15f;
        pressurePercentage /= 0.7f;
        pressurePercentage *= 500000.0f;

        float temperaturePercentage = (((uint32_t)PressSensorBuffer[4]<<8)|((uint32_t)PressSensorBuffer[5]));
        temperaturePercentage /= 65535.0f;        
        temperaturePercentage = ((150.0f - (-40.0f))*temperaturePercentage) - 40.0f;

        printf("Pressure: %f  Temperature: %f \n", pressurePercentage, temperaturePercentage);
        
        pressure = pressurePercentage/1000.0f;
        temperature = temperaturePercentage;
    }
}


void displayInit()
{
    //------------------------------------------------
    // SSD1306 Initialization
    //------------------------------------------------
    ESP_LOGI(TAG, "Install panel IO");

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_display_bus, &io_config, &io_handle));
    

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = 
    {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
    };
    esp_lcd_panel_ssd1306_config_t ssd1306_config = 
    {
        .height = LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    //------------------------------------------------
    
    //------------------------------------------------
    // LVGL Initialization
    //------------------------------------------------
    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = 
    {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * LCD_V_RES,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = true,
        .rotation = 
        {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        }
    };
    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);

    /* Rotation of the screen */
    lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);
    //------------------------------------------------
}
