extern "C" 
{
    #include "driver/i2c_master.h"
    #include "driver/gpio.h"
    #include <stdio.h>
    #include "esp_err.h"
    #include "esp_log.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"    
    #include "esp_lvgl_port.h"
    #include "lvgl.h"
    #include "displaySSD1306.h"    
}

#include "cSMP3011.h"
#include "CGlobalResources.h"

#define LED_PIN     gpio_num_t::GPIO_NUM_16  

/*
    PROTOTYPES
*/
void sensorTask(void *pvParameters);
void statusLedTask(void *pvParameters);

/*
    VARIABLES
*/
cSMP3011    SMP3011;

/**
 * @brief Entry point of the application.
 *
 * This function configures the I2C master mode and scans the bus for devices.
 * The bus is configured to use GPIO 5 for SDA and GPIO 4 for SCL, and the
 * clock speed is set to 100000 Hz. The scan starts from address 1 and goes
 * to address 126 (inclusive). If a device is found at an address, a message
 * is printed to the console with the address of the device.
 */
extern "C" void app_main() 
{
    //------------------------------------------------
    // Status LED
    //------------------------------------------------
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    xTaskCreate(statusLedTask, "statusLedTask", 4096, NULL, 1, NULL);

    //------------------------------------------------
    // I2C Initialization
    //------------------------------------------------    
    I2C.init();

    //------------------------------------------------
    // SMP3011 Initialization
    //------------------------------------------------ 
    SMP3011.init();
    xTaskCreate(sensorTask, "sensorTask", 4096, NULL, 1, NULL);

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
    lv_label_set_text_fmt(lblPressure, "P: %5.1f", SMP3011.getPressure());    
    lv_obj_set_width(lblPressure, LCD_H_RES);
    lv_obj_align(lblPressure, LV_ALIGN_TOP_MID, 0, 0);    
    lv_obj_set_y(lblPressure, 0);

    lv_obj_t *lblTemperature = lv_label_create(scr);    
    lv_label_set_text_fmt(lblTemperature, "T: %3.0f", SMP3011.getTemperature());    
    lv_obj_set_width(lblTemperature, LCD_H_RES);
    lv_obj_align(lblTemperature, LV_ALIGN_TOP_MID, 0, 0);    
    lv_obj_set_y(lblTemperature, 16);

    lvgl_port_unlock();

    while(1)
    {
        lvgl_port_lock(portMAX_DELAY);        
        lv_label_set_text_fmt(lblPressure, "P: %5.1f kPa", SMP3011.getPressure());    
        lv_label_set_text_fmt(lblTemperature, "T: %3.0f oC", SMP3011.getTemperature());    
        lvgl_port_unlock();
        vTaskDelay(100/portTICK_PERIOD_MS);
    }    
}

void sensorTask(void *pvParameters) 
{
    while(1)
    {
        SMP3011.poll(); 
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void statusLedTask(void *pvParameters) 
{
    while(1)
    {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(250/portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(250/portTICK_PERIOD_MS);
    }
}   