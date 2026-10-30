# satAMPERV.1 by RockNewVenture & Citizen Tech

Firmware embarcado para monitoramento energético remoto via satélite de baixo custo, com governança distribuída em ESP32.

## Visao Geral

O satAMPERV.1 e um firmware para ESP32 projetado para monitoramento de infraestrutura energética em locais sem conectividade terrestre confiável. O sistema usa Wi-Fi/MQTT como link primário de baixo custo e comunicação satelital via modem Swarm M138 como fallback automático. O objetivo e manter telemetria contínua mesmo em áreas remotas.

O projeto também implementa o conceito de IoT Governance: os dispositivos não apenas coletam dados, mas participam de decisões distribuídas, emitindo votos assinados com base em condições locais dos sensores.

A arquitetura foi otimizada para eficiência energética, integridade de dados e viabilidade comercial. O payload de telemetria usa CBOR (Concise Binary Object Representation) em vez de JSON, reduzindo o tamanho da mensagem e viabilizando o uso econômico de links satelitais.

## Caracteristicas Principais

- Comunicação híbrida: Wi-Fi/MQTT como link primário; fallback automático para satélite Swarm M138 quando o Wi-Fi não está disponível.
- Payload compacto com CBOR: serialização binária que reduz o payload em cerca de 28% em relação ao JSON e mantém a mensagem dentro do limite de 160 bytes do Swarm.
- Integridade de dados: assinatura HMAC-SHA256 em todas as mensagens de telemetria e votos de governança.
- Memória otimizada: uso de buffers estáticos e StaticJsonDocument nas rotas críticas; nenhuma alocação dinâmica de String no loop principal.
- Governança distribuída: dispositivos avaliam propostas localmente e emitem votos criptografados.
- Alta disponibilidade: reconexão automática de Wi-Fi/MQTT, watchdog de reinício e comandos remotos de configuração.
- Otimização em assembly: filtros de sinal implementados em assembly Xtensa para desempenho em aquisição de dados.

## Arquitetura do Sistema

Fluxo resumido:

1. Sensores são lidos pelo sensor_driver (C).
2. O CommManager codifica a telemetria em CBOR, assina com HMAC-SHA256 e decide o link de envio.
3. Se o Wi-Fi estiver disponível, o payload é publicado via MQTT no backend.
4. Se o Wi-Fi não estiver disponível, o payload é enviado ao modem Swarm M138 via UART, que o transmite pela rede satelital.

Diagrama em texto:

```
ESP32
  Sensores -> Sensor Driver (C) -> Comm Manager (C++)
                                      |
                                      +-> PayloadCodec (CBOR)
                                      +-> Crypto Engine (HMAC-SHA256)
                                      |
                                      +-> Wi-Fi / MQTT (link primario)
                                      +-> Swarm M138 (fallback satelital)
```

O backend recebe os dados, valida a assinatura HMAC, decodifica o CBOR e persiste as informações.

## Estrutura do Projeto

```
satAMPERV.1/
├── README.md
├── LICENSE
├── cloud_seed/
└── iot-device/
    ├── CMakeLists.txt
    ├── partitions.csv
    ├── sdkconfig.defaults
    ├── include/
    │   ├── config.h
    │   ├── sensor_driver.h
    │   ├── crypto.h
    │   ├── comm_manager.h
    │   ├── swarm_driver.h
    │   └── payload_codec.h
    └── src/
        ├── main.cpp
        ├── sensor_driver.c
        ├── crypto_engine.cpp
        ├── assembly_opt.S
        ├── comm_manager.cpp
        ├── swarm_driver.cpp
        └── payload_codec.cpp
```

## Requisitos de Hardware

- MCU: ESP32-WROOM-32 ou equivalente.
- Modem satelital: Swarm M138, interface UART 3,3 V CMOS, 115200 baud, 8N1.
- Antena: antena Swarm para 137–138 MHz (downlink) e 148–150 MHz (uplink).
- Alimentação: 3,3 V. Consumo do modem: cerca de 80 uA em sleep, 26 mA típico em recepção e 850 mA de pico em transmissão.
- Interface: UART entre ESP32 e M138, com pinos configuráveis no comm_manager.cpp.

O Swarm M138 oferece taxa de dados de aproximadamente 1 kbps e suporta mensagens de até 160 bytes de payload binário. O CBOR mantém a telemetria típica em cerca de 30 a 50 bytes.

## Requisitos de Software

- ESP-IDF v5.1 ou superior.
- Python 3.8 ou superior.
- CMake 3.16 ou superior.
- Componente CBOR: espressif/cbor (TinyCBOR), instalável via Component Registry.

O CBOR não faz parte do núcleo do ESP-IDF desde a versão 5.0. É necessário instalá-lo separadamente antes da compilação.

## Instalação e Compilação

1. Clonar o repositório:

```bash
git clone https://github.com/kauecodify/satAMPERV.1.git
cd satAMPERV.1/iot-device
```

2. Instalar a dependência CBOR:

```bash
idf.py add-dependency "espressif/cbor^0.6.0~1"
```

