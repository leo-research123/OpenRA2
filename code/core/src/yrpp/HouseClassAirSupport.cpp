#include "yrpp/AnimClass.h"
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 reinf.cpp Create_Air_Reinforcement, calibrated against
// YR 0x65E660 and 0x65EAB0. Each aircraft selects its own entry cell.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"

void YRPP_FASTCALL HouseClass::SendParadropPlanes(int typeIndex, int count, Mission mission, AbstractClass* target,
                                                AbstractClass* destination, int infantryIndex, int passengers) {
    auto* type = AircraftTypeClass::Array[typeIndex];
    for (int created = 0; created < count; ++created) {
        ++Unsorted::ScenarioInit;
        auto* aircraft = static_cast<TechnoClass*>(type->CreateObject(this));
        --Unsorted::ScenarioInit;
        if (!aircraft) return;
        aircraft->IsALoaner = true;
        // 0x50DA80 returns the house edge, clamped to North when invalid.
        const auto edge = unsigned(StartingEdge) < 4 ? StartingEdge : (unsigned(Edge) < 4 ? Edge : ::Edge::North);
        CellStruct at;
        MapClass::Instance.PickCellOnEdge(at, edge, CellStruct::Empty, CellStruct::Empty, SpeedType::Winged,
                                        true, MovementZone::Normal);
        aircraft->QueueMission(mission, false);
        if (destination) aircraft->SetDestination(destination, true);
        if (target) aircraft->SetTarget(target);
        ++Unsorted::ScenarioInit;
        const bool placed = aircraft->Unlimbo({int(at.X) * 256 + 128, int(at.Y) * 256 + 128, 0}, DirType::North);
        --Unsorted::ScenarioInit;
        if (!placed) {
            delete aircraft;
            return;
        }
        if (aircraft->WhatAmI() == AbstractType::Aircraft && infantryIndex != -1 && passengers) {
            static_cast<AircraftClass*>(aircraft)->HasPassengers = true;
            auto* infantry = InfantryTypeClass::Array[infantryIndex];
            if (infantry) {
                // Valid callers supply nonnegative counts; failed cargo creation
                // consumes its attempt without cancelling the remaining cargo.
                for (int remaining = passengers; remaining; --remaining) {
                    if (auto* cargo = static_cast<FootClass*>(infantry->CreateObject(this))) {
                        cargo->Limbo();
                        aircraft->Passengers.AddPassenger(cargo);
                    }
                }
            }
        }
        aircraft->NextMission();
    }
}

void YRPP_FASTCALL HouseClass::SendSpyPlanes(int typeIndex, int count, Mission mission, AbstractClass* target,
                                           AbstractClass* destination) {
    auto* type = AircraftTypeClass::Array[typeIndex];
    for (int created = 0; created < count; ++created) {
        ++Unsorted::ScenarioInit;
        auto* aircraft = static_cast<TechnoClass*>(type->CreateObject(this));
        --Unsorted::ScenarioInit;
        if (!aircraft) return;
        aircraft->IsALoaner = true;
        const auto edge = unsigned(StartingEdge) < 4 ? StartingEdge : (unsigned(Edge) < 4 ? Edge : ::Edge::North);
        CellStruct at;
        MapClass::Instance.PickCellOnEdge(at, edge, CellStruct::Empty, CellStruct::Empty, SpeedType::Winged,
                                        true, MovementZone::Normal);
        aircraft->QueueMission(mission, false);
        if (destination) aircraft->SetDestination(destination, true);
        if (target) aircraft->SetTarget(target);
        ++Unsorted::ScenarioInit;
        const bool placed = aircraft->Unlimbo({int(at.X) * 256 + 128, int(at.Y) * 256 + 128, 0}, DirType::North);
        --Unsorted::ScenarioInit;
        if (!placed) {
            delete aircraft;
            return;
        }
        aircraft->NextMission();
    }
}

// Original nuclear target warning, 0x0043B5E0.
AnimClass* YRPP_FASTCALL HouseClass::PsiWarn(CellClass* target, BulletClass* bullet, char* name) {
    auto* type = AnimTypeClass::Find(name);
    auto* memory = YRMemory::Allocate(sizeof(AnimClass));
    if (!memory) return nullptr;
    auto* anim = ::new (memory) AnimClass(type, target->GetCoords(), 0, 1, 0x600, 0, false);
    anim->AttachedBullet = bullet;
    anim->Owner = this;
    anim->Invisible = true;
    return anim;
}
