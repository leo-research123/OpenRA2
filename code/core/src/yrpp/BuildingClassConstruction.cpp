// OpenTS 44fac744 building.cpp Do_MISSION_CONSTRUCTION / YR 0x449A50.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/VocClass.h"
#include "type_resources.hpp"

int BuildingClass::Mission_Construction() {
    if (MissionStatus == 0) {
        BeginMode(BStateType::Construction);
        SendToFirstLink(RadioCommand::RequestBeginProduction);
        const int sound = Type->BuildupSound != -1 ? Type->BuildupSound : RulesClass::Instance->Construction;
        if (sound != -1 && !game::type_resources().audio_unavailable) VocClass::PlayAt(sound, Location, &Audio7);
        NeedsRedraw = true;
        MissionStatus = 1;
    } else if (MissionStatus == 1) {
        NeedsRedraw = true;
        if (!game::type_resources().audio_unavailable) VocClass::UpdatePosition(&Location, &Audio7);
        if (IsReadyToCommence) {
            SendToFirstLink(RadioCommand::RequestEndProduction);
            SendToFirstLink(RadioCommand::NotifyUnlink);
            BeginMode(BStateType::Idle);
            Place(false);
            QueueMission(Mission::Guard, false);
            if (!Type->LaserFence) PrimaryFacing.SetCurrent(DirStruct(short((Type->StartFacing & 0xFF) << 8)));
            Type->ClearBuildUp();
            Audio7.EndLooping();
        }
    }
    return 1;
}

void BuildingClass::UpdateConstructionOptions() {
    if (Owner != HouseClass::CurrentPlayer || InLimbo || !DiscoveredByCurrentPlayer || !HasPower) return;
    const auto add = [&](auto& types) {
        for (auto* type : types)
            if (Owner->CanBuild(type, false, true) != CanBuildResult::Unbuildable)
                SidebarClass::Instance.AddCameo(type->WhatAmI(), type->GetArrayIndex());
    };
    switch (Type->Factory) {
        case AbstractType::BuildingType:
            add(BuildingTypeClass::Array);
            break;
        case AbstractType::InfantryType:
            add(InfantryTypeClass::Array);
            break;
        case AbstractType::UnitType:
            add(UnitTypeClass::Array);
            break;
        case AbstractType::AircraftType:
            add(AircraftTypeClass::Array);
            break;
        default:
            break;
    }
}
