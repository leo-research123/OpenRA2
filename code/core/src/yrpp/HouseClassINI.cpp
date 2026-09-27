// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9 house.cpp Read_INI; YR house-list
// reader 0x5009B0. Native subset preserves implicit owners used by map-only
// sessions, including maps without a [Houses] list.
#include "scenario_object_ini.hpp"
#include "yrpp/CCINIClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/HouseClass.h"
#include <cstdio>
#include <memory>
namespace {
std::string text(CCINIClass &ini, const char *section, const char *key,
                 const char *fallback = "") {
  char value[512]{};
  ini.ReadString(section, key, fallback, value, sizeof(value));
  return value;
}
bool read_house(CCINIClass &rules, const char *name, int first) {
  if (!name || !*name || INIClass::IsBlankValue(name))
    return true;
  if (game::scenario_house(name, first))
    return true;
  const auto country = text(rules, name, "Country", name);
  auto *type = HouseTypeClass::Find(country.c_str());
  if (!type)
    type = GameCreate<HouseTypeClass>(country.c_str());
  if (!type)
    return false;
  if (rules.GetSection(country.c_str()) && !type->LoadFromINI(&rules))
    return false;
  std::unique_ptr<HouseClass> house(new HouseClass(type));
  std::snprintf(house->PlainName, sizeof(house->PlainName), "%s", name);
  house->TechLevel = rules.ReadInteger(name, "TechLevel", 1);
  house->StartingCredits = rules.ReadInteger(name, "Credits", 0) * 100;
  house->Balance = house->StartingCredits;
  const auto color =
      text(rules, name, "Color", text(rules, country.c_str(), "Color").c_str());
  house->Color = ColorScheme::HSVToRGB(
      rules.ReadColor("Colors", color.c_str(), ColorStruct{0, 0, 128}));
  house->LaserColor = house->Color;
  const int scheme=ColorScheme::FindIndex(color.c_str());
  if(scheme>=0)house->ColorSchemeIndex=scheme;
  house.release();
  return true;
}
} // namespace
bool HouseClass::LoadFromINIList(CCINIClass &map, CCINIClass &rules,
                                 int firstHouse) noexcept {
  try {
    if (firstHouse < 0 || firstHouse > Array.Count)
      return false;
    if (auto *section = map.GetSection("Houses"))
      for (auto *entry : section->Entries)
        if (!read_house(rules, entry->Value, firstHouse))
          return false;
    const auto player = text(map, "Basic", "Player");
    if (!read_house(rules, player.c_str(), firstHouse))
      return false;
    // Resolve implicit owners before readers create any objects. The readers
    // only look up houses; they do not allocate or initialize them themselves.
    for (const char *name : {"Structures", "Infantry", "Units"})
      if (auto *section = map.GetSection(name))
        for (auto *entry : section->Entries) {
          const auto row = game::scenario_fields(entry->Value);
          if (!row.empty() && !read_house(rules, row[0].c_str(), firstHouse))
            return false;
        }
    // All houses must exist before resolving the map's alliance lists.
    // Scenario loading has no live combat targets to detach; preserve the
    // directional masks and initial-alliance state used by the original.
    for (int i = firstHouse; i < Array.Count; ++i) {
      auto* house = Array[i];
      house->Allies.data |= 1u << (unsigned(house->ArrayIndex) & 31u);
      const auto allies = game::scenario_fields(text(map, house->PlainName, "Allies").c_str());
      for (const auto& name : allies)
        if (auto* ally = game::scenario_house(name.c_str(), firstHouse))
          house->Allies.data |= 1u << (unsigned(ally->ArrayIndex) & 31u);
      house->StartingAllies = house->Allies;
    }
    return true;
  } catch (...) {
    return false;
  }
}
