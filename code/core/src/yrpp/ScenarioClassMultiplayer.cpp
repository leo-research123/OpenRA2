#include "yrpp/ScenarioClass.h"
#include "yrpp/MPGameModeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/YRMath.h"
#include "scenario_runtime.hpp"
#include "x87_integer.hpp"
#include <algorithm>
#include <stdexcept>

void ScenarioClass::AssignSecretLabProduction() {
    auto& rules = *RulesClass::Instance;
    if (!BuildingClass::SecretLabArray.Count || rules.SecretSum <= 0 ||
        rules.SecretSum < BuildingClass::SecretLabArray.Count) return;
    DynamicVectorClass<int> choices;
    for (int i = 0; i < rules.SecretSum; ++i) if (!choices.AddItem(i)) return;
    TechnoTypeClass* chosen = nullptr;
    for (auto* lab : BuildingClass::SecretLabArray) {
        int index = Random.RandomRanged(0, choices.Count - 1);
        choices.RemoveItem(index);
        // The original uses the shrinking-list index, not its removed value.
        if (index < rules.SecretInfantry.Count) chosen = rules.SecretInfantry[index];
        else if ((index -= rules.SecretInfantry.Count) < rules.SecretUnits.Count) chosen = rules.SecretUnits[index];
        else if ((index -= rules.SecretUnits.Count) < rules.SecretBuildings.Count) chosen = rules.SecretBuildings[index];
        if (chosen) lab->SecretProduction = chosen;
    }
}
void YRPP_FASTCALL ScenarioClass::InitializeMultiplayer(bool official) {
    auto& session = SessionClass::Instance;
    auto& rules = *RulesClass::Instance;
    const auto& services = game::scenario_runtime();
    const auto pump = [&] { if (services.ini && services.ini->pump_events) services.ini->pump_events(services.ini->context); };
    if (session.Config.AIPlayers + NodeNameType::Array.Count < rules.Players && !Unsorted::ArmageddonMode)
        for (int i = 0; i < HouseClass::Array.Count; ++i) {
            auto* house = HouseClass::Array[i];
            if (!house->IsHumanPlayer && !house->Type->MultiplayPassive && session.Config.AIPlayers < 1)
                house->SDDTORAllAndTriggers();
        }
    pump();
    if (!Unsorted::ArmageddonMode) {
        struct Restore { int value; ~Restore(){ Unsorted::ScenarioInit = value; } } restore{Unsorted::ScenarioInit};
        Unsorted::ScenarioInit = 0;
        if (session.MPGameMode) {
            if (!session.MPGameMode->StartingPositionsToHouseBaseCells2(official) ||
                !session.MPGameMode->GenerateAllStartingUnits())
                throw std::runtime_error("Multiplayer starting forces could not be placed");
        } else GenerateLegacyStartingUnits(official);
    }
    if (services.ini && services.ini->progress) services.ini->progress(services.ini->context, 93);
    pump();
    if (session.Config.Crates) {
        int crates = std::min(std::max(Game::PlayerCount, rules.CrateMinimum), rules.CrateMaximum);
        while (crates-- > 0) MapClass::Instance.PlaceRandomCrate();
    }
    pump();
    DisplayClass::Instance.InitializeMultiplayerView();
    HouseClass* special = nullptr;
    for (auto* house : HouseClass::Array) if (!_strcmpi(house->Type->ID, "Special")) { special = house; break; }
    for (auto* house : HouseClass::Array) {
        house->Base.Owner = house;
        if (!house->IsHumanPlayer && !house->Type->MultiplayPassive) {
            const int difficulty = static_cast<int>(house->AIDifficulty);
            house->Unknown_Timer_5640.Start(rules.AIHateDelays[difficulty]);
            house->GiveMoney(game::x87_integer(double(rules.MultiplayerAICM[difficulty]) * 0.01 * house->Available_Money()));
        }
        house->Type->WallOwner = !house->Type->MultiplayPassive;
        if (special && house != special) { house->MakeAlly(special, false); special->MakeAlly(house, false); }
    }
    if (session.MPGameMode) { session.MPGameMode->AllyTeams(); session.MPGameMode->vt_entry_8C(); }
    Instance->AssignSecretLabProduction();
    std::fill_n(Game::ChatMask, 8, byte{1});
}
void YRPP_FASTCALL ScenarioClass::GenerateLegacyStartingUnits(bool) {
    auto& rules = *RulesClass::Instance;
    auto& config = SessionClass::Instance.Config;
    auto& random = Instance->Random;
    int total = 0, count = 0;
    for (auto* type : UnitTypeClass::Array)
        if (type->AllowedToStartInMultiplayer && rules.BaseUnit.FindItemIndex(type) < 0) { total += type->GetCost(); ++count; }
    for (auto* type : InfantryTypeClass::Array)
        if (type->AllowedToStartInMultiplayer) { total += type->GetCost(); ++count; }
    if (!count) throw std::runtime_error("No multiplayer starting-unit types");
    const int budget = (config.UnitCount - int(config.Bases)) * (total / count);
    DynamicVectorClass<CellStruct> cells;
    if (!CollectStartingCells(&cells) || !cells.Count) throw std::runtime_error("No multiplayer starting positions");
    bool used[16]{};
    bool first = true;
    for (auto* house : HouseClass::Array) {
        if (house->Type->MultiplayPassive) continue;
        DynamicVectorClass<UnitTypeClass*> units;
        DynamicVectorClass<InfantryTypeClass*> infantry;
        const unsigned owners = 1u << (unsigned(house->Type->ArrayIndex) & 31u);
        for (auto* type : UnitTypeClass::Array)
            if (type->AllowedToStartInMultiplayer && (type->OwnerFlags & owners) && type->TechLevel <= house->TechLevel &&
                rules.BaseUnit.FindItemIndex(type) < 0) units.AddItem(type);
        for (auto* type : InfantryTypeClass::Array)
            if (type->AllowedToStartInMultiplayer && (type->OwnerFlags & owners) && type->TechLevel <= house->TechLevel)
                infantry.AddItem(type);
        int chosen = 0;
        if (first) { chosen = random.RandomRanged(0, cells.Count - 1); first = false; }
        else {
            int best = 0;
            for (int i = 0; i < cells.Count; ++i) {
                int score = 0;
                if (!used[i]) for (int j = 0; j < cells.Count; ++j) if (used[j]) {
                    const int x = short(cells[i].X - cells[j].X), y = short(cells[i].Y - cells[j].Y);
                    score += short(int(Math::sqrt(double(x) * x + double(y) * y)));
                }
                if (score > best || !best) { best = score; chosen = i; }
            }
        }
        used[chosen] = true;
        house->BaseSpawnCell = cells[chosen];
        if (config.Bases) {
            auto* type = house->FirstOwnedUnitFromArray(rules.BaseUnit);
            if (!type) throw std::runtime_error("Missing country base unit");
            auto* unit = new UnitClass(type, house);
            CoordStruct at{cells[chosen].X * 256 + 128, cells[chosen].Y * 256 + 128, 0};
            if (unit->Unlimbo(at, DirType::North) || PlaceStartingUnit(unit, &cells[chosen], 1)) {
                house->OurFlagCarrier = nullptr;
                house->OurFlagCoords = {0, 0};
                if (Instance->SpecialFlags.CTFMode) house->PickedUpFlag(unit, 1);
            } else delete unit;
        }
        int spent = 0, attempts = 0;
        while (spent < budget) {
            if (++attempts > 4096) throw std::runtime_error("Legacy starting-unit placement exhausted");
            TechnoTypeClass* type = nullptr;
            if (spent < 2 * budget / 3 && units.Count) type = units[random.RandomRanged(0, units.Count - 1)];
            else if (infantry.Count) type = infantry[random.RandomRanged(0, infantry.Count - 1)];
            if (!type) throw std::runtime_error("Missing eligible starting infantry");
            auto* object = static_cast<TechnoClass*>(type->CreateObject(house));
            if (PlaceStartingUnit(object, &cells[chosen], 4)) {
                const int cost = type->GetCost();
                if (cost <= 0) throw std::runtime_error("Starting units require positive costs");
                spent += cost;
                if (Instance->SpecialFlags.InitialVeteran) object->Veterancy.SetElite();
                object->ForceMission(house->IsControlledByHuman() ? Mission::Guard : Mission::Area_Guard);
            } else delete object;
        }
    }
    random.RandomRanged(0, 0xFFFF);
}
