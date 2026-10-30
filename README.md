# satAMPERV.1

Embedded firmware for remote energy monitoring via low-cost satellite, with distributed governance on ESP32.

## Overview

satAMPERV.1 is an ESP32 firmware designed for monitoring energy infrastructure in locations without reliable terrestrial connectivity. The system uses Wi-Fi/MQTT as a low-cost primary link and satellite communication via the Swarm M138 modem as an automatic fallback. The goal is to maintain continuous telemetry even in remote areas.

The project also implements the concept of IoT Governance: devices do not just collect data, but participate in distributed decisions, emitting signed votes based on local sensor conditions.

The architecture has been optimized for energy efficiency, data integrity, and commercial viability. The telemetry payload uses CBOR (Concise Binary Object Representation) instead of JSON, reducing message size and enabling the economical use of satellite links.

## Key Features

- **Hybrid communication**: Wi-Fi/MQTT as the primary link; automatic fallback to Swarm M138 satellite when Wi-Fi is unavailable.
- **Compact CBOR payload**: Binary serialization that reduces payload size by approximately 28% compared to JSON, keeping the message well within Swarm's 160-byte limit.
- **Data integrity**: HMAC-SHA256 signature on all telemetry messages and governance votes.
- **Optimized memory**: Use of static buffers and `StaticJsonDocument` in critical paths; no dynamic String allocation in the main loop.
- **Distributed governance**: Devices evaluate proposals locally and emit cryptographically signed votes.
- **High availability**: Automatic Wi-Fi/MQTT reconnection, restart watchdog, and remote configuration commands.
- **Assembly optimization**: Signal filters implemented in Xtensa assembly for high-performance data acquisition.

## System Architecture

Summarized flow:

1. Sensors are read by the `sensor_driver` (C).
2. The `CommManager` encodes the telemetry into CBOR, signs it with HMAC-SHA256, and decides the transmission link.
3. If Wi-Fi is available, the payload is published via MQTT to the backend.
4. If Wi-Fi is unavailable, the payload is sent to the Swarm M138 modem via UART, which transmits it over the satellite network.

Text diagram:

```text
ESP32
  Sensors -> Sensor Driver (C) -> Comm Manager (C++)
                                      |
                                      +-> PayloadCodec (CBOR)
                                      +-> Crypto Engine (HMAC-SHA256)
                                      |
                                      +-> Wi-Fi / MQTT (primary link)
                                      +-> Swarm M138 (satellite fallback)
```

The backend receives the data, validates the HMAC signature, decodes the CBOR, and persists the information.

## Project Structure

```text
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

## Hardware Requirements

- **MCU**: ESP32-WROOM-32 or equivalent.
- **Satellite modem**: Swarm M138, 3.3V CMOS UART interface, 115200 baud, 8N1.
- **Antenna**: Swarm antenna for 137–138 MHz (downlink) and 148–150 MHz (uplink).
- **Power supply**: 3.3V. Modem consumption: ~80 µA in sleep, 26 mA typical in reception, and 850 mA peak in transmission.
- **Interface**: UART between ESP32 and M138, with configurable pins in `comm_manager.cpp`.

The Swarm M138 offers a data rate of approximately 1 kbps and supports messages with up to 160 bytes of binary payload. CBOR keeps typical telemetry around 30 to 50 bytes.

## Software Requirements

- ESP-IDF v5.1 or higher.
- Python 3.8 or higher.
- CMake 3.16 or higher.
- CBOR component: `espressif/cbor` (TinyCBOR), installable via the Component Registry.

*Note:* CBOR has not been part of the ESP-IDF core since version 5.0. It must be installed separately before compilation.

## Installation and Compilation

1. Clone the repository:

```bash
git clone https://github.com/kauecodify/satAMPERV.1.git
cd satAMPERV.1/iot-device
```

2. Install the CBOR dependency:

```bash
idf.py add-dependency "espressif/cbor^0.6.0~1"
```

This command creates or updates the `idf_component.yml` and downloads TinyCBOR from the Component Registry.

3. Configure the target and build:

```bash
idf.py set-target esp32
idf.py build
```

4. Flash and monitor:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Replace `/dev/ttyUSB0` with the serial port corresponding to your system.

## Configuration

Before compilation, edit `include/config.h` with your network and backend parameters:

```c
#define CONFIG_WIFI_SSID      "your_network"
#define CONFIG_WIFI_PASSWORD  "your_password"

