#include "sensor_driver.h"
#include <Arduino.h>

#define SENSOR_POWER_PIN 34  // ADC1_CH6
#define SENSOR_VOLTAGE_PIN 35 // ADC1_CH7
#define SENSOR_CURRENT_PIN 32 // ADC1_CH4
#define SENSOR_TEMP_PIN 33    // ADC1_CH5

static int sampling_rate_ms = 100;
static float calibration_factor = 1.0;
static bool sensor_initialized = false;

void init_sensor() {
    analogReadResolution(12); // 12-bit resolution (0-4095)
    analogSetAttenuation(ADC_11db); // 0-3.3V range
    
    // Calibrar sensor
    calibrate_sensor();
    
    sensor_initialized = true;
    Serial.println("Sensor inicializado");
}

// Ler dados do sensor
sensor_data_t read_sensor_data() {
    sensor_data_t data;
    
    if (!sensor_initialized) {
        data.power = 0;
        data.voltage = 0;
        data.current = 0;
        data.temperature = 0;
        data.status = SENSOR_ERROR;
        return data;
    }
    
    // Ler valores brutos do ADC
    int raw_power = analogRead(SENSOR_POWER_PIN);
    int raw_voltage = analogRead(SENSOR_VOLTAGE_PIN);
    int raw_current = analogRead(SENSOR_CURRENT_PIN);
    int raw_temp = analogRead(SENSOR_TEMP_PIN);
    
    data.power = (raw_power / 4095.0) * 100.0 * calibration_factor; // 0-100 kW
    data.voltage = (raw_voltage / 4095.0) * 400.0; // 0-400 V
    data.current = (raw_current / 4095.0) * 50.0; // 0-50 A
    data.temperature = (raw_temp / 4095.0) * 100.0 - 20.0; // -20 a 80 °C
    
    // Aplicar processamento Assembly para filtragem
    data.power = assembly_filter(data.power);
    data.voltage = assembly_filter(data.voltage);
    data.current = assembly_filter(data.current);
    data.temperature = assembly_filter(data.temperature);
    
    // Determinar status
    if (data.power > 90.0 || data.temperature > 70.0) {
        data.status = SENSOR_WARNING;
    } else {
        data.status = SENSOR_OK;
    }
    
    return data;
}

// Definir taxa de amostragem
void set_sensor_sampling_rate(int rate_ms) {
    if (rate_ms >= 50 && rate_ms <= 10000) {
        sampling_rate_ms = rate_ms;
    }
}

// Obter taxa de amostragem atual
int get_sensor_sampling_rate() {
    return sampling_rate_ms;
}

// Calibrar sensor
void calibrate_sensor() {
    // Ler valor de referência (sem carga)
    int baseline = analogRead(SENSOR_POWER_PIN);
    
    // fator de calibração
    calibration_factor = 1.0 / ((baseline / 4095.0) * 100.0);
    
    Serial.printf("Sensor calibrado. Fator: %.4f\n", calibration_factor);
}

// Obter fator de calibração
float get_calibration_factor() {
    return calibration_factor;
}
