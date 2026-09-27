// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9, unit.cpp Read_INI; YR VA calibrated
// below. Native status overloads reject malformed records and contain
// exceptions; successfully created earlier objects remain on failure. ReadINI:
// 0x743270.
#include "scenario_object_ini.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <memory>

bool UnitClass::ReadINI(CCINIClass &ini, unsigned int &rejectedRecords,
                        int firstHouse) noexcept {
  rejectedRecords = 0;
  try {
    std::vector<std::pair<UnitClass *, int>> followers;
    if (auto *section = ini.GetSection("Units"))
      for (auto *entry : section->Entries) {
        followers.emplace_back(nullptr, -1);
        auto row = game::scenario_fields(entry->Value);
        int x, y, hp;
        auto *type =
            row.size() >= 7 ? UnitTypeClass::Find(row[1].c_str()) : nullptr;
        if (!type || !game::scenario_number(row[2], hp) ||
            !game::scenario_number(row[3], x) ||
            !game::scenario_number(row[4], y) || x < 0 || x >= 512 || y < 0 ||
            y >= 512) {
          ++rejectedRecords;
          continue;
        }
        auto *cell =
            MapClass::Instance.TryGetCellAt(CellStruct{short(x), short(y)});
        if (!cell) {
          ++rejectedRecords;
          continue;
        }
        auto *owner = game::scenario_house(row[0].c_str(), firstHouse);
        if (!owner) {
          ++rejectedRecords;
          continue;
        }
        std::unique_ptr<UnitClass> object(
            static_cast<UnitClass *>(type->CreateObject(owner)));
        if (!object)
          throw std::bad_alloc();
        if (!object->InitializeLocomotor()) {
          ++rejectedRecords;
          continue;
        }
        object->Location = {x * 256 + 128, y * 256 + 128, 0};
        object->CurrentMapCoords = {short(x), short(y)};
        object->OnBridge = game::scenario_integer(row, 10, 0) != 0;
        object->Veterancy.Veterancy =
            std::clamp(game::scenario_integer(row, 8, 0), 0, 2);
        object->Group = game::scenario_integer(row, 9, -1);
        object->RecruitableA = game::scenario_integer(row, 12, 1) != 0;
        object->RecruitableB = game::scenario_integer(row, 13, 1) != 0;
        object->Location.Z =
            MapClass::Instance.GetCellFloorHeight(object->Location) +
            (object->OnBridge ? CellClass::BridgeHeight : 0);
        if (!object->Unlimbo(
                object->Location,
                DirType(game::scenario_integer(row, 5, 0) & 255))) {
          ++rejectedRecords;
          continue;
        }
        object->SecondaryFacing.SetCurrent(object->PrimaryFacing.Current());
        object->Health = int(static_cast<long long>(type->Strength) *
                             std::clamp(hp, 0, 256) / 256);
        if (object->Health > type->Strength - 3)
          object->Health = type->Strength;
        object->Health = std::max(object->Health, 1);
        object->EstimatedHealth = object->Health;
        object->QueueMission(MissionControlClass::FindIndex(row[6].c_str()),
                             false);
        if (object->ReadyToNextMission())
          object->NextMission();
        followers.back() = {object.get(), game::scenario_integer(row, 11, -1)};
        object.release();
      }
    for (auto [unit, index] : followers)
      if (unit && index >= 0 && index < int(followers.size()) &&
          followers[index].first && followers[index].first != unit) {
        unit->FollowerCar = followers[index].first;
        unit->FollowerCar->IsFollowerCar = true;
      }
    return true;
  } catch (...) {
    return false;
  }
}
