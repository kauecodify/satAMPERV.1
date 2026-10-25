#ifndef WEATHER_SENSOR_DRIVER_H
#define WEATHER_SENSOR_DRIVER_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float temperature_c;
    float humidity_pct;
    float pressure_hpa;
    float altitude_m;
    bool is_valid;
} environmental_data_t;

bool init_environmental_sensors();
environmental_data_t read_environmental_data();
#endif
