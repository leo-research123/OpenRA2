// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 teamtype.cpp Read_INI; calibrated to YR 0x6F1090.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "scenario_script_ini.hpp"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TeamTypeClass.h"

bool TeamTypeClass::LoadFromINI(CCINIClass *ini) {
  if (!ini)
    return false;
  try {
    if (!AbstractTypeClass::LoadFromINI(ini))
      return false;
    const int country = ini->ReadHouseType(
        ID, "House", Owner && Owner->Type ? Owner->Type->ArrayIndex2 : -1);
    if (HouseClass::Index_IsMP(country)) {
      idxHouse = country;
      Owner = nullptr;
    } else if (country != -1) {
      Owner = nullptr;
      idxHouse = -1;
      for (auto *candidate : HouseClass::Array)
        if (candidate->Type && candidate->Type->ArrayIndex2 == country) {
          Owner = candidate;
          break;
        }
    }
#define READ_INT(field) field = ini->ReadInteger(ID, #field, field)
    READ_INT(VeteranLevel);
    READ_INT(MindControlDecision);
    READ_INT(Priority);
    READ_INT(Max);
    READ_INT(TechLevel);
    READ_INT(Group);
#undef READ_INT
#define READ_BOOL(field) field = ini->ReadBool(ID, #field, field)
    READ_BOOL(Loadable);
    READ_BOOL(Full);
    READ_BOOL(Annoyance);
    READ_BOOL(GuardSlower);
    READ_BOOL(Autocreate);
    READ_BOOL(Prebuild);
    READ_BOOL(Reinforce);
    READ_BOOL(Droppod);
    READ_BOOL(UseTransportOrigin);
    READ_BOOL(Recruiter);
    READ_BOOL(Whiner);
    READ_BOOL(Suicide);
    READ_BOOL(LooseRecruit);
    READ_BOOL(Aggressive);
    READ_BOOL(OnTransOnly);
    READ_BOOL(AvoidThreats);
    READ_BOOL(IonImmune);
    READ_BOOL(IsBaseDefense);
    READ_BOOL(OnlyTargetHouseEnemy);
    READ_BOOL(TransportsReturnOnUnload);
    READ_BOOL(AreTeamMembersRecruitable);
#undef READ_BOOL
    const auto tag = game::scenario_text(*ini, ID, "Tag");
    if (!tag.empty())
      Tag = TagTypeClass::FindOrAllocate(tag.c_str());
    for (auto entry : {std::pair{"Waypoint", &Waypoint},
                       std::pair{"TransportWaypoint", &TransportWaypoint}}) {
      const auto value = game::scenario_text(*ini, ID, entry.first);
      if (!value.empty() && !game::scenario_waypoint(value, *entry.second))
        return false;
    }
    const auto script = game::scenario_text(*ini, ID, "Script"),
               task = game::scenario_text(*ini, ID, "TaskForce");
    if (!script.empty())
      ScriptType = ScriptTypeClass::FindOrAllocate(script.c_str());
    if (!task.empty())
      TaskForce = TaskForceClass::FindOrAllocate(task.c_str());
    // The target falls back to the first entry for missing references.
    if (!ScriptType && ScriptTypeClass::Array.Count)
      ScriptType = ScriptTypeClass::Array[0];
    if (!TaskForce && TaskForceClass::Array.Count)
      TaskForce = TaskForceClass::Array[0];
    return ScriptType && TaskForce;
  } catch (...) {
    return false;
  }
}

// Read_All: the original ignores individual record return values, and retains
// allocated identities even for an incomplete record. Allocation failure is
// fatal there; never propagate an exception through this original entry point.
void YRPP_FASTCALL TeamTypeClass::LoadFromINIList(CCINIClass *ini,
                                                  int scope) noexcept {
  try {
    if (!ini)
      return;
    for (int i = 0; i < ini->GetKeyCount("TeamTypes"); ++i) {
      char id[32]{};
      ini->ReadString("TeamTypes", ini->GetKeyName("TeamTypes", i), "", id,
                      sizeof(id));
      auto *type = FindOrAllocate(id);
      if (!type)
        std::abort();
      type->LoadFromINI(ini);
      type->IsGlobal = scope;
    }
  } catch (...) {
    std::abort();
  }
}

TeamTypeClass *YRPP_FASTCALL TeamTypeClass::FindByNameOrID(const char *name) {
  if (!name)
    return nullptr;
  for (auto *type : Array)
    if (!_strcmpi(type->ID, name) || !_strcmpi(type->Name, name))
      return type;
  return nullptr;
}
TeamTypeClass *YRPP_FASTCALL
TeamTypeClass::FindOrAllocate(const char *id) noexcept {
  try {
    return game::allocate_type<TeamTypeClass>(id);
  } catch (...) {
    return nullptr;
  }
}
