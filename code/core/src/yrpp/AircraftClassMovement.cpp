// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 aircraft.cpp waypoint Move and AI; YR 0x4166C0/0x414BB0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/AircraftClass.h"
#include "yrpp/FlyLocomotionClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/TemporalClass.h"
#include "map_world.hpp"
#include <bit>
bool AircraftClass::ReadyToNextMission() const {
    return CurrentMission != Mission::Sticky && CurrentMission != Mission::Rescue &&
           (!IsLocked || CurrentMission == Mission::SpyplaneApproach) && IsCarryallNotLanding;
}
void AircraftClass::Update() {
    if (!IsAlive || InLimbo) return;
    // 0x414BDB: advance the victim's temporal manager before mission AI.
    if (TemporalTargetingMe) TemporalTargetingMe->Update();
    if (!IsAlive || InLimbo) return;
    if (IsBeingWarpedOut()) {
        if (Target) SetTarget(nullptr);
        if (Destination) SetDestination(nullptr, true);
        return;
    }
    if (ReadyToNextMission()) NextMission();
    FootClass::Update();
    if (!IsAlive || InLimbo) return;
    // 0x00414F42 precedes map admission and the second mission dispatch.
    if (Type->Trailer && Type->SpawnDelay > 0 && Unsorted::CurrentFrame % Type->SpawnDelay == 0)
        if (auto* memory = YRMemory::Allocate(sizeof(AnimClass)))
            ::new (memory) AnimClass(Type->Trailer, GetCoords(), 1, 1, 0x600, 0, false);
    // YR 0x414BB0/0x41B890: a flyover team is removed after entering and
    // leaving the playable rectangle, not upon touching its final waypoint.
    if (((!Type->FlyBy && !Type->FlyBack && !MapClass::Instance.IsWithinUsableArea(GetMapCoords(), true)) ||
         !MapClass::Instance.CoordinatesLegal(GetMapCoords())) &&
        IsLeavingMap()) {
        UnInit();
        return;
    }
    if (ReadyToNextMission()) NextMission();
    // 0x41505E settles a pending attack round when another mission takes over.
    if (ShouldLoseAmmo && CurrentMission != Mission::Attack) {
        ShouldLoseAmmo = false;
        Ammo = std::bit_cast<int>(unsigned(Ammo) - 1u);
    }
    // 0x00415127: a distinct animation path, including a live falling aircraft at zero HP.
    if (GetHealthPercentage() < RulesClass::Instance->ConditionRed && GetHeight() > 0 &&
        ScenarioClass::Instance->Random.RandomRanged(0, 99) < (Health ? 10 : 80)) {
        if (auto* type = AnimTypeClass::Find("SGRYSMK1"))
            if (auto* memory = YRMemory::Allocate(sizeof(AnimClass)))
                ::new (memory) AnimClass(type, GetCoords(), 0, 1, 0x600, 0, false);
    }
}
bool AircraftClass::IsLeavingMap() const {
    if ((Target && CurrentMission != Mission::SpyplaneOverfly) || !IsInPlayfield ||
        GetCurrentMission() == Mission::ParadropApproach || GetCurrentMission() == Mission::ParadropOverfly)
        return false;
    return Team ? Team->IsLeavingMapNow() : IsALoaner;
}
int AircraftClass::Mission_Move() {
    // OpenTS Do_MISSION_MOVE_Normal status codes, retained by the YR handler.
    enum MoveState : int { ValidateLZ = 0, TakeOff = 1, FlyToLZ = 2, Idle = 3, Land = 4 };
    switch (MissionStatus) {
        case MoveState::ValidateLZ:
            if (Destination) {
                SetDestination(Destination, true);
                MissionStatus = MoveState::TakeOff;
            } else
                EnterIdleMode(false, true);
            break;
        case MoveState::TakeOff:
            if (Destination) {
                CoordStruct at;
                Destination->GetDestination(&at, this);
                Locomotor->Move_To(at);
                MissionStatus = MoveState::FlyToLZ;
            } else
                EnterIdleMode(false, true);
            break;
        case MoveState::FlyToLZ:
        case MoveState::Land:
            if (!Locomotor->Is_Moving() || !Destination ||
                GetMapCoords() == CellClass::Coord2Cell(Destination->GetDestination(this)))
                MissionStatus = MoveState::Idle;
            break;
        case MoveState::Idle:
            if (!Locomotor->Is_Moving()) EnterIdleMode(false, true);
            break;
    }
    if (MissionStatus == MoveState::TakeOff)
        return int(CurrentMissionControl()->Rate * 900.0) + ScenarioClass::Instance->Random.RandomRanged(0, 2);
    return 1;
}
bool AircraftClass::EnterIdleMode(bool initial, bool resume) {
    // 0x4176F0 restores suspended missions before any idle decision.
    if (MissionIsOverriden()) {
        Mission_Revert();
        if (CurrentMission == Mission::Patrol) {
            MissionStatus = 0;
            IsLocked = false;
        }
        return false;
    }
    // Preserve flyover support before Foot::Unlimbo's idle pass.
    if ((CurrentMission == Mission::Retreat || CurrentMission == Mission::ParadropApproach ||
         CurrentMission == Mission::ParadropOverfly || CurrentMission == Mission::SpyplaneApproach ||
         CurrentMission == Mission::SpyplaneOverfly) &&
        !Airstrike)
        return false;
    if (QueuedMission == Mission::ParadropApproach || QueuedMission == Mission::SpyplaneApproach) {
        NextMission();
        return false;
    }
    const bool result = FootClass::EnterIdleMode(initial, resume);
    Mission next = Owner->IsControlledByHuman() || Team || !IsArmed() ? Mission::Guard : Mission::Area_Guard;
    const int landing = Landing_Altitude();
    const auto fallback = [&] {
        SetDestination(FindLandingLocation(), true);
        next = Mission::Move;
    };
    if (InWhichLayer() != Layer::Ground && GetHeight() > landing && !Type->MissileSpawn) {
        if (Passengers.NumPassengers) {
            if (IsALoaner) {
                if (!Team) {
                    next = Mission::Unload;
                    SetDestination(FindLandingLocation(), true);
                } else
                    next = Mission::Guard;
            } else
                fallback();
        } else {
            bool allInPlayfield = true;
            if (Team)
                for (auto* member = Team->FirstUnit; member; member = member->NextTeamMember)
                    if (!member->IsInPlayfield) { allInPlayfield = false; break; }
            if (((IsALoaner && Owner->IsControlledByHuman()) ||
                 (!Owner->IsControlledByHuman() && !Type->Ammo)) && Team && allInPlayfield)
                Team->LiberateMember(this);
            if (GetWeapon(0)->WeaponType) {
                if (IsALoaner) {
                    if (!Ammo) {
                        if (Team) Team->LiberateMember(this);
                        SetTarget(nullptr);
                        SetDestination(nullptr, true);
                        next = Mission::Retreat;
                    } else if (!Team)
                        next = Mission::Hunt;
                } else if ((Ammo && Target && GetCurrentMission() == Mission::Attack) ||
                           QueuedMission == Mission::Attack)
                    next = Mission::Attack;
                else if (IsInAir() && (!Destination || CurrentMission != Mission::Enter) &&
                         Type->Dock.Count > 0 && (IsInPlayfield || !Team)) {
                    auto* dock = TryNearestDockBuilding(&Type->Dock, 0, 0);
                    SetDestination(nullptr, true);
                    if (dock && SendCommand(RadioCommand::RequestLink, dock) == RadioCommand::AnswerPositive) {
                        SetDestination(dock, true);
                        next = Mission::Enter;
                    } else if (Type->AirportBound) {
                        Crash(nullptr);
                        return false;
                    } else
                        fallback();
                }
            } else if (!Team)
                fallback();
            else
                return false;
        }
    } else if (IsALoaner) {
        if (Passengers.NumPassengers)
            next = Team ? Mission::Guard : Mission::Unload;
        else if (!Team) {
            SetTarget(nullptr);
            SetDestination(nullptr, true);
            next = Mission::Retreat;
        }
    } else {
        SetDestination(nullptr, true);
        SetTarget(nullptr);
        next = Owner->IsControlledByHuman() || Team || !IsArmed() ? Mission::Guard : Mission::Area_Guard;
    }
    if (!Ammo && IsArmed() && !HasAnyLink()) {
        if (auto* dock = TryNearestDockBuilding(&Type->Dock, 0, 0)) {
            SetDestination(dock, true);
            SetTarget(nullptr);
            next = Mission::Enter;
        } else if (Type->AirportBound) {
            Crash(nullptr);
            return false;
        }
    }
    if (HasAnyLink()) next = Mission::Enter;
    QueueMission(next, false);
    if (ReadyToNextMission()) NextMission();
    return result;
}
