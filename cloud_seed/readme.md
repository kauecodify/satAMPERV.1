<img width="900" height="483" alt="image" src="https://github.com/user-attachments/assets/48ea9d63-8ef1-4157-afae-df44fa621bb1" />

# Módulo de Monitoramento Ambiental e Telemetria Segura para Drones

Firmware para ESP32 projetado para atuar como computador de bordo (companion computer) integrado a controladores de voo (ArduPilot/PX4). Este módulo é responsável pela coleta de dados atmosféricos, gestão de missões com validação de geofence e comunicação criptografada bidirecional via protocolos MAVLink e MQTT.

## Funcionalidades Principais

- Leitura de sensores I2C (ex: BME280) com aplicação das fórmulas de compensação de fábrica.
- Comunicação MAVLink bidirecional com assinatura e verificação HMAC-SHA256.
- Validação local de waypoints contra limites de geofence definidos em tempo de compilação.
- Failsafes autônomos, incluindo acionamento de retorno à base (RTL) por bateria crítica.
- Alocação de memória estritamente estática (stack) para evitar fragmentação de heap em operações de longa duração.

## Estrutura de Arquivos

- `CMakeLists.txt`: Configuração de build do componente ESP-IDF.
- `companion_computer.h` / `.cpp`: Orquestração principal, loop de tarefas e integração MQTT/MAVLink.
- `mission_planner.h` / `.cpp`: Estruturas de dados de waypoints e lógica de validação de geofence.
- `mission_executor.h` / `.cpp`: Máquina de estados de voo e execução de failsafes.
- `secure_mavlink.h` / `.cpp`: Serialização, envio e verificação criptográfica de mensagens MAVLink.
- `weather_sensor_driver.h` / `.cpp`: Driver I2C para leitura e processamento de dados de sensores ambientais.

## Requisitos

### Hardware
- Microcontrolador ESP32 (WROOM-32 ou S3).
- Sensor ambiental I2C (ex: BME280 ou SHT31).
- Controlador de voo compatível com MAVLink (ex: Pixhawk).

### Software
- ESP-IDF v5.1 ou superior.
- Componentes do ESP-IDF: `driver` (I2C), `mbedtls`, `mqtt`, `json`, `nvs_flash`.

## Fluxo de Dados

1. O ESP32 lê a telemetria de voo (GPS, altitude, bateria) via porta serial do controlador de voo (MAVLink).
2. O ESP32 lê os dados ambientais via barramento I2C.
3. Os dados são fusionados em um payload JSON e assinados com HMAC-SHA256.
4. O payload assinado é publicado no broker MQTT da estação base.
5. Comandos recebidos via MQTT são verificados quanto à assinatura HMAC. Se válidos, são traduzidos para comandos MAVLink e enviados ao controlador de voo.

## Configuração

Antes da compilação, ajuste as macros em `config_drone.h` para o ambiente de operação:

```c
#define DRONE_ID "DRONE-MONITOR-01"

// Coordenadas da Base para procedimentos de retorno (RTL)
#define HOME_LATITUDE  -23.550520
#define HOME_LONGITUDE -46.633308
#define HOME_ALTITUDE  760.0

// Limites de Geofence (Área de operação autorizada)
#define GEOFENCE_MIN_LAT -23.600000
#define GEOFENCE_MAX_LAT -23.500000
#define GEOFENCE_MIN_LNG -46.700000
#define GEOFENCE_MAX_LNG -46.500000
#define MAX_ALTITUDE_M   120.0
```

## Referências Técnicas

- Protocolo MAVLink v2.0: https://mavlink.io/en/
- Datasheet e Fórmulas de Compensação BME280 (Bosch)
- Regulamentação de Espaço Aéreo para Aeronaves Remotamente Pilotadas (ANAC/DECEA)
- RFC 2104: HMAC (Keyed-Hashing for Message Authentication)
