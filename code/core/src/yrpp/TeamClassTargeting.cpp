// Team target coordination, calibrated to YR 0x6EB490 and script helpers.
#include "yrpp/TeamClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include <bit>
#include <climits>
#include "x87_integer.hpp"

namespace {
bool active(const FootClass* foot) {
    return foot->IsAlive && foot->Health && (Unsorted::ScenarioInit || !foot->InLimbo);
}
bool armed(const TeamClass& team) {
    for (auto* foot = team.FirstUnit; foot; foot = foot->NextTeamMember)
        if (foot->GetTechnoType()->Ammo <= 0 || foot->Ammo > 0) return true;
    return false;
}
int building_score(BuildingClass* building, FootClass* origin, int property) {
    if (property == 0 || property == 1) {
        const auto at = building->GetCoords();
        const int value =
            MapClass::Instance.GetThreatPosed(CellStruct{short(at.X / 256), short(at.Y / 256)}, origin->Owner);
        return property == 0 ? std::bit_cast<int>(unsigned(INT_MAX) - unsigned(value)) : value;
    }
    if (property == 2 || property == 3) {
        const auto difference = [](int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); };
        const double x = difference(building->Location.X, origin->Location.X);
        const double y = difference(building->Location.Y, origin->Location.Y);
        const double z = difference(building->Location.Z, origin->Location.Z);
        const int distance = game::x87_integer(Math::sqrt(x * x + z * z + y * y));
        return property == 2 ? std::bit_cast<int>(unsigned(INT_MAX) - unsigned(distance)) : distance;
    }
    return -1;
}
HouseClass* enemy_of(FootClass* origin) {
    const int index = origin->Owner->EnemyHouseIndex;
    return index == -1 ? nullptr : HouseClass::Array[index];
}
CellClass* approach_building(BuildingClass* building, FootClass* foot) {
    const auto movement = foot->GetTechnoType()->MovementZone;
    const auto& at = building->Location;
    const CellStruct target{short(at.X / 256), short(at.Y / 256)};
    const auto position = foot->GetCoords();
    const int zone = MapClass::Instance.GetMovementZoneType(
        CellStruct{short(position.X / 256), short(position.Y / 256)}, movement, foot->OnBridge);
    const auto cell =
        MapClass::Instance.NearByLocation(target, foot->GetTechnoType()->SpeedType, zone, movement, false, 1, 1, false,
                                          false, false, true, CellStruct{0, 0}, false, false);
    return cell == CellStruct::Empty ? nullptr : MapClass::Instance.GetCellAt(cell);
}
}

BuildingClass* YRPP_FASTCALL TeamClass::PickBuilding(BuildingTypeClass* type, HouseClass* enemy, FootClass* origin,
                                                     int property, bool onlyEnemy) {
    BuildingClass* preferred = nullptr;
    BuildingClass* best = nullptr;
    int preferredScore = -1, bestScore = -1;
    for (int i = 0; i < BuildingClass::Array.Count; ++i) {
        auto* building = BuildingClass::Array[i];
        const bool wanted = building->Owner == enemy;
        if (building->Type != type) continue;
        if (!wanted && origin->Owner->IsAlliedWith(building->Owner) && !building->Owner->Type->MultiplayPassive)
            continue;
        const int score = building_score(building, origin, property);
        if (score > preferredScore && wanted) {
            preferred = building;
            preferredScore = score;
        }
        if (score > bestScore) {
            best = building;
            bestScore = score;
        }
    }
    if (preferred && preferredScore > bestScore) return preferred;
    return onlyEnemy ? preferred : best;
}
BuildingClass* YRPP_FASTCALL TeamClass::PickFriendlyBuilding(BuildingTypeClass* type, void*, FootClass* origin,
                                                             int property, int) {
    BuildingClass* best = nullptr;
    int bestScore = -1;
    const int count = origin->Owner->Buildings.Count;
    for (int i = 0; i < count; ++i) {
        auto* building = origin->Owner->Buildings[i];
        if (building->Type != type) continue;
        const int score = building_score(building, origin, property);
        if (score > bestScore) {
            best = building;
            bestScore = score;
        }
    }
    return best;
}

