#include "weather_sensor_driver.h"
#include "config_drone.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <math.h>
#include "weather_sensor_driver.h"

bool init_environmental_sensors() { return true; }
environmental_data_t read_environmental_data() {
    return {24.5, 65.0, 1013.25, 760.0, true};
}

static const char *TAG = "BME280_DRIVER";

#define BME280_ADDRESS      0x76 // Endereço I2C padrão (pode ser 0x77 dependendo do jumper)
#define BME280_CHIP_ID_REG  0xD0
#define BME280_CHIP_ID      0x60

// Registradores de dados
#define BME280_PRESS_MSB    0xF7
#define BME280_TEMP_MSB     0xFA
#define BME280_HUM_MSB      0xFD

// Estrutura para armazenar os coeficientes de calibração do sensor
typedef struct {
    uint16_t dig_T1; int16_t dig_T2; int16_t dig_T3;
    uint16_t dig_P1; int16_t dig_P2; int16_t dig_P3; int16_t dig_P4;
    int16_t dig_P5; int16_t dig_P6; int16_t dig_P7; int16_t dig_P8; int16_t dig_P9;
    uint8_t  dig_H1; int16_t dig_H2; uint8_t  dig_H3; int16_t dig_H4;
    int16_t dig_H5; int8_t  dig_H6;
} bme280_calib_data_t;

static bme280_calib_data_t calib_data;
static int32_t t_fine = 0; // Variável global necessária para compensação de pressão/umidade
static bool is_initialized = false;

// --- Funções Auxiliares de I2C ---
static esp_err_t bme280_write_reg(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME280_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t bme280_read_regs(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME280_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME280_ADDRESS << 1) | I2C_MASTER_READ, true);
    if (len > 1) i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

// --- Fórmulas de Compensação da Bosch ---
static int32_t compensate_T(int32_t adc_T) {
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * ((int32_t)calib_data.dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) * ((int32_t)calib_data.dig_T3)) >> 14;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8;
}

static uint32_t compensate_P(int32_t adc_P) {
    int32_t var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    int32_t var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)calib_data.dig_P6);
    var2 = var2 + ((var1 * ((int32_t)calib_data.dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)calib_data.dig_P4) << 16);
    var1 = (((calib_data.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)calib_data.dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)calib_data.dig_P1)) >> 15);
    if (var1 == 0) return 0;
    uint32_t p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
    if (p < 0x80000000) p = (p << 1) / ((uint32_t)var1);
    else p = (p / (uint32_t)var1) * 2;
    var1 = (((int32_t)calib_data.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)calib_data.dig_P8)) >> 13;
    return (uint32_t)((int32_t)p + ((var1 + var2 + calib_data.dig_P7) >> 4));
}

static uint32_t compensate_H(int32_t adc_H) {
    int32_t v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib_data.dig_H4) << 20) - (((int32_t)calib_data.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)calib_data.dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)calib_data.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)calib_data.dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calib_data.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint32_t)(v_x1_u32r >> 12);
}

// --- Funções Públicas ---
bool init_environmental_sensors() {
    // 1. Inicializar I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    // 2. Verificar Chip ID
    uint8_t chip_id = 0;
    if (bme280_read_regs(BME280_CHIP_ID_REG, &chip_id, 1) != ESP_OK || chip_id != BME280_CHIP_ID) {
        ESP_LOGE(TAG, "Falha ao detectar BME280. Chip ID: 0x%02X", chip_id);
        return false;
    }
    ESP_LOGI(TAG, "BME280 detectado com sucesso (ID: 0x%02X)", chip_id);

    // 3. Ler dados de calibração (Registradores 0x88 a 0xA1 e 0xE1 a 0xE7)
    uint8_t calib_data_24[24], calib_data_7[7];
    bme280_read_regs(0x88, calib_data_24, 24);
    bme280_read_regs(0xE1, calib_data_7, 7);

    calib_data.dig_T1 = (calib_data_24[1] << 8) | calib_data_24[0];
    calib_data.dig_T2 = (calib_data_24[3] << 8) | calib_data_24[2];
    calib_data.dig_T3 = (calib_data_24[5] << 8) | calib_data_24[4];
    calib_data.dig_P1 = (calib_data_24[7] << 8) | calib_data_24[6];
    calib_data.dig_P2 = (calib_data_24[9] << 8) | calib_data_24[8];
    calib_data.dig_P3 = (calib_data_24[11] << 8) | calib_data_24[10];
    calib_data.dig_P4 = (calib_data_24[13] << 8) | calib_data_24[12];
    calib_data.dig_P5 = (calib_data_24[15] << 8) | calib_data_24[14];
    calib_data.dig_P6 = (calib_data_24[17] << 8) | calib_data_24[16];
    calib_data.dig_P7 = (calib_data_24[19] << 8) | calib_data_24[18];
    calib_data.dig_P8 = (calib_data_24[21] << 8) | calib_data_24[20];
    calib_data.dig_P9 = (calib_data_24[23] << 8) | calib_data_24[22];
    calib_data.dig_H1 = calib_data_24[25];
    calib_data.dig_H2 = (calib_data_7[1] << 8) | calib_data_7[0];
    calib_data.dig_H3 = calib_data_7[2];
    calib_data.dig_H4 = (calib_data_7[3] << 4) | (calib_data_7[4] & 0x0F);
    calib_data.dig_H5 = ((calib_data_7[4] >> 4) & 0x0F) | (calib_data_7[5] << 4);
    calib_data.dig_H6 = (int8_t)calib_data_7[6];

    // 4. Configurar sensor: Oversampling x1 para T, P, H e Modo Normal
    bme280_write_reg(0xF2, 0x01); // ctrl_hum (osrs_h = 1)
    bme280_write_reg(0xF4, 0x27); // ctrl_meas (osrs_t = 1, osrs_p = 1, mode = normal)
    bme280_write_reg(0xF5, 0x00); // config (filter off, standby 0.5ms)

    is_initialized = true;
    ESP_LOGI(TAG, "BME280 inicializado e configurado.");
    return true;
}

environmental_data_t read_environmental_data() {
    environmental_data_t data = {0};
    data.is_valid = false;

    if (!is_initialized) return data;

    uint8_t raw_data[8];
    // Lê 8 bytes consecutivos: Press (3), Temp (3), Hum (2)
    if (bme280_read_regs(BME280_PRESS_MSB, raw_data, 8) != ESP_OK) {
        ESP_LOGE(TAG, "Falha na leitura dos registradores do BME280");
        return data;
    }

    // Montar valores brutos (ADC)
    int32_t adc_P = (raw_data[0] << 12) | (raw_data[1] << 4) | (raw_data[2] >> 4);
    int32_t adc_T = (raw_data[3] << 12) | (raw_data[4] << 4) | (raw_data[5] >> 4);
    int32_t adc_H = (raw_data[6] << 8) | raw_data[7];

    // Aplicar compensação
    int32_t temp_raw = compensate_T(adc_T);
    data.temperature_c = temp_raw / 100.0f;

    uint32_t press_raw = compensate_P(adc_P);
    data.pressure_hpa = press_raw / 256.0f;

    uint32_t hum_raw = compensate_H(adc_H);
    data.humidity_pct = hum_raw / 1024.0f;

    // Calcular altitude barométrica (Fórmula internacional do ar padrão)
    // Assumindo pressão ao nível do mar padrão de 1013.25 hPa
    data.altitude_m = 44330.0f * (1.0f - pow(data.pressure_hpa / 1013.25f, 0.1903f));

    data.is_valid = true;
    return data;
}
