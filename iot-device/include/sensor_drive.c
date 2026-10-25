#include "sensor_driver.h"
#include <stdio.h>

void init_sensor() {
    printf("[C] Sensor inicializado.\n");
}

sensor_data_t read_sensor_data() {
    sensor_data_t data;
    
    // leitura de ADC
    float raw_power = 45.5; 
    
    // otimização em Assembly
    data.power = assembly_filter(raw_power);
    data.voltage = 220.0;
    data.temperature = 35.2;
    data.status = SENSOR_OK;
    
    return data;
}
