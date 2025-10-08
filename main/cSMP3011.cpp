#include "cSMP3011.h"

extern "C" 
{
    #include "driver/i2c_master.h"
    #include <stdio.h>
    #include "esp_err.h"
    #include "esp_log.h"
}

#define I2C_SENSOR_BUS_PORT     1
#define I2C_SENSOR_SDA          gpio_num_t::GPIO_NUM_33
#define I2C_SENSOR_SCL          gpio_num_t::GPIO_NUM_32


//I2C Port Configuration for sensor

//I2C Bus Handler and Configuration for sensor
i2c_master_bus_handle_t i2c_sensor_bus = NULL;    
i2c_master_bus_config_t sensor_bus_config = 
{
    .i2c_port = I2C_SENSOR_BUS_PORT,    
    .sda_io_num = I2C_SENSOR_SDA,
    .scl_io_num = I2C_SENSOR_SCL,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,    
    .flags
    {
        .enable_internal_pullup = true,
    }
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


cSMP3011::cSMP3011()
{
    pressure = 0;
    temperature = 0;
}

cSMP3011::~cSMP3011()
{

}

void cSMP3011::init()
{
    ESP_ERROR_CHECK(i2c_new_master_bus(&sensor_bus_config, &i2c_sensor_bus));
    i2c_master_bus_add_device(i2c_sensor_bus, &i2c_smp3011_config, &i2c_smp3011_handle);    
    
    uint8_t PressSensorCommand = 0xAC;  //Comando para iniciar conversor ADC
    i2c_master_transmit(i2c_smp3011_handle, (uint8_t *)(&PressSensorCommand), 1, 20); 
}

void cSMP3011::poll()
{
    uint8_t PressSensorBuffer[6];
    if( i2c_master_receive(i2c_smp3011_handle, (uint8_t *)(&PressSensorBuffer), sizeof(PressSensorBuffer), 20) != ESP_OK )
    {
        return;
    }

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

float cSMP3011::getPressure()
{
    return pressure;
}

float cSMP3011::getTemperature()
{
    return temperature;
}