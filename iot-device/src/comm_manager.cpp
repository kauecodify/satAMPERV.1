#include "comm_manager.h"
#include "payload_codec.h"
#include "swarm_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "CommMgr";

// Instância global do Swarm (UART1, GPIO 17/16 — ajusta no hardware)
static SwarmDriver swarm(UART_NUM_1, 17, 16);

void CommManager::begin() {
    esp_err_t err = swarm.begin();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Swarm M138 não inicializado (Wi-Fi será o único link).");
    }
}

void CommManager::publishTelemetry(const SensorData& s) {
    // 1) Preenche estrutura
    PayloadCodec::Telemetry t;
    t.voltage         = s.voltage;
    t.current         = s.current;
    t.temperature     = s.temperature;
    t.timestamp       = (uint32_t)(esp_log_timestamp() / 1000);
    t.governance_vote = s.governance_vote;

    // 2) Codifica em CBOR (payload compacto)
    uint8_t payload[128];
    size_t len = PayloadCodec::encode(t, payload, sizeof(payload));
    if (len == 0) {
        ESP_LOGE(TAG, "Falha na codificação CBOR");
        return;
    }

    // 3) (Opcional) Assinatura HMAC-SHA256 já existente no crypto_engine.cpp
    // crypto_hmac_sha256(payload, len, key, signature);

    // 4) Roteamento de rede
    if (wifiIsConnected()) {
        mqttPublishBinary("telemetry/device", payload, len);   // Wi-Fi: custo ~zero
        ESP_LOGI(TAG, "Enviado via Wi-Fi/MQTT (%d bytes CBOR)", (int)len);
    } else {
        ESP_LOGW(TAG, "Wi-Fi indisponível. Fallback satelital (Swarm M138)...");

        if (len > 160) {
            ESP_LOGE(TAG, "Payload %d > 160 bytes: impossível enviar via Swarm", (int)len);
            return;
        }

        esp_err_t err = swarm.sendPayload(payload, len);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Payload CBOR enfileirado no Swarm (%d bytes)", (int)len);
        } else {
            ESP_LOGE(TAG, "Falha no envio satelital");
        }
    }
}
