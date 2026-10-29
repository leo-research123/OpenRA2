// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9, unit.cpp Read_INI; YR VA calibrated
// below. The native reader skips malformed records and throws on failure;
// successfully created earlier objects remain. ReadINI: 0x743270.
#include "scenario_object_ini.hpp"
#include "techno_spatial.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <memory>
#include <stdexcept>

void YRPP_FASTCALL UnitClass::ReadINI(CCINIClass* pINI) {
    if (!pINI) throw std::invalid_argument("UnitClass::ReadINI requires an INI");
    CCINIClass& ini = *pINI;
    std::vector<std::pair<UnitClass*, int>> followers;
    if (auto* section = ini.GetSection("Units"))
        for (auto* entry : section->Entries) {
            followers.emplace_back(nullptr, -1);
            auto row = game::scenario_fields(entry->Value);
            int x, y, hp;
            auto* type = row.size() >= 7 ? UnitTypeClass::Find(row[1].c_str()) : nullptr;
            if (!type || !game::scenario_number(row[2], hp) || !game::scenario_number(row[3], x) ||
                !game::scenario_number(row[4], y) || x < 0 || x >= 512 || y < 0 || y >= 512) {
                continue;
            }
            auto* cell = MapClass::Instance.TryGetCellAt(CellStruct{short(x), short(y)});
            if (!cell) {
                continue;
            }
            auto* owner = game::scenario_house(row[0].c_str());
            if (!owner) {
                continue;
            }
            std::unique_ptr<UnitClass> object(static_cast<UnitClass*>(type->CreateObject(owner)));
            if (!object) throw std::bad_alloc();
            if (!object->InitializeLocomotor()) {
                continue;
            }
            object->Location = {x * 256 + 128, y * 256 + 128, 0};
            object->CurrentMapCoords = {short(x), short(y)};
            object->OnBridge = game::scenario_integer(row, 10, 0) != 0;
            object->Veterancy.Veterancy = std::clamp(game::scenario_integer(row, 8, 0), 0, 2);
            object->Group = game::scenario_integer(row, 9, -1);
            object->RecruitableA = game::scenario_integer(row, 12, 1) != 0;
            object->RecruitableB = game::scenario_integer(row, 13, 1) != 0;
            if (row.size() > 7)
                if (auto* tag = TagTypeClass::FindByNameOrID(row[7].c_str()))
                    object->AttachTrigger(TagClass::GetInstance(tag));
            object->Location.Z = MapClass::Instance.GetCellFloorHeight(object->Location) +
                                 (object->OnBridge ? CellClass::BridgeHeight : 0);
            if (!object->Unlimbo(object->Location, DirType(game::scenario_integer(row, 5, 0) & 255))) {
                object->AttachTrigger(nullptr);
                continue;
            }
            object->SecondaryFacing.SetCurrent(object->PrimaryFacing.Current());
            // Dormant map actors can be drawn before their first Techno.Update.
            // A charge turret starts at its uncharged stage, not the sentinel.
            if (type->IsChargeTurret && type->HasMultipleTurrets() && !type->IsGattling)
                object->CurrentTurretNumber = 0;
            object->Health = int(static_cast<long long>(type->Strength) * std::clamp(hp, 0, 256) / 256);
            if (object->Health > type->Strength - 3) object->Health = type->Strength;
            object->Health = std::max(object->Health, 1);
            object->EstimatedHealth = object->Health;
            game::techno_spatial_dirty(object.get());
            object->QueueMission(MissionControlClass::FindIndex(row[6].c_str()), false);
            if (object->ReadyToNextMission()) object->NextMission();
            followers.back() = {object.get(), game::scenario_integer(row, 11, -1)};
            object.release();
        }
    for (auto [unit, index] : followers)
        if (unit && index >= 0 && index < int(followers.size()) && followers[index].first &&
            followers[index].first != unit) {
            unit->FollowerCar = followers[index].first;
            unit->FollowerCar->IsFollowerCar = true;
        }
}
