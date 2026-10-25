// mission_planner.cpp - Sistema de missão com geofencing
#include <Arduino.h>
#include <ArduinoJson.h>
#include <math.h>

#define MAX_WAYPOINTS 50
#define EARTH_RADIUS 6371000  // Raio da Terra em metros

// Estrutura de waypoint
struct Waypoint {
    float latitude;
    float longitude;
    float altitude;
    float hold_time;  // Tempo de espera no waypoint (segundos)
    bool valid;
};

// Zona de geofencing (polígono)
struct GeofenceZone {
    float vertices_lat[10];
    float vertices_lng[10];
    int num_vertices;
};

class MissionPlanner {
private:
    Waypoint waypoints[MAX_WAYPOINTS];
    int current_waypoint_index;
    int total_waypoints;
    GeofenceZone geofence;
    bool mission_active;
    
public:
    MissionPlanner() {
        current_waypoint_index = 0;
        total_waypoints = 0;
        mission_active = false;
        clearWaypoints();
    }
    
    // Limpar todos os waypoints
    void clearWaypoints() {
        for (int i = 0; i < MAX_WAYPOINTS; i++) {
            waypoints[i].valid = false;
        }
        total_waypoints = 0;
        current_waypoint_index = 0;
    }
    
    // Adicionar waypoint à rota
    bool addWaypoint(float lat, float lng, float alt, float hold_time = 0) {
        if (total_waypoints >= MAX_WAYPOINTS) {
            Serial.println("Erro: Limite de waypoints atingido");
            return false;
        }
        
        // Verificar se está dentro do geofence
        if (!isInsideGeofence(lat, lng)) {
            Serial.printf("Erro: Waypoint (%.6f, %.6f) fora do geofence\n", lat, lng);
            return false;
        }
        
        waypoints[total_waypoints].latitude = lat;
        waypoints[total_waypoints].longitude = lng;
        waypoints[total_waypoints].altitude = alt;
        waypoints[total_waypoints].hold_time = hold_time;
        waypoints[total_waypoints].valid = true;
        
        total_waypoints++;
        Serial.printf("Waypoint %d adicionado: (%.6f, %.6f, %.1f)\n", 
                      total_waypoints, lat, lng, alt);
        return true;
    }
    
    // Obter próximo waypoint
    Waypoint* getNextWaypoint() {
        if (current_waypoint_index >= total_waypoints) {
            return nullptr;  // Missão concluída
        }
        
        return &waypoints[current_waypoint_index];
    }
    
    // Avançar para o próximo waypoint
    bool advanceToNextWaypoint() {
        if (current_waypoint_index < total_waypoints) {
            current_waypoint_index++;
            return true;
        }
        return false;
    }
    
    // Verificar se drone chegou ao waypoint atual
    bool hasReachedWaypoint(float current_lat, float current_lng, float current_alt, float threshold = 5.0) {
        if (current_waypoint_index >= total_waypoints) {
            return false;
        }
        
        Waypoint* wp = &waypoints[current_waypoint_index];
        
        float distance = calculateDistance(current_lat, current_lng, wp->latitude, wp->longitude);
        float altitude_diff = abs(current_alt - wp->altitude);
        
        return (distance <= threshold && altitude_diff <= 2.0);
    }
    
    // Definir zona de geofencing
    void setGeofence(float* lat_vertices, float* lng_vertices, int num_vertices) {
        if (num_vertices > 10) {
            Serial.println("Erro: Geofence limitado a 10 vértices");
            return;
        }
        
        geofence.num_vertices = num_vertices;
        for (int i = 0; i < num_vertices; i++) {
            geofence.vertices_lat[i] = lat_vertices[i];
            geofence.vertices_lng[i] = lng_vertices[i];
        }
        
        Serial.printf("Geofence definido com %d vértices\n", num_vertices);
    }
    
    // Verificar se coordenada está dentro do geofence
    bool isInsideGeofence(float lat, float lng) {
        if (geofence.num_vertices < 3) {
            return true;  // Geofence não configurado, permite qualquer posição
        }
        
        // Algoritmo ray-casting para verificar se ponto está dentro do polígono
        int intersections = 0;
        
        for (int i = 0; i < geofence.num_vertices; i++) {
            int j = (i + 1) % geofence.num_vertices;
            
            float yi = geofence.vertices_lat[i];
            float xi = geofence.vertices_lng[i];
            float yj = geofence.vertices_lat[j];
            float xj = geofence.vertices_lng[j];
            
            if (((yi <= lng && lng < yj) || (yj <= lng && lng < yi)) &&
                (lat < (xj - xi) * (lng - yi) / (yj - yi) + xi)) {
                intersections++;
            }
        }
        
        return (intersections % 2) == 1;
    }
    
    // Calcular distância entre dois pontos (fórmula de Haversine)
    float calculateDistance(float lat1, float lng1, float lat2, float lng2) {
        float dLat = radians(lat2 - lat1);
        float dLng = radians(lng2 - lng1);
        
        float a = sin(dLat / 2) * sin(dLat / 2) +
                  cos(radians(lat1)) * cos(radians(lat2)) *
                  sin(dLng / 2) * sin(dLng / 2);
        
        float c = 2 * atan2(sqrt(a), sqrt(1 - a));
        
        return EARTH_RADIUS * c;
    }
    
    // Iniciar missão
    void startMission() {
        if (total_waypoints == 0) {
            Serial.println("Erro: Nenhum waypoint definido");
            return;
        }
        
        current_waypoint_index = 0;
        mission_active = true;
        Serial.printf("Missão iniciada com %d waypoints\n", total_waypoints);
    }
    
    // Verificar se missão está ativa
    bool isMissionActive() {
        return mission_active;
    }
    
    // Obter progresso da missão
    float getMissionProgress() {
        if (total_waypoints == 0) return 0;
        return (float)current_waypoint_index / total_waypoints * 100.0;
    }
};

// Instância global
MissionPlanner mission;
