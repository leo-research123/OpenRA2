// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp Set_Waypoint_Path; YR 0x004DC810.
// Electronic Arts / OpenTS; EA Section 7: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/WaypointPathClass.h"
void FootClass::AssignPlanningPath(int path,signed char index) {
    PlanningPathIdx=path;
    if(path==-1) {
        WaypointIndex=0;WaypointCell={0,0};
    } else {
        WaypointIndex=index;
        auto* point=HouseClass::CurrentPlayer->PlanningPaths[path]->GetWaypoint(index);
        WaypointCell={static_cast<short>(point->Coords.X/256),static_cast<short>(point->Coords.Y/256)};
    }
    WaypointNearbyAccessibleCellDelta={0,0};
}
