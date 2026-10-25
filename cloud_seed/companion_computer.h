// cloud_seed/companion_computer.h
#include "mission_planner.h"
#include "mission_executor.h"
#include "weather_sensor_driver.h"
#include "secure_mavlink.h"

// instâncias globais
MissionPlanner missionPlanner;
MissionExecutor missionExecutor(&missionPlanner);
// secureMAVLink secureLink(&ardupilotSerial, CONFIG_HMAC_SECRET);

void main_loop() {
    // telemetria do ArduPilot via MAVLink
    float lat, lng, alt, battery;
    // ... (código de decode MAVLink) ...

    // atualiza o executor de missão (verifica geofence, progresso e failsafes)
    missionExecutor.update(lat, lng, alt, battery);

    // sensores ambientais
    environmental_data_t env_data = read_environmental_data();

    // publica telemetria enriquecida (posição + dados ambientais) via MQTT com HMAC
    if (millis() - last_telemetry >= 2000) {
        publish_secure_telemetry(lat, lng, alt, battery, env_data);
        last_telemetry = millis();
    }
}
