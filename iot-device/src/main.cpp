#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include "config.h"
#include "sensor_driver.h"
#include "crypto.h"

// Configurações de rede
const char* WIFI_SSID = CONFIG_WIFI_SSID;
const char* WIFI_PASSWORD = CONFIG_WIFI_PASSWORD;
const char* MQTT_BROKER = CONFIG_MQTT_BROKER;
const int MQTT_PORT = CONFIG_MQTT_PORT;
const char* MQTT_USER = CONFIG_MQTT_USER;
const char* MQTT_PASS = CONFIG_MQTT_PASS;
const char* DEVICE_ID = CONFIG_DEVICE_ID;

// Clientes
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Variáveis globais
unsigned long lastTelemetry = 0;
unsigned long lastGovernanceCheck = 0;
bool isVoting = false;
String currentProposalId = "";
int currentVote = -1; // -1: não votou, 0: opção X, 1: opção Y

// Callback para mensagens MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.printf("Mensagem recebida [%s]: %s\n", topic, message.c_str());
    
    // Processar proposta de governança
    if (String(topic) == "governance/proposals") {
        processGovernanceProposal(message);
    }
    
    // Processar comando de configuração
    if (String(topic) == String("devices/") + DEVICE_ID + "/config") {
        processConfigCommand(message);
    }
}

// Conectar ao WiFi
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
        
        String clientId = String("device-") + DEVICE_ID;
        
        if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
            Serial.println("conectado!");
            
            mqttClient.subscribe("governance/proposals");
            mqttClient.subscribe(String("devices/") + DEVICE_ID + "/config");
            
            publishDeviceStatus("online");
        } else {
            Serial.printf("falha, rc=%d. Tentando novamente em 5s\n", mqttClient.state());
            delay(5000);
        }
    }
}

void publishTelemetry() {
    sensor_data_t data = read_sensor_data();
    
    StaticJsonDocument<512> doc;
    doc["deviceId"] = DEVICE_ID;
    doc["timestamp"] = millis();
    doc["power"] = data.power;
    doc["voltage"] = data.voltage;
    doc["current"] = data.current;
    doc["temperature"] = data.temperature;
    doc["status"] = data.status == SENSOR_OK ? "normal" : "warning";
    
    // HMAC-SHA256
    String payload;
    serializeJson(doc, payload);
    String signature = calculateHMAC(payload.c_str(), CONFIG_HMAC_SECRET);
    
    doc["signature"] = signature;
    
    String finalPayload;
    serializeJson(doc, finalPayload);
    
    String topic = String("telemetry/") + DEVICE_ID;
    mqttClient.publish(topic.c_str(), finalPayload.c_str());
    
    Serial.printf("Telemetria publicada: %.2f kW, %.1f V, %.2f A, %.1f °C\n",
                  data.power, data.voltage, data.current, data.temperature);
}

void processGovernanceProposal(String message) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.printf("Erro ao parsear proposta: %s\n", error.c_str());
        return;
    }
    
    String proposalId = doc["id"];
    String description = doc["description"];
    String optionX = doc["optionX"];
    String optionY = doc["optionY"];
    unsigned long expiresAt = doc["expiresAt"];
    
    Serial.printf("\n=== PROPOSTA DE GOVERNANÇA ===\n");
    Serial.printf("ID: %s\n", proposalId.c_str());
    Serial.printf("Descrição: %s\n", description.c_str());
    Serial.printf("Opção X: %s\n", optionX.c_str());
    Serial.printf("Opção Y: %s\n", optionY.c_str());
    Serial.printf("Expira em: %lu ms\n", expiresAt);
    
    sensor_data_t data = read_sensor_data();
    
    int vote;
    if (data.temperature > 35.0) {
        vote = 1;
        Serial.println("Decisão: Votar Y (temperatura alta)");
    } else {
        vote = 0;
        Serial.println("Decisão: Votar X (temperatura normal)");
    }
    
    publishVote(proposalId, vote);
}

void publishVote(String proposalId, int vote) {
    StaticJsonDocument<256> doc;
    doc["proposalId"] = proposalId;
    doc["deviceId"] = DEVICE_ID;
    doc["vote"] = vote; // 0: X, 1: Y
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    
    String signature = calculateHMAC(payload.c_str(), CONFIG_HMAC_SECRET);
    doc["signature"] = signature;
    
    String finalPayload;
    serializeJson(doc, finalPayload);
    
    mqttClient.publish("governance/votes", finalPayload.c_str());
    
    Serial.printf("Voto publicado: Proposta %s, Voto: %s\n", 
                  proposalId.c_str(), vote == 0 ? "X" : "Y");
}

void processConfigCommand(String message) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.printf("Erro ao parsear comando: %s\n", error.c_str());
        return;
    }
    
    String command = doc["command"];
    
    if (command == "set_sampling_rate") {
        int rate = doc["value"];
        set_sensor_sampling_rate(rate);
        Serial.printf("Taxa de amostragem alterada para: %d ms\n", rate);
    } else if (command == "calibrate") {
        calibrate_sensor();
        Serial.println("Sensor calibrado");
    } else if (command == "restart") {
        Serial.println("Reiniciando dispositivo...");
        delay(1000);
        ESP.restart();
    }
}

void publishDeviceStatus(String status) {
    StaticJsonDocument<256> doc;
    doc["deviceId"] = DEVICE_ID;
    doc["status"] = status;
    doc["uptime"] = millis();
    doc["wifiRSSI"] = WiFi.RSSI();
    doc["freeHeap"] = ESP.getFreeHeap();
    
    String payload;
    serializeJson(doc, payload);
    
    mqttClient.publish(String("devices/") + DEVICE_ID + "/status", payload.c_str());
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== IoT Energy Monitor Device ===");
    Serial.printf("Device ID: %s\n", DEVICE_ID);
    
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
    
    // Publicar telemetria a cada 2 segundos
    if (millis() - lastTelemetry >= 2000) {
        publishTelemetry();
        lastTelemetry = millis();
    }
    
    // Verificar governança a cada 5 segundos
    if (millis() - lastGovernanceCheck >= 5000) {
        // Lógica de verificação de propostas ativas
        lastGovernanceCheck = millis();
    }
    
    // Publicar status a cada 3 segundos
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus >= 3000) {
        publishDeviceStatus("online");
        lastStatus = millis();
    }
    
    delay(10);
}
