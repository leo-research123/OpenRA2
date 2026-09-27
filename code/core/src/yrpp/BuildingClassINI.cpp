// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9, building.cpp Read_INI; YR VA
// calibrated below. Native status overloads reject malformed records and
// contain exceptions; successfully created earlier objects remain on failure.
// ReadINI: 0x44F820.
#include "scenario_object_ini.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <memory>

bool BuildingClass::ReadINI(CCINIClass &ini, unsigned int &rejectedRecords,
                            int firstHouse) noexcept {
  rejectedRecords = 0;
  try {
    if (auto *s = ini.GetSection("Structures"))
      for (auto *e : s->Entries) {
        auto r = game::scenario_fields(e->Value);
        int x, y, hp;
        auto *t =
            r.size() >= 6 ? BuildingTypeClass::Find(r[1].c_str()) : nullptr;
        if (!t || !game::scenario_number(r[2], hp) ||
            !game::scenario_number(r[3], x) ||
            !game::scenario_number(r[4], y) || x < 0 || x >= 512 || y < 0 ||
            y >= 512) {
          ++rejectedRecords;
          continue;
        }
        auto *c =
            MapClass::Instance.TryGetCellAt(CellStruct{short(x), short(y)});
        if (!c) {
          ++rejectedRecords;
          continue;
        }
        auto *owner = game::scenario_house(r[0].c_str(), firstHouse);
        if (!owner) {
          ++rejectedRecords;
          continue;
        }
        std::unique_ptr<BuildingClass> object(new BuildingClass(t, owner));
        auto *o = object.get();
        // YR 0x0044F820 attaches the map's tag before Unlimbo. ALL07's
        // Conyard_Fixed/Reactor_Fixed ownership actions depend on this link;
        // loading only the trigger definitions cannot deliver object events.
        if(r.size()>6)if(auto* tag=TagTypeClass::FindByNameOrID(r[6].c_str()))
          o->AttachTrigger(TagClass::GetInstance(tag));
        o->Location = {x * 256 + 128, y * 256 + 128, 0};
        o->Health = int(static_cast<long long>(t->Strength) *
                        std::clamp(hp, 0, 256) / 256);
        if (o->Health > t->Strength - 3)
          o->Health = t->Strength;
        o->EstimatedHealth = o->Health;
        o->IsAlive = o->Health > 0;
        o->PrimaryFacing.SetCurrent(DirStruct(static_cast<unsigned short>(
            (game::scenario_integer(r, 5, 0) & 255) * 256)));
        o->AI_Sellable = game::scenario_integer(r, 7, 1) != 0;
        o->BeingProduced = game::scenario_integer(r, 8, 0) != 0;
        o->StuffEnabled = game::scenario_integer(r, 9, 1) != 0;
        o->ShouldRebuild = game::scenario_integer(r, 15, 0) != 0;
        o->ShowRealName = game::scenario_integer(r, 16, 0) != 0;
        o->BState = -1;
        o->BeginMode(BStateType::Idle);
        if (o->IsAlive &&
            !o->Unlimbo(o->Location,
                        DirType(game::scenario_integer(r, 5, 0) & 255))) {
          ++rejectedRecords;
          continue;
        }
        o->IsDamaged = !o->IsGreenHP();
        o->UpdateAnimations();
        for (int i = 0; i < 3; ++i)
          if (r.size() > std::size_t(12 + i) &&
              !INIClass::IsBlankValue(r[12 + i].c_str()))
            if (auto *u = BuildingTypeClass::Find(r[12 + i].c_str())) {
              o->Upgrades[i] = u;
              ++o->UpgradeLevel;
              o->PlayNthAnim(static_cast<BuildingAnimSlot>(i), o->IsDamaged,
                             false);
            }
        object.release();
      }
    return true;
  } catch (...) {
    return false;
  }
}
