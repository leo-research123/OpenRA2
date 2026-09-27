// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 aitrig.cpp Read_INI / Read_All; YR 0x41F580 / 0x41F2E0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "scenario_runtime.hpp"
#include "scenario_script_ini.hpp"
#include "yrpp/AITriggerTypeClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
namespace {
double weight(const std::string &value) {
  // The EXE truncates to an integer, then stores the unsigned low DWORD as a
  // double. In particular a fractional INI weight is not retained.
  const double parsed = std::strtod(value.c_str(), nullptr);
  if (!std::isfinite(parsed))
    return 0; // Undefined original conversion input.
  double result = std::fmod(std::trunc(parsed), 4294967296.0);
  if (result < 0)
    result += 4294967296.0;
  return result;
}
} // namespace
bool AITriggerTypeClass::LoadFromINI(CCINIClass *ini) {
  if (!ini)
    return false;
  try {
    char buffer[512]{};
    if (!ini->ReadString("AITriggerTypes", ID, "", buffer, sizeof(buffer)))
      return false;
    const auto row = game::scenario_fields(buffer);
    if (row.size() < 7)
      return false;
    std::snprintf(Name, sizeof(Name), "%s", row[0].c_str());
    Team1 = !_strcmpi(row[1].c_str(), "<none>")
                ? nullptr
                : TeamTypeClass::FindByNameOrID(row[1].c_str());
    OwnerHouseType = AITriggerHouseType::None;
    HouseIndex = -1;
    if (!_strcmpi(row[2].c_str(), "<all>"))
      OwnerHouseType = AITriggerHouseType::Any;
    else if (!INIClass::IsBlankValue(row[2].c_str())) {
      HouseIndex = HouseTypeClass::FindIndexOfName(row[2].c_str());
      if (HouseIndex != -1)
        OwnerHouseType = AITriggerHouseType::Single;
    }
    // The serialized tech-level field is ignored; derive it from both teams.
    TechLevel = 0;
    ConditionType = AITriggerCondition(std::atoi(row[4].c_str()));
    ConditionObject = InfantryTypeClass::Find(row[5].c_str());
    if (!ConditionObject)
      ConditionObject = UnitTypeClass::Find(row[5].c_str());
    if (!ConditionObject)
      ConditionObject = AircraftTypeClass::Find(row[5].c_str());
    if (!ConditionObject)
      ConditionObject = BuildingTypeClass::Find(row[5].c_str());
    auto *bytes = reinterpret_cast<unsigned char *>(Conditions);
    const char *cursor = row[6].c_str();
    for (unsigned i = 0; *cursor && i < sizeof(Conditions); ++i) {
      while (*cursor && std::isspace(static_cast<unsigned char>(*cursor)))
        ++cursor;
      if (!*cursor)
        break;
      char pair[3]{*cursor++, 0, 0};
      if (*cursor)
        pair[1] = *cursor++;
      bytes[i] = static_cast<unsigned char>(std::strtoul(pair, nullptr, 16));
    }
    if (row.size() > 7)
      Weight_Current = weight(row[7]);
    if (row.size() > 8)
      Weight_Minimum = weight(row[8]);
    if (row.size() > 9)
      Weight_Maximum = weight(row[9]);
    if (row.size() > 10)
      IsForSkirmish = std::atoi(row[10].c_str()) != 0;
    // Field 11 is unused in the target.
    if (row.size() > 12)
      SideIndex = std::atoi(row[12].c_str());
    if (row.size() > 13)
      IsForBaseDefense = std::atoi(row[13].c_str()) != 0;
    if (row.size() > 14)
      Team2 = !_strcmpi(row[14].c_str(), "<none>")
                  ? nullptr
                  : TeamTypeClass::FindByNameOrID(row[14].c_str());
    if (row.size() > 15)
      Enabled_Easy = std::atoi(row[15].c_str()) != 0;
    if (row.size() > 16)
      Enabled_Normal = std::atoi(row[16].c_str()) != 0;
    if (row.size() > 17)
      Enabled_Hard = std::atoi(row[17].c_str()) != 0;
    for (const auto *team : {Team1, Team2})
      if (team && team->TaskForce)
        TechLevel =
            std::max(TechLevel, team->TaskForce->GetRequiredTechLevel());
    return true;
  } catch (...) {
    return false;
  }
}
void YRPP_FASTCALL AITriggerTypeClass::LoadFromINIList(CCINIClass *ini,
                                                       int scope) noexcept {
  try {
    if (!ini)
      return;
    for (int i = 0; i < ini->GetKeyCount("AITriggerTypes"); ++i) {
      auto *type = game::allocate_type<AITriggerTypeClass>(
          ini->GetKeyName("AITriggerTypes", i));
      if (!type)
        std::abort();
      type->LoadFromINI(ini);
      type->IsGlobal = scope;
      if (scope == 1)
        type->IsEnabled = true;
    }
    if (scope == 0) {
      const auto &runtime = game::scenario_runtime();
      const bool campaign = runtime.session_mode(runtime.context) == 0;
      for (int i = 0; i < ini->GetKeyCount("AITriggerTypesEnable"); ++i) {
        const char *id = ini->GetKeyName("AITriggerTypesEnable", i);
        if (auto *type = Find(id))
          type->IsEnabled =
              ini->ReadBool("AITriggerTypesEnable", id, false) || !campaign;
      }
    }
  } catch (...) {
    std::abort();
  }
}
