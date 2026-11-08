void app_main(void)
{
    // 1) NVS (necessário antes de tudo que persiste)
    ESP_ERROR_CHECK(nvs_flash_init());

    // 2) Log ring buffer (para não bloquear caminho crítico)
    log_ring_init();

    // 3) Barramentos
    ESP_ERROR_CHECK(i2c_init(ROOKCONTROL_I2C_PORT,
                             ROOKCONTROL_SDA_PIN,
                             ROOKCONTROL_SCL_PIN, 400000));
    ESP_ERROR_CHECK(mavlink_bridge_init());
    ESP_ERROR_CHECK(mqtt_transport_init());
    ESP_ERROR_CHECK(event_log_init());

    // 4) Filas estáticas (em .bss, sem heap)
    queues_init_static();

    // 5) Scheduler + tasks
    xTaskCreatePinnedToCore(task_i2c_reader, "i2c",  4096, NULL, 10, NULL, 0);
    xTaskCreatePinnedToCore(task_fusion,     "fuse", 4096, NULL,  8, NULL, 0);
    xTaskCreatePinnedToCore(task_mqtt_tx,    "mqtt", 6144, NULL,  5, NULL, 0);
    xTaskCreatePinnedToCore(task_event_log,  "log",  4096, NULL,  4, NULL, 0);
    xTaskCreatePinnedToCore(task_fsm,        "fsm",  3072, NULL, 12, NULL, 1);
    xTaskCreatePinnedToCore(task_mavlink_rx, "mrx",  4096, NULL, 14, NULL, 1);
    xTaskCreatePinnedToCore(task_diag,       "diag", 2048, NULL,  2, NULL, 0);

    // 6) Watchdog
    esp_task_wdt_add(NULL);  // task main
    // cada task de segurança chama esp_task_wdt_reset()

    ESP_LOGI(TAG, "RookControl up, fw=%s", FW_VERSION_STR);
    vTaskDelay(portMAX_DELAY);
}
