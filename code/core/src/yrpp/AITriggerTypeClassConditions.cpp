// YR AI trigger qualification and conditions, 0x0041E720..0x0041FFCF.
#include "yrpp/AITriggerTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/MapClass.h"
#include <bit>

namespace {
bool compare(int count, const AITriggerConditionComparator& condition) {
    switch (condition.ComparatorType) {
        case AITriggerConditionComparatorType::Less:
            return count < condition.ComparatorOperand;
        case AITriggerConditionComparatorType::LessOrEqual:
            return count <= condition.ComparatorOperand;
        case AITriggerConditionComparatorType::Equal:
            return count == condition.ComparatorOperand;
        case AITriggerConditionComparatorType::GreaterOrEqual:
            return count >= condition.ComparatorOperand;
        case AITriggerConditionComparatorType::Greater:
            return count > condition.ComparatorOperand;
        case AITriggerConditionComparatorType::NotEqual:
            return count != condition.ComparatorOperand;
        default:
            return false;
    }
}
int count_type(HouseClass* house, TechnoTypeClass* type) {
    if (!type) return 0;
    const int index = type->GetArrayIndex();
    switch (type->WhatAmI()) {
        case AbstractType::AircraftType:
            return house->ActiveAircraftTypes.GetItemCount(index);
        case AbstractType::BuildingType:
            return house->ActiveBuildingTypes.GetItemCount(index);
        case AbstractType::InfantryType:
            return house->ActiveInfantryTypes.GetItemCount(index);
        case AbstractType::UnitType:
            return house->ActiveUnitTypes.GetItemCount(index);
        default:
            return 0;
    }
}
bool charged(HouseClass* house, SuperWeaponType kind) {
    if (!house) return false;
    for (auto* super : house->Supers) {
        if (super->Type->Type != kind) continue;
        if (!super->IsPresent) return false;
        return 1.0 - double(RulesClass::Instance->AIMinorSuperReadyPercent) >=
               double(super->RechargeTimer.GetTimeLeft()) / double(super->GetRechargeTime());
    }
    return false;
}
}
bool AITriggerTypeClass::EnemyHouseOwns(HouseClass*, HouseClass* target) const {
    return target && compare(count_type(target, ConditionObject), Conditions[0]);
}
bool AITriggerTypeClass::OwnerHouseOwns(HouseClass* owner, HouseClass*) const {
    return owner && compare(count_type(owner, ConditionObject), Conditions[0]);
}
bool AITriggerTypeClass::CivilianHouseOwns(HouseClass*, HouseClass*) const {
    for (auto* house : HouseClass::Array)
        if (house->Type->SideIndex == SideClass::FindIndex("Civilian"))
            return compare(count_type(house, ConditionObject), Conditions[0]);
    return false;
}
bool AITriggerTypeClass::HouseCredits(HouseClass*, HouseClass* target) const {
    return target && compare(int(target->Available_Money()), Conditions[0]);
}
bool AITriggerTypeClass::IronCurtainCharged(HouseClass* owner, HouseClass*) const {
    return charged(owner, SuperWeaponType::IronCurtain);
}
bool AITriggerTypeClass::ChronoSphereCharged(HouseClass* owner, HouseClass*) const {
    return charged(owner, SuperWeaponType::ChronoSphere);
}
bool YRPP_STDCALL AITriggerTypeClass::TeamTargetSuitable(const TeamTypeClass* team, HouseClass* owner,
                                                         HouseClass* target) {
    if (!team->field_F0 || !target) return true;
    const auto theirs = target->GetBaseCenter(), ours = owner->GetBaseCenter();
    auto& map = MapClass::Instance;
    const auto zone = static_cast<MovementZone>(team->field_EC);
    const int other = map.GetMovementZoneType(theirs, zone, false);
    const int own = map.GetMovementZoneType(ours, zone, false);
    if (!team->field_F1) return own == other;
    if (own == other) return false;
    const int amphibiousOther = map.GetMovementZoneType(theirs, static_cast<MovementZone>(5), false);
    const int amphibiousOwn = map.GetMovementZoneType(ours, static_cast<MovementZone>(5), false);
    return amphibiousOwn == amphibiousOther;
}
bool AITriggerTypeClass::ConditionMet(HouseClass* owner, HouseClass* target, bool enoughDefense) const {
    if (!Team1) return false;
    const bool defense = Team1->IsBaseDefense || (Team2 && Team2->IsBaseDefense);
    const auto& rules = *RulesClass::Instance;
    const int ai = int(owner->AIDifficulty);
    const int minimum = ai >= 0 && ai < rules.MinimumAIDefensiveTeams.Count ? rules.MinimumAIDefensiveTeams[ai] : 0;
    if ((!target || (rules.UseMinDefenseRule && owner->BaseDefenseTeamCount < minimum)) && !defense) return false;
    if (defense && enoughDefense) return false;
    if ((IsGlobal == 1 && ScenarioClass::Instance->IgnoreGlobalAITriggers) || !IsEnabled) return false;
    if (SessionClass::Instance.GameMode != GameMode::Campaign) {
        if (!IsForSkirmish) return false;
        if ((ai == 0 && !Enabled_Hard) || (ai == 1 && !Enabled_Normal) || (ai == 2 && !Enabled_Easy)) return false;
    } else {
        const int level = ScenarioClass::Instance->Difficulty1;
        if ((level == 0 && !Enabled_Easy) || (level == 1 && !Enabled_Normal) || (level == 2 && !Enabled_Hard))
            return false;
    }
    if (OwnerHouseType == AITriggerHouseType::None) return false;
    if (OwnerHouseType == AITriggerHouseType::Single && owner->Type->ArrayIndex2 != HouseIndex) return false;
    if (SideIndex >= 1 && SideIndex <= 3 && owner->SideIndex != SideIndex - 1) return false;
    if (TechLevel > owner->TechLevel) return false;
    bool met = false;
    const auto kind = ConditionType;
    if (target) {
        switch (kind) {
            case AITriggerCondition::Pool:
                met = true;
                break;
            case AITriggerCondition::EnemyOwns:
                met = EnemyHouseOwns(owner, target);
                break;
            case AITriggerCondition::AIOwns:
                met = OwnerHouseOwns(owner, target);
                break;
            case AITriggerCondition::EnemyYellowPowe:
            case AITriggerCondition::EnemyRedPower: {
                const int drain = int(target->Power_Drain()), output = int(target->Power_Output());
                const int spare = std::bit_cast<int>(unsigned(output) - unsigned(drain));
                met = spare < (kind == AITriggerCondition::EnemyYellowPowe ? 100 : 0);
                break;
            }
            case AITriggerCondition::EnemyCashExceeds:
                met = HouseCredits(owner, target);
                break;
            case AITriggerCondition::IronCharged:
                met = IronCurtainCharged(owner, target);
                break;
            case AITriggerCondition::ChronoCharged:
                met = ChronoSphereCharged(owner, target);
                break;
            case AITriggerCondition::NeutralOwns:
                met = CivilianHouseOwns(owner, target);
                break;
            default:
                return false;
        }
    } else
        met = kind == AITriggerCondition::AIOwns ? OwnerHouseOwns(owner, nullptr) : kind == AITriggerCondition::Pool;
    if (!met || !TeamTargetSuitable(Team1, owner, target) || (Team2 && !TeamTargetSuitable(Team2, owner, target)))
        return false;
    if (!owner->CanBuildTeam(Team1) || (Team2 && !owner->CanBuildTeam(Team2))) return false;
    if (Team1->Max >= 0 && owner->CountTeams(Team1) >= Team1->Max) return false;
    return !Team2 || Team2->Max < 0 || owner->CountTeams(Team2) < Team2->Max;
}
