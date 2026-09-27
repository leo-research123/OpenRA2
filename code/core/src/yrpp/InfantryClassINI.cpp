// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9, infantry.cpp Read_INI; YR VA
// calibrated below. Native status overloads reject malformed records and
// contain exceptions; successfully created earlier objects remain on failure.
// ReadINI: 0x51FB00.
#include "scenario_object_ini.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <memory>

bool InfantryClass::ReadINI(CCINIClass &ini, unsigned int &rejectedRecords,
                            int firstHouse) noexcept {
  rejectedRecords = 0;
  try {
    if (auto *s = ini.GetSection("Infantry"))
      for (auto *e : s->Entries) {
        auto r = game::scenario_fields(e->Value);
        int x, y, hp, subcell;
        auto *t =
            r.size() >= 8 ? InfantryTypeClass::Find(r[1].c_str()) : nullptr;
        if (!t || !game::scenario_number(r[2], hp) ||
            !game::scenario_number(r[3], x) ||
            !game::scenario_number(r[4], y) ||
            !game::scenario_number(r[5], subcell) || x < 0 || x >= 512 ||
            y < 0 || y >= 512 || subcell < 0 || subcell > 4) {
          ++rejectedRecords;
          continue;
        }
        auto *c =
            MapClass::Instance.TryGetCellAt(CellStruct{short(x), short(y)});
        if (!c || !t->Image || !t->Sequence) {
          ++rejectedRecords;
          continue;
        }
        auto *owner = game::scenario_house(r[0].c_str(), firstHouse);
        if (!owner) {
          ++rejectedRecords;
          continue;
        }
        std::unique_ptr<InfantryClass> object(
            static_cast<InfantryClass *>(t->CreateObject(owner)));
        if (!object)
          throw std::bad_alloc();
        if (!object->InitializeLocomotor())
          throw std::runtime_error(
              std::string("Unsupported infantry locomotor: ") + t->ID);
        constexpr Point2D offsets[]{{128, 128},
                                    {64, 64},
                                    {192, 64},
                                    {64, 192},
                                    {192, 192}}; // 0x0048E480
        object->Location = {x * 256 + offsets[subcell].X,
                            y * 256 + offsets[subcell].Y, 0};
        object->OnBridge = game::scenario_integer(r, 11, 0) != 0;
        object->Veterancy.Veterancy =
            std::clamp(game::scenario_integer(r, 9, 0), 0, 2);
        object->Group = game::scenario_integer(r, 10, -1);
        object->RecruitableA = game::scenario_integer(r, 12, 1) != 0;
        object->RecruitableB = game::scenario_integer(r, 13, 1) != 0;
        object->Location.Z =
            MapClass::Instance.GetCellFloorHeight(object->Location) +
            (object->OnBridge ? CellClass::BridgeHeight : 0);
        if (!object->Unlimbo(object->Location,
                             DirType(game::scenario_integer(r, 7, 0) & 255))) {
          ++rejectedRecords;
          continue;
        }
        object->Health = int(static_cast<long long>(t->Strength) *
                             std::clamp(hp, 0, 256) / 256);
        if (object->Health > t->Strength - 3)
          object->Health = t->Strength;
        object->Health = std::max(object->Health, 1);
        object->EstimatedHealth = object->Health;
        object->QueueMission(MissionControlClass::FindIndex(r[6].c_str()),
                             false);
        object->NextMission();
        object->CurrentMapCoords = {short(x), short(y)};
        object.release();
      }
    return true;
  } catch (...) {
    return false;
  }
}
