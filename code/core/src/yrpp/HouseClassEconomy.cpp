// Original economic solvency and urgency queries, YR 0x4F6540 / 0x4FD9A0.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/UnitClass.h"
#include <bit>
#include <limits>

namespace {
UnitTypeClass* country_unit(const HouseClass& house, const TypeList<UnitTypeClass*>& list) {
    const unsigned owners = 1u << (unsigned(house.Type->FindParentCountryIndex()) & 31u);
    for (auto* type : list)
        if (type->OwnerFlags & owners) return type;
    return nullptr;
}
int wrap_sum(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
}

bool HouseClass::AI_IsEconomyRecoverable() {
    const bool refinery = CountResourceDestinations > 0;
    const bool miner = CountResourceGatherers > 0;
    const bool factory = CountWarfactories > 0;
    bool builder = ConYards.Count > 0;
    auto& rules = *RulesClass::Instance;
    auto* mcv = country_unit(*this, rules.BaseUnit);
    // The EXE assumes a configured MCV. Native malformed rules must not
    // dereference a missing type; valid-rule behavior remains unchanged.
    if (mcv && ActiveUnitTypes.GetItemCount(mcv->ArrayIndex)) builder = true;
    if (refinery && miner) return true;
    const int money = Available_Money();
    const bool canRefinery = FirstBuildableFromArray(rules.BuildRefinery) != nullptr;
    const bool canMiner = country_unit(*this, rules.HarvesterUnit) != nullptr;
    const int minerCost = canMiner ? country_unit(*this, rules.HarvesterUnit)->GetActualCost(this)
                                  : std::numeric_limits<int>::max();
    auto* weapons = FirstBuildableFromArray(rules.BuildWeapons);
    if (!weapons) return false;
    const int factoryCost = weapons->GetActualCost(this);
    if (!canRefinery && canMiner) {
        if (miner) return true;
        if (factory && money >= minerCost) return true;
        return builder && money >= wrap_sum(minerCost, factoryCost);
    }
    auto* refineryType = FirstBuildableFromArray(rules.BuildRefinery);
    if (!refineryType) return false;
    const int refineryCost = refineryType->GetActualCost(this);
    if (canRefinery && !canMiner) {
        if (miner) return true;
        return builder && money >= refineryCost;
    }
    if (refinery) {
        if (factory && money >= minerCost) return true;
        if (builder) {
            if (money < wrap_sum(minerCost, factoryCost)) return money >= refineryCost;
            return true;
        }
    } else if (builder && money > refineryCost) {
        // 0x4F680F is strict > here; the preceding branch uses >=.
        return true;
    }
    return false;
}

int HouseClass::AI_EvaluateEconomy() {
    if (IsControlledByHuman() || AI_IsEconomyRecoverable()) return 0;
    auto& rules = *RulesClass::Instance;
    auto* miner = country_unit(*this, rules.HarvesterUnit);
    if (CountResourceDestinations > 0) {
        // 0x4FDA65 traverses the GLOBAL building pointers, bounded by this
        // House's building count. Do not replace it with an owner-filtered list.
        for (int i = 0; i < Buildings.Count; ++i) {
            auto* building = BuildingClass::Array[i];
            if ((building->Type == rules.BuildRefinery[0] || building->Type == rules.BuildRefinery[1]) &&
                building->CurrentMission == Mission::Construction)
                return 0;
        }
        if (!miner) return 1;
        if (ProducingUnitTypeIndex != miner->ArrayIndex) {
            const int cost = miner->GetActualCost(this);
            return Available_Money() < cost ? 1 : 0;
        }
        for (int i = 0; i < FactoryClass::Array.Count; ++i) {
            auto* factory = FactoryClass::Array[i];
            if (factory->Owner != this || !factory->Object || factory->Object->WhatAmI() != AbstractType::Unit ||
                !factory->Object->GetTechnoType()->ResourceGatherer)
                continue;
            // The original uses remaining debt, not the product's full cost.
            return Available_Money() < factory->Balance ? 1 : 0;
        }
        return 1;
    }
    auto* building = FirstBuildableFromArray(rules.BuildRefinery);
    if (!building) building = FirstBuildableFromArray(rules.BuildWeapons);
    if (!building) return 1;
    if (ProducingBuildingTypeIndex != building->ArrayIndex) {
        const int cost = building->GetActualCost(this);
        return Available_Money() < cost ? 1 : 0;
    }
    for (int i = 0; i < FactoryClass::Array.Count; ++i) {
        auto* factory = FactoryClass::Array[i];
        if (factory->Owner != this || !factory->Object || factory->Object->WhatAmI() != AbstractType::Building ||
            factory->Object->GetType() != building)
            continue;
        return Available_Money() < factory->Balance ? 1 : 0;
    }
    return 1;
}
