// Original YR movement handoffs: 0x4D55F0, 0x4DB9B0 and 0x4DDC60.
#include "yrpp/FootClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/MapClass.h"
#include <cstdlib>

bool FootClass::MoveTo(CoordStruct* where) {
    if (!Locomotor) std::abort();
    Locomotor->Move_To(*where);
    if (!Locomotor) std::abort();
    return Locomotor->Is_Moving();
}

#if !defined(RA2_YRPP_GAME)
void FootClass::QueueNavigationList(AbstractClass* target) {
    if (!target) return;
    if (target == this && NavQueue.Count > 0)
        unknown_bool_6B1 = true;
    else {
        if (!NavQueue.Count) unknown_bool_6B1 = false;
        NavQueue.AddItem(target);
    }
    if (!Destination && GetCurrentMission() == Mission::Guard) {
        auto* link = GetNthLink();
        if (!link || link->WhatAmI() != AbstractType::Building ||
            !static_cast<BuildingClass*>(link)->Type->WeaponsFactory)
            EnterIdleMode(false, true);
    }
}
#endif

bool FootClass::TryEnterIdle() {
    if (Destination || Target) return false;
    auto* link = GetNthLink();
    const auto* building =
        link && link->WhatAmI() == AbstractType::Building ? static_cast<BuildingClass*>(link) : nullptr;
    const bool repairing = CurrentMission == Mission::Enter && building && building->Type->UnitRepair;
    return EnterIdleMode(false, !repairing);
}

bool FootClass::IsLandZoneClear(AbstractClass* destination) {
    if (!destination) return false;
    const auto coord = destination->GetCoords();
    const CellStruct cell{short(coord.X / 256), short(coord.Y / 256)};
    auto& map = MapClass::Instance;
    if (!map.IsWithinUsableArea(cell, true)) return false;
    auto* occupant = map.GetCellAt(cell)->FindTechnoNearestTo({0, 0}, false, nullptr);
    if (occupant) {
        return occupant == this || occupant == GetNthLink() ||
               (GetTechnoType()->Spawned && (occupant->SpawnManager || occupant->GetTechnoType()->Spawned));
    }
    if (!map.GetCellAt(cell)->IsClearToMove(SpeedType::Track, false, false, -1, MovementZone::Normal, -1, true))
        return false;
    for (auto* aircraft : AircraftClass::Array) {
        if (aircraft->IsAlive && !aircraft->InLimbo && aircraft->Destination == destination && aircraft != this)
            return false;
    }
    return true;
}
