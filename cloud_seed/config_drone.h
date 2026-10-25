#ifndef CONFIG_DRONE_H
#define CONFIG_DRONE_H

#define DRONE_ID "DRONE-MONITOR-01"

// Coordenadas da Base (Home) para Return-to-Launch (RTL)
#define HOME_LATITUDE  -23.550520
#define HOME_LONGITUDE -46.633308
#define HOME_ALTITUDE  760.0

// Limites de Geofence (Exemplo: Área autorizada)
#define GEOFENCE_MIN_LAT -23.600000
#define GEOFENCE_MAX_LAT -23.500000
#define GEOFENCE_MIN_LNG -46.700000
#define GEOFENCE_MAX_LNG -46.500000
#define MAX_ALTITUDE_M   120.0  // Limite legal padrão ANAC/DECEA

// Pinos I2C para sensores (Ex: BME280)
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_SCL_IO   22
#define I2C_MASTER_FREQ_HZ  100000

#endif
