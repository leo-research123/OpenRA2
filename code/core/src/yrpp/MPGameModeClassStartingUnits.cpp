#include "yrpp/MPGameModeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/YRMath.h"
#include <cwchar>
#include <stdexcept>

namespace {
bool firstObserver = false, sharedObserver = false;
bool observer(const HouseClass* house) {
    for (auto* player : NodeNameType::Array)
        if (house->IsControlledByHuman() && !std::wcscmp(house->UIName, player->Name))
            return player->SpectatorFlag == 0xFFFFFFFFu;
    return false;
}
int distance(CellStruct a, CellStruct b) {
    const int x = static_cast<short>(a.X - b.X), y = static_cast<short>(a.Y - b.Y);
    return static_cast<short>(int(Math::sqrt(double(x) * x + double(y) * y)));
}
}
bool& MPGameModeClass::FirstStartIsObserver = firstObserver;
bool& MPGameModeClass::ObserverStartShared = sharedObserver;

bool MPGameModeClass::UnfixAlliances() {
    ScenarioClass::Instance->SpecialFlags.FixedAlliance = false;
    return true;
}
bool MPGameModeClass::StartingPositionsToHouseBaseCells(char) {
    DynamicVectorClass<CellStruct> cells;
    if (!ScenarioClass::CollectStartingCells(&cells)) return false;
    for (int i = 0; i < HouseClass::Array.Count; ++i) {
        auto* house = HouseClass::Array[i];
        if (house->Type->MultiplayPassive || house->StartingPoint == -2) continue;
        if (!cells.ValidIndex(house->StartingPoint)) return false;
        house->BaseSpawnCell = cells[house->StartingPoint];
        ScenarioClass::Instance->HouseIndices[house->StartingPoint] = i;
    }
    return true;
}
bool MPGameModeClass::StartingPositionsToHouseBaseCells2(bool) {
    DynamicVectorClass<CellStruct> cells;
    if (!ScenarioClass::CollectStartingCells(&cells)) return false;
    byte used[16];
    auto& scenario = *ScenarioClass::Instance;
    for (int i = 0; i < 16; ++i) used[i] = scenario.HouseIndices[i] != -1;
    for (int i = 0; i < HouseClass::Array.Count; ++i) {
        auto* house = HouseClass::Array[i];
        if (house->Type->MultiplayPassive) continue;
        int found = -1;
        for (int j = 0; j < 16; ++j) if (scenario.HouseIndices[j] == i) found = j;
        if (found >= 0) {
            if (!cells.ValidIndex(found)) return false;
            house->BaseSpawnCell = cells[found];
        } else if (!AssignStartingPositionsToHouse(&house->BaseSpawnCell, i, &cells, used)) return false;
    }
    return true;
}
CellStruct* MPGameModeClass::AssignStartingPositionsToHouse(CellStruct* output, int index,
    DynamicVectorClass<CellStruct>* cells, byte* used) {
    if (!output || !cells || !used || cells->Count < 1 || cells->Count > 16 || !HouseClass::Array.ValidIndex(index))
        return nullptr;
    auto& scenario = *ScenarioClass::Instance;
    int first = 0;
    while (first < cells->Count && scenario.HouseIndices[first] == -1) ++first;
    int chosen = -1;
    if (first == cells->Count) {
        FirstStartIsObserver = ObserverStartShared = false;
        const bool spectator = observer(HouseClass::Array[index]);
        chosen = scenario.Random.RandomRanged(0, cells->Count - 1);
        FirstStartIsObserver = spectator;
    } else if (FirstStartIsObserver && !ObserverStartShared) {
        ObserverStartShared = true;
        scenario.HouseIndices[first] = index;
        *output = (*cells)[first];
        return output;
    } else if (observer(HouseClass::Array[index])) {
        FirstStartIsObserver = ObserverStartShared = true;
        *output = (*cells)[first];
        return output;
    } else {
        int best = -1;
        for (int i = 0; i < cells->Count; ++i) {
            if (used[i]) continue;
            int score = 0;
            for (int j = 0; j < cells->Count; ++j) if (used[j]) score += distance((*cells)[i], (*cells)[j]);
            if (score > best) { best = score; chosen = i; }
        }
    }
    if (chosen < 0) return nullptr;
    used[chosen] = 1;
    scenario.HouseIndices[chosen] = index;
    *output = (*cells)[chosen];
    return output;
}
bool MPGameModeClass::SpawnBaseUnits(HouseClass* house, std::uintptr_t) {
    if (!SessionClass::Instance.Config.Bases) return true;
    auto* type = house->FirstOwnedUnitFromArray(RulesClass::Instance->BaseUnit);
    if (!type) return false;
    auto* unit = new UnitClass(type, house);
    const auto center = house->GetBaseCenter();
    CoordStruct location;
    house->GetBaseCenterCoords(&location);
    if (unit->Unlimbo(location, DirType::North) ||
        ScenarioClass::PlaceStartingUnit(unit, &center, 1)) return true;
    delete unit;
    return false;
}
bool MPGameModeClass::GenerateStartingUnits(HouseClass* house, int& budget) {
    if (budget <= 0) return true;
    const int initial = budget;
    const unsigned owners = 1u << (unsigned(house->Type->ArrayIndex) & 31u);
    DynamicVectorClass<UnitTypeClass*> units;
    DynamicVectorClass<InfantryTypeClass*> infantry;
    for (auto* type : UnitTypeClass::Array)
        if (type->AllowedToStartInMultiplayer && (type->OwnerFlags & owners) && type->TechLevel <= house->TechLevel &&
            RulesClass::Instance->BaseUnit.FindItemIndex(type) < 0 && !units.AddItem(type)) return false;
    for (auto* type : InfantryTypeClass::Array)
        if (type->AllowedToStartInMultiplayer && (type->OwnerFlags & owners) && type->TechLevel <= house->TechLevel &&
            !infantry.AddItem(type)) return false;
    if (!units.Count && !infantry.Count) return false;
    int failures = 20;
    while (budget > 0 && failures > 0) {
        TechnoTypeClass* type;
        if (budget > initial / 3 && units.Count)
            type = units[ScenarioClass::Instance->Random.RandomRanged(0, units.Count - 1)];
        else {
            // The original dereferences an empty infantry list on malformed rules.
            if (!infantry.Count) return false;
            type = infantry[ScenarioClass::Instance->Random.RandomRanged(0, infantry.Count - 1)];
        }
        auto* object = static_cast<TechnoClass*>(type->CreateObject(house));
        const auto center = house->GetBaseCenter();
        if (ScenarioClass::PlaceStartingUnit(object, &center, 4)) {
            const int cost = type->GetCost();
            if (cost <= 0) throw std::runtime_error("Starting units require positive costs");
            budget -= cost;
            if (ScenarioClass::Instance->SpecialFlags.InitialVeteran) object->Veterancy.SetElite();
            object->ForceMission(house->IsControlledByHuman() ? Mission::Guard : Mission::Area_Guard);
        } else { delete object; --failures; }
    }
    return true;
}
bool MPGameModeClass::GenerateAllStartingUnits() {
    auto& session = SessionClass::Instance;
    int budget = 0;
    if (session.Config.UnitCount > 0) {
        unsigned owners = 0;
        for (auto* house : HouseClass::Array)
            if (!house->Type->MultiplayPassive) owners |= 1u << (unsigned(house->Type->ArrayIndex) & 31u);
        int total = 0, count = 0;
        const auto eligible = [&](TechnoTypeClass* type) {
            return type->AllowedToStartInMultiplayer && (type->OwnerFlags & owners) && type->TechLevel <= Game::TechLevel;
        };
        for (auto* type : UnitTypeClass::Array)
            if (eligible(type) && RulesClass::Instance->BaseUnit.FindItemIndex(type) < 0) { total += type->GetCost(); ++count; }
        for (auto* type : InfantryTypeClass::Array)
            if (eligible(type)) { total += type->GetCost(); ++count; }
        if (!count) count = 1;
        budget = session.Config.UnitCount * ((total + count / 2) / count);
    }
    for (auto* house : HouseClass::Array) {
        if (house->Type->MultiplayPassive || observer(house)) continue;
        int amount = budget;
        // Preserve the original budget pointer without narrowing it on native hosts.
        if (!SpawnBaseUnits(house, reinterpret_cast<std::uintptr_t>(&amount)) ||
            !GenerateStartingUnits(house, amount)) return false;
        if (amount > 0) house->GiveMoney(amount);
        house->FactoryProducedBuildingTypes.Clear();
        house->FactoryProducedAircraftTypes.Clear();
        house->FactoryProducedInfantryTypes.Clear();
        house->FactoryProducedUnitTypes.Clear();
    }
    ScenarioClass::Instance->Random.RandomRanged(0, 0xFFFF);
    return true;
}
bool MPGameModeClass::AllyTeams() {
    for (int i = 0; i < HouseClass::Array.Count; ++i) {
        auto* a = HouseClass::Array[i];
        const int team = static_cast<int>(a->StartingAllies.data);
        if (a->Type->MultiplayPassive || team == -2 || team == -1) continue;
        for (int j = i + 1; j < HouseClass::Array.Count; ++j) {
            auto* b = HouseClass::Array[j];
            if (!b->Type->MultiplayPassive && b->StartingAllies.data == a->StartingAllies.data) {
                a->MakeAlly(b, false); b->MakeAlly(a, false);
            }
        }
    }
    return true;
}
