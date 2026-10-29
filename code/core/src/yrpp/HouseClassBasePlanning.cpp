// Base plan generation and initialization, YR 0x5054B0 / 0x505180 / 0x50C210.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include <cstring>
#include <utility>
namespace {
void insert_after(DynamicVectorClass<BuildingTypeClass*>& list, int index, BuildingTypeClass* value) {
    if (!list.AddItem(value)) return;
    for (int i = list.Count - 1; i > index + 1; --i)
        list[i] = list[i - 1];
    list[index + 1] = value;
}
}
void HouseClass::GenerateBasePlan() noexcept {
    try {
        const int side = Type->SideIndex;
        Base.BaseNodes.Clear();
        const unsigned owners = 1u << (unsigned(Type->FindParentCountryIndex()) & 31u);
        const unsigned country = 1u << (unsigned(Type->ArrayIndex2) & 31u);
        const auto& rules = *RulesClass::Instance;
        DynamicVectorClass<BuildingTypeClass*> eligible;
        DynamicVectorClass<bool> used;
        for (auto* type : BuildingTypeClass::Array) {
            if (!(type->OwnerFlags & owners) || (type->AIBasePlanningSide != -1 && type->AIBasePlanningSide != side) ||
                !type->AIBuildThis || type->TechLevel > TechLevel ||
                (type->RequiredHouses != -1 && !(unsigned(type->RequiredHouses) & country)) ||
                (type->ForbiddenHouses != -1 && (unsigned(type->ForbiddenHouses) & country)))
                continue;
            if (!SessionClass::Instance.Config.SWAllowed && type->SuperWeapon != -1 &&
                rules.BuildTech.FindItemIndex(type) < 0 && Supers[type->SuperWeapon]->Type->DisableableFromShell)
                continue;
            // The EXE assumes its two scratch arrays grow together. Stop safely on
            // native allocation failure rather than indexing an incomplete pair.
            if (!eligible.AddItem(type) || !used.AddItem(false)) return;
        }
        DynamicVectorClass<BuildingTypeClass*> plan;
        auto* yard = FirstBuildableFromArray(rules.BuildConst);
        const int yardIndex = eligible.FindItemIndex(yard);
        if (yardIndex >= 0) {
            used[yardIndex] = true;
            plan.AddItem(yard);
        }
        auto* power = FirstBuildableFromArray(rules.BuildPower);
        // Missing power rules are undefined in the original conversion loop.
        if (!power) return;
        plan.AddItem(power);
        const auto reorder = [&](BuildingTypeClass* type, int slot) {
            const int index = eligible.FindItemIndex(type);
            if (index >= 0 && slot < eligible.Count) {
                std::swap(eligible[slot], eligible[index]);
                used[index] = used[slot];
                used[slot] = false;
            }
        };
        reorder(FirstBuildableFromArray(rules.BuildBarracks), 0);
        reorder(FirstBuildableFromArray(rules.BuildWeapons), 1);
        int remaining = eligible.Count - 1;
        while (remaining > 0) {
            int deferredPlug = -1;
            bool progressed = false;
            const int available = plan.Count;
            for (int i = 0; i < eligible.Count; ++i) {
                if (used[i]) continue;
                auto* type = eligible[i];
                if (!_strcmpi(type->ID, "GAPLUG")) {
                    deferredPlug = i;
                } else if (AllPrerequisitesAvailable(type, plan, available)) {
                    used[i] = true;
                    plan.AddItem(type);
                    progressed = true;
                    --remaining;
                }
            }
            if (!progressed) {
                if (deferredPlug < 0) break;
                used[deferredPlug] = true;
                plan.AddItem(eligible[deferredPlug]);
                --remaining;
            }
        }
        auto* refinery = FirstBuildableFromArray(rules.BuildRefinery);
        const unsigned minerOwners = 1u << (unsigned(Type->FindParentCountryIndex()) & 31u);
        UnitTypeClass* miner = nullptr;
        for (auto* type : rules.HarvesterUnit)
            if (type->OwnerFlags & minerOwners) {
                miner = type;
                break;
            }
        const int difficulty = int(AIDifficulty);
        const int extra = miner ? rules.AIExtraRefineries[difficulty] : rules.AISlaveMinerNumber[difficulty] - 1;
        for (int i = 0; i < plan.Count - 1; ++i) {
            if (plan[i] != refinery) continue;
            for (int n = 0; n < extra; ++n)
                insert_after(plan, ScenarioClass::Instance->Random.RandomRanged(i, plan.Count - 1), refinery);
            break;
        }
        // The original copies the first three entries unconditionally. Malformed rules
        // or allocation failure can leave fewer entries; do not dereference garbage.
        if (plan.Count < 3) return;
        DynamicVectorClass<BuildingTypeClass*> expanded;
        for (auto* type : plan)
            expanded.AddItem(type);
        const int defenses = side == 0   ? rules.AlliedBaseDefenseCounts[difficulty]
                             : side == 1 ? rules.SovietBaseDefenseCounts[difficulty]
                             : side == 2 ? rules.ThirdBaseDefenseCounts[difficulty]
                                         : 0;
        for (int i = 0; i < defenses; ++i) {
            const int at = ScenarioClass::Instance->Random.RandomRanged(3, expanded.Count - 1);
            insert_after(expanded, at, reinterpret_cast<BuildingTypeClass*>(intptr_t(-1)));
        }
        for (auto* type : expanded) {
            const auto raw = reinterpret_cast<uintptr_t>(type);
            const int index = raw >= uintptr_t(-4) ? int(static_cast<intptr_t>(raw)) : type->ArrayIndex;
            Base.BaseNodes.AddItem({index, CellStruct::Empty, false, 0});
        }
    } catch (...) {
        // Native allocation/service failure stops this phase; never unwind into the caller.
    }
}
void HouseClass::EnsureBasePlan() noexcept {
    try {
        if (!IsControlledByHuman() && SessionClass::Instance.GameMode != GameMode::Campaign) AllyAIHouses();
        if (!Base.BaseNodes.Count) {
            const int saved = Unsorted::ScenarioInit;
            struct RestoreScenarioInit {
                int value;
                ~RestoreScenarioInit() { Unsorted::ScenarioInit = value; }
            } restore{saved};
            Unsorted::ScenarioInit = 0;
            GenerateBasePlan();
        }
    } catch (...) {
        // Native allocation/service failure stops this phase; never unwind into the caller.
    }
}
void HouseClass::InitializeBase() noexcept {
    try {
        for (int i = Buildings.Count - 1; i >= 0; --i) {
            auto* building = Buildings[i];
            if (RulesClass::Instance->BuildConst.FindItemIndex(building->Type) < 0 || building->InLimbo) continue;
            const CellStruct cell{short(building->Location.X / 256), short(building->Location.Y / 256)};
            BaseSpawnCell = cell;
            EnsureBasePlan();
            if (Base.BaseNodes.Count) Base.BaseNodes[0].MapCoords = cell;
            Base.Center = cell;
            return;
        }
    } catch (...) {
        // Native allocation/service failure stops this phase; never unwind into the caller.
    }
}
// 0x50C920 gathers existing mobile objects after AI construction-yard deployment.
void HouseClass::GatherUnitsAtBase() noexcept {
    try {
        const int count = TechnoClass::Array.Count;
        const CellStruct closeTo{};
        CellStruct cell;
        MapClass::Instance.NearByLocation(cell, GetBaseCenter(), SpeedType(0), -1, MovementZone(0), false, 4, 4, false,
                                          false, false, true, closeTo, false, false);
        if (cell == CellStruct::Empty) return;
        cell.X = short(cell.X + 1);
        cell.Y = short(cell.Y + 1);
        for (int i = 0; i < count; ++i) {
            auto* techno = TechnoClass::Array[i];
            if (!techno || techno->Owner != this || techno->WhatAmI() == AbstractType::Building ||
                techno->WhatAmI() == AbstractType::Aircraft)
                continue;
            auto* deploysInto = techno->GetTechnoType()->DeploysInto;
            if (deploysInto && techno->GetTechnoType()->DeploysInto->ConstructionYard) continue;
            if (GetBaseCenter() == CellStruct::Empty) continue;
            techno->SetDestination(MapClass::Instance.GetCellAt(cell), true);
            techno->SetArchiveTarget(nullptr);
            techno->QueueMission(Mission::Move, false);
        }
    } catch (...) {
        // Native allocation/service failure stops this phase; never unwind into the caller.
    }
}
