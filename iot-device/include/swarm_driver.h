#pragma once
#include "driver/uart.h"
#include "esp_err.h"

class SwarmDriver {
public:
    SwarmDriver(uart_port_t uart_num, int tx_pin, int rx_pin);
    esp_err_t begin();
    // Envia um buffer binário. O Swarm tem limite de 160 bytes por mensagem [1†L11-L12].
    esp_err_t sendPayload(const uint8_t* data, size_t len);
    // O Swarm enfileira mensagens se não houver satélite no momento [7†L7-L9].
    int getQueuedMessageCount(); 
private:
    uart_port_t _uart_num;
    // ... métodos privados para enviar comandos AT ($MT, $TD) ...
};