#define CONFIG_MQTT_BROKER    "192.168.1.100"
#define CONFIG_MQTT_PORT      1883
#define CONFIG_MQTT_USER      "device_user"
#define CONFIG_MQTT_PASS      "device_password"

#define CONFIG_DEVICE_ID      "DEV-AM-001"
#define CONFIG_HMAC_SECRET    "your_strong_secret_key"
```

The Swarm M138 UART pins are defined in `comm_manager.cpp`:

```cpp
static SwarmDriver swarm(UART_NUM_1, 17, 16);
```

The driver uses the Swarm NMEA/AT protocol, with commands such as `$CS` (check status), `$TD` (transmit data), and `$MT C=U` (message count).

## Security and Best Practices

1. **Mandatory static buffers**: Maintain fixed-size buffers and `StaticJsonDocument` in publishing functions. Do not introduce dynamic Strings in the main loop.
2. **Key management**: `CONFIG_HMAC_SECRET` must not be committed to the repository. In production, load the key from NVS or use ESP32 secure boot and flash encryption.
3. **MQTT over TLS**: The example uses plaintext MQTT on port 1883. For real-world environments, enable MQTT over TLS on port 8883 with mutual authentication.
4. **CBOR payload signature**: The `CommManager` applies HMAC-SHA256 to the binary CBOR payload before sending. The backend must validate the signature after CBOR decoding.

## IoT Governance Mechanism

The system implements a lightweight distributed consensus protocol:

1. The cloud administrator publishes a proposal to the `governance/proposals` topic.
2. Each ESP32 evaluates the proposal locally based on its sensor data.
3. The device emits an HMAC-SHA256 signed vote to the `governance/votes` topic.
4. The backend collects the votes, calculates the quorum and consensus threshold, and applies the global decision.

This mechanism allows the network to adapt to local environmental conditions without relying exclusively on centralized decisions.

## Satellite Communication and CBOR Optimization

JSON strings are prohibitive for satellite links, as costs are measured per byte transmitted. CBOR solves this problem for three reasons:

- Payload is approximately 28% smaller than the equivalent JSON.
- Direct binary encoding and decoding, without text parsing overhead.
- Short numeric keys (1 byte) instead of long strings.

Typical telemetry (voltage, current, temperature, timestamp, and governance vote) is encoded in about 30 to 50 bytes of CBOR, leaving ample room within Swarm's 160-byte limit.

Fallback flow:

1. Sensor reading.
2. CBOR encoding and HMAC signature.
3. Wi-Fi availability check.
4. If Wi-Fi is available: publish via MQTT.
5. If Wi-Fi is unavailable: send via Swarm M138 over UART and the satellite network.

The `CommManager` attempts Wi-Fi first, with practically zero operational cost, and resorts to the satellite only when necessary.

## Operational Cost of the Satellite Link

The Swarm M138 was chosen for its lower cost per message compared to competitors:

- **Swarm M138**: ~$0.05 per message, global coverage, 1 kbps.
- **Iridium 9603N**: ~$0.50 to $1.00 per message, global coverage, 2.4 kbps.
- **RockBLOCK 9603**: ~$0.04 to $0.14 per message, global coverage, 340 bytes per message.

In a fleet of 10,000 devices sending one message per day, the difference between Swarm and Iridium can amount to thousands of dollars per day in operational savings.

Swarm's base plan includes a fixed monthly fee, typically around $5 per month, which covers a message allowance. Please verify updated commercial conditions directly with Swarm.

## License

This project is distributed under the GNU Affero General Public License v3.0 (AGPL-3.0).

## Technical References

- **ESP-IDF**: https://docs.espressif.com/projects/esp-idf/
- **Swarm M138**: https://swarm.space/
- **TinyCBOR**: https://intel.github.io/tinycbor/current/
- **RFC 8949 (CBOR)**: https://datatracker.ietf.org/doc/html/rfc8949
- **RFC 2104 (HMAC)**: https://datatracker.ietf.org/doc/html/rfc2104
- **MQTT v5.0**: https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html
