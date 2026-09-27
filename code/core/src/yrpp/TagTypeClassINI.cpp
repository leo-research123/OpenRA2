// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tagtype.cpp Read_INI; YR 0x6E6080.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "scenario_script_ini.hpp"
#include "yrpp/SwizzleManagerClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include <cstdio>
bool TagTypeClass::LoadFromINI(CCINIClass *ini) {
  if (!ini)
    return false;
  try {
    const auto row =
        game::scenario_fields(game::scenario_text(*ini, "Tags", ID).c_str());
    int mode = 0;
    if (row.size() < 3 || !game::scenario_number(row[0], mode) || mode < 0 ||
        mode > 2)
      return false;
    Persistence = TriggerPersistence(mode);
    std::snprintf(Name, sizeof(Name), "%s", row[1].c_str());
    FirstTrigger = TriggerTypeClass::FindOrAllocate(row[2].c_str());
    return true;
  } catch (...) {
    return false;
  }
}

TagTypeClass *YRPP_FASTCALL
TagTypeClass::FindOrAllocate(const char *id) noexcept {
  try {
    return game::allocate_type<TagTypeClass>(id);
  } catch (...) {
    return nullptr;
  }
}

void YRPP_FASTCALL TagTypeClass::LoadFromINIList(CCINIClass *ini) noexcept {
  try {
    if (!ini)
      return;
    for (int i = 0; i < ini->GetKeyCount("Tags"); ++i) {
      const char *id = ini->GetKeyName("Tags", i);
      char value[24]{};
      if (!ini->ReadString("Tags", id, "", value, sizeof(value)))
        continue;
      auto *type = FindOrAllocate(id);
      if (!type)
        std::abort();
      SwizzleManagerClass::Instance.Here_I_Am(game::scenario_hex_identity(id),
                                              type);
      type->LoadFromINI(ini);
    }
  } catch (...) {
    std::abort();
  }
}
