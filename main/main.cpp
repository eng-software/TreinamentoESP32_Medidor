extern "C" 
{
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
    #include "displaySSD1306.h"
    #include "cSMP3011.h"
}

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
    // SMP3011 Initialization
    //------------------------------------------------    
    SMP3011.init();

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
        SMP3011.poll(); 
        vTaskDelay(100/portTICK_PERIOD_MS);

        lvgl_port_lock(portMAX_DELAY);        
        lv_label_set_text_fmt(lblPressure, "P: %5.1f kPa", SMP3011.getPressure());    
        lv_label_set_text_fmt(lblTemperature, "T: %3.0f oC", SMP3011.getTemperature());    
        lvgl_port_unlock();
    }    
}
