// Original team-demand production, YR 0x004FEA60 / 0x004FEEE0 / 0x004FF210.
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/ScenarioClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstdlib>
#include <limits>
#include <vector>

namespace {
template <class Type, class Object>
void select_team_product(HouseClass& house, int& requested, const DynamicVectorClass<Type*>& types,
                         const DynamicVectorClass<Object*>& objects, AbstractType kind) noexcept {
    try {
        // The EXE uses 100 stack slots. Size scratch storage to the actual type
        // table so larger mod tables do not inherit its out-of-bounds writes.
        struct Demand {
            int count = 0;
            int oldest = std::numeric_limits<int>::max();
            int choice = 0;
        };
        Demand originalScratch[100];
        std::vector<Demand> extendedScratch;
        if (types.Count > 100) extendedScratch.resize(types.Count);
        auto* scratch = types.Count > 100 ? extendedScratch.data() : originalScratch;
        for (int teamIndex = 0; teamIndex < TeamClass::Array.Count; ++teamIndex) {
            auto* team = TeamClass::Array[teamIndex];
            if (!team ||
                !((team->Type->Reinforce && !team->IsFullStrength) || (!team->IsForcedActive && !team->IsHasBeen)) ||
                team->Owner != &house)
                continue;
            DynamicVectorClass<TechnoTypeClass*> missing;
            const int creationFrame = team->CreationFrame;
            team->GetTaskForceMissingMemberTypes(missing);
            for (auto* type : missing) {
                if (type->WhatAmI() != kind) continue;
                const int index = static_cast<Type*>(type)->ArrayIndex;
                if (index < 0 || index >= types.Count) continue;
                scratch[index].count = std::bit_cast<int>(unsigned(scratch[index].count) + 1u);
                if (creationFrame < scratch[index].oldest) scratch[index].oldest = creationFrame;
            }
        }
        for (int objectIndex = 0; objectIndex < objects.Count; ++objectIndex) {
            auto* object = objects[objectIndex];
            if (!object) continue;
            const int index = object->Type->ArrayIndex;
            if (index >= 0 && index < types.Count && scratch[index].count > 0 && object->CanBeRecruited(&house))
                --scratch[index].count;
        }
        int maximum = -1, earliest = -1, firstFrame = std::numeric_limits<int>::max();
        int choiceCount = 0;
        for (int i = 0; i < types.Count; ++i) {
            if (scratch[i].count <= 0 || house.CanBuild(types[i], false, false) == CanBuildResult::Unbuildable)
                continue;
            const int cost = types[i]->GetActualCost(&house);
            if (cost > int(house.Available_Money())) continue;
            // 0x4FEDF8 / 0x4FF122: only a NEW maximum clears the list.
            // Later smaller demands are still appended; do not filter to ties.
            if (maximum == -1 || maximum < scratch[i].count) {
                maximum = scratch[i].count;
                choiceCount = 0;
            }
            scratch[choiceCount++].choice = i;
            if (earliest == -1 || scratch[i].oldest < firstFrame) {
                earliest = i;
                firstFrame = scratch[i].oldest;
            }
        }
        auto& random = ScenarioClass::Instance->Random;
        // Even an empty candidate list consumes this first random draw.
        const int roll = random.RandomRanged(0, 0x7FFFFFFE);
        const int probability = RulesClass::Instance->FillEarliestTeamProbability[int(house.AIDifficulty)];
        if (game::x87_truncated_product(double(probability), 0.01) >
            game::x87_truncated_product(double(roll), 4.656612877414201e-10))
            requested = earliest;
        else if (choiceCount)
            requested = scratch[random.RandomRanged(0, choiceCount - 1)].choice;
    } catch (...) {
        // Scratch allocation failure must not propagate across the class ABI.
        std::abort();
    }
}
}

int HouseClass::AI_VehicleConstructionUpdate() {
    if (ProducingUnitTypeIndex != -1) return 15;
    auto& rules = *RulesClass::Instance;
    const unsigned owners = 1u << (unsigned(Type->FindParentCountryIndex()) & 31u);
    UnitTypeClass* harvester = nullptr;
    for (auto* type : rules.HarvesterUnit)
        if (type->OwnerFlags & owners) {
            harvester = type;
            break;
        }
    const int difficulty = int(AIDifficulty);
    if (harvester) {
        int desired =
            std::bit_cast<int>(unsigned(CountResourceDestinations) * unsigned(rules.HarvestersPerRefinery[difficulty]));
        if (!FirstBuildableFromArray(rules.BuildRefinery)) desired = rules.AISlaveMinerNumber[difficulty];
        if (IQLevel2 >= rules.Harvester && !IsTiberiumShort && !IsControlledByHuman() &&
            desired > CountResourceGatherers && unsigned(harvester->TechLevel) <= unsigned(TechLevel)) {
            ProducingUnitTypeIndex = harvester->ArrayIndex;
            return 15;
        }
    } else if (CountResourceGatherers < rules.AISlaveMinerNumber[difficulty]) {
        auto* refinery = FirstBuildableFromArray(rules.BuildRefinery);
        if (refinery && refinery->UndeploysInto) {
            ProducingUnitTypeIndex = refinery->UndeploysInto->ArrayIndex;
            return 15;
        }
    }
    select_team_product(*this, ProducingUnitTypeIndex, UnitTypeClass::Array, UnitClass::Array, AbstractType::UnitType);
    return 15;
}
int HouseClass::AI_InfantryConstructionUpdate() {
    if (ProducingInfantryTypeIndex == -1)
        select_team_product(*this, ProducingInfantryTypeIndex, InfantryTypeClass::Array, InfantryClass::Array,
                            AbstractType::InfantryType);
    return 15;
}
int HouseClass::AI_AircraftConstructionUpdate() {
    if (ProducingAircraftTypeIndex == -1)
        select_team_product(*this, ProducingAircraftTypeIndex, AircraftTypeClass::Array, AircraftClass::Array,
                            AbstractType::AircraftType);
    return 15;
}
