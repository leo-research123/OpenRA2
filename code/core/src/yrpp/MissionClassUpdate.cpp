// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 mission.cpp::AI; YR 0x5B3060 changes QMove and adds missions.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/MissionClass.h"
#include <bit>

void MissionClass::Update() {
    ObjectClass::Update();
    if(!IsAlive)return;
    // Preserve the inline target timer test, including a paused nonzero timer
    // and signed 32-bit frame subtraction. Expired() has different semantics.
    int left=UpdateTimer.TimeLeft;
    if(UpdateTimer.StartTime!=-1) {
        const auto elapsed=std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame)-static_cast<unsigned>(UpdateTimer.StartTime));
        left=elapsed>=left?0:std::bit_cast<int>(static_cast<unsigned>(left)-static_cast<unsigned>(elapsed));
    }
    if(left || Health<=0)return;
    int delay;
    switch(CurrentMission) {
        case Mission::Attack:delay=Mission_Attack();break;
        case Mission::Move:delay=Mission_Move();break;
        case Mission::Retreat:delay=Mission_Retreat();break;
        case Mission::Guard:case Mission::Sticky:delay=Mission_Guard();break;
        case Mission::Enter:delay=Mission_Enter();break;
        case Mission::Capture:case Mission::Sabotage:delay=Mission_Capture();break;
        case Mission::Eaten:delay=Mission_Eaten();break;
        case Mission::Harvest:delay=Mission_Harvest();break;
        case Mission::Area_Guard:delay=Mission_AreaGuard();break;
        case Mission::Return:delay=Mission_Return();break;
        case Mission::Stop:delay=Mission_Stop();break;
        case Mission::Ambush:delay=Mission_Ambush();break;
        case Mission::Hunt:delay=Mission_Hunt();break;
        case Mission::Unload:delay=Mission_Unload();break;
        case Mission::Construction:delay=Mission_Construction();break;
        case Mission::Selling:delay=Mission_Selling();break;
        case Mission::Repair:delay=Mission_Repair();break;
        case Mission::Rescue:delay=Mission_Rescue();break;
        case Mission::Missile:delay=Mission_Missile();break;
        case Mission::Harmless:delay=Mission_Harmless();break;
        case Mission::Open:delay=Mission_Open();break;
        case Mission::Patrol:delay=Mission_Patrol();break;
        case Mission::ParadropApproach:delay=Mission_ParaDropApproach();break;
        case Mission::ParadropOverfly:delay=Mission_ParaDropOverfly();break;
        case Mission::Wait:delay=Mission_Wait();break;
        case Mission::SpyplaneApproach:delay=Mission_SpyPlaneApproach();break;
        case Mission::SpyplaneOverfly:delay=Mission_SpyPlaneOverfly();break;
        // YR QMove and AttackMove land here; no OpenTS QMove -> Move alias.
        default:delay=Mission_Sleep();break;
    }
    UpdateTimer.Start(delay);
}
