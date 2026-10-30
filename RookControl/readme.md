---

# RookControl: Integração de Monitoramento Ambiental para Drones

<p align="center">
  <img width="353" height="520" alt="RookControl Sensor" src="https://github.com/user-attachments/assets/33027b3d-2ea1-4440-83f2-360c26d1e7f1" />
  <img width="896" height="1200" alt="RookAll Dashboard" src="https://github.com/user-attachments/assets/ae4abd4e-8153-4b08-a641-3913a8cf3942" />
</p>

## Visão Geral

O **RookControl** é um módulo sensorial de alta precisão projetado para monitoramento de temperatura e umidade relativa do ar. Este documento descreve sua integração nativa com a arquitetura de *Companion Computer* (ESP32) para drones, atuando como a principal fonte de dados ambientais para missões críticas.

Ao integrar o RookControl ao ecossistema de voo (ArduPilot/PX4), o sistema não apenas coleta dados, mas os utiliza para tomada de decisão autônoma, como acionamento de failsafes (RTL) em caso de condições ambientais perigosas para a carga ou para a própria aeronave.

---

## Arquitetura de Integração

O RookControl se comunica diretamente com o ESP32 Companion Computer via barramento I2C, substituindo ou complementando sensores genéricos (como BME280/SHT31) com calibração específica para aplicações aeronáuticas.

### Fluxo de Dados:
1. **Aquisição:** O `rookcontrol_driver` lê os dados brutos de temperatura e umidade via I2C a cada 500ms.
2. **Compensação:** Aplicação das fórmulas de compensação de fábrica do RookControl para garantir precisão em variações de pressão altimétrica.
3. **Fusão de Dados:** O ESP32 funde os dados do RookControl com a telemetria de voo (GPS, Altitude, Voltagem da Bateria) recebida via MAVLink.
4. **Segurança:** O payload fusionado é serializado e assinado com **HMAC-SHA256**.
5. **Transmissão e Ação:** 
   - *Uplink:* Envio via MQTT para a estação base (RookAll Dashboard).
   - *Downlink/Local:* Se os limites críticos forem ultrapassados, o ESP32 injeta um comando MAVLink de `DO_SET_MODE` para acionar o Retorno à Base (RTL) automaticamente.

---

## Integração de Hardware

O RookControl foi projetado para "Plug and Play" com a arquitetura ESP32, operando dentro dos padrões de aeronaves remotamente pilotadas.

| Pino RookControl | Pino ESP32 (Exemplo) | Função | Observação |
| :--- | :--- | :--- | :--- |
| `VCC` | `3.3V` | Alimentação | Não conectar em 5V para evitar danos. |
| `GND` | `GND` | Terra | Terra comum com o controlador de voo. |
| `SDA` | `GPIO 21` | Dados I2C | Requer resistor de pull-up (já presente no módulo). |
| `SCL` | `GPIO 22` | Clock I2C | Requer resistor de pull-up (já presente no módulo). |
| `INT` | `GPIO 34` | Interrupção (Opcional) | Aciona o ESP32 apenas quando há mudança significativa, economizando energia. |

---

## Integração de Software (ESP-IDF)

A integração é feita através de um driver dedicado que respeita as regras de alocação estática de memória do sistema.

### 1. Estrutura de Arquivos Adicionada
```text
iot-device/
└── src/
    ├── rookcontrol_driver.h   # Definições de registradores e structs de dados
    └── rookcontrol_driver.cpp # Lógica de leitura I2C e compensação
```

### 2. Configuração (`config_drone.h`)
Adicione os limites operacionais específicos para a missão do RookControl:

```c
// Configuração do Sensor RookControl
#define ROOKCONTROL_I2C_ADDR    0x44
#define ROOKCONTROL_I2C_PORT    I2C_NUM_0
#define ROOKCONTROL_SDA_PIN     21
#define ROOKCONTROL_SCL_PIN     22

// Limites Críticos de Missão (Failsafes)
#define MAX_SAFE_TEMP_C         45.0    // Acima disso, aciona RTL
#define MIN_SAFE_HUMIDITY_PCT   10.0    // Abaixo disso, alerta de ar extremamente seco
#define MAX_SAFE_HUMIDITY_PCT   95.0    // Acima disso, risco de condensação na eletrônica
```

### 3. Exemplo de Leitura Segura (Sem alocação dinâmica)
```cpp
#include "rookcontrol_driver.h"
#include "secure_mavlink.h"

// Buffer estático para evitar fragmentação de heap
static rook_data_t env_data; 

void task_read_environment(void *pvParameters) {
    while (true) {
        if (rookcontrol_read(&env_data) == ESP_OK) {
            // Verificação de Failsafe Ambiental
            if (env_data.temperature > MAX_SAFE_TEMP_C) {
                mission_executor_trigger_rtl("CRITICAL_TEMP");
            }
            
            // Fusão e envio seguro
            telemetry_payload_t payload = fuse_telemetry(env_data, get_mavlink_data());
            sign_and_publish_mqtt(&payload);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

---

## Casos de Uso Principais

1. **Agricultura de Precisão:** A eficiência de pulverização depende diretamente da umidade relativa e temperatura. O RookControl valida em tempo real se as condições são ideais antes de autorizar a abertura das válvulas via MAVLink.
2. **Cadeia de Frio (Logística):** Monitoramento contínuo de cargas sensíveis (medicamentos, alimentos). Se a temperatura sair da faixa, o sistema registra o evento com timestamp assinado (prova criptográfica) e alerta a base.
3. **Saúde da Bateria (LiPo):** Temperaturas ambiente muito baixas afetam a descarga de baterias LiPo. O sistema pode ajustar os limites de voltagem crítica dinamicamente com base nos dados do RookControl.

---

## Segurança e Confiabilidade

- **Integridade de Dados:** Toda leitura do RookControl incluída no payload MQTT é protegida por HMAC-SHA256, impedindo que dados ambientais falsos sejam injetados no sistema (ex: esconder um superaquecimento).
- **Memória Estática:** O driver do RookControl utiliza apenas variáveis estáticas e `StaticJsonDocument`, garantindo zero fragmentação de heap durante voos de longa duração (LR).
- **Watchdog:** Se o sensor RookControl parar de responder no barramento I2C por mais de 2 segundos, o ESP32 reinicia o barramento I2C automaticamente sem reiniciar o controlador de voo.

---

## Instalação e Build

1. Clone o repositório do projeto Companion Computer:
   ```bash
   git clone https://github.com/kauecodify/satAMPERV.1.git
   cd satAMPERV.1/iot-device
   ```
2. Certifique-se de que o componente `rookcontrol_driver` está incluído no `CMakeLists.txt`.
3. Configure o alvo e compile:
   ```bash
   idf.py set-target esp32
   idf.py build
   ```
4. Faça o flash no ESP32 Companion Computer:
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

---

## Referências

- [Datasheet e Protocolo I2C do RookControl](#) *(Link interno do projeto)*
- [Protocolo MAVLink v2.0 - Environmental Data](https://mavlink.io/en/messages/common.html#SCALED_PRESSURE)
- [RFC 2104: HMAC](https://datatracker.ietf.org/doc/html/rfc2104)
- Regulamentação de Espaço Aéreo para Aeronaves Remotamente Pilotadas (ANAC/DECEA)

---
*Desenvolvido para FIAP Citizen Tech Global Solution | Licença AGPL-3.0*
