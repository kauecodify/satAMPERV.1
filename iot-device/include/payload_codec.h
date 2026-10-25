#pragma once
#include <cstdint>
#include <cstddef>

class PayloadCodec {
public:
    // Estrutura mínima para telemetria de energia
    struct Telemetry {
        float voltage;
        float current;
        float temperature;
        uint32_t timestamp;
        uint8_t governance_vote; // Voto de governança (0=Nulo, 1=Sim, 2=Não)
    };

    // Codifica a estrutura em um buffer binário CBOR.
    // Retorna o número de bytes escritos (0 em caso de erro).
    static size_t encodeTelemetry(const Telemetry& data, uint8_t* out_buffer, size_t max_len);
    
    // (Opcional) Decodifica para debug ou recebimento de propostas
    static bool decodeTelemetry(const uint8_t* buffer, size_t len, Telemetry& out_data);
};
