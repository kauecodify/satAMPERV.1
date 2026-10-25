#pragma once
#include <cstdint>
#include <cstddef>

class PayloadCodec {
public:
    struct Telemetry {
        float voltage;          // V
        float current;          // A
        float temperature;      // °C
        uint32_t timestamp;     // Unix epoch (ou uptime ms)
        uint8_t governance_vote; // 0=nulo, 1=sim, 2=não
    };

    // Retorna o número de bytes escritos (0 em erro)
    static size_t encode(const Telemetry& data, uint8_t* out, size_t max_len);

    // Decodifica para debug/teste (opcional)
    static bool decode(const uint8_t* in, size_t len, Telemetry& out);
};
