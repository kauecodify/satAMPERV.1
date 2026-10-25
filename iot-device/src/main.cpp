#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include "config.h"
#include "sensor_driver.h"
#include "crypto.h"
#include "comm_manager.h"


void processGovernanceProposal(DynamicJsonDocument& doc);
void processConfigCommand(DynamicJsonDocument& doc);
void publishDeviceStatus(const char* status);
void publishVote(const char* proposalId, int vote);

const char* WIFI_SSID = CONFIG_WIFI_SSID;
const char* WIFI_PASSWORD = CONFIG_WIFI_PASSWORD;
const char* MQTT_BROKER = CONFIG_MQTT_BROKER;
const int MQTT_PORT = CONFIG_MQTT_PORT;
const char* MQTT_USER = CONFIG_MQTT_USER;
const char* MQTT_PASS = CONFIG_MQTT_PASS;
const char* DEVICE_ID = CONFIG_DEVICE_ID;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastTelemetry = 0;
unsigned long lastGovernanceCheck = 0;
unsigned long lastStatus = 0;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.printf("Erro ao parsear MQTT: %s\n", error.c_str());
        return;
    }
    
    Serial.printf("Mensagem recebida no tópico: %s\n", topic);
    
    if (strcmp(topic, "governance/proposals") == 0) {
        processGovernanceProposal(doc);
    } else {
        char expectedTopic[64];
        snprintf(expectedTopic, sizeof(expectedTopic), "devices/%s/config", DEVICE_ID);
        if (strcmp(topic, expectedTopic) == 0) {
            processConfigCommand(doc);
        }
    }
}

void connectWiFi() {
    Serial.printf("Conectando ao WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi conectado! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nFalha ao conectar ao WiFi. Reiniciando...");
        ESP.restart();
    }
}

void connectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Conectando ao MQTT...");
        
        char clientId[64];
        snprintf(clientId, sizeof(clientId), "device-%s", DEVICE_ID);
        
        if (mqttClient.connect(clientId, MQTT_USER, MQTT_PASS)) {
            Serial.println("conectado!");
            
            mqttClient.subscribe("governance/proposals");
            
            char configTopic[64];
            snprintf(configTopic, sizeof(configTopic), "devices/%s/config", DEVICE_ID);
            mqttClient.subscribe(configTopic);
            
            publishDeviceStatus("online");
        } else {
            Serial.printf("falha, rc=%d. Tentando novamente em 5s\n", mqttClient.state());
            delay(5000);
        }
    }
}

void publishTelemetry() {
    sensor_data_t data = read_sensor_data();
    
    char jsonBuffer[512];
    char finalBuffer[600];
    char topic[64];
    StaticJsonDocument<512> doc;
    
    doc["deviceId"] = DEVICE_ID;
    doc["timestamp"] = millis();
    doc["power"] = data.power;
    doc["voltage"] = data.voltage;
    doc["current"] = data.current;
    doc["temperature"] = data.temperature;
    doc["status"] = (data.status == SENSOR_OK) ? "normal" : "warning";
    
    serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
    
    String signature = calculateHMAC(jsonBuffer, CONFIG_HMAC_SECRET);
    doc["signature"] = signature.c_str();
    
    serializeJson(doc, finalBuffer, sizeof(finalBuffer));
    
    snprintf(topic, sizeof(topic), "telemetry/%s", DEVICE_ID);
    mqttClient.publish(topic, finalBuffer);
    
    Serial.printf("Telemetria publicada: %.2f kW, %.1f V, %.2f A, %.1f °C | Assinatura: %s\n",
                  data.power, data.voltage, data.current, data.temperature, signature.c_str());
}

