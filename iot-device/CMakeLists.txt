# componente firmware

idf_component_register(
    SRCS "src/main.cpp" 
         "src/sensor_driver.c" 
         "src/crypto_engine.cpp" 
         "src/assembly_opt.S"
         "src/comm_manager.cpp"
         "src/swarm_driver.cpp"
         "src/payload_codec.cpp"
         # "src/lora_driver.cpp" # Descomente se usar LoRaWAN
    INCLUDE_DIRS "include"
    REQUIRES 
        driver 
        esp_timer 
        nvs_flash 
        mbedtls 
        json
        # TinyCBOR já faz parte do núcleo do ESP-IDF, mas garantimos a dependência:
        esp_ringbuf
        espressif/cbor
        # Para LoRaWAN (se usado), adicione:
        # RadioLib
)
