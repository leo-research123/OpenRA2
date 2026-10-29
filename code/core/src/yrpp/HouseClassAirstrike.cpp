// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 reinf.cpp Create_Air_Reinforcement; YR 0x65E850 adds
// the caller's linked aircraft list, nearby placement and passenger counts.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
int YRPP_FASTCALL HouseClass::SendAirstrikePlanes(int typeIndex, int count, Mission mission, AbstractClass* target,
                                                  AbstractClass* destination, int infantryIndex, int passengers,
                                                  AirstrikeClass* sender) {
    auto* type = AircraftTypeClass::Array[typeIndex];
    const auto edge = unsigned(StartingEdge) < 4 ? StartingEdge : (unsigned(Edge) < 4 ? Edge : ::Edge::North);
    const auto start = MapClass::Instance.PickCellOnEdge(edge, CellStruct::Empty, CellStruct::Empty, SpeedType::Winged,
                                                         true, MovementZone::Normal);
    int created = 0;
    for (; created < count; ++created) {
        ++Unsorted::ScenarioInit;
        auto* aircraft = static_cast<TechnoClass*>(type->CreateObject(this));
        --Unsorted::ScenarioInit;
        if (!aircraft) break;
        aircraft->Airstrike = sender;
        aircraft->IsALoaner = true;
        if (sender && (aircraft->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None) {
            auto* foot = static_cast<FootClass*>(aircraft);
            foot->NextTeamMember = sender->FirstObject;
            sender->FirstObject = foot;
        }
        const CellStruct empty{0, 0};
        const auto at = MapClass::Instance.NearByLocation(start, SpeedType::Winged, -1, MovementZone::Normal, false, 1,
                                                          1, false, false, false, true, empty, false, false);
        aircraft->QueueMission(mission, false);
        if (destination) aircraft->SetDestination(destination, true);
        if (target) aircraft->SetTarget(target);
        ++Unsorted::ScenarioInit;
        const bool placed = aircraft->Unlimbo({int(at.X) * 256 + 128, int(at.Y) * 256 + 128, 0}, DirType::North);
        --Unsorted::ScenarioInit;
        if (!placed) {
            delete aircraft;
            return created - 1;
        }
        if (aircraft->WhatAmI() == AbstractType::Aircraft && infantryIndex != -1 && passengers) {
            static_cast<AircraftClass*>(aircraft)->HasPassengers = true;
            auto* infantry = InfantryTypeClass::Array[infantryIndex];
            if (infantry)
                for (int i = 0; i < passengers; ++i)
                    if (auto* cargo = static_cast<FootClass*>(infantry->CreateObject(this))) {
                        cargo->Limbo();
                        aircraft->Passengers.AddPassenger(cargo);
                    }
        }
        aircraft->NextMission();
    }
    return created;
}
