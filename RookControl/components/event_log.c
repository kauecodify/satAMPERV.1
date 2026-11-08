esp_err_t event_log_init(void);
esp_err_t event_log_append(const char *type, const rook_data_t *d);
esp_err_t event_log_drain_to_mqtt(void);   // chamado quando reconecta
uint32_t  event_log_count(void);
esp_err_t event_log_erase_oldest(uint32_t n);
