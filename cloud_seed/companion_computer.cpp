// companion_computer.cpp - ESP32 como computador complementar ao ArduPilot
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <MAVLink.h>  // Biblioteca MAVLink para Arduino
#include "config.h"
#include "crypto.h"

// Comunicação serial com ArduPilot (via telemetry port)
HardwareSerial ardupilotSerial(2);

// Rede
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Estado do drone (sincronizado com ArduPilot)
struct DroneState {
    float latitude;
    float longitude;
    float altitude;
    float battery_voltage;
    float battery_remaining;
    uint8_t flight_mode;
    bool is_armed;
    bool is_flying;
    unsigned long last_heartbeat;
} state;

void setup() {
    Serial.begin(115200);
    
    // Comunicação com ArduPilot (57600 baud é padrão para telemetry)
    ardupilotSerial.begin(57600, SERIAL_8N1, 16, 17);
    
    Serial.println("=== Computador Complementar - ArduPilot ===");
    
    initWiFi();
    initMQTT();
    
    state.last_heartbeat = 0;
}

void loop() {
    // Processar mensagens MAVLink do ArduPilot
    processMAVLinkMessages();
    
    // Manter conexão MQTT
    mqttClient.loop();
    
    // Enviar heartbeat para ArduPilot a cada 0,1 segundo
    if (millis() - state.last_heartbeat >= 100) {
        sendHeartbeat();
        state.last_heartbeat = millis();
    }
    
    // Publicar telemetria para o servidor a cada 0,5 segundos
    static unsigned long lastTelemetry = 0;
    if (millis() - lastTelemetry >= 500) {
        publishDroneTelemetry();
        lastTelemetry = millis();
    }
    
    delay(10);
}

// Processar mensagens MAVLink recebidas do ArduPilot
void processMAVLinkMessages() {
    mavlink_message_t msg;
    mavlink_status_t status;
    
    while (ardupilotSerial.available()) {
        uint8_t c = ardupilotSerial.read();
        
        if (mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {
            handleMAVLinkMessage(msg);
        }
    }
}

// Tratar mensagens MAVLink específicas
void handleMAVLinkMessage(mavlink_message_t& msg) {
    switch (msg.msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT: {
            mavlink_heartbeat_t heartbeat;
            mavlink_msg_heartbeat_decode(&msg, &heartbeat);
            
            state.flight_mode = heartbeat.custom_mode;
            state.is_armed = (heartbeat.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) ? true : false;
            break;
        }
        
        case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
            mavlink_global_position_int_t pos;
            mavlink_msg_global_position_int_decode(&msg, &pos);
            
            state.latitude = pos.lat / 1e7;
            state.longitude = pos.lon / 1e7;
            state.altitude = pos.relative_alt / 1000.0;  // mm para metros
            break;
        }
        
        case MAVLINK_MSG_ID_SYS_STATUS: {
            mavlink_sys_status_t sys_status;
            mavlink_msg_sys_status_decode(&msg, &sys_status);
            
            state.battery_voltage = sys_status.voltage_battery / 1000.0;  // mV para V
            state.battery_remaining = sys_status.battery_remaining;
            break;
        }
    }
}

// Enviar heartbeat para ArduPilot
void sendHeartbeat() {
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(
        1,  // System ID (computador complementar)
        1,  // Component ID
        &msg,
        MAV_TYPE_ONBOARD_CONTROLLER,
        MAV_AUTOPILOT_INVALID,
        0,  // Base mode
        0,  // Custom mode
        0   // System status
    );
    
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    ardupilotSerial.write(buffer, len);
}

// Publicar telemetria via MQTT (com HMAC)
void publishDroneTelemetry() {
    char jsonBuffer[512];
    char finalBuffer[600];
    StaticJsonDocument<512> doc;
    
    doc["droneId"] = CONFIG_DRONE_ID;
    doc["timestamp"] = millis();
    doc["location"] = {
        ["lat"] = state.latitude,
        ["lng"] = state.longitude,
        ["alt"] = state.altitude
    };
    doc["battery"] = {
        ["voltage"] = state.battery_voltage,
        ["remaining"] = state.battery_remaining
    };
    doc["status"] = {
        ["armed"] = state.is_armed,
        ["flying"] = state.is_flying,
        ["mode"] = state.flight_mode
    };
    
    serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
    
    String signature = calculateHMAC(jsonBuffer, CONFIG_HMAC_SECRET);
    doc["signature"] = signature.c_str();
    
    serializeJson(doc, finalBuffer, sizeof(finalBuffer));
    
    char topic[64];
    snprintf(topic, sizeof(topic), "drone/telemetry/%s", CONFIG_DRONE_ID);
    mqttClient.publish(topic, finalBuffer);
}

