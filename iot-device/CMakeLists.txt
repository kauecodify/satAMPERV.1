# componente firmware
idf_component_register(
    SRCS 
        "src/main.cpp"
        "src/sensor_driver.c"
        "src/crypto_engine.cpp"
        "src/assembly_opt.S"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi nvs_flash esp_netif esp_mqtt mbedtls json driver
)
