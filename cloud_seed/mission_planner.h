#ifndef MISSION_PLANNER_H
#define MISSION_PLANNER_H
#include <stdint.h>

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
