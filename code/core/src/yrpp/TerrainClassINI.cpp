// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9, terrain.cpp Read_INI; YR VA
// calibrated below. Native status overloads reject malformed records and
// contain exceptions; successfully created earlier objects remain on failure.
// ReadINI: 0x71CA70.
#include "scenario_object_ini.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <memory>

bool TerrainClass::ReadINI(CCINIClass &ini,
                           unsigned int &rejectedRecords) noexcept {
  rejectedRecords = 0;
  try {
    if (auto *s = ini.GetSection("Terrain"))
      for (auto *e : s->Entries) {
        int index;
        auto r = game::scenario_fields(e->Value);
        auto *t = r.empty() ? nullptr : TerrainTypeClass::Find(r[0].c_str());
        if (!t || !game::scenario_number(e->Key, index) || index < 0 ||
            index / 1000 >= 512 || index % 1000 >= 512) {
          ++rejectedRecords;
          continue;
        }
        CellStruct c{short(index % 1000), short(index / 1000)};
        if (!MapClass::Instance.TryGetCellAt(c)) {
          ++rejectedRecords;
          continue;
        }
        std::unique_ptr<TerrainClass> o(new TerrainClass(t, c));
        auto at = o->Location;
        at.Z = MapClass::Instance.GetCellFloorHeight(at);
        if (!o->Unlimbo(at, DirType::North)) {
          ++rejectedRecords;
          continue;
        }
        o.release();
      }
    return true;
  } catch (...) {
    return false;
  }
}
