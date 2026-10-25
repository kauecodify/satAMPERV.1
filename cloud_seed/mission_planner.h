#ifndef MISSION_PLANNER_H
#define MISSION_PLANNER_H
#include <stdint.h>
#pragma once

struct Waypoint { float lat, lng, alt; bool valid; };
class MissionPlanner {
public:
    Waypoint* getNextWaypoint();
    bool advanceToNextWaypoint();
    bool hasReachedWaypoint(float, float, float, float = 5.0);
};

struct Waypoint { float latitude, longitude, altitude; bool valid; };
class MissionPlanner {
public:
    bool addWaypoint(float lat, float lng, float alt);
    Waypoint* getNextWaypoint();
    bool advanceToNextWaypoint();
    bool hasReachedWaypoint(float cur_lat, float cur_lng, float cur_alt, float threshold = 5.0);
    bool isInsideGeofence(float lat, float lng);
};
#endif
