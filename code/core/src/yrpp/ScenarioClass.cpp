// YRpp 9402d7da; RA1 SCENARIO.CPP has a different waypoint representation.
// Target 68BD80/68BCC0; sentinel is initialized to (0,0) by 683210.
#include "yrpp/YRPPCore.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/CCINIClass.h"
#include <algorithm>
#include <cstdio>

// 689D30: map header resource fields only; no scenario startup or renderer.
void ScenarioClass::ReadStartPoints(INIClass& ini) {
    StartX = ini.ReadInteger("Header", "StartX", -1);
    StartY = ini.ReadInteger("Header", "StartY", -1);
    Width = ini.ReadInteger("Header", "Width", -1);
    Height = ini.ReadInteger("Header", "Height", -1);
    NumberStartingPoints = ini.ReadInteger("Header", "NumberStartingPoints", -1);
    NumCoopHumanStartSpots = ini.ReadInteger("Header", "NumCoopHumanStartSpots", NumCoopHumanStartSpots);
    for (auto& point : StartingPoints) point = {0, 0};
    // Retain the supplied metadata, but never let malformed counts overwrite
    // HouseIndices and the rest of the live Scenario object (original bug).
    for (int i = 0; i < std::min(NumberStartingPoints, 8); ++i) {
        char key[16]; std::snprintf(key, sizeof(key), "Waypoint%d", i + 1);
        ini.ReadPoint2D(StartingPoints[i], "Header", key, StartingPoints[i]);
    }
}

bool ScenarioClass::IsDefinedWaypoint(int index) {
    return static_cast<unsigned>(index) < 702u &&
        (Waypoints[index].X != 0 || Waypoints[index].Y != 0);
}
CellStruct* ScenarioClass::GetWaypointCoords(CellStruct* dest, int index) {
    *dest = Waypoints[index]; // Original requires a valid index and destination.
    return dest;
}

// Non-template interface helpers; bodies retained from the corresponding header.

CellStruct ScenarioClass::GetWaypointCoords(int idx)
{
    CellStruct dest;
    GetWaypointCoords(&dest, idx);
    return dest;
}
