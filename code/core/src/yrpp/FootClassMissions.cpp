// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp rescue/retreat/patrol, calibrated to YR.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AStarClass.h"
#include "yrpp/RulesClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstdlib>
#if !defined(RA2_YRPP_GAME)
namespace {
int mission_delay(const FootClass& object) {
    const int ticks = game::x87_integer(object.CurrentMissionControl()->Rate * 900.0);
    return std::bit_cast<int>(unsigned(ticks) + unsigned(ScenarioClass::Instance->Random.RandomRanged(0, 2)));
}
ObjectClass* as_object(AbstractClass* target) {
    return target && (target->AbstractFlags & AbstractFlags::Object) != AbstractFlags::None
               ? static_cast<ObjectClass*>(target)
               : nullptr;
}
}
int FootClass::Mission_Rescue() {
    if (MissionStatus == 0) {
        if (Target)
            ApproachTarget(0);
        else {
            if (!ArchiveTarget) SetArchiveTarget(MapClass::Instance.GetCellAt(Location));
            IsScanLimited = false;
            auto origin = ArchiveTarget->GetCoords();
            auto* threat = GreatestThreat(static_cast<ThreatType>(0), &origin, false);
            if (threat) {
                const auto from = ArchiveTarget->GetCoords(), to = threat->GetCoords();
                const int x = std::bit_cast<int>(unsigned(to.X) - unsigned(from.X));
                const int y = std::bit_cast<int>(unsigned(to.Y) - unsigned(from.Y));
                const int z = std::bit_cast<int>(unsigned(to.Z) - unsigned(from.Z));
                const int distance = game::x87_integer(Math::sqrt(double(x) * x + double(y) * y + double(z) * z));
                if (GetGuardRange(1) * 1.5 > double(distance)) SetTarget(threat);
            }
            if (Target) return 1;
            MissionStatus = 1;
            CellStruct where;
            Owner->WhereToGo(&where, this);
            SetDestination(where == CellStruct::Empty ? nullptr : MapClass::Instance.GetCellAt(where), true);
            SetArchiveTarget(nullptr);
        }
    } else if (MissionStatus == 1 && !Destination) {
        SetArchiveTarget(nullptr);
        QueueMission(Mission::Area_Guard, false);
        NextMission();
    }
    return mission_delay(*this);
}
int FootClass::Mission_Retreat() {
    if (MissionStatus == 0) {
        if (Destination)
            MissionStatus = 1;
        else {
            CellStruct where{0, 0};
            const auto edge = unsigned(Owner->StartingEdge) < 4 ? Owner->StartingEdge : Edge::North;
            if (Team) {
                CellStruct entry;
                Team->Type->GetWaypoint(&entry);
                if (entry != CellStruct::Empty) {
                    Team->Type->GetWaypoint(&entry);
                    MapClass::Instance.PickCellOnEdge(where, edge, entry, CellClass::Coord2Cell(GetCoords()),
                                                      GetTechnoType()->SpeedType, true, MovementZone::Normal);
                }
            }
            if (where == CellStruct::Empty)
                MapClass::Instance.PickCellOnEdge(where, edge, CellStruct::Empty, CellClass::Coord2Cell(GetCoords()),
                                                  GetTechnoType()->SpeedType, true, MovementZone::Normal);
            SetDestination(MapClass::Instance.GetCellAt(where), true);
            MissionStatus = 1;
        }
    } else if (MissionStatus == 1 && !Destination)
        MissionStatus = 0;
    return mission_delay(*this);
}
int FootClass::Mission_Patrol() {
    enum PatrolState : int { FindTarget = 0, EngageTarget = 1, Return = 2, Reset = 3 };
    const int distanceLimit = GetGuardRange(2) / 256 + 6;
    unknown_bool_430 = true;
    const auto walk = [&](const CellStruct& from, const CellStruct& to, bool fromBridge, bool toBridge) {
        return AStarClass::Instance.AttemptPath(const_cast<CellStruct*>(&from), const_cast<CellStruct*>(&to), this,
                                                fromBridge, toBridge, MovementZone::None);
    };
    const auto reachable = [&](ObjectClass* target, int weapon, CellClass* origin) {
        if (!target) return false;
        if (IsCloseEnough(target, weapon)) return true;
        const auto there = CellClass::Coord2Cell(target->GetDestination());
        return walk(origin->MapCoords, there, origin->ContainsBridgeEx(), target->IsOnBridge(nullptr)) < distanceLimit;
    };
    const auto acquire = [&] {
        auto origin = PatrolCell->GetCoords();
        auto* threat = GreatestThreat(static_cast<ThreatType>(2), &origin, false);
        auto* object = as_object(threat);
        const int weapon = SelectWeapon(object);
        if (!reachable(object, weapon, PatrolCell)) return false;
        SetTarget(threat);
        SetDestination(nullptr, true);
        return true;
    };
    switch (MissionStatus) {
        case PatrolState::FindTarget: {
            auto origin = Location;
            auto* threat = GreatestThreat(static_cast<ThreatType>(2), &origin, false);
            auto* object = as_object(threat);
            const int weapon = SelectWeapon(object);
            bool engage = object && (IsCloseEnough(object, weapon) ||
                                     walk(CellClass::Coord2Cell(GetDestination()),
                                          CellClass::Coord2Cell(object->GetDestination()), IsOnBridge(nullptr),
                                          object->IsOnBridge(nullptr)) < distanceLimit);
            if (engage) {
                PatrolCell = GetCellAgain();
                if (PatrolCell->ContainsBridgeEx() && !IsOnBridge(nullptr)) {
                    CellStruct nearby;
                    const auto zone = GetTechnoType()->MovementZone;
                    const int index = MapClass::Instance.GetMovementZoneType(PatrolCell->MapCoords, zone, false);
                    MapClass::Instance.NearByLocation(nearby, PatrolCell->MapCoords, GetTechnoType()->SpeedType, index,
                                                      zone, false, 1, 1, false, true, false, false, CellStruct::Empty,
                                                      false, false);
                    if (nearby == CellStruct::Empty) {
                        PatrolCell = nullptr;
                        return 60;
                    }
                    PatrolCell = MapClass::Instance.GetCellAt(nearby);
                }
                if (!ArchiveTarget)
                    SetArchiveTarget(unknown_abstract_array_588.Count > 0
                                         ? unknown_abstract_array_588[unknown_abstract_array_588.Count - 1]
                                     : Destination ? Destination
                                                   : PatrolCell);
                SetDestination(nullptr, true);
                SetTarget(threat);
                MissionStatus = PatrolState::EngageTarget;
            } else {
                if (Destination) {
                    if (!Locomotor) std::abort();
                    if (Locomotor->Is_Moving()) break;
                }
                EnterIdleMode(false, true);
                MissionStatus = PatrolState::Reset;
                return 1;
            }
            break;
        }
        case PatrolState::EngageTarget: {
            auto* object = as_object(Target);
            const int weapon = SelectWeapon(object);
            if (IsEngineer() && object && object->WhatAmI() == AbstractType::Building &&
                Owner->IsAlliedWith(static_cast<TechnoClass*>(object)->Owner) &&
                object->GetHealthPercentage() > RulesClass::Instance->ConditionRed)
                SetTarget(nullptr);
            else if (reachable(object, weapon, PatrolCell)) {
                ApproachTarget(0);
                if (Target) break;
                SetDestination(ArchiveTarget, true);
                SetArchiveTarget(nullptr);
                PatrolCell = nullptr;
                MissionStatus = PatrolState::FindTarget;
                return 45;
            }
            if (!Target && acquire()) return 1;
            MissionStatus = PatrolState::Return;
            SetTarget(nullptr);
            if (ArchiveTarget) {
                auto* current = GetCellAgain();
                const bool bridge = IsOnBridge(nullptr);
                auto* archived = MapClass::Instance.GetCellAt(ArchiveTarget->GetCoords());
                const int back =
                    walk(current->MapCoords, PatrolCell->MapCoords, bridge, PatrolCell->ContainsBridgeEx());
                if (back >= walk(current->MapCoords, archived->MapCoords, bridge, archived->ContainsBridgeEx()))
                    SetDestination(ArchiveTarget, true);
                else
                    SetDestination(PatrolCell, true);
                return 1;
            }
            MissionStatus = PatrolState::FindTarget;
            EnterIdleMode(false, true);
            MissionStatus = PatrolState::Reset;
            return 1;
        }
        case PatrolState::Return:
            if (acquire()) {
                MissionStatus = PatrolState::EngageTarget;
                break;
            }
            if (!Destination) {
                SetTarget(nullptr);
                if (ArchiveTarget) {
                    SetDestination(ArchiveTarget, true);
                    SetArchiveTarget(nullptr);
                    MissionStatus = PatrolState::FindTarget;
                    PatrolCell = nullptr;
                } else {
                    MissionStatus = PatrolState::FindTarget;
                    EnterIdleMode(false, true);
                    MissionStatus = PatrolState::Reset;
                    return 1;
                }
            }
            break;
        case PatrolState::Reset:
            if (Destination) {
                MissionStatus = PatrolState::FindTarget;
                return 1;
            }
            break;
    }
    return mission_delay(*this);
}
#endif
