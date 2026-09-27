// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 house.cpp Waypoint_At / Fetch_Waypoint_Data / Ensure_Path.
// Electronic Arts / OpenTS; EA Section 7: third_party/opents/LICENSE.md.
// YR 0x005023B0 / 0x00502460 / 0x00504740.
#include "yrpp/HouseClass.h"
#include "yrpp/WaypointPathClass.h"
#if !defined(RA2_YRPP_GAME)
void HouseClass::PointerExpired(AbstractClass* object,bool) {
    // The native class previously inherited Abstract's no-op. Restore the path
    // ownership arm needed by these queries; this is not the full House detach.
    for(auto*& path:PlanningPaths)if(path==object)path=nullptr;
}
WaypointPathClass* HouseClass::EnsurePlanningPathExists(int index) noexcept {
    if(!PlanningPaths[index]) {
        // 0x00504740 uses nullable operator new, not the checked/fatal wrapper.
        // The original query callers require this allocation to have succeeded.
        if(void* storage=YRMemory::Allocate(sizeof(WaypointPathClass)))
            PlanningPaths[index]=::new(storage) WaypointPathClass(index);
    }
    return PlanningPaths[index];
}
WaypointClass* HouseClass::GetPlanningWaypointAt(CellStruct* cell) {
    for(int path=0;path<12;++path) {
        EnsurePlanningPathExists(path);
        for(int i=0;i<PlanningPaths[path]->Waypoints.Count;++i) {
            auto* waypoint=PlanningPaths[path]->GetWaypoint(i);
            if(static_cast<short>(waypoint->Coords.X/256)==cell->X &&
                static_cast<short>(waypoint->Coords.Y/256)==cell->Y)return waypoint;
        }
    }
    return nullptr;
}
bool HouseClass::GetPlanningWaypointProperties(WaypointClass* waypoint,int& path,BYTE& index) {
    for(int p=0;p<12;++p) {
        EnsurePlanningPathExists(p);
        for(int i=0;i<PlanningPaths[p]->Waypoints.Count;++i)
            if(PlanningPaths[p]->GetWaypoint(i)==waypoint) {
                path=p;index=static_cast<BYTE>(i);return true;
            }
    }
    path=-1;index=0;return false;
}
#endif
