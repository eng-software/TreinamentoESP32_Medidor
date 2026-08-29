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
    #include "wifi_http.h"
    #include "http_server.h"
    #include "esp_vfs_fat.h"
    #include "driver/sdmmc_host.h"
    #include "sdmmc_cmd.h"
    #include "esp_spiffs.h"  
}

#include "cSMP3011.h"
#include "CGlobalResources.h"

#define BAR     0.01f
#define PSI     0.145038f
#define LED_PIN gpio_num_t::GPIO_NUM_16  

//-------------------
// PROTOTYPES
//-------------------
void sensorSMP3011Task(void *pvParameters);
void statusLedTask(void *pvParameters);
void init_filesystem(void);

//-------------------
// VARIABLES
//-------------------
cSMP3011 SMP3011;
SemaphoreHandle_t sensorMutex;

/**
 * @brief Entry point
 */
extern "C" void app_main() 
{
    //------------------------------------------------
    // I2C + Sensor
    //------------------------------------------------    
    I2C.init();
    SMP3011.init();
    
    sensorMutex = xSemaphoreCreateMutex();
    xTaskCreate(sensorSMP3011Task, "sensorSMP3011Task", 4096, NULL, 1, NULL);

    //------------------------------------------------
    // LVGL Display
    //------------------------------------------------
    displayInit();
    
    //------------------------------------------------
    // Wi-Fi Connection
    //------------------------------------------------
    display_loading_start();
    bool wifi_ok = wifi_connect(10000); // timeout 10 segundos
    display_loading_stop();

    if (wifi_ok) {
        ESP_LOGI("MAIN", "✅ Wi-Fi conectado com sucesso!");
        init_filesystem();
        start_webserver(); // servidor HTTP ativo
    } else {
        ESP_LOGE("MAIN", "❌ Falha na conexão Wi-Fi.");
    }

    //------------------------------------------------
    // Labels no display
    //------------------------------------------------
    lvgl_port_lock(portMAX_DELAY);
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);
        
    lv_obj_t *lblPressureBAR = lv_label_create(scr);    
    lv_label_set_text_fmt(lblPressureBAR, "Bar: %5.1f", SMP3011.getPressure() * BAR);    
    lv_obj_set_width(lblPressureBAR, LCD_H_RES);
    lv_obj_align(lblPressureBAR, LV_ALIGN_TOP_MID, 0, 0);    
    lv_obj_set_y(lblPressureBAR, 0);

    lv_obj_t *lblPressurePSI = lv_label_create(scr);    
    lv_label_set_text_fmt(lblPressurePSI, "PSI: %5.1f", SMP3011.getPressure() * PSI);    
    lv_obj_set_width(lblPressurePSI, LCD_H_RES);
    lv_obj_align(lblPressurePSI, LV_ALIGN_TOP_MID, 0, 0);    
    lv_obj_set_y(lblPressurePSI, 16);

    lv_obj_t *lblTemperature = lv_label_create(scr);    
    lv_label_set_text_fmt(lblTemperature, "T: %3.0f", SMP3011.getTemperature());    
    lv_obj_set_width(lblTemperature, LCD_H_RES);
    lv_obj_align(lblTemperature, LV_ALIGN_TOP_MID, 0, 0);    
    lv_obj_set_y(lblTemperature, 32);
    lvgl_port_unlock();

    //------------------------------------------------
    // Loop principal
    //------------------------------------------------
    while (1)
    {
        lvgl_port_lock(portMAX_DELAY);        
        lv_label_set_text_fmt(lblPressureBAR, "BAR: %5.1f", SMP3011.getPressure() * BAR);   
        lv_label_set_text_fmt(lblPressurePSI, "PSI: %5.1f", SMP3011.getPressure() * PSI);  
        lv_label_set_text_fmt(lblTemperature, "T: %3.0f °C", SMP3011.getTemperature());     
        lvgl_port_unlock();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }    
}

//------------------------------------------------
// Filesystem Init
//------------------------------------------------
void init_filesystem(void)
{
    ESP_LOGI("FS", "Inicializando SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = "spiffs",
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL)
            ESP_LOGE("FS", "Falha ao montar/formatar SPIFFS");
        else if (ret == ESP_ERR_NOT_FOUND)
            ESP_LOGE("FS", "Partição SPIFFS não encontrada");
        else
            ESP_LOGE("FS", "Erro SPIFFS: %s", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK)
        ESP_LOGI("FS", "SPIFFS: total=%d, usado=%d", total, used);
    else
        ESP_LOGE("FS", "Erro ao obter info SPIFFS (%s)", esp_err_to_name(ret));
}

//------------------------------------------------
// Tasks
//------------------------------------------------
void sensorSMP3011Task(void *pvParameters) 
{
    while (1)
    {
        SMP3011.poll(); 
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
