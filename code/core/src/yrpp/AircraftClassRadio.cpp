// Original aircraft radio and guard decisions: YR 0x4190B0/0x41A5C0/0x41A940.
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TeamClass.h"
#include "com_error.hpp"
#include "x87_integer.hpp"

bool AircraftClass::IsStrange() const { return IsOnFloor(); }

bool AircraftClass::IsOnFloor() const {
    if (Type == RulesClass::Instance->V3Rocket.Type || Type == RulesClass::Instance->DMisl.Type) {
        if (!Locomotor) game::raise_com_error(static_cast<HRESULT>(0x80004003u));
        return !Locomotor->Is_Moving_Now();
    }
    return ObjectClass::IsOnFloor();
}

bool AircraftClass::IsInAir() const {
    if (Type == RulesClass::Instance->V3Rocket.Type || Type == RulesClass::Instance->DMisl.Type) {
        if (!Locomotor) game::raise_com_error(static_cast<HRESULT>(0x80004003u));
        return Locomotor->Is_Moving_Now();
    }
    return ObjectClass::IsInAir();
}

bool AircraftClass::QueueMission(Mission mission, bool start) {
    const auto protectedMission = [](Mission value) {
        return value == Mission::Retreat || value == Mission::ParadropApproach || value == Mission::ParadropOverfly ||
               value == Mission::SpyplaneApproach || value == Mission::SpyplaneOverfly;
    };
    if (protectedMission(CurrentMission) && !Airstrike && !protectedMission(mission))
        return ((static_cast<unsigned>(mission) - 4u) & 0xFFu) != 0;
    return MissionClass::QueueMission(mission, start);
}

RadioCommand AircraftClass::ReceiveCommand(TechnoClass* sender, RadioCommand command, AbstractClass*& data) {
    if (!Airstrike && (CurrentMission == Mission::Retreat || CurrentMission == Mission::ParadropApproach ||
                       CurrentMission == Mission::ParadropOverfly || CurrentMission == Mission::SpyplaneApproach ||
                       CurrentMission == Mission::SpyplaneOverfly))
        return RadioCommand::AnswerInvalid;
    switch (command) {
        case RadioCommand::RequestMoveTo: {
            FootClass::ReceiveCommand(sender, command, data);
            auto* next = data;
            if (next && next->WhatAmI() == AbstractType::Building) {
                if (SendCommand(RadioCommand::QueryCanEnter, static_cast<TechnoClass*>(next)) != RadioCommand::AnswerPositive)
                    return RadioCommand::AnswerNegative;
                QueueMission(Mission::Enter, false);
            } else
                QueueMission(Mission::Move, false);
            SetDestination(next, true);
            NextMission();
            return RadioCommand::AnswerPositive;
        }
        case RadioCommand::QueryMoving:
            FootClass::ReceiveCommand(sender, command, data);
            if (!Locomotor) game::raise_com_error(static_cast<HRESULT>(0x80004003u));
            if (!Locomotor->Is_Moving() || !Destination) return RadioCommand::AnswerPositive;
            if (Type->AirportBound && Destination->WhatAmI() == AbstractType::Cell && Destination == GetCell())
                return RadioCommand::AnswerPositive;
            return RadioCommand::AnswerNegative;
        case RadioCommand::QueryReadiness:
            return !Target && Ammo == Type->Ammo ? RadioCommand::AnswerPositive : RadioCommand::AnswerNegative;
        case RadioCommand::RequestReload:
            if (Ammo >= Type->Ammo / 2 && Target) return RadioCommand::AnswerPositive;
            break;
        case RadioCommand::QueryDone:
            return Ammo == Type->Ammo ? RadioCommand::AnswerPositive : RadioCommand::AnswerNegative;
        case RadioCommand::QueryCanEnter:
            if (!Type->Passengers || !sender || !Owner->IsAlliedWith(sender)) return RadioCommand::AnswerInvalid;
            return Passengers.NumPassengers < Type->Passengers ? RadioCommand::AnswerPositive : RadioCommand::AnswerNegative;
        case RadioCommand::RequestCompleteEnter:
            return RadioCommand::unknown_5;
        case RadioCommand::NotifyLeave:
            QueueMission(Mission::Move, false);
            SetDestination(FindLandingLocation(), true);
            SendToFirstLink(RadioCommand::NotifyUnlink);
            break;
        case RadioCommand::NotifyUnloaded:
            if (Type->Carryall && GetCurrentMission() == Mission::Move && IsTether &&
                ((Passengers.NumPassengers && Passengers.GetFirstPassenger() == sender) || Destination == sender))
                return RadioCommand::AnswerNegative;
            break;
        default:
            break;
    }
    return FootClass::ReceiveCommand(sender, command, data);
}

int AircraftClass::Mission_Guard() {
    const auto delay = [&] { return game::x87_integer(CurrentMissionControl()->Rate * 900.0); };
    if (GetHeight() == Type->GetFlightLevel()) {
        if (Team) {
            if (Destination) {
                IsCarryallNotLanding = true;
                QueueMission(Mission::Move, false);
            }
            return delay();
        }
        if (GetWeapon(0)->WeaponType) {
            if (!Team) {
                IsCarryallNotLanding = true;
                EnterIdleMode(false, true);
            }
        } else {
            SetDestination(MapClass::Instance.GetCellAt(Location), true);
            QueueMission(Mission::Move, false);
        }
        return 1;
    }
    // The two original reload-threshold switches have no writers in YR;
    // their zero-initialized path uses the full-ammunition threshold.
    if (IsArmed() && Ammo < Type->Ammo && !HasAnyLink()) {
        if (auto* dock = TryNearestDockBuilding(&Type->Dock, 0, 0)) {
            QueueMission(Mission::Enter, false);
            SetDestination(dock, true);
            SetTarget(nullptr);
            return 1;
        }
    }
    if (Ammo != -1 && Ammo < Type->Ammo / 2 && HasAnyLink()) {
        auto* link = GetNthLink();
        if (link->WhatAmI() == AbstractType::Building && static_cast<BuildingClass*>(link)->Type->UnitReload)
            return 1;
    }
    if (Target) {
        QueueMission(Mission::Attack, false);
        return 1;
    }
    if (!IsArmed()) return 45;
    if (!GetHeight() && !HasAnyLink()) return 45;
    if (!Owner->IsControlledByHuman()) {
        if (auto* target = Owner->FindNearbyEnemy(Location)) {
            SetTarget(target);
            QueueMission(Mission::Attack, false);
        }
    }
    if (!Owner->IsControlledByHuman() || IsInAir()) return FootClass::Mission_Guard();
    return delay() + ScenarioClass::Instance->Random.RandomRanged(0, 2);
}

int AircraftClass::Mission_AreaGuard() {
    if (GetHeight() == Type->GetFlightLevel()) {
        if (!Team) EnterIdleMode(false, true);
        return 1;
    }
    if (!Ammo && IsArmed() && !HasAnyLink()) EnterIdleMode(false, true);
    if (Target) {
        QueueMission(Mission::Attack, false);
        return 1;
    }
    return FootClass::Mission_AreaGuard();
}
