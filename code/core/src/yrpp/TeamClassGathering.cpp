// Team gathering, scouting and superweapon requests calibrated to YR helpers.
#include "yrpp/TeamClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "x87_integer.hpp"
#include <bit>

namespace {
int difference(int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); }
int gathering_direction(const CoordStruct& from, const CoordStruct& to) {
    const auto raw = static_cast<short>(game::x87_integer(
        (Math::atan2(double(difference(from.Y, to.Y)), double(difference(to.X, from.X))) - 1.570796326794897) *
        -10430.06004058427));
    return int(raw) - 0x3FFF;
}
CellStruct gathering_cell(const CoordStruct& center, int direction) {
    const double angle = double(direction) * -0.00009587672516830327;
    const double distance = double(std::bit_cast<int>(unsigned(RulesClass::Instance->AISafeDistance) << 8));
    const int y = game::x87_integer(double(center.Y) - Math::sin(angle) * distance);
    const int x = game::x87_integer(Math::cos(angle) * distance + double(center.X));
    return CellStruct{short(x / 256), short(y / 256)};
}
bool super_wait_complete(SuperClass* super) {
    const int left = super->RechargeTimer.GetTimeLeft();
    const int duration = super->GetRechargeTime();
    return !super->IsPresent ||
           1.0 - double(RulesClass::Instance->AIMinorSuperReadyPercent) < double(left) / double(duration);
}
void chrono_pair(HouseClass* owner, SuperClass*& sphere, SuperClass*& warp) {
    sphere = nullptr;
    warp = nullptr;
    for (int i = 0; i < owner->Supers.Count; ++i) {
        auto* super = owner->Supers[i];
        if (super->Type->Type == SuperWeaponType::ChronoSphere) sphere = super;
        if (super->Type->Type == SuperWeaponType::ChronoWarp) warp = super;
    }
}
CellStruct cell_of(AbstractClass* object) {
    const auto at = object->GetCoords();
    return CellStruct{short(at.X / 256), short(at.Y / 256)};
}
ThreatType quarry_mask(int value) {
    switch (static_cast<QuarryType>(value)) {
        case QuarryType::Buildings:
            return ThreatType::Buildings;
        case QuarryType::Harvesters:
            return ThreatType::Tiberium;
        case QuarryType::Infantry:
            return ThreatType::Infantry;
        case QuarryType::Vehicles:
            return ThreatType::Vehicles;
        case QuarryType::Factories:
            return ThreatType::Factories;
        case QuarryType::Defenses:
            return ThreatType::BaseDefense;
        case QuarryType::Power:
            return ThreatType::Power;
        case QuarryType::OccupiableBuildings:
            return ThreatType::OccupiableBuildings;
        case QuarryType::TechBuildings:
            return ThreatType::TechBuildings;
        default:
            return ThreatType::Normal;
    }
}
}

void TeamClass::Scout(const ScriptActionNode&, bool) {
    bool pending = true;
    if (Target) {
        auto* leader = FetchALeader();
        if (leader && leader->CurrentMission != Mission::Move && leader->QueuedMission != Mission::Move) {
            Owner->MarkScouted(Target);
            Target = nullptr;
        }
    } else {
        int available = 0;
        for (int i = 0; i < Owner->ScoutNodes.Count; ++i)
            if (!Owner->ScoutNodes[i].IsPreferred) ++available;
        if (!available)
            pending = false;
        else {
            int selected = ScenarioClass::Instance->Random.RandomRanged(0, available - 1);
            for (int i = 0; i < Owner->ScoutNodes.Count; ++i) {
                if (Owner->ScoutNodes[i].IsPreferred) continue;
                if (selected) {
                    --selected;
                    continue;
                }
                Target = Owner->ScoutNodes[i].House;
                break;
            }
            DynamicVectorClass<BuildingClass*> candidates;
            for (int i = 0; i < BuildingClass::Array.Count; ++i)
                if (BuildingClass::Array[i]->Owner == Target) candidates.AddItem(BuildingClass::Array[i]);
            if (!candidates.Count) {
                Owner->MarkScouted(Target);
                Target = nullptr;
            } else {
                auto* building = candidates[ScenarioClass::Instance->Random.RandomRanged(0, candidates.Count - 1)];
                auto* leader = FetchALeader();
                const auto cell = MapClass::Instance.NearByLocation(
                    CellStruct{short(building->Location.X / 256), short(building->Location.Y / 256)},
                    leader->GetTechnoType()->SpeedType, -1, MovementZone::Normal, false, 1, 1, false, false, false,
                    true, CellStruct{0, 0}, false, false);
                AssignMissionTarget(cell == CellStruct::Empty ? nullptr : MapClass::Instance.GetCellAt(cell));
            }
        }
    }
    CoordinateMove();
    StepCompleted = !pending;
}

