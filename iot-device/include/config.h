#ifndef CONFIG_H
#define CONFIG_H

// Configurações de rede
#define CONFIG_WIFI_SSID "SuaRedeWiFi"
#define CONFIG_WIFI_PASSWORD "SuaSenhaWiFi"

// Configurações MQTT
#define CONFIG_MQTT_BROKER "192.168.1.100"  // IP do seu servidor
#define CONFIG_MQTT_PORT 1883
#define CONFIG_MQTT_USER "device_user"
#define CONFIG_MQTT_PASS "device_password"

// Identificação do dispositivo
#define CONFIG_DEVICE_ID "DEV-AM-001"

// Chave secreta para HMAC (deve ser a mesma no servidor)
#define CONFIG_HMAC_SECRET "sua-chave-secreta"

// Configurações de hardware
#define CONFIG_SAMPLING_RATE_MS 2000
#define CONFIG_TELEMETRY_INTERVAL_MS 2000
#define CONFIG_STATUS_INTERVAL_MS 30000

#endif // CONFIG_H
