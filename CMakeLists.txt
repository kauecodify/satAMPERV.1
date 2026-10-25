idf_component_register(
    SRCS "src/main.cpp"
         "src/sensor_driver.c"
         "src/crypto_engine.cpp"
         "src/assembly_opt.S"
         "src/comm_manager.cpp"
         "src/swarm_driver.cpp"
         "src/payload_codec.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        driver
        esp_timer
        nvs_flash
        mbedtls
        json
        espressif/cbor        # TinyCBOR empacotado para ESP-IDF[reference:8]
)
