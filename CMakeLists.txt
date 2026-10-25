cmake_minimum_required(VERSION 3.16)

# ferramentas padrão do ESP-IDF
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Nome do projeto (gera o binário final)
project(iot-device)

# Lista todos os arquivos fonte
set(SOURCES
    src/main.cpp
    src/network_manager.cpp
    src/sensor_driver.c
    src/crypto_engine.cpp
    src/governance_client.cpp
    src/assembly_opt.S  # Nota: No ESP-IDF, usamos .S (maiúsculo) para Assembly com pré-processador C
)

# Registra o componente principal
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "include"
    # Dependências do ESP-IDF para WiFi, MQTT, Criptografia e JSON
    REQUIRES esp_wifi nvs_flash esp_netif esp_mqtt mbedtls esp_json
)