void TeamClass::AttackWaypoint(const ScriptActionNode& action, bool first) {
    if (first) {
        AbstractClass* target = ScenarioClass::Instance->GetWaypointCell(action.Argument);
        if (target && target->WhatAmI() == AbstractType::Cell) {
            auto* cell = static_cast<CellClass*>(target);
            if (auto* object = cell->GetSomeObject(Point2D{0, 0}, (cell->Flags & CellFlags::BridgeHead) != CellFlags{}))
                target = object;
        }
        AssignMissionTarget(target);
    }
    if (QueuedFocus && armed(*this))
        CoordinateAttack();
    else {
        AssignMissionTarget(nullptr);
        StepCompleted = true;
    }
}
void TeamClass::GarrisonWaypoint(const ScriptActionNode& action, bool first) {
    if (first) {
        AbstractClass* target = ScenarioClass::Instance->GetWaypointCell(action.Argument);
        if (target && target->WhatAmI() == AbstractType::Cell) {
            auto* cell = static_cast<CellClass*>(target);
            target = cell->GetSomeObject(Point2D{0, 0}, (cell->Flags & CellFlags::BridgeHead) != CellFlags{});
            if (target && target->WhatAmI() != AbstractType::Building) target = nullptr;
        } else
            target = nullptr;
        AssignMissionTarget(target);
    }
    if (QueuedFocus && armed(*this))
        CoordinateAttack();
    else {
        AssignMissionTarget(nullptr);
        StepCompleted = true;
    }
}
void TeamClass::Spy(const ScriptActionNode& action, bool first) {
    if (first) AssignMissionTarget(ScenarioClass::Instance->GetWaypointCell(action.Argument));
    if (QueuedFocus && QueuedFocus->WhatAmI() == AbstractType::Cell) {
        auto* cell = QueuedFocus && QueuedFocus->WhatAmI() == AbstractType::Cell ? static_cast<CellClass*>(QueuedFocus)
                                                                                 : nullptr;
        if (auto* object = cell->GetSomeObject(Point2D{0, 0}, false)) {
            AssignMissionTarget(object);
            CoordinateAttack();
        }
    } else if (QueuedFocus)
        CoordinateAttack();
    else {
        AssignMissionTarget(nullptr);
        StepCompleted = true;
    }
}
void TeamClass::Patrol(const ScriptActionNode& action, bool first) {
    auto* waypoint = ScenarioClass::Instance->GetWaypointCell(action.Argument);
    if (first) AssignMissionTarget(waypoint);
    if (!Focus && unsigned(action.Argument) < 702u) AssignMissionTarget(waypoint);
    if (Unsorted::CurrentFrame % game::x87_integer(RulesClass::Instance->PatrolScan * 900.0) == 0) {
        if (auto* leader = FetchALeader()) {
            auto at = leader->Location;
            auto* target = leader->GreatestThreat(static_cast<ThreatType>(1), &at, false);
            if (target)
                AssignMissionTarget(target);
            else if (Focus != waypoint)
                AssignMissionTarget(nullptr);
        }
    }
    if (Focus && (Focus->AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None)
        CoordinateAttack();
    else
        CoordinateMove();
}
void TeamClass::AttackBuilding(const ScriptActionNode& action, bool first) {
    if (first && !QueuedFocus && FirstUnit) {
        auto* building =
            PickBuilding(BuildingTypeClass::Array[unsigned(action.Argument) & 0xFFFFu], enemy_of(FirstUnit), FirstUnit,
                         (unsigned(action.Argument) >> 16) & 0xFFFFu, Type->OnlyTargetHouseEnemy);
        if (building) AssignMissionTarget(building);
    }
    if (QueuedFocus && armed(*this))
        CoordinateAttack();
    else {
        AssignMissionTarget(nullptr);
        StepCompleted = true;
    }
}
void TeamClass::MoveToBuilding(const ScriptActionNode& action, bool) {
    if (!QueuedFocus && FirstUnit) {
        auto* building =
            PickBuilding(BuildingTypeClass::Array[unsigned(action.Argument) & 0xFFFFu], enemy_of(FirstUnit), FirstUnit,
                         (unsigned(action.Argument) >> 16) & 0xFFFFu, Type->OnlyTargetHouseEnemy);
        if (building) AssignMissionTarget(approach_building(building, FirstUnit));
        if (!QueuedFocus) StepCompleted = true;
    }
    CoordinateMove();
}
void TeamClass::MoveToFriendlyBuilding(const ScriptActionNode& action, bool first) {
    if (first && !QueuedFocus && FirstUnit) {
        auto* leader = FetchALeader();
        auto* building = PickFriendlyBuilding(BuildingTypeClass::Array[unsigned(action.Argument) & 0xFFFFu], nullptr,
                                              leader, (unsigned(action.Argument) >> 16) & 0xFFFFu, 0);
        if (building) AssignMissionTarget(approach_building(building, leader));
    }
    if (!QueuedFocus)
        StepCompleted = true;
    else
        CoordinateMove();
}

void TeamClass::DoMission(const ScriptActionNode& action, bool) {
    const auto mission = static_cast<Mission>(action.Argument);
    for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
        if (active(foot) && !foot->IsInitiated) {
            if (foot->DistanceFrom3D(SpawnCell) <= GetStrayDistance())
                foot->IsInitiated = true;
            else if (!foot->Destination) {
                foot->QueueMission(Mission::Move, false);
                foot->SetTarget(nullptr);
                foot->SetDestination(SpawnCell, true);
            }
        }
        if (!active(foot) || (!foot->IsInitiated && foot->WhatAmI() != AbstractType::Aircraft) || foot->Target)
            continue;
        if (foot->Destination || foot->DistanceFrom3D(SpawnCell) <= 2 * GetStrayDistance() ||
            mission == Mission::Area_Guard) {
            if (foot->Target || foot->Destination) continue;
            if (foot->GetCurrentMission() != mission &&
                (mission != Mission::Guard || foot->GetCurrentMission() != Mission::Unload)) {
                foot->ArchiveTarget = nullptr;
                foot->QueueMission(mission, false);
                foot->SetTarget(nullptr);
                foot->SetDestination(nullptr, true);
            }
        } else {
            foot->QueueMission(Mission::Move, false);
            foot->SetDestination(SpawnCell, true);
            foot->QueueMission(Mission::Move, false);
            const auto where = SpawnCell->GetCoords();
            foot->SetDestination(MapClass::Instance.GetCellAt(CellStruct{short(where.X / 256), short(where.Y / 256)}),
                                 true);
        }
    }
}
void TeamClass::ForceFacing(const ScriptActionNode& action, bool) {
    bool complete = true;
    for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
        if (foot->Locomotor->Is_Moving())
            complete = false;
        else {
            const DirStruct direction{int((unsigned(action.Argument) << 13) & 0xFFFFu)};
            if (foot->PrimaryFacing.Current() != direction) foot->Locomotor->Do_Turn(direction);
        }
    }
    if (complete) StepCompleted = true;
}
void TeamClass::CoordinateAttack() {
    if (!Focus) Focus = QueuedFocus;
    auto* leader = FetchALeader();
    if (Focus && Focus->WhatAmI() == AbstractType::Cell && FirstUnit && leader->WhatAmI() != AbstractType::Aircraft) {
        auto* cell = Focus && Focus->WhatAmI() == AbstractType::Cell ? static_cast<CellClass*>(Focus) : nullptr;
        if (cell->GetSomeObject(Point2D{0, 0}, false)) Focus = cell->GetSomeObject(Point2D{0, 0}, false);
    }
    if (Unsorted::CurrentFrame % 8 == 4)
        if (leader->GetFireError(Focus, leader->SelectWeapon(Focus), true) == FireError::ILLEGAL)
            TargetNotAssigned = true;
    if (!Focus) {
        StepCompleted = true;
        return;
    }
    ScriptActionNode action;
    CurrentScript->GetCurrentAction(&action);
    bool pending = false;
    for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
        if (active(foot) && !foot->IsInitiated) {
            if (foot->DistanceFrom3D(SpawnCell) <= GetStrayDistance())
                foot->IsInitiated = true;
            else if (!foot->Destination) {
                foot->QueueMission(Mission::Move, false);
                foot->SetTarget(nullptr);
                foot->SetDestination(SpawnCell, true);
            }
        }
        if (active(foot) && (foot->IsInitiated || foot->WhatAmI() == AbstractType::Aircraft)) {
            if (action.Action == TMISSION_SPY && foot->WhatAmI() == AbstractType::Infantry &&
                static_cast<InfantryClass*>(foot)->Type->Infiltrate) {
                foot->QueueMission(Mission::Capture, false);
                foot->SetTarget(Focus);
            } else {
                if (foot->GetCurrentMission() != Mission::Attack && foot->GetCurrentMission() != Mission::Enter &&
                    foot->GetCurrentMission() != Mission::Capture && foot->GetCurrentMission() != Mission::Sabotage &&
                    (foot->GetCurrentMission() != Mission::Unload || !foot->CanDeployNow())) {
                    foot->SendToFirstLink(RadioCommand::NotifyUnlink);
                    foot->QueueMission(Mission::Attack, false);
                    foot->SetTarget(nullptr);
                    foot->SetDestination(nullptr, true);
                }
            }
            if (!foot->Target) foot->SetTarget(Focus);
            if (foot->WhatAmI() != AbstractType::Aircraft || !foot->GetWeapon(0)->WeaponType || foot->Ammo > 0)
                pending = true;
        } else if (Type->Droppod && foot->InLimbo)
            pending = true;
    }
    if (!pending) StepCompleted = true;
}
