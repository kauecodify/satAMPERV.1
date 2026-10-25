// cloud_seed/mission_executor.h
#ifndef MISSION_EXECUTOR_H
#define MISSION_EXECUTOR_H
#pragma once

#include "mission_planner.h"

typedef enum { STATE_IDLE, STATE_NAVIGATING, STATE_RETURNING_HOME } mission_state_t;
class MissionExecutor {
public:
    MissionExecutor(MissionPlanner* p);
    void update(float, float, float, float);
    mission_state_t getState();
};

#include "mission_planner.h"

typedef enum {
    STATE_IDLE,
    STATE_TAKING_OFF,
    STATE_NAVIGATING,
    STATE_RETURNING_HOME,
    STATE_LANDING
} mission_state_t;

class MissionExecutor {
private:
    mission_state_t current_state;
    MissionPlanner* planner;
    float current_battery_pct;

    void checkFailsafes();
    void commandNextWaypoint();

public:
    MissionExecutor(MissionPlanner* p);
    void update(float current_lat, float current_lng, float current_alt, float battery_pct);
    mission_state_t getState();
};

#endif // MISSION_EXECUTOR_H
