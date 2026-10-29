// Original AI superweapon targeting and dispatch, YR 0x005098F0 family.
#include "yrpp/HouseClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/ScenarioClass.h"
#include <bit>

namespace {
CellStruct cell_of(const AbstractClass* object) {
    const auto at = object->GetCoords();
    return {short(at.X / 256), short(at.Y / 256)};
}
ThreatType quarry_threat(QuarryType quarry) {
    // Original 0x00645BB0 also maps None, Anything and Threats to zero.
    switch (quarry) {
        case QuarryType::Buildings:
            return ThreatType(0x20);
        case QuarryType::Harvesters:
            return ThreatType(0x40);
        case QuarryType::Infantry:
            return ThreatType(0x8);
        case QuarryType::Vehicles:
            return ThreatType(0x10);
        case QuarryType::Factories:
            return ThreatType(0x1000);
        case QuarryType::Defenses:
            return ThreatType(0x2000);
        case QuarryType::Power:
            return ThreatType(0x800);
        case QuarryType::OccupiableBuildings:
            return ThreatType(0x8000);
        case QuarryType::TechBuildings:
            return ThreatType(0x10000);
        default:
            return ThreatType(0);
    }
}
CellStruct offensive_target(const HouseClass& house) {
    return house.PreferredTargetCell != CellStruct::Empty      ? house.PreferredTargetCell
           : house.PreferredTargetType == QuarryType::Anything ? house.PickIonCannonTarget()
                                                               : house.PickTargetByType(house.PreferredTargetType);
}
}
CellStruct* HouseClass::PickTargetByType(CellStruct& output, QuarryType quarry) const {
    output = CellStruct::Empty;
    const auto threat = quarry_threat(quarry);
    for (auto* team : TeamClass::Array)
        if (team->Owner == this) {
            auto* leader = team->FetchALeader();
            if (!leader) return &output;
            auto origin = leader->GetCoords();
            if (auto* target = leader->GreatestThreat(threat, &origin, true)) output = cell_of(target);
            break;
        }
    return &output;
}
CellStruct* HouseClass::PickIonCannonTarget(CellStruct& output) const {
    output = CellStruct::Empty;
    auto* enemy = Array[EnemyHouseIndex];
    DynamicVectorClass<AbstractClass*> choices;
    int best = 0;
    for (auto* object : TechnoClass::Array) {
        bool eligible = false;
        int value = 0;
        if (object->Owner == enemy) {
            eligible = IsIonCannonEligibleTarget(object);
            value = object->GetIonCannonValue(AIDifficulty);
        }
        if (!MapClass::Instance.IsWithinUsableArea(cell_of(object), true)) value = 0;
        if (object->CloakState == CloakState::Cloaked ||
            (object->WhatAmI() == AbstractType::Building && static_cast<BuildingClass*>(object)->Translucency == 15))
            value = ScenarioClass::Instance->Random.RandomRanged(0, best + 10);
        if (!eligible) continue;
        if (value > best) {
            choices.Clear();
            best = value;
            choices.AddItem(object);
        } else if (value == best)
            choices.AddItem(object);
    }
    if (choices.Count > 0) {
        auto* selected = choices[ScenarioClass::Instance->Random.RandomRanged(0, choices.Count - 1)];
        if (selected) output = cell_of(selected);
    }
    return &output;
}
bool HouseClass::Fire_ParaDrop(SuperClass* super) {
    const auto* target = EnemyHouseIndex == -1 ? this : Array[EnemyHouseIndex];
    auto cell = PreferredTargetCell;
    if (cell == CellStruct::Empty) {
        if (PreferredTargetType == QuarryType::Anything) {
            const auto center = target->GetBaseCenter();
            MapClass::Instance.NearByLocation(cell, center, SpeedType(0), -1, MovementZone(0), false, 5, 5, false,
                                              false, false, true, CellStruct{}, false, false);
            cell.X = short(cell.X + 2);
            cell.Y = short(cell.Y + 2);
        } else
            cell = PickTargetByType(PreferredTargetType);
    }
    return cell != CellStruct::Empty && Fire_SW(Supers.FindItemIndex(super), cell);
}
bool HouseClass::Fire_LightningStorm(SuperClass* super) {
    if (LightningStorm::Active || EnemyHouseIndex == -1) return false;
    const auto cell = offensive_target(*this);
    return cell != CellStruct::Empty && Fire_SW(Supers.FindItemIndex(super), cell);
}
bool HouseClass::Fire_GenMutator(SuperClass* super) {
    // The executable does not fire when an explicit preferred cell is set.
    if (PreferredTargetCell != CellStruct::Empty) return false;
    int best = 0;
    CellStruct target = CellStruct::Empty;
    for (int i = InfantryClass::Array.Count - 1; i >= 0; --i) {
        auto* unit = InfantryClass::Array[i];
        if (unit->InLimbo) continue;
        const auto center = unit->GetCell()->MapCoords;
        int count = 0;
        for (int j = 0; j < 10; ++j) {
            const auto offset = CellSpread::GetCell(j);
            auto* cell =
                MapClass::Instance.GetCellAt(CellStruct{short(center.X + offset.X), short(center.Y + offset.Y)});
            for (auto* object = static_cast<ObjectClass*>(cell->GetInfantry(unit->OnBridge));
                 object && object->WhatAmI() == AbstractType::Infantry; object = object->NextObject)
                if (!IsAlliedWith(object->GetOwningHouse()) && !object->IsInAir()) ++count;
        }
        if (count > best) {
            best = count;
            target = center;
        }
    }
    return best && target != CellStruct::Empty && MapClass::Instance.IsWithinUsableArea(target, true) &&
           Fire_SW(Supers.FindItemIndex(super), target);
}
bool HouseClass::Fire_PsyDom(SuperClass* super) {
    if (PsyDom::Active() || EnemyHouseIndex == -1 || PreferredTargetCell != CellStruct::Empty) return false;
    int best = 0;
    CellStruct target = CellStruct::Empty;
    for (int i = FootClass::Array.Count - 1; i >= 0; --i) {
        auto* unit = FootClass::Array[i];
        if (unit->InLimbo) continue;
        const auto center = unit->GetCell()->MapCoords;
        int count = 0;
        for (int j = 0; j < 38; ++j) {
            const auto offset = CellSpread::GetCell(j);
            auto* cell =
                MapClass::Instance.GetCellAt(CellStruct{short(center.X + offset.X), short(center.Y + offset.Y)});
            for (auto* object = cell->FirstObject; object && (unsigned(object->AbstractFlags) & 4u);
                 object = object->NextObject) {
                if (IsAlliedWith(object->GetOwningHouse()) || object->IsInAir()) continue;
                if (static_cast<TechnoClass*>(object)->CanBePermaMindControlled()) ++count;
            }
        }
        if (count > best) {
            best = count;
            target = center;
        }
    }
    return best && target != CellStruct::Empty && MapClass::Instance.IsWithinUsableArea(target, true) &&
           Fire_SW(Supers.FindItemIndex(super), target);
}
void HouseClass::AI_TryFireSW() {
    if (IsControlledByHuman()) return;
    for (int i = 0; i < Supers.Count; ++i) {
        auto* super = Supers[i];
        if (!super || !super->IsReady) continue;
        switch (super->Type->Type) {
            case SuperWeaponType::Nuke:
                if (EnemyHouseIndex != -1) {
                    const auto cell = offensive_target(*this);
                    if (cell != CellStruct::Empty) Fire_SW(Supers.FindItemIndex(super), cell);
                }
                break;
            case SuperWeaponType::LightningStorm:
                Fire_LightningStorm(super);
                break;
            case SuperWeaponType::ParaDrop:
            case SuperWeaponType::AmerParaDrop:
            case SuperWeaponType::SpyPlane:
            case SuperWeaponType::PsychicReveal:
                Fire_ParaDrop(super);
                break;
            case SuperWeaponType::PsychicDominator:
                Fire_PsyDom(super);
                break;
            case SuperWeaponType::GeneticMutator:
                Fire_GenMutator(super);
                break;
            case SuperWeaponType::ForceShield: {
                auto cell = PreferredDefensiveCell2;
                const int expires = std::bit_cast<int>(unsigned(PreferredDefensiveCellStartTime) +
                                                       unsigned(RulesClass::Instance->AISuperDefenseFrames));
                if (cell == CellStruct::Empty && expires > Unsorted::CurrentFrame) cell = PreferredDefensiveCell;
                if (cell != CellStruct::Empty) Fire_SW(Supers.FindItemIndex(super), cell);
                break;
            }
            default:
                break;
        }
    }
}
