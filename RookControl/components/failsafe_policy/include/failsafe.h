#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "rookcontrol.h"   // apenas o struct de dados, sem impl

typedef enum {
    ENV_STATE_OK = 0,
    ENV_STATE_WARN,
    ENV_STATE_CRITICAL,
    ENV_STATE_STALE,
} env_state_t;

typedef enum {
    FS_ACTION_NONE = 0,
    FS_ACTION_LOG,
    FS_ACTION_RTL,
} fs_action_t;

typedef struct {
    // Entradas
    rook_data_t  sensor;
    bool         fc_healthy;        // MAVLink link OK
    bool         fc_in_failsafe;    // FC já está em failsafe próprio
    uint32_t     now_ms;

    // Limites (vindos de app_config via init)
    float        temp_warn_c;
    float        temp_critical_c;
    float        temp_hyst_c;
    float        hum_min_warn, hum_max_warn;
    float        hum_min_crit, hum_max_crit;
    uint32_t     min_dwell_ms;
    uint32_t     stale_ms;

    // Estado interno (opaco ao chamador)
    env_state_t  state;
    uint32_t     state_since_ms;
    uint32_t     last_sample_ms;
} failsafe_ctx_t;

void       failsafe_init(failsafe_ctx_t *ctx);
fs_action_t failsafe_step(failsafe_ctx_t *ctx,
                          const rook_data_t *s,
                          bool fc_healthy,
                          bool fc_in_failsafe,
                          uint32_t now_ms);
env_state_t failsafe_state(const failsafe_ctx_t *ctx);
