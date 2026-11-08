#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// ── Contrato de dados (versionado) ───────────────────────────
#define ROOK_MAGIC        0x524F4F4Bu
#define ROOK_VERSION      1

typedef enum {
    ROOK_FLAG_STALE      = 1u << 0,  // sem leitura há > SENSOR_STALE_MS
    ROOK_FLAG_OUTLIER    = 1u << 1,  // rejeitada pela mediana de 3
    ROOK_FLAG_COMPENSATED= 1u << 2,  // fórmula de fábrica aplicada
} rook_flags_t;

typedef struct {
    uint32_t magic;          // = ROOK_MAGIC (integridade)
    uint8_t  version;        // = ROOK_VERSION
    uint8_t  flags;          // rook_flags_t
    uint8_t  quality;        // 0..100 (CRC interno do sensor)
    int16_t  temp_c_x100;    // 4525 = 45.25 °C
    uint16_t rh_pct_x100;    // 6325 = 63.25 %
    uint32_t ts_ms;          // esp_timer_get_time()/1000
} rook_data_t;

// ── Interface (porta) ────────────────────────────────────────
typedef struct IRookSensor IRookSensor;

struct IRookSensor {
    esp_err_t (*read)(IRookSensor *self, rook_data_t *out);
    bool      (*is_healthy)(IRookSensor *self);
    void      (*reset)(IRookSensor *self);
    void      *impl;         // ctx privado do adapter
};

// ── Factory (adapter concreto) ───────────────────────────────
IRookSensor *rookcontrol_create(int i2c_port, uint8_t addr);
void         rookcontrol_destroy(IRookSensor *s);
