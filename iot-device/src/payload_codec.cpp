#include "payload_codec.h"
#include "cbor.h" // TinyCBOR

size_t PayloadCodec::encodeTelemetry(const Telemetry& data, uint8_t* out_buffer, size_t max_len) {
    CborEncoder encoder, map_encoder;
    cbor_encoder_init(&encoder, out_buffer, max_len, 0);
    
    // mapa CBOR com 5 pares chave-valor
    CborError err = cbor_encoder_create_map(&encoder, &map_encoder, 5);
    if (err != CborNoError) return 0;

    // chaves numéricas curtas para economizar bytes (em vez de strings longas)
    cbor_encode_uint(&map_encoder, 1); // Chave "v" para tensão
    cbor_encode_float(&map_encoder, data.voltage);
    
    cbor_encode_uint(&map_encoder, 2); // Chave "i" para corrente
    cbor_encode_float(&map_encoder, data.current);
    
    cbor_encode_uint(&map_encoder, 3); // Chave "t" para temperatura
    cbor_encode_float(&map_encoder, data.temperature);
    
    cbor_encode_uint(&map_encoder, 4); // Chave "ts" para timestamp
    cbor_encode_uint(&map_encoder, data.timestamp);
    
    cbor_encode_uint(&map_encoder, 5); // Chave "gv" para voto de governança
    cbor_encode_uint(&map_encoder, data.governance_vote);

    err = cbor_encoder_close_container(&encoder, &map_encoder);
    if (err != CborNoError) return 0;

    return cbor_encoder_get_buffer_size(&encoder, out_buffer);
}
