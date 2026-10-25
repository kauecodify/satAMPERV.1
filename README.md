# satAMPERV.1

基于 ESP32 的卫星物联网能源监控与分布式治理系统

## 项目简介

本项目是一个专为偏远地区能源基础设施监控设计的嵌入式物联网（IoT）固件，运行于 ESP32 微控制器。系统旨在通过卫星链路（如 SGDC 卫星）实现高可靠性的实时遥测，并创新性地引入了“物联网治理”（IoT Governance）机制，允许边缘设备基于本地传感器数据自主参与网络的分布式决策与共识。

项目采用混合语言架构，结合了 C 语言（底层硬件驱动）、C++（网络通信与业务逻辑）以及 Xtensa 汇编（信号滤波优化），并在内存管理上进行了极致优化，确保设备在无人值守环境下长期稳定运行。

## 核心特性

- **混合语言架构**：C 语言负责高精度传感器数据采集，C++ 处理 MQTT 通信与 JSON 序列化，汇编语言用于底层信号滤波优化。
- **端到端数据完整性**：所有上传的遥测数据和投票指令均使用 HMAC-SHA256 算法进行数字签名，防止数据在传输过程中被篡改或伪造。
- **极致内存优化**：彻底摒弃了在循环和回调函数中使用动态分配的 `String` 类。所有 JSON 序列化和主题构建均采用固定大小的栈（Stack）字符数组，有效杜绝了长期运行导致的堆内存碎片化（Heap Fragmentation）问题。
- **边缘计算与分布式治理**：设备不仅是被动的数据上报者，还能接收来自云端的“治理提案”，并根据本地环境状态（如温度阈值）自主计算并投出加密选票，实现去中心化的设备协同管理。
- **高可用性设计**：内置完善的 WiFi/MQTT 断线重连机制、看门狗重启逻辑以及远程配置指令（采样率调整、传感器校准、远程重启）支持。

## 系统架构

```text
[ESP32 边缘设备] 
   │
   ├─> 读取传感器 (ADC) 
   ├─> 构建 JSON 并使用 HMAC-SHA256 签名
   └─> 通过 MQTT 发布至 `telemetry/{device_id}` 或 `governance/votes`
         │
         ▼
[MQTT Broker (后端)] 
   │
   ├─> 验证 HMAC 签名 (拒绝非法或篡改数据)
   ├─> 持久化存储至 PostgreSQL
   └─> 通过 WebSocket 广播至前端
         │
         ▼
[Web 监控仪表盘 (React)] 
   └─> 实时数据可视化与治理提案管理
```

## 目录结构

```text
satAMPERV.1/
│
├── iot-device/                 # 嵌入式固件源码目录
│   ├── CMakeLists.txt          # ESP-IDF 构建配置文件
│   ├── include/                # 头文件目录
│   │   ├── config.h            # 网络与设备全局配置 (SSID, MQTT, HMAC密钥)
│   │   ├── sensor_driver.h     # 传感器驱动接口定义
│   │   └── crypto.h            # 加密与签名函数声明
│   └── src/                    # 源代码目录
│       ├── main.cpp            # 主程序入口、任务调度与 MQTT 回调
│       ├── sensor_driver.c     # 传感器底层读取与校准逻辑 (纯 C)
│       ├── crypto_engine.cpp   # mbedTLS HMAC-SHA256 实现
│       └── assembly_opt.S      # Xtensa 汇编信号滤波优化
│
├── README.md                   # 项目说明文档
└── LICENSE                     # AGPL-3.0 开源许可证
```

## 环境要求

### 硬件
- ESP32 开发板（推荐 ESP32-WROOM-32 或同等型号）
- USB 数据线（用于烧录与串口监控）

### 软件
- **选项 A (推荐)**: PlatformIO (基于 VS Code) + Arduino Core for ESP32
- **选项 B**: ESP-IDF v5.1 或更高版本 + Python 3.8+
- CMake 3.16+

## 安装与编译

### 使用 PlatformIO
1. 在 VS Code 中打开 `iot-device` 文件夹。
2. 确保 `platformio.ini` 已正确配置目标平台。
3. 点击底部状态栏的 **Build** (编译) 按钮，或运行：
   ```bash
   pio run
   ```
4. 编译成功后，点击 **Upload and Monitor** (烧录并监控)，或运行：
   ```bash
   pio run --target upload
   pio device monitor
   ```

### 使用原生 ESP-IDF
1. 打开 ESP-IDF 命令行终端，导航至 `iot-device` 目录。
2. 设置目标芯片并编译：
   ```bash
   idf.py set-target esp32
   idf.py build
   ```
3. 烧录固件并打开串口监控（请将 `/dev/ttyUSB0` 替换为您的实际串口）：
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

## 配置说明

在编译前，请修改 `include/config.h` 文件以匹配您的实际网络环境和后端服务：

```c
// 网络配置
#define CONFIG_WIFI_SSID "您的WiFi名称"
#define CONFIG_WIFI_PASSWORD "您的WiFi密码"

// MQTT 代理配置
#define CONFIG_MQTT_BROKER "192.168.1.100"  // 后端服务器 IP 或域名
#define CONFIG_MQTT_PORT 1883
#define CONFIG_MQTT_USER "device_user"
#define CONFIG_MQTT_PASS "device_password"

// 设备标识与安全
#define CONFIG_DEVICE_ID "DEV-AM-001"
#define CONFIG_HMAC_SECRET "您的高强度共享密钥" // 必须与后端保持一致
```

## 安全与最佳实践

1. **内存安全**：本项目已针对 `publishTelemetry`、`publishVote` 等高频调用函数进行了栈内存优化。在扩展功能时，请继续保持使用固定大小的 `char` 数组和 `StaticJsonDocument`，避免在 `loop()` 中使用动态内存分配。
2. **密钥管理**：`CONFIG_HMAC_SECRET` 切勿提交至公共代码仓库。在生产环境中，建议使用 ESP-IDF 的 NVS (Non-Volatile Storage) 或安全启动机制来动态加载密钥。
3. **传输加密**：当前示例使用明文 MQTT (端口 1883)。在部署至真实卫星或公共网络环境时，强烈建议启用 MQTT over TLS (端口 8883) 并配置双向证书认证 (mTLS)。

## 物联网治理机制说明

系统实现了一种轻量级的分布式共识协议：
1. 云端管理员创建包含选项 X 和 Y 的治理提案，并发布至 `governance/proposals` 主题。
2. ESP32 设备接收到提案后，执行本地评估逻辑（例如：当前代码中设定，若传感器温度 > 35.0°C，则投票给 Y；否则投票给 X）。
3. 设备将带有 HMAC 签名的投票结果发布至 `governance/votes` 主题。
4. 云端后端收集所有设备的投票，计算法定人数（Quorum）和共识阈值，从而决定最终的配置变更。

## 开源协议

本项目采用 **GNU Affero General Public License v3.0 (AGPL-3.0)** 协议开源。详情请参阅 `LICENSE` 文件。

## 技术参考

- [ESP-IDF 官方文档](https://docs.espressif.com/projects/esp-idf/)
- [ArduinoJson 文档 (v6)](https://arduinojson.org/v6/doc/)
- [RFC 2104: HMAC (Keyed-Hashing for Message Authentication)](https://datatracker.ietf.org/doc/html/rfc2104)
- [MQTT Version 5.0 规范](https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html)
