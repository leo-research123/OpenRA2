// Original YR building infiltration, 0x4571E0.
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/RadarEventClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstdint>

void BuildingClass::SpiedBy(HouseClass* house) {
    if (Owner == house) return;
    bool visible = false;
    if (Owner->IsControlledByCurrentPlayer() || house->IsControlledByCurrentPlayer())
        visible = RadarEventClass::Create(RadarEventType::BuildingInfiltrated, GetMapCoords());
    const auto play = [&](const char* name) { VoxClass::Play(name, -1, -1); };
    const auto infiltrated = [&] {
        if (house->IsControlledByCurrentPlayer() && visible) play("EVA_BuildingInfiltrated");
    };
    auto& rules = *RulesClass::Instance;
    if (Type->Radar) {
        Owner->ReshroudMap();
        if (Owner->IsControlledByCurrentPlayer() && !Owner->SpySatActive && visible) play("EVA_RadarSabotaged");
        if (house->IsControlledByCurrentPlayer() && !Owner->SpySatActive && visible)
            play("EVA_BuildingInfRadarSabotaged");
    } else if (Type->PowerBonus > 0) {
        Owner->CreatePowerOutage(rules.SpyPowerBlackout);
        if (Owner->IsControlledByCurrentPlayer() && visible)
            play("EVA_PowerSabotaged");
        else if (house->IsControlledByCurrentPlayer() && visible) {
            play("EVA_BuildingInfiltrated");
            play("EVA_EnemyBasePoweredDown");
        }
    } else if (rules.BuildTech.FindItemIndex(Type) >= 0) {
        if (Type->AIBasePlanningSide == 0)
            house->Side0TechInfiltrated = true;
        else if (Type->AIBasePlanningSide == 1)
            house->Side1TechInfiltrated = true;
        else
            house->Side2TechInfiltrated = true;
        house->RecheckTechTree = true;
        if (Owner->IsControlledByCurrentPlayer() && visible) play("EVA_TechnologyStolen");
        if (house->IsControlledByCurrentPlayer() && visible) {
            play("EVA_BuildingInfiltrated");
            play("EVA_NewTechnologyAcquired");
        }
    } else if (Type->SuperWeapon != -1) {
        Owner->Supers[Type->SuperWeapon]->Reset();
        if (Owner->IsControlledByCurrentPlayer() || (house->IsControlledByCurrentPlayer() && visible))
            play("EVA_BuildingInfiltrated");
    } else if (Type->Storage > 0) {
        const int money = Owner->Available_Money();
        const double value = double(money) * double(rules.SpyMoneyStealPercent);
        const int stolen = game::x87_integer(value);
        Owner->TakeMoney(stolen);
        house->GiveMoney(stolen);
        if (Owner->IsControlledByCurrentPlayer() && visible) play("EVA_CashStolen");
        if (house->IsControlledByCurrentPlayer() && visible) play("EVA_BuildingInfCashStolen");
    } else if (Type->Factory == AbstractType::UnitType || Type->Factory == AbstractType::InfantryType) {
        if (Type->Factory == AbstractType::UnitType)
            house->WarFactoryInfiltrated = true;
        else
            house->BarracksInfiltrated = true;
        house->RecheckTechTree = true;
        if (house->IsControlledByCurrentPlayer()) SidebarClass::Instance.SidebarNeedsRedraw = true;
        if (Owner->IsControlledByCurrentPlayer() && visible) play("EVA_TechnologyStolen");
        if (house->IsControlledByCurrentPlayer() && visible) {
            play("EVA_BuildingInfiltrated");
            play("EVA_NewTechnologyAcquired");
        }
    }
    Mark(MarkType::Change);
}
