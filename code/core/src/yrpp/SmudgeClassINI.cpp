// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS smudge.cpp
// 44fac744f70235e0d5ddca107364a68f95132ce9, Read_INI/Mark; YR
// 0x6B4C80/0x6B4BE0, scenario-loading path to SmudgeTypeClass::Place 0x6B6080.
#include "scenario_object_ini.hpp"
#include "yrpp/CCINIClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/SmudgeClass.h"

bool SmudgeClass::ReadINI(CCINIClass &ini,
                          unsigned int &rejectedRecords) noexcept {
  rejectedRecords = 0;
  try {
    auto *section = ini.GetSection("Smudge");
    if (!section)
      return true;
    for (auto *entry : section->Entries) {
      const auto row = game::scenario_fields(entry->Value);
      int x, y;
      auto *type =
          row.size() >= 3 ? SmudgeTypeClass::Find(row[0].c_str()) : nullptr;
      // Keep the native loader's defined handling of malformed records;
      // the original strtok/atoi path assumes valid map coordinates.
      if (!type || !game::scenario_number(row[1], x) ||
          !game::scenario_number(row[2], y) || x < 0 || x >= 512 || y < 0 ||
          y >= 512) {
        ++rejectedRecords;
        continue;
      }
      int data = 0;
      if (row.size() > 3 && game::scenario_number(row[3], data) && data != 0)
        continue;
      const CellStruct origin{short(x), short(y)};
      if (!MapClass::Instance.TryGetCellAt(origin)) {
        ++rejectedRecords;
        continue;
      }
      // Read_INI only creates zero/omitted-data entries. During scenario
      // initialization Mark bypasses CanPlaceHere and delegates the whole
      // footprint to Place. Runtime placement still owns its own checks.
      type->Place(origin);
    }
    return true;
  } catch (...) {
    return false;
  }
}
