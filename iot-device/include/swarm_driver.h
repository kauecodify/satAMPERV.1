#pragma once
#include "driver/uart.h"
#include "esp_err.h"
#include <cstdint>
#include <cstddef>

class SwarmDriver {
public:
    SwarmDriver(uart_port_t uart_num, int tx_pin, int rx_pin);
    esp_err_t begin();
    esp_err_t sendPayload(const uint8_t* data, size_t len);
    int getQueuedMessageCount();
    esp_err_t sleep();
    esp_err_t wake();

private:
    uart_port_t _uart_num;
    int _tx_pin, _rx_pin;

    esp_err_t sendCommand(const char* cmd, const char* expected, uint32_t timeout_ms);
    esp_err_t readResponse(char* buf, size_t max_len, uint32_t timeout_ms);
    static void bytesToHex(const uint8_t* in, size_t len, char* out);
};
