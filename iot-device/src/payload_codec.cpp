#include "payload_codec.h"
#include "cbor.h"
#include "esp_log.h"

static const char* TAG = "CBOR";

size_t PayloadCodec::encode(const Telemetry& d, uint8_t* out, size_t max_len) {
    CborEncoder encoder, map;
    cbor_encoder_init(&encoder, out, max_len, 0);          //[reference:6]

    // Mapa CBOR com 5 pares chave→valor
    if (cbor_encoder_create_map(&encoder, &map, 5) != CborNoError) return 0;

    // Chaves numéricas curtas (1 byte) em vez de strings longas
    cbor_encode_uint(&map, 1);
    cbor_encode_float(&map, d.voltage);                    //[reference:7]

    cbor_encode_uint(&map, 2);
    cbor_encode_float(&map, d.current);

    cbor_encode_uint(&map, 3);
    cbor_encode_float(&map, d.temperature);

    cbor_encode_uint(&map, 4);
    cbor_encode_uint(&map, d.timestamp);

    cbor_encode_uint(&map, 5);
    cbor_encode_uint(&map, d.governance_vote);

    if (cbor_encoder_close_container(&encoder, &map) != CborNoError) return 0;

    size_t written = cbor_encoder_get_buffer_size(&encoder, out);
    ESP_LOGD(TAG, "CBOR: %d bytes (JSON equivalente: ~%d bytes)", (int)written, (int)(written * 1.4));
    return written;
}

bool PayloadCodec::decode(const uint8_t* in, size_t len, Telemetry& d) {
    CborParser parser;
    CborValue it, map;
    if (cbor_parser_init(in, len, 0, &parser, &it) != CborNoError) return false;
    if (!cbor_value_is_map(&it)) return false;
    if (cbor_value_enter_container(&it, &map) != CborNoError) return false;

    while (!cbor_value_at_end(&map)) {
        uint64_t key;
        if (cbor_value_get_uint64(&map, &key) != CborNoError) return false;
        cbor_value_advance(&map);

        switch (key) {
            case 1: cbor_value_get_float(&map, &d.voltage); break;
            case 2: cbor_value_get_float(&map, &d.current); break;
            case 3: cbor_value_get_float(&map, &d.temperature); break;
            case 4: cbor_value_get_uint64(&map, (uint64_t*)&d.timestamp); break;
            case 5: { uint64_t v; cbor_value_get_uint64(&map, &v); d.governance_vote = (uint8_t)v; break; }
            default: break;
        }
        cbor_value_advance(&map);
    }
    return true;
}
