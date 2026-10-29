// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Do_MISSION_REPAIR, calibrated to YR 0x44B780.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/RadarEventClass.h"
#include "RulesClassReaders.hpp"
#include "type_resources.hpp"
#include "x87_integer.hpp"
#include <cstdlib>
#include <cstring>

int BuildingClass::Mission_Repair() {
    enum RepairState : int { Initial = 0, Idle = 1, Repairing = 2 };
    if (Type->Bunker) UpdateBunker();
    if (Type->ConstructionYard) {
        if (MissionStatus == 0) {
            BeginMode(static_cast<BStateType>(2));
            PlayNthAnim(BuildingAnimSlot::PreProduction, !IsGreenHP(), false);
            MissionStatus = 2;
        } else if (MissionStatus == 2 && !HasAnyLink()) {
            QueueMission(Mission::Guard, false);
            DestroyNthAnim(BuildingAnimSlot::PreProduction);
        }
        return 1;
    }
    if (!Type->UnitRepair && Type->UnitReload) {
        bool servicing = false;
        for (int i = 0; i < RadioLinks.Capacity; ++i) {
            auto* client = GetNthLink(i);
            if (!client) continue;
            if (SendCommand(RadioCommand::QueryReadiness, client) == RadioCommand::AnswerPositive &&
                client->Health == client->GetTechnoType()->Strength) {
                client->EnterIdleMode(false, true);
                client->ForceMission(Mission::Guard);
                client->ProceedToNextPlanningWaypoint();
                continue;
            }
            if (client->GetCurrentMission() == Mission::Enter ||
                SendCommand(RadioCommand::QueryMoving, client) != RadioCommand::AnswerPositive)
                continue;
            servicing = true;
            if (client->GetCurrentMission() != Mission::Sleep) {
                client->QueueMission(Mission::Sleep, false);
                continue;
            }
            if (SendCommand(RadioCommand::RequestReload, client) != RadioCommand::AnswerPositive &&
                SendCommand(RadioCommand::RequestRepair, client) != RadioCommand::AnswerPositive) {
                client->EnterIdleMode(false, true);
                client->ForceMission(Mission::Guard);
                client->ProceedToNextPlanningWaypoint();
            }
        }
        if (servicing) return game::x87_integer(RulesClass::Instance->ReloadRate * 900.0);
        QueueMission(Mission::Guard, false);
        return 3;
    }
    if (!Type->UnitRepair) return Type->Bunker ? 15 : TechnoClass::Mission_Repair();
    const auto delay = [&] { return rule_integer(CurrentMissionControl()->Rate * 900.0); };
    const auto speak = [&](const char* message) {
        if (!game::type_resources().audio_unavailable) VoxClass::Play(message, -1, -1);
    };
    const auto play = [&](BuildingAnimSlot slot) {
        PlayNthAnim(slot, GetHealthPercentage() <= RulesClass::Instance->ConditionYellow, false);
    };
    const auto idle_animations = [&] {
        DestroyNthAnim(BuildingAnimSlot::Production);
        DestroyNthAnim(BuildingAnimSlot::SpecialTwo);
        play(BuildingAnimSlot::SpecialThree);
        play(BuildingAnimSlot::Active);
    };
    const auto exit = [&](FootClass* unit, bool clearArchive) {
        if (unit->ArchiveTarget && !unit->Owner->IsControlledByHuman()) {
            unit->QueueMission(Mission::Move, false);
            unit->SetDestination(unit->ArchiveTarget, true);
            unit->SetArchiveTarget(nullptr);
            SendToFirstLink(RadioCommand::NotifyUnlink);
            unit->QueueUpToEnter = nullptr;
            return;
        }
        auto cell = FindExitCell(unit, CellStruct::Empty);
        if (ArchiveTarget) cell = CellClass::Coord2Cell(ArchiveTarget->GetCoords());
        if (cell != CellStruct::Empty) {
            unit->QueueMission(Mission::Move, false);
            unit->SetDestination(MapClass::Instance.GetCellAt(cell), true);
            if (clearArchive) unit->SetArchiveTarget(nullptr);
            SendToFirstLink(RadioCommand::NotifyUnlink);
            unit->QueueUpToEnter = nullptr;
        }
    };
    switch (MissionStatus) {
        case RepairState::Initial: {
            if (!HasAnyLink()) {
                idle_animations();
                QueueMission(Mission::Guard, true);
                return 1;
            }
            IsReadyToCommence = false;
            auto* unit = static_cast<FootClass*>(GetNthLink());
            constexpr GUID persistID{0x109, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
            IPersist* persist = nullptr;
            if (!unit->Locomotor ||
                unit->Locomotor->QueryInterface(persistID, reinterpret_cast<void**>(&persist)) < 0 || !persist)
                std::abort();
            CLSID id{};
            persist->GetClassID(&id);
            const int distance = (!std::memcmp(&id, &LocomotionClass::CLSIDs::Hover, sizeof(id)) ||
                                  !std::memcmp(&id, &LocomotionClass::CLSIDs::Ship, sizeof(id)))
                                     ? 200
                                     : 100;
            if (SendToFirstLink(RadioCommand::QueryMoving) == RadioCommand::AnswerPositive &&
                DistanceToDockingCoord(GetNthLink()) < distance) {
                MissionStatus = RepairState::Idle;
                persist->Release();
                return 3;
            }
            if (!Owner->IonSensitivesShouldBeOffline()) unit->Locomotor->Power_On();
            persist->Release();
            break;
        }
        case RepairState::Idle: {
            if (!HasAnyLink()) {
                if (Anims[int(BuildingAnimSlot::Production)] || Anims[int(BuildingAnimSlot::SpecialTwo)]) {
                    play(BuildingAnimSlot::SpecialThree);
                    play(BuildingAnimSlot::Active);
                    DestroyNthAnim(BuildingAnimSlot::Production);
                    DestroyNthAnim(BuildingAnimSlot::SpecialTwo);
                }
                if (!Anims[int(BuildingAnimSlot::SpecialThree)]) {
                    IsReadyToCommence = true;
                    QueueMission(Mission::Guard, true);
                }
                return 1;
            }
            if (Anims[int(BuildingAnimSlot::Production)]) return 1;
            auto* unit = static_cast<FootClass*>(GetNthLink());
            if (DistanceToDockingCoord(GetNthLink()) < 200) {
                if (!unit->Locomotor) std::abort();
                if (unit->Locomotor->Is_Powered()) {
                    if (!unit->Locomotor->Is_Moving()) unit->Locomotor->Power_Off();
                    return 1;
                }
                if (!unit->Locomotor->Is_Powered() && unit->Destination)
                    static_cast<FootClass*>(GetNthLink())->AbortMotion();
            }
            if (SendToFirstLink(RadioCommand::QueryMoving) == RadioCommand::AnswerPositive) {
                auto* client = GetNthLink();
                const bool damaged = client->GetHealthPercentage() < RulesClass::Instance->ConditionGreen;
                const bool manual = client->GetTechnoType()->ManualReload;
                const auto result = SendToFirstLink(RadioCommand::RequestRepair);
                if ((damaged || manual) &&
                    (result == RadioCommand::AnswerPositive || result == RadioCommand::AnswerDone)) {
                    if (!client->IsUseless || client->Owner->IsControlledByHuman()) {
                        if (IsOwnedByCurrentPlayer) speak("EVA_Repairing");
                        MissionStatus = RepairState::Repairing;
                        play(BuildingAnimSlot::Special);
                        DestroyNthAnim(BuildingAnimSlot::Active);
                        DestroyNthAnim(BuildingAnimSlot::Idle);
                        IsReadyToCommence = false;
                        RepairProgress.Value = 0;
                        RepairProgress.Start(1);
                    } else {
                        client->Sell(1);
                        MissionStatus = RepairState::Initial;
                        IsReadyToCommence = true;
                    }
                } else if (client->GetHealthPercentage() == RulesClass::Instance->ConditionGreen) {
                    auto* mover = static_cast<FootClass*>(client);
                    if (!mover->Locomotor) std::abort();
                    if (!mover->Locomotor->Is_Powered()) {
                        mover->Locomotor->Power_On();
                        exit(mover, true);
                    }
                }
            } else if (!Owner->IonSensitivesShouldBeOffline()) {
                auto* mover = static_cast<FootClass*>(GetNthLink());
                if (!mover->Locomotor) std::abort();
                if (!mover->Locomotor->Is_Powered()) mover->Locomotor->Power_On();
            }
            break;
        }
        case RepairState::Repairing: {
            if (!HasAnyLink()) {
                idle_animations();
                MissionStatus = RepairState::Idle;
                return 1;
            }
            if (!RepairProgress.Rate) RepairProgress.Rate = 1;
            RepairProgress.HasChanged = !RepairProgress.Timer.GetTimeLeft() && RepairProgress.Rate != 0;
            if (RepairProgress.HasChanged) {
                RepairProgress.Value =
                    std::bit_cast<int>(unsigned(RepairProgress.Value) + unsigned(RepairProgress.Step));
                RepairProgress.Timer.Start(RepairProgress.Rate);
            }
            if (RepairProgress.Value >= RulesClass::Instance->URepairRate * 900.0 &&
                SendToFirstLink(RadioCommand::QueryMoving) == RadioCommand::AnswerPositive) {
                IsReadyToCommence = false;
                RepairProgress.Value = 0;
                const auto result = SendToFirstLink(RadioCommand::RequestRepair);
                if (result != RadioCommand::AnswerPositive) {
                    if (result == RadioCommand::AnswerBlocked) {
                        if (IsOwnedByCurrentPlayer && !Owner->Available_Money()) speak("EVA_InsufficientFunds");
                        idle_animations();
                        MissionStatus = RepairState::Idle;
                    } else {
                        if (IsOwnedByCurrentPlayer &&
                            RadarEventClass::Create(RadarEventType::UnitRepaired, GetMapCoords()))
                            speak("EVA_UnitRepaired");
                        idle_animations();
                        MissionStatus = RepairState::Idle;
                        exit(static_cast<FootClass*>(GetNthLink()), false);
                    }
                }
            }
            return 1;
        }
        default:
            break;
    }
    return delay();
}
