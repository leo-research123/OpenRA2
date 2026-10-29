// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 house.cpp Waypoint_At / Fetch_Waypoint_Data / Ensure_Path.
// Electronic Arts / OpenTS; EA Section 7: third_party/opents/LICENSE.md.
// YR 0x005023B0 / 0x00502460 / 0x00504740.
#include "yrpp/HouseClass.h"
#include "yrpp/WaypointPathClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/BuildingClass.h"
#if !defined(RA2_YRPP_GAME)
bool HouseClass::CanAddPlanningWaypoint() const {
    if (unsigned(SelectedPathIndex) >= 12) return false;
    const auto* path = PlanningPaths[SelectedPathIndex];
    return path->Waypoints.Count < RulesClass::Instance->MaxWaypointPathLength && path->CurrentWaypointIndex == -1;
}
void HouseClass::PointerExpired(AbstractClass* object, bool removed) {
    if (ToCapture == object) ToCapture = nullptr;
    // 0x4FB9C9..0x4FBA1B removes the first matching Tag even when removed=false.
    if (auto* tag = dynamic_cast<TagClass*>(object)) RelatedTags.Remove(tag);
    if (object->WhatAmI() == AbstractType::Building && removed) {
        auto* building = static_cast<BuildingClass*>(object);
        ConYards.Remove(building);
        UnitRepairStations.Remove(building);
        Grinders.Remove(building);
        Absorbers.Remove(building);
        Bunkers.Remove(building);
        Occupiables.Remove(building);
        CloningVats.Remove(building);
        SecretLabs.Remove(building);
        PsychicDetectionBuildings.Remove(building);
        FactoryPlants.Remove(building);
        Buildings.Remove(building);
        CalculateCostMultipliers();
    }
    for (auto*& path : PlanningPaths)
        if (path == object) path = nullptr;
    // 0x4FB9B0 also detaches every production slot, independent of removal.
    FactoryClass** factories[]{&Primary_ForAircraft, &Primary_ForInfantry,  &Primary_ForVehicles,
                               &Primary_ForShips,    &Primary_ForBuildings, &Primary_Unused1,
                               &Primary_Unused2,     &Primary_Unused3,      &Primary_ForDefenses};
    for (auto** factory : factories)
        if (*factory == object) *factory = nullptr;
}
WaypointPathClass* HouseClass::EnsurePlanningPathExists(int index) noexcept {
    if (!PlanningPaths[index]) {
        // 0x00504740 uses nullable operator new, not the checked/fatal wrapper.
        // The original query callers require this allocation to have succeeded.
        if (void* storage = YRMemory::Allocate(sizeof(WaypointPathClass)))
            PlanningPaths[index] = ::new (storage) WaypointPathClass(index);
    }
    return PlanningPaths[index];
}
WaypointClass* HouseClass::GetPlanningWaypointAt(CellStruct* cell) {
    for (int path = 0; path < 12; ++path) {
        EnsurePlanningPathExists(path);
        for (int i = 0; i < PlanningPaths[path]->Waypoints.Count; ++i) {
            auto* waypoint = PlanningPaths[path]->GetWaypoint(i);
            if (static_cast<short>(waypoint->Coords.X / 256) == cell->X &&
                static_cast<short>(waypoint->Coords.Y / 256) == cell->Y)
                return waypoint;
        }
    }
    return nullptr;
}
bool HouseClass::GetPlanningWaypointProperties(WaypointClass* waypoint, int& path, BYTE& index) {
    for (int p = 0; p < 12; ++p) {
        EnsurePlanningPathExists(p);
        for (int i = 0; i < PlanningPaths[p]->Waypoints.Count; ++i)
            if (PlanningPaths[p]->GetWaypoint(i) == waypoint) {
                path = p;
                index = static_cast<BYTE>(i);
                return true;
            }
    }
    path = -1;
    index = 0;
    return false;
}
#endif
