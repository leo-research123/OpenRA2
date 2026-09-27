// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9 scenario.cpp; YR 0x686B20
// object-reader order, native map-session subset. Full campaign, network and UI
// initialization remains in InitializeWorldINI.
#include "scenario_loading.hpp"
#include "scenario_object_ini.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SmudgeClass.h"
#include "yrpp/SwizzleManagerClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/Unsorted.h"
namespace game {
namespace {
struct LoadingScope {
  const bool active = Game::IsActive;
  const int depth = Unsorted::ScenarioInit;
  LoadingScope() {
    Game::IsActive = true;
    Unsorted::ScenarioInit = depth + 1;
  }
  ~LoadingScope() {
    Unsorted::ScenarioInit = depth;
    Game::IsActive = active;
  }
};
} // namespace
bool load_scenario_objects(CCINIClass &ini, CCINIClass &rules, CCINIClass &art,
                           unsigned int &rejectedRecords) noexcept {
  rejectedRecords = 0;
  try {
    LoadingScope loading;
    const int firstHouse = HouseClass::Array.Count,
              firstBuilding = BuildingClass::Array.Count;
    if (!HouseClass::LoadFromINIList(ini, rules, firstHouse))
      return false;
    char player[512]{};
    ini.ReadString("Basic", "Player", "", player, sizeof(player));
    if (*player) {
      HouseClass::CurrentPlayer = game::scenario_house(player, firstHouse);
      if (!HouseClass::CurrentPlayer)
        return false;
      HouseClass::CurrentPlayer->IsHumanPlayer =
          HouseClass::CurrentPlayer->IsInPlayerControl = true;
      if (ScenarioClass::Instance && HouseClass::CurrentPlayer->Type)
        ScenarioClass::Instance->HumanPlayerHouseTypeIndex =
            HouseClass::CurrentPlayer->Type->ArrayIndex2;
    }
    // AI definitions are a distinct pass, not merged into the map INI.
    CCINIClass ai;
    CCFileClass file("AIMD.INI");
    if (!file.Exists())
      file.SetFileName("AI.INI");
    if (file.Exists() && ai.ReadCCFile(&file, false, false) <= 0)
      return false;
    // This native reader resolves object references directly. Keep
    // announcements alive through loading, then discard only the announcements
    // owned here.
    auto &swizzle = SwizzleManagerClass::Instance;
    struct Announcements {
      SwizzleManagerClass &swizzle;
      int count;
      ~Announcements() {
        while (swizzle.Swizzles_New.Count > count)
          swizzle.Swizzles_New.RemoveItem(swizzle.Swizzles_New.Count - 1);
      }
    } announcements{swizzle, swizzle.Swizzles_New.Count};
    load_scenario_type_definitions(ai, ini);
    // Cell/overlay animations must have their frame ranges before placement.
    for (int i = 0; i < AnimTypeClass::Array.Count; ++i)
      if (auto *type = AnimTypeClass::Array[i]; !type->Image)
        type->LoadFromINI(&art);
    unsigned int rejected = 0;
    const auto read = [&](bool result) {
      rejectedRecords += rejected;
      rejected = 0;
      return result;
    };
    if (!read(OverlayClass::ReadINI(ini, rejected)))
      return false;
    auto &map = MapClass::Instance;
    for (int i = 0; i < map.Cells.Capacity; ++i)
      if (auto *cell = map.Cells[i])
        cell->RecalcAttributes(-1);
    if (!read(TerrainClass::ReadINI(ini, rejected)))
      return false;
    if (!read(UnitClass::ReadINI(ini, rejected, firstHouse)))
      return false;
    if (!read(InfantryClass::ReadINI(ini, rejected, firstHouse)))
      return false;
    if (!read(BuildingClass::ReadINI(ini, rejected, firstHouse)))
      return false;
    // Buildings clear intersecting smudges when placed. Map-authored smudges
    // are read last in the target and must survive that placement side effect.
    if (!read(SmudgeClass::ReadINI(ini, rejected)))
      return false;
    for (auto *type : TiberiumClass::Array) {
      type->RebuildSpread();
      type->RebuildGrowth();
    }
    for (int i = firstHouse; i < HouseClass::Array.Count; ++i)
      HouseClass::Array[i]->RecheckPower = true;
    map.ComputeZoneConnections();
    map.ResetAllZones();
    map.ResetAllSubzones();
    // Complete loaded-building opening while ScenarioInit suppresses FreeUnit.
    // The target's later 0x452D40 pass connects laser fences; it is not a
    // second construction/Grand_Opening pass.
    for (int i = firstBuilding; i < BuildingClass::Array.Count; ++i) {
      auto *building = BuildingClass::Array[i];
      if (building->IsAlive && building->IsOnMap)
        building->Place(false);
    }
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace game
