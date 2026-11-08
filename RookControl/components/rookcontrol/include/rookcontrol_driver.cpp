struct RookCtx {
    int         port;
    uint8_t     addr;
    bool        bus_ok;
    uint32_t    last_ok_ms;
    int16_t     history[3];      // mediana de 3
    uint8_t     hist_idx;
    uint8_t     identical_cnt;   // sensor travado
    rook_data_t last;
};
static RookCtx s_ctxs[MAX_ROOK_SENSORS];  // normalmente 1


// static IRookSensor *s_sensor;
// static rook_data_t  s_sample;

// void task_i2c_reader(void *arg) {
//     TickType_t last = xTaskGetTickCount();
//     for (;;) {
//         if (s_sensor->read(s_sensor, &s_sample) == ESP_OK) {
//             xQueueSend(q_env, &s_sample, 0);
//         } else {
//             rook_data_t stale = { .magic = ROOK_MAGIC, .flags = ROOK_FLAG_STALE,
//                                   .ts_ms = esp_log_timestamp() };
//             xQueueSend(q_env, &stale, 0);
//         }
//         vTaskDelayUntil(&last, pdMS_TO_TICKS(SENSOR_READ_PERIOD_MS));
//     }
// }
