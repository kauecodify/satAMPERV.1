<img width="900" height="483" alt="image" src="https://github.com/user-attachments/assets/48ea9d63-8ef1-4157-afae-df44fa621bb1" />

# satAMPERV.1: Sistema de Modificação Climática Distribuída e Governança Meteorológica via IoT

## Descrição

O **satAMPERV.1** é um firmware embarcado de alta confiabilidade, projetado para estações meteorológicas de ponta que atuam como nós de controle em sistemas de modificação climática. O sistema permite o gerenciamento democrático e descentralizado do clima local, onde bairros de uma cidade podem votar nas condições meteorológicas desejadas (ex: precipitação para agricultura/limpeza urbana ou céu limpo para eventos/turismo). 

Com base no consenso desses votos, o sistema coordena a execução de operações de semeadura de nuvens (cloud seeding) através de frotas de drones autônomos e uplinks para satélites de órbita baixa, liberando agentes de nucleação (como iodeto de prata) de forma precisa e segura.

O firmware é desenvolvido em arquitetura híbrida (C, C++ e Assembly Xtensa), otimizado para operar em microcontroladores ESP32 em locais remotos, garantindo integridade criptográfica e estabilidade de memória em missões de longa duração.

---

## Arquitetura do Sistema

O fluxo de operação segue uma topologia de borda para nuvem (Edge-to-Cloud):

1. **Coleta Atmosférica:** A estação ESP32 lê sensores de pressão, umidade, densidade de nuvens, velocidade do vento e temperatura.
2. **Governança Distribuída:** O nó recebe propostas de votação (ex: "Bairro A solicita chuva", "Bairro B solicita sol"). O firmware avalia a viabilidade local (ex: se a umidade for inferior a 40%, a semeadura é ineficaz e o voto é ponderado ou vetado).
3. **Assinatura e Transmissão:** Os dados de telemetria e os votos consolidados são serializados em JSON e assinados com HMAC-SHA256.
4. **Orquestração Central:** O backend valida a assinatura, calcula o consenso da cidade e, se aprovado, envia comandos criptografados para a frota de drones ou estações terrestres de uplink satelital para iniciar a semeadura nas coordenadas alvo.

---

## Funcionalidades Principais

- **Arquitetura Híbrida de Alto Desempenho:** Utiliza C puro para leitura de sensores meteorológicos, C++ para lógica de rede/governança e Assembly Xtensa para filtragem digital de sinais de sensores ruidosos.
- **Integridade de Comandos Críticos:** Toda comunicação (telemetria e ordens de disparo de drones/satélites) é protegida por HMAC-SHA256, impedindo que atores maliciosos invoquem chuvas ou limpezas de céu não autorizadas.
- **Otimização Extrema de Memória:** Eliminação total de alocação dinâmica de memória (`String` do Arduino) nas rotinas de loop e callback. Uso de buffers fixos na pilha (stack) para evitar fragmentação de heap e garantir operação contínua por meses sem reinicialização.
- **Mecanismo de Veto por Condição Ambiental:** O dispositivo não é um mero repetidor de votos; ele aplica lógica de borda para garantir que as ordens de semeadura sejam fisicamente viáveis com base nas condições atmosféricas atuais.

---

## Estrutura de Diretórios

```text
satAMPERV.1/
│
├── iot-device/                 # Firmware da Estação Meteorológica
│   ├── CMakeLists.txt          # Configuração de build (ESP-IDF / PlatformIO)
│   ├── include/                
│   │   ├── config.h            # Credenciais de rede, MQTT e chaves HMAC
│   │   ├── weather_driver.h    # Interface do driver de sensores atmosféricos
│   │   └── crypto.h            # Funções de criptografia e assinatura
│   └── src/                    
│       ├── main.cpp            # Orquestração principal, loop e callbacks MQTT
│       ├── weather_driver.c    # Leitura de ADC para sensores meteorológicos (C)
│       ├── crypto_engine.cpp   # Implementação de HMAC-SHA256 via mbedTLS
│       └── assembly_opt.S      # Otimização em assembly para filtragem de sinal
│
├── README.md                   # Documentação do projeto
└── LICENSE                     # Licença de uso (AGPL-3.0)
```

---

## Requisitos

### Hardware
- Microcontrolador ESP32 (Recomendado: ESP32-WROOM-32 ou ESP32-S3 para maior memória).
- Sensores meteorológicos compatíveis com ADC (ex: BME280 para pressão/umidade/temperatura, anemômetro digital).
- Fonte de alimentação estável (solar com banco de baterias para estações remotas).

