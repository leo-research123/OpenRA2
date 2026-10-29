// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp Set_Waypoint_Path; YR 0x004DC810.
// Electronic Arts / OpenTS; EA Section 7: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/WaypointPathClass.h"
void FootClass::AssignPlanningPath(int path, signed char index) {
    PlanningPathIdx = path;
    if (path == -1) {
        WaypointIndex = 0;
        WaypointCell = {0, 0};
    } else {
        WaypointIndex = index;
        auto* point = HouseClass::CurrentPlayer->PlanningPaths[path]->GetWaypoint(index);
        WaypointCell = {static_cast<short>(point->Coords.X / 256), static_cast<short>(point->Coords.Y / 256)};
    }
    WaypointNearbyAccessibleCellDelta = {0, 0};
}

#if !defined(RA2_YRPP_GAME)
void FootClass::ProceedToNextPlanningWaypoint() {
    if (PlanningPathIdx == -1) return;
    auto* path = HouseClass::CurrentPlayer->PlanningPaths[PlanningPathIdx];
    auto* current = path->GetWaypoint(WaypointIndex);
    auto* next = HouseClass::CurrentPlayer->PlanningPaths[PlanningPathIdx]->GetWaypointAfter(current);
    FollowWaypoint(next);
}
#endif

// OpenTS Execute_Waypoint_Path, calibrated to YR 0x4DC8C0.
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/AStarClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <cstdlib>
#if !defined(RA2_YRPP_GAME)
bool FootClass::FollowWaypoint(WaypointClass* waypoint) {
    bool success = false;
    if (unknown_bool_430) unknown_bool_43A = true;
    if (waypoint) {
        HouseClass::CurrentPlayer->GetPlanningWaypointProperties(waypoint, PlanningPathIdx,
                                                                 reinterpret_cast<BYTE&>(WaypointIndex));
        auto at = waypoint->Coords;
        auto* occupying = MapClass::Instance.GetCellAt(at)->FirstObject;
        auto* techno = occupying && (occupying->AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None
                           ? static_cast<TechnoClass*>(occupying)
                           : nullptr;
        auto cell = CellClass::Coord2Cell(at);
        WaypointCell = cell;
        const bool feedback = Unsorted::MoveFeedback;
        Unsorted::MoveFeedback = false;
        bool direct = true, clickObject = false;
        if (techno) {
            const auto action = MouseOverObject(occupying, true);
            if (HouseClass::CurrentPlayer->IsAlliedWith(techno->Owner)) {
                if (techno->WhatAmI() == AbstractType::Building) {
                    const auto* building = static_cast<BuildingClass*>(techno)->Type;
                    const bool repair = building->UnitRepair && Health < GetTechnoType()->Strength &&
                                        WhatAmI() != AbstractType::Infantry;
                    const bool reload = building->UnitReload && Ammo < GetTechnoType()->InitialAmmo && Ammo != -1;
                    if ((repair || reload) && techno->Owner->IsAlliedWith(HouseClass::CurrentPlayer) &&
                        int(action) != 7 && int(action) != 8)
                        clickObject = true;
                }
            } else if (int(action) != 8 && int(action) != 7 && int(action) != 1 && int(action) != 2)
                clickObject = true;
            if (!clickObject) {
                CellStruct nearby;
                const auto zone = GetTechnoType()->MovementZone;
                const int zoneIndex = MapClass::Instance.GetMovementZoneType(
                    cell, zone, MapClass::Instance.GetCellAt(cell)->ContainsBridgeEx());
                MapClass::Instance.NearByLocation(nearby, cell, GetTechnoType()->SpeedType, zoneIndex, zone, false, 1,
                                                  1, false, true, false, true, CellStruct::Empty, false, false);
                if (nearby != CellStruct::Empty) {
                    WaypointNearbyAccessibleCellDelta = {short(nearby.X - cell.X), short(nearby.Y - cell.Y)};
                    direct = false;
                }
            }
        }
        if (clickObject) {
            success = ObjectClickedAction(MouseOverObject(occupying, true), occupying, true);
            WaypointNearbyAccessibleCellDelta = {0, 0};
        } else {
            const auto action = MouseOverCell(&cell, false, true);
            const auto delta = WaypointNearbyAccessibleCellDelta;
            CellStruct adjusted{short(cell.X + delta.X), short(cell.Y + delta.Y)};
            auto* adjustedCell = MapClass::Instance.GetCellAt(adjusted);
            auto* waypointCell = MapClass::Instance.GetCellAt(cell);
            const auto adjustedAction = MouseOverCell(&adjusted, false, true);
            const auto zone = GetTechnoType()->MovementZone;
            const int waypointZone =
                MapClass::Instance.GetMovementZoneType(cell, zone, waypointCell->ContainsBridgeEx());
            const int adjustedZone =
                MapClass::Instance.GetMovementZoneType(adjusted, zone, adjustedCell->ContainsBridgeEx());
            const auto destination = CellClass::Coord2Cell(GetDestination());
            const bool bridge = IsOnBridge(nullptr);
            const int destinationZone =
                MapClass::Instance.GetMovementZoneType(destination, GetTechnoType()->MovementZone, bridge);
            const auto entry =
                IsCellOccupied(adjustedCell, FacingType(-1),
                               adjustedCell->Level + 4 * int(adjustedCell->ContainsBridgeEx()), nullptr, true);
            const int limit = std::min(5, std::max(std::abs(int(delta.X)), std::abs(int(delta.Y))) + 3);
            const bool useOriginal = (action != adjustedAction && direct) || entry != Move::OK ||
                                     adjustedZone != waypointZone || destinationZone != adjustedZone ||
                                     (direct && AStarClass::Instance.AttemptPath(
                                                    &adjusted, &cell, this, adjustedCell->ContainsBridgeEx(),
                                                    waypointCell->ContainsBridgeEx(), MovementZone::None) > limit);
            CellStruct noFollow{-1, -1};
            if (useOriginal) {
                success = CellClickedAction(action, &cell, &noFollow, true);
                WaypointNearbyAccessibleCellDelta = {0, 0};
            } else {
                success = CellClickedAction(action, &adjusted, &noFollow, true);
                WaypointCell = adjusted;
            }
        }
        if (feedback) Unsorted::MoveFeedback = true;
    }
    if (!success) {
        PlanningPathIdx = -1;
        WaypointIndex = 0;
        WaypointNearbyAccessibleCellDelta = {0, 0};
        WaypointCell = {0, 0};
    }
    unknown_bool_43A = false;
    return success;
}
#endif
