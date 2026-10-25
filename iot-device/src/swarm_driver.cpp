#include "swarm_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <cstdio>

static const char* TAG = "SwarmM138";

SwarmDriver::SwarmDriver(uart_port_t uart_num, int tx_pin, int rx_pin)
    : _uart_num(uart_num), _tx_pin(tx_pin), _rx_pin(rx_pin) {}

esp_err_t SwarmDriver::begin() {
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_RETURN_ON_ERROR(uart_param_config(_uart_num, &cfg), TAG, "uart_param_config");
    ESP_RETURN_ON_ERROR(uart_set_pin(_uart_num, _tx_pin, _rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG, "uart_set_pin");
    ESP_RETURN_ON_ERROR(uart_driver_install(_uart_num, 1024, 1024, 0, NULL, 0), TAG, "uart_driver_install");

    vTaskDelay(pdMS_TO_TICKS(200));
    return sendCommand("$CS*10\r", "OK", 3000);  // Check status
}

esp_err_t SwarmDriver::sendPayload(const uint8_t* data, size_t len) {
    if (len == 0 || len > 160) {
        ESP_LOGE(TAG, "Payload inválido: %d bytes (limite: 160)", (int)len);
        return ESP_ERR_INVALID_SIZE;
    }

    // Swarm espera hex-ascii: cada byte vira 2 caracteres[reference:3]
    char hex[321];
    bytesToHex(data, len, hex);

    char cmd[340];
    snprintf(cmd, sizeof(cmd), "$TD %s\r", hex);

    esp_err_t err = sendCommand(cmd, "$TD OK", 10000);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Payload CBOR enfileirado (%d bytes / %d hex chars)", (int)len, (int)strlen(hex));
    } else {
        ESP_LOGE(TAG, "Falha ao enfileirar payload no Swarm");
    }
    return err;
}

int SwarmDriver::getQueuedMessageCount() {
    char resp[64];
    if (sendCommand("$MT C=U\r", "OK", 2000) != ESP_OK) return -1;
    if (readResponse(resp, sizeof(resp), 2000) != ESP_OK) return -1;

    // Resposta típica: "$MT C=U,<count>*xx"
    char* p = strstr(resp, "C=U,");
    if (!p) return -1;
    return atoi(p + 4);
}

esp_err_t SwarmDriver::sleep() {
    return sendCommand("$SL\r", "OK", 2000);
}

esp_err_t SwarmDriver::wake() {
    // Envia caractere de wake e aguarda boot
    uart_write_bytes(_uart_num, "\r", 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    return sendCommand("$CS*10\r", "OK", 5000);
}

// ---------- helpers privados ----------

esp_err_t SwarmDriver::sendCommand(const char* cmd, const char* expected, uint32_t timeout_ms) {
    uart_flush(_uart_num);
    uart_write_bytes(_uart_num, cmd, strlen(cmd));

    char resp[512] = {0};
    esp_err_t err = readResponse(resp, sizeof(resp), timeout_ms);
    if (err != ESP_OK) return err;

    if (strstr(resp, expected) || strstr(resp, "OK")) return ESP_OK;
    if (strstr(resp, "ERR")) {
        ESP_LOGE(TAG, "Swarm retornou ERR: %s", resp);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t SwarmDriver::readResponse(char* buf, size_t max_len, uint32_t timeout_ms) {
    size_t total = 0;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (xTaskGetTickCount() < deadline && total < max_len - 1) {
        uint8_t tmp[64];
        int n = uart_read_bytes(_uart_num, tmp, sizeof(tmp), pdMS_TO_TICKS(100));
        if (n > 0) {
            memcpy(buf + total, tmp, n);
            total += n;
            buf[total] = '\0';
            if (strstr(buf, "\n")) break;  // Swarm termina respostas com \n
        }
    }
    return (total > 0) ? ESP_OK : ESP_ERR_TIMEOUT;
}

void SwarmDriver::bytesToHex(const uint8_t* in, size_t len, char* out) {
    static const char hexchars[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[i * 2]     = hexchars[(in[i] >> 4) & 0x0F];
        out[i * 2 + 1] = hexchars[in[i] & 0x0F];
    }
    out[len * 2] = '\0';
}