void processGovernanceProposal(DynamicJsonDocument& doc) {
    const char* proposalId = doc["id"] | "unknown";
    const char* description = doc["description"] | "";
    const char* optionX = doc["optionX"] | "";
    const char* optionY = doc["optionY"] | "";
    unsigned long expiresAt = doc["expiresAt"] | 0;
    
    Serial.printf("\n=== PROPOSTA DE GOVERNANÇA ===\n");
    Serial.printf("ID: %s\n", proposalId);
    Serial.printf("Descrição: %s\n", description);
    Serial.printf("Opção X: %s | Opção Y: %s\n", optionX, optionY);
    Serial.printf("Expira em: %lu ms\n", expiresAt);
    
    sensor_data_t data = read_sensor_data();
    
    int vote = (data.temperature > 35.0) ? 1 : 0;
    Serial.println(vote == 1 ? "Decisão: Votar Y (temperatura alta)" : "Decisão: Votar X (temperatura normal)");
    
    publishVote(proposalId, vote);
}

void publishVote(const char* proposalId, int vote) {
    char jsonBuffer[256];
    char finalBuffer[300];
    StaticJsonDocument<256> doc;
    
    doc["proposalId"] = proposalId;
    doc["deviceId"] = DEVICE_ID;
    doc["vote"] = vote;
    doc["timestamp"] = millis();
    
    serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
    
    String signature = calculateHMAC(jsonBuffer, CONFIG_HMAC_SECRET);
    doc["signature"] = signature.c_str();
    
    serializeJson(doc, finalBuffer, sizeof(finalBuffer));
    
    mqttClient.publish("governance/votes", finalBuffer);
    
    Serial.printf("Voto publicado: Proposta %s, Voto: %s\n", proposalId, vote == 0 ? "X" : "Y");
}

void processConfigCommand(DynamicJsonDocument& doc) {
    const char* command = doc["command"] | "";
    
    if (strcmp(command, "set_sampling_rate") == 0) {
        int rate = doc["value"] | 100;
        set_sensor_sampling_rate(rate);
        Serial.printf("Taxa de amostragem alterada para: %d ms\n", rate);
    } else if (strcmp(command, "calibrate") == 0) {
        calibrate_sensor();
        Serial.println("Sensor calibrado");
    } else if (strcmp(command, "restart") == 0) {
        Serial.println("Reiniciando dispositivo...");
        delay(1000);
        ESP.restart();
    }
}

void publishDeviceStatus(const char* status) {
    char jsonBuffer[256];
    char topic[64];
    StaticJsonDocument<256> doc;
    
    doc["deviceId"] = DEVICE_ID;
    doc["status"] = status;
    doc["uptime"] = millis();
    doc["wifiRSSI"] = WiFi.RSSI();
    doc["freeHeap"] = ESP.getFreeHeap();
    
    serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
    
    snprintf(topic, sizeof(topic), "devices/%s/status", DEVICE_ID);
    mqttClient.publish(topic, jsonBuffer);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== IoT Energy Monitor Device ===");
    Serial.printf("Device ID: %s\n", DEVICE_ID);
    Serial.printf("Free Heap inicial: %u bytes\n", ESP.getFreeHeap());
    
    init_sensor();
    connectWiFi();
    
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(1024);
    
    connectMQTT();
    
    Serial.println("Dispositivo inicializado e pronto!");
}

void loop() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
    
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastTelemetry >= 2000) {
        publishTelemetry();
        lastTelemetry = currentMillis;
    }
    
    if (currentMillis - lastGovernanceCheck >= 5000) {
        lastGovernanceCheck = currentMillis;
    }
    
    if (currentMillis - lastStatus >= 30000) {
        publishDeviceStatus("online");
        lastStatus = currentMillis;
    }
    
    delay(10);
}

CommManager comm;

extern "C" void app_main() {
    // ... inicialização existente (NVS, sensores, Wi-Fi, MQTT) ...

    // Inicializa o gerenciador de comunicação (que por sua vez inicializa o Swarm)
    comm.begin();

    while (true) {
        // 1. Lê os sensores (código existente em sensor_driver.c) [5†L20-L21]
        SensorData data = read_sensors();

        // 2. Em vez de montar JSON e publicar no MQTT diretamente,
        //    delegamos para o CommManager, que fará a codificação CBOR
        //    e escolherá o melhor link (Wi-Fi ou Satélite).
        comm.publishTelemetry(data);

        vTaskDelay(pdMS_TO_TICKS(60000)); // Intervalo de amostragem
    }
}
