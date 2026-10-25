# componente firmware
idf_component_register(
    SRCS 
        "src/main.cpp"
        "src/network_manager.cpp"
        "src/sensor_driver.c"
        "src/crypto_engine.cpp"
        "src/governance_client.cpp"
        "src/assembly_opt.S"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        esp_wifi 
        nvs_flash 
        esp_netif 
        esp_mqtt 
        mbedtls 
        esp_json
        driver          # I2C do BME280
)
