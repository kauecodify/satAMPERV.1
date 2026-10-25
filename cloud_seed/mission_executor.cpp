// cloud_seed/mission_executor.cpp
#include "mission_executor.h"
#include "config_drone.h"
#include "esp_log.h"
#include "mission_executor.h"

MissionExecutor::MissionExecutor(MissionPlanner* p) {}
void MissionExecutor::update(float, float, float, float) {}
mission_state_t MissionExecutor::getState() { return STATE_IDLE; }

static const char *TAG = "MISSION_EXECUTOR";

MissionExecutor::MissionExecutor(MissionPlanner* p) {
    planner = p;
    current_state = STATE_IDLE;
    current_battery_pct = 100.0;
}

void MissionExecutor::checkFailsafes() {
    // Failsafe crítico: Se a bateria cair abaixo de 20%, abortar missão e retornar
    if (current_battery_pct < 20.0 && current_state == STATE_NAVIGATING) {
        ESP_LOGW(TAG, "BATERIA CRÍTICA (%.1f%%). Acionando RTL (Return to Launch).", current_battery_pct);
        current_state = STATE_RETURNING_HOME;
        // comando MAV_CMD_NAV_RETURN_TO_LAUNCH para o ArduPilot
    }
}

void MissionExecutor::commandNextWaypoint() {
    Waypoint* next_wp = planner->getNextWaypoint();
    if (next_wp != nullptr) {
        ESP_LOGI(TAG, "Navegando para Waypoint: Lat=%.6f, Lng=%.6f, Alt=%.1f", 
                 next_wp->latitude, next_wp->longitude, next_wp->altitude);
        
        // Enviar comando MAVLink MAV_CMD_NAV_WAYPOINT para o ArduPilot
        // (Implementação via SecureMAVLink omitida para brevidade)
        
        planner->advanceToNextWaypoint();
    } else {
        ESP_LOGI(TAG, "Missão concluída. Retornando à base.");
        current_state = STATE_RETURNING_HOME;
    }
}

void MissionExecutor::update(float current_lat, float current_lng, float current_alt, float battery_pct) {
    current_battery_pct = battery_pct;
    checkFailsafes();

    if (current_state == STATE_NAVIGATING) {
        if (planner->hasReachedWaypoint(current_lat, current_lng, current_alt, 5.0)) {
            ESP_LOGI(TAG, "Waypoint atingido. Avançando...");
            commandNextWaypoint();
        }
    }
    // Lógica para STATE_TAKING_OFF, STATE_RETURNING_HOME, etc...
}

mission_state_t MissionExecutor::getState() {
    return current_state;
}