Esse comando cria ou atualiza o idf_component.yml e baixa o TinyCBOR do Component Registry.

3. Configurar o alvo e compilar:

```bash
idf.py set-target esp32
idf.py build
```

4. Gravar e monitorar:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Substitua /dev/ttyUSB0 pela porta serial correspondente ao sistema.

## Configuração

Antes da compilação, edite include/config.h com os parâmetros da rede e do backend:

```c
#define CONFIG_WIFI_SSID      "sua_rede"
#define CONFIG_WIFI_PASSWORD  "sua_senha"

#define CONFIG_MQTT_BROKER    "192.168.1.100"
#define CONFIG_MQTT_PORT      1883
#define CONFIG_MQTT_USER      "device_user"
#define CONFIG_MQTT_PASS      "device_password"

#define CONFIG_DEVICE_ID      "DEV-AM-001"
#define CONFIG_HMAC_SECRET    "sua_chave_secreta_forte"
```

Os pinos UART do Swarm M138 são definidos no comm_manager.cpp:

```cpp
static SwarmDriver swarm(UART_NUM_1, 17, 16);
```

O driver usa o protocolo NMEA/AT do Swarm, com comandos como $CS (check status), $TD (transmit data) e $MT C=U (message count).

## Segurança e Boas Práticas

1. Buffer estático obrigatório: mantenha buffers de tamanho fixo e StaticJsonDocument nas funções de publicação. Não introduza String dinâmica no loop principal.

2. Gerenciamento de chaves: o CONFIG_HMAC_SECRET não deve ser commitado no repositório. Em produção, carregue a chave da NVS ou use secure boot e flash encryption do ESP32.

3. MQTT sobre TLS: o exemplo usa MQTT em texto claro na porta 1883. Para ambientes reais, habilite MQTT over TLS na porta 8883 com autenticação mútua.

4. Assinatura de payload CBOR: o CommManager aplica HMAC-SHA256 sobre o payload binário CBOR antes do envio. O backend deve validar a assinatura após a decodificação CBOR.

## Mecanismo de Governança IoT

O sistema implementa um protocolo leve de consenso distribuído:

1. O administrador na nuvem publica uma proposta no tópico governance/proposals.
2. Cada ESP32 avalia a proposta localmente com base nos sensores.
3. O dispositivo emite um voto assinado com HMAC-SHA256 no tópico governance/votes.
4. O backend coleta os votos, calcula quórum e limiar de consenso, e aplica a decisão global.

Esse mecanismo permite que a rede se adapte a condições ambientais locais sem depender exclusivamente de decisões centralizadas.

## Comunicação Satelital e Otimização com CBOR

Strings JSON são proibitivas para links satelitais, pois o custo é medido por byte transmitido. O CBOR resolve esse problema por três motivos:

- Payload cerca de 28% menor que o JSON equivalente.
- Codificação e decodificação binária direta, sem parsing de texto.
- Chaves numéricas curtas (1 byte) em vez de strings longas.

A telemetria típica (tensão, corrente, temperatura, timestamp e voto de governança) é codificada em cerca de 30 a 50 bytes CBOR, com folga dentro do limite de 160 bytes do Swarm.

Fluxo de fallback:

1. Leitura dos sensores.
2. Codificação CBOR e assinatura HMAC.
3. Verificação do Wi-Fi.
4. Se Wi-Fi disponível: publicação via MQTT.
5. Se Wi-Fi indisponível: envio via Swarm M138 por UART e rede satelital.

O CommManager tenta o Wi-Fi primeiro, com custo operacional praticamente nulo, e recorre ao satélite apenas quando necessário.

## Custo Operacional do Link Satelital

O Swarm M138 foi escolhido por seu custo por mensagem inferior ao de concorrentes:

- Swarm M138: cerca de US$ 0,05 por mensagem, cobertura global, 1 kbps.
- Iridium 9603N: cerca de US$ 0,50 a US$ 1,00 por mensagem, cobertura global, 2,4 kbps.
- RockBLOCK 9603: cerca de US$ 0,04 a US$ 0,14 por mensagem, cobertura global, 340 bytes por mensagem.

Em uma frota de 10.000 dispositivos enviando uma mensagem por dia, a diferença entre Swarm e Iridium pode chegar a milhares de dólares por dia em economia operacional.

O plano base do Swarm inclui taxa mensal fixa, tipicamente cerca de US$ 5 por mês, que cobre uma franquia de mensagens. Verifique as condições comerciais atualizadas junto à Swarm.

## Licença

Este projeto é distribuído sob a GNU Affero General Public License v3.0 (AGPL-3.0).

## Referências Técnicas

- ESP-IDF: https://docs.espressif.com/projects/esp-idf/
- Swarm M138: https://swarm.space/
- TinyCBOR: https://intel.github.io/tinycbor/current/
- RFC 8949 (CBOR): https://datatracker.ietf.org/doc/html/rfc8949
- RFC 2104 (HMAC): https://datatracker.ietf.org/doc/html/rfc2104
- MQTT v5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html