void TeamClass::GatherAtEnemy(const ScriptActionNode&, bool first) {
    if (first) {
        auto* leader = FetchALeader();
        if (!leader) {
            StepCompleted = true;
            return;
        }
        const int index = leader->Owner->EnemyHouseIndex;
        auto* enemy = index == -1 ? nullptr : HouseClass::Array[index];
        if (!enemy) {
            StepCompleted = true;
            return;
        }
        CoordStruct destination;
        enemy->GetBaseCenterCoords(&destination);
        if (destination == CoordStruct::Empty) {
            StepCompleted = true;
            return;
        }
        CoordStruct origin;
        leader->Owner->GetBaseCenterCoords(&origin);
        if (origin == CoordStruct::Empty) origin = leader->GetCoords();
        const auto cell = MapClass::Instance.NearByLocation(
            gathering_cell(destination, gathering_direction(destination, origin)), leader->GetTechnoType()->SpeedType,
            -1, MovementZone::Normal, false, 3, 3, false, false, false, true, CellStruct{0, 0}, false, false);
        AssignMissionTarget(MapClass::Instance.GetCellAt(cell));
    }
    CoordinateMove();
}
void TeamClass::GatherAtBase(const ScriptActionNode&, bool first) {
    if (first) {
        auto* leader = FetchALeader();
        const int index = leader->Owner->EnemyHouseIndex;
        auto* enemy = index == -1 ? nullptr : HouseClass::Array[index];
        CoordStruct origin;
        leader->Owner->GetBaseCenterCoords(&origin);
        int direction;
        if (enemy) {
            CoordStruct destination;
            enemy->GetBaseCenterCoords(&destination);
            direction = gathering_direction(origin, destination);
        } else
            direction = int(static_cast<short>(ScenarioClass::Instance->Random.RandomRanged(0, 255) << 8)) - 0x3FFF;
        const auto cell = MapClass::Instance.NearByLocation(
            gathering_cell(origin, direction), leader->GetTechnoType()->SpeedType, -1, MovementZone::Normal, false, 3,
            3, false, false, false, true, CellStruct{0, 0}, false, false);
        AssignMissionTarget(MapClass::Instance.GetCellAt(cell));
    }
    CoordinateMove();
}
void TeamClass::AskForSW(const ScriptActionNode&, bool) {
    auto* leader = FetchALeader();
    if (!leader) {
        StepCompleted = true;
        return;
    }
    auto* owner = leader->Owner;
    SuperClass* selected = nullptr;
    for (int i = 0; i < owner->Supers.Count; ++i)
        if (owner->Supers[i]->Type->Type == SuperWeaponType::IronCurtain) {
            selected = owner->Supers[i];
            break;
        }
    if (!selected) {
        StepCompleted = true;
        return;
    }
    if (selected->IsReady && owner->GetPowerPercentage() >= 1.0) {
        owner->Fire_SW(static_cast<int>(selected->Type->Type), cell_of(SpawnCell));
        StepCompleted = true;
    } else if (super_wait_complete(selected))
        StepCompleted = true;
}
void TeamClass::ChronoshiftToBuilding(const ScriptActionNode& action, bool) {
    auto* leader = FetchALeader();
    if (!leader) {
        StepCompleted = true;
        return;
    }
    auto* owner = leader->Owner;
    SuperClass* sphere;
    SuperClass* warp;
    chrono_pair(owner, sphere, warp);
    if (!sphere || !warp) {
        StepCompleted = true;
        return;
    }
    if (sphere->IsReady && owner->GetPowerPercentage() >= 1.0) {
        const int index = leader->Owner->EnemyHouseIndex;
        auto* enemy = index == -1 ? nullptr : HouseClass::Array[index];
        auto* building = PickBuilding(BuildingTypeClass::Array[unsigned(action.Argument) & 0xFFFFu], enemy, leader,
                                      (unsigned(action.Argument) >> 16) & 0xFFFFu, Type->OnlyTargetHouseEnemy);
        if (building) {
            owner->Fire_SW(static_cast<int>(sphere->Type->Type), cell_of(SpawnCell));
            owner->Fire_SW(static_cast<int>(warp->Type->Type), cell_of(building));
            AssignMissionTarget(building);
        }
        StepCompleted = true;
    } else if (super_wait_complete(sphere))
        StepCompleted = true;
}
void TeamClass::ChronoshiftToTarget(const ScriptActionNode& action, bool) {
    auto* leader = FetchALeader();
    if (!leader) {
        StepCompleted = true;
        return;
    }
    auto* owner = leader->Owner;
    SuperClass* sphere;
    SuperClass* warp;
    chrono_pair(owner, sphere, warp);
    if (!sphere || !warp) {
        StepCompleted = true;
        return;
    }
    if (sphere->IsReady && owner->GetPowerPercentage() >= 1.0) {
        auto at = leader->Location;
        auto* target = leader->GreatestThreat(quarry_mask(action.Argument), &at, Type->OnlyTargetHouseEnemy);
        if (target) {
            owner->Fire_SW(static_cast<int>(sphere->Type->Type), cell_of(SpawnCell));
            owner->Fire_SW(static_cast<int>(warp->Type->Type), cell_of(target));
            AssignMissionTarget(target);
        }
        StepCompleted = true;
    } else if (super_wait_complete(sphere))
        StepCompleted = true;
}