### Software
- PlatformIO (VS Code) com Arduino Core para ESP32 **OU** ESP-IDF v5.1+.
- Python 3.8+.
- Backend com Broker MQTT (ex: Mosquitto ou Aedes) e banco de dados para gestão de votos e frota.

---

## Instalação e Compilação

### Via PlatformIO (Recomendado)
1. Abra a pasta `iot-device` no VS Code com a extensão PlatformIO instalada.
2. Configure o arquivo `platformio.ini` conforme sua placa.
3. Compile o projeto:
   ```bash
   pio run
   ```
4. Faça o upload e abra o monitor serial:
   ```bash
   pio run --target upload
   pio device monitor
   ```

### Via ESP-IDF Nativo
1. No terminal do ESP-IDF, navegue até `iot-device`.
2. Configure e compile:
   ```bash
   idf.py set-target esp32
   idf.py build
   ```
3. Grave e monitore (ajuste a porta serial conforme seu sistema):
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

---

## Configuração

Edite o arquivo `include/config.h` antes da compilação para definir os parâmetros da sua rede e infraestrutura:

```c
#define CONFIG_WIFI_SSID "REDE_ESTACAO_METEO"
#define CONFIG_WIFI_PASSWORD "senha_segura"

#define CONFIG_MQTT_BROKER "192.168.1.50" // IP do servidor de controle climático
#define CONFIG_MQTT_PORT 1883
#define CONFIG_MQTT_USER "estacao_user"
#define CONFIG_MQTT_PASS "senha_mqtt"

#define CONFIG_DEVICE_ID "ESTACAO-ZONA-NORTE-01"

// Chave crítica: deve ser idêntica à configurada no backend de controle de drones/satélites
#define CONFIG_HMAC_SECRET "chave_secreta_de_assinatura_climatica_256bits"
```

---

## Mecanismo de Governança Climática

O sistema implementa um protocolo de decisão para resolver conflitos de interesse meteorológico entre regiões:

1. **Proposta:** O sistema central recebe solicitações de bairros (ex: Zona Rural pede chuva; Centro Urbano pede sol para um evento) e cria uma proposta de governança no tópico `governance/proposals`.
2. **Avaliação de Borda:** A estação ESP32 recebe a proposta. O firmware executa uma lógica local (ex: `if (umidade < 40.0) { voto = VETO; } else { voto = APROVA_CHUVA; }`).
3. **Votação Assinada:** O dispositivo publica seu voto no tópico `governance/votes`, garantindo que apenas estações autênticas participem da decisão.
4. **Execução:** Ao atingir o quorum e o consenso, o backend emite um comando criptografado para os drones ou satélites iniciarem a semeadura de nuvens nas coordenadas validadas.

---

## Segurança e Boas Práticas Operacionais

- **Integridade de Comandos:** Devido à natureza crítica da modificação climática, o backend deve rejeitar sumariamente qualquer payload sem uma assinatura HMAC-SHA256 válida.
- **Gestão de Segredos:** A `CONFIG_HMAC_SECRET` nunca deve ser versionada em repositórios públicos. Em produção, utilize o NVS (Non-Volatile Storage) criptografado do ESP32 para armazenar chaves.
- **Resiliência de Memória:** O firmware foi projetado para evitar vazamentos de memória. Ao adicionar novas funcionalidades, mantenha o uso de `StaticJsonDocument` e buffers `char` fixos, evitando a classe `String` em rotinas cíclicas.
- **Comunicação Segura:** Em implantações reais, o tráfego MQTT deve ser protegido por TLS/SSL (porta 8883) para evitar interceptação de dados atmosféricos ou comandos de frota.

---

## Licença

Este projeto é distribuído sob a licença **GNU Affero General Public License v3.0 (AGPL-3.0)**. Consulte o arquivo `LICENSE` para mais detalhes.

---

## Referências Técnicas

- [Documentação Oficial do ESP-IDF](https://docs.espressif.com/projects/esp-idf/)
- [RFC 2104: HMAC (Keyed-Hashing for Message Authentication)](https://datatracker.ietf.org/doc/html/rfc2104)
- [Protocolo MQTT v5.0](https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html)
- [Diretrizes de Modificação Climática e Semeadura de Nuvens (WMO)](https://public.wmo.int/en/our-mandate/focus-areas/weather-modification)
