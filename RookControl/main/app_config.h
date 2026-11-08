#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// ── Identidade ───────────────────────────────────────────────
#define FW_VERSION_STR          "1.2.0"
#define DEVICE_ID_PREFIX        "ROOK"
#define DRONE_CLASS             "RPA-LR"     // Light/Logistics

// ── RookControl I2C ──────────────────────────────────────────
#define ROOKCONTROL_I2C_ADDR    0x44
#define ROOKCONTROL_I2C_PORT    I2C_NUM_0
#define ROOKCONTROL_SDA_PIN     21
#define ROOKCONTROL_SCL_PIN     22
#define ROOKCONTROL_INT_PIN     34           // opcional

// ── MAVLink UART ─────────────────────────────────────────────
#define MAVLINK_UART_PORT       UART_NUM_1
#define MAVLINK_TX_PIN          17
#define MAVLINK_RX_PIN          16
#define MAVLINK_BAUD            115200
#define MAVLINK_SYSID           42
#define MAVLINK_COMPID          MAV_COMP_ID_ONBOARD_COMPUTER

// ── Limites críticos de missão (failsafe ambiental) ──────────
// Temperatura
#define TEMP_WARN_C             50.0f        // alerta operador
#define TEMP_CRITICAL_C         55.0f        // dispara RTL
#define TEMP_HYST_C             2.0f         // histerese p/ voltar a WARN

// Umidade
#define HUM_MIN_WARN_PCT        10.0f
#define HUM_MAX_WARN_PCT        95.0f
#define HUM_MIN_CRITICAL_PCT    5.0f
#define HUM_MAX_CRITICAL_PCT    98.0f

// ── Debounce / FSM ───────────────────────────────────────────
#define FSM_TICK_MS             100          // cadência da task_fsm
#define FSM_MIN_DWELL_MS        5000         // permanência mínima por estado
#define SENSOR_STALE_MS         60000        // sensor mudo → flag STALE
#define SENSOR_READ_PERIOD_MS   500

// ── MQTT ─────────────────────────────────────────────────────
#define MQTT_BROKER_HOST        "mqtt.rookall.io"
#define MQTT_BROKER_PORT        8883         // TLS
#define MQTT_TOPIC_ENV          "rook/env"
#define MQTT_TOPIC_EVT          "rook/events"
#define MQTT_QOS                1
#define MQTT_BACKOFF_MIN_MS     1000
#define MQTT_BACKOFF_MAX_MS     60000

// ── Filas / tasks ────────────────────────────────────────────
#define Q_ENV_DEPTH             16
#define Q_MAVLINK_DEPTH         8
#define Q_TX_DEPTH              4
#define Q_LOG_DEPTH             8

// ── Segurança ────────────────────────────────────────────────
#define HMAC_PERIOD_MS          1000         // assina o agregado 1 Hz
#define MAVLINK_SIGNING_ENABLED 1

#endif // APP_CONFIG_H
