#include "comm_manager.h"
#include "payload_codec.h"
#include "crypto.h" // Para o HMAC-SHA256 existente

// Instâncias globais (inicializadas no main.cpp)
SwarmDriver swarm(...); 
// LoraDriver lora(...); // Opcional para fallback terrestre

void CommManager::publishTelemetry(const SensorData& sensor_data) {
    // 1. Preenche a estrutura de telemetria
    PayloadCodec::Telemetry telemetry;
    telemetry.voltage = sensor_data.voltage;
    telemetry.current = sensor_data.current;
    telemetry.temperature = sensor_data.temperature;
    telemetry.timestamp = esp_log_timestamp(); // Ou NTP se Wi-Fi disponível
    
    // 2. Serializa em CBOR (Payload compacto)
    uint8_t binary_payload[128]; // Buffer pequeno, pois CBOR é compacto
    size_t payload_len = PayloadCodec::encodeTelemetry(telemetry, binary_payload, sizeof(binary_payload));
    if (payload_len == 0) return;

    // 3. (Opcional) Assina o payload binário com HMAC-SHA256
    // O código existente já possui crypto_engine.cpp para isso. [5†L8-L9]
    // uint8_t signature[32];
    // crypto_hmac_sha256(binary_payload, payload_len, key, signature);

    // 4. Lógica de Roteamento de Rede
    if (wifiIsConnected()) {
        // Tenta enviar via MQTT (Wi-Fi) - muito mais barato que satélite.
        // O payload CBOR é publicado em vez da string JSON.
        mqttPublishBinary("telemetry/device_id", binary_payload, payload_len);
    } else {
        // Fallback: Wi-Fi indisponível, tenta satélite.
        ESP_LOGW(TAG, "Wi-Fi indisponível. Tentando link satelital...");
        
        // O Swarm M138 aceita mensagens de até 160 bytes [1†L11-L12].
        if (payload_len <= 160) {
            esp_err_t err = swarm.sendPayload(binary_payload, payload_len);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Payload CBOR enviado via satélite (%d bytes).", payload_len);
            } else {
                ESP_LOGE(TAG, "Falha no envio satelital.");
                // Aqui pode entrar o fallback para LoRaWAN (lora.send(...)).
            }
        } else {
            ESP_LOGE(TAG, "Payload muito grande para o limite de 160 bytes do satélite.");
        }
    }
}
