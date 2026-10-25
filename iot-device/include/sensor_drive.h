#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

// Status do sensor
typedef enum {
    SENSOR_OK = 0,
    SENSOR_WARNING = 1,
    SENSOR_ERROR = 2
} sensor_status_t;

// Estrutura de dados do sensor
typedef struct {
    float power;        // kW
    float voltage;      // V
    float current;      // A
    float temperature;  // °C
    sensor_status_t status;
} sensor_data_t;

void init_sensor();
sensor_data_t read_sensor_data();
void set_sensor_sampling_rate(int rate_ms);
int get_sensor_sampling_rate();
void calibrate_sensor();
float get_calibration_factor();

// Função Assembly (assembly_opt.asm)
float assembly_filter(float input);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_DRIVER_H
