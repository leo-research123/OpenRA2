// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9, terrain.cpp Read_INI; YR VA
// calibrated below. The native reader skips malformed records and throws
// on failure; successfully created earlier objects remain.
// ReadINI: 0x71CA70.
#include "scenario_object_ini.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <cstdio>
#include "yrpp/ScenarioClass.h"
#include <stdexcept>

void YRPP_FASTCALL TerrainClass::ReadINI(CCINIClass* pINI) {
    if (!pINI) throw std::invalid_argument("TerrainClass::ReadINI requires an INI");
    CCINIClass& ini = *pINI;
    if (auto* s = ini.GetSection("Terrain"))
        for (auto* e : s->Entries) {
            int index;
            auto r = game::scenario_fields(e->Value);
            auto* t = r.empty() ? nullptr : TerrainTypeClass::FindOrAllocate(r[0].c_str());
            if (!t || !game::scenario_number(e->Key, index) || index < 0 || index / (ScenarioClass::NewINIFormat < 4 ? 128 : 1000) >= 512 ||
                index % (ScenarioClass::NewINIFormat < 4 ? 128 : 1000) >= 512) {
                continue;
            }
            const int divisor = ScenarioClass::NewINIFormat < 4 ? 128 : 1000;
            CellStruct c{short(index % divisor), short(index / divisor)};
            if (!MapClass::Instance.TryGetCellAt(c)) {
                continue;
            }
            new TerrainClass(t, c);
        }
}

void YRPP_FASTCALL TerrainClass::WriteINI(CCINIClass* ini) {
    if (!ini) throw std::invalid_argument("TerrainClass::WriteINI requires an INI");
    ini->Clear("Terrain");
    for (const auto* terrain : Array) {
        if (!terrain || terrain->InLimbo || !terrain->IsAlive || !terrain->Type) continue;
        const auto cell = terrain->GetMapCoords();
        char key[16];
        std::snprintf(key, sizeof(key), "%d", int(cell.X) + 1000 * int(cell.Y));
        ini->WriteString("Terrain", key, terrain->Type->ID);
    }
}