// Receber comandos do operador via MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    if (String(topic) == String("drone/command/") + CONFIG_DRONE_ID) {
        processCommand(message);
    }
}

void processCommand(String message) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) return;
    
    const char* command = doc["command"];
    
    if (strcmp(command, "arm") == 0) {
        sendArmCommand();
    } else if (strcmp(command, "disarm") == 0) {
        sendDisarmCommand();
    } else if (strcmp(command, "takeoff") == 0) {
        float altitude = doc["altitude"] | 10.0;
        sendTakeoffCommand(altitude);
    } else if (strcmp(command, "land") == 0) {
        sendLandCommand();
    } else if (strcmp(command, "return_to_launch") == 0) {
        sendRTLCommand();
    } else if (strcmp(command, "goto_waypoint") == 0) {
        float lat = doc["lat"];
        float lng = doc["lng"];
        float alt = doc["alt"];
        sendGotoWaypoint(lat, lng, alt);
    }
}

// Enviar comandos MAVLink para ArduPilot
void sendArmCommand() {
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 1, &msg,
        1,  // Target system (ArduPilot)
        1,
        MAV_CMD_COMPONENT_ARM_DISARM,  // Command
        0,
        1,
        0, 0, 0, 0, 0, 0
    );
    
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    ardupilotSerial.write(buffer, len);
    
    Serial.println("Comando ARM enviado");
}

void sendDisarmCommand() {
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 1, &msg,
        1, 1,
        MAV_CMD_COMPONENT_ARM_DISARM,
        0,
        0,  // Param 1 (0 = disarm)
        0, 0, 0, 0, 0, 0
    );
    
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    ardupilotSerial.write(buffer, len);
    
    Serial.println("Comando DISARM enviado");
}

void sendTakeoffCommand(float altitude) {
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 1, &msg,
        1, 1,
        MAV_CMD_NAV_TAKEOFF,
        0,
        0, 0, 0, 0,
        0, 0,
        altitude  // Altitude alvo
    );
    
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    ardupilotSerial.write(buffer, len);
    
    Serial.printf("Comando TAKEOFF para %.1f m enviado\n", altitude);
}

void sendLandCommand() {
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 1, &msg,
        1, 1,
        MAV_CMD_NAV_LAND,
        0,
        0, 0, 0, 0, 0, 0, 0
    );
    
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    ardupilotSerial.write(buffer, len);
    
    Serial.println("Comando LAND enviado");
}

void sendRTLCommand() {
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 1, &msg,
        1, 1,
        MAV_CMD_NAV_RETURN_TO_LAUNCH,
        0,
        0, 0, 0, 0, 0, 0, 0
    );
    
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    ardupilotSerial.write(buffer, len);
    
    Serial.println("Comando RETURN_TO_LAUNCH enviado");
}

void sendGotoWaypoint(float lat, float lng, float alt) {
    mavlink_message_t msg;
    mavlink_msg_mission_item_pack(
        1, 1, &msg,
        1, 1,
        0,
        MAV_FRAME_GLOBAL_RELATIVE_ALT,
        MAV_CMD_NAV_WAYPOINT,
        0,
        1,
        0, 0, 0, 0,
        lat,
        lng,
        alt
    );
    
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    ardupilotSerial.write(buffer, len);
    
    Serial.printf("Waypoint enviado: Lat=%.6f, Lng=%.6f, Alt=%.1f\n", lat, lng, alt);
}

void initWiFi() {
    WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado");
}

void initMQTT() {
    mqttClient.setServer(CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    
    while (!mqttClient.connected()) {
        if (mqttClient.connect(CONFIG_DRONE_ID, CONFIG_MQTT_USER, CONFIG_MQTT_PASS)) {
            Serial.println("MQTT conectado");
            
            char commandTopic[64];
            snprintf(commandTopic, sizeof(commandTopic), "drone/command/%s", CONFIG_DRONE_ID);
            mqttClient.subscribe(commandTopic);
        } else {
            delay(5000);
        }
    }
}
