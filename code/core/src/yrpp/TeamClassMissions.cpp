// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 team.cpp TMission_LOAD/TMission_UNLOAD, calibrated to
// YR 0x6ED200/0x6EF110; deployment entry 0x6ED4D0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TeamClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/AnimTypeClass.h"

void TeamClass::TMission_LOAD(const ScriptActionNode&, bool) {
    bool aircraftTransport = false;
    for (int i = 0; i < Type->TaskForce->CountEntries; ++i) {
        const auto* type = Type->TaskForce->Entries[i].Type;
        if (type->WhatAmI() == AbstractType::AircraftType && type->Passengers > 0) {
            aircraftTransport = true;
            break;
        }
    }
    FootClass* transport = nullptr;
    for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
        if (foot->GetTechnoType()->Passengers <= 0) continue;
        if (foot->Passengers.NumPassengers >= foot->GetTechnoType()->Passengers) continue;
        if (foot->WhatAmI() != (aircraftTransport ? AbstractType::Aircraft : AbstractType::Unit)) continue;
        transport = foot;
        break;
    }
    if (!transport) {
        StepCompleted = true;
        return;
    }
    if (transport->HasAnyLink()) return;

    bool finished = true;
    for (auto* foot = FirstUnit; foot && TotalObjects > 1; foot = foot->NextTeamMember) {
        if (!foot->IsAlive) continue;
        if (foot->Health && (Unsorted::ScenarioInit || !foot->InLimbo) && !foot->IsInitiated) {
            ScriptActionNode current;
            CurrentScript->GetCurrentAction(&current);
            const int stray = current.Action == TMISSION_GATHER_AT_ENEMY || current.Action == TMISSION_GATHER_AT_BASE
                                  ? RulesClass::Instance->RelaxedStray
                                  : RulesClass::Instance->Stray;
            if (foot->DistanceFrom3D(SpawnCell) <= stray)
                foot->IsInitiated = true;
            else if (!foot->Destination) {
                foot->QueueMission(Mission::Move, false);
                foot->SetTarget(nullptr);
                foot->SetDestination(SpawnCell, true);
            }
        }
        if (!foot->IsAlive || !foot->Health || (!Unsorted::ScenarioInit && foot->InLimbo) ||
            (!foot->IsInitiated && foot->WhatAmI() != AbstractType::Aircraft) || foot == transport)
            continue;
        finished = false;
        if (foot->GetCurrentMission() != Mission::Enter) {
            foot->QueueMission(Mission::Enter, false);
            foot->SetTarget(nullptr);
            foot->SetDestination(transport, true);
            return;
        }
    }
    if (!finished) return;
    if (Type->TransportsReturnOnUnload) transport->SetArchiveTarget(MapClass::Instance.GetCellAt(transport->Location));
    StepCompleted = true;
}

// Script 8 is the transport unload/release operation. Script 9 below is
// deployment; the two original entries have different completion rules.
void TeamClass::TransportUnload(const ScriptActionNode& action, bool) {
    bool finished = true;
    for (auto* foot = FirstUnit; foot;) {
        auto* next = foot->NextTeamMember;
        if (foot->IsAlive) {
            if (foot->Health && (Unsorted::ScenarioInit || !foot->InLimbo) && !foot->IsInitiated) {
                ScriptActionNode current;
                CurrentScript->GetCurrentAction(&current);
                const int stray =
                    current.Action == TMISSION_GATHER_AT_ENEMY || current.Action == TMISSION_GATHER_AT_BASE
                        ? RulesClass::Instance->RelaxedStray
                        : RulesClass::Instance->Stray;
                if (foot->DistanceFrom3D(SpawnCell) <= stray)
                    foot->IsInitiated = true;
                else if (!foot->Destination) {
                    foot->QueueMission(Mission::Move, false);
                    foot->SetTarget(nullptr);
                    foot->SetDestination(SpawnCell, true);
                }
            }
            if (foot->IsAlive && foot->Health && (Unsorted::ScenarioInit || !foot->InLimbo) &&
                (foot->IsInitiated || foot->WhatAmI() == AbstractType::Aircraft) && foot->Passengers.FirstPassenger) {
                finished = false;
                const auto at = foot->GetCoords();
                if (!MapClass::Instance.GetCellAt(at)->GetBuilding() && foot->GetCurrentMission() != Mission::Unload) {
                    foot->SetDestination(nullptr, true);
                    foot->SetTarget(nullptr);
                    foot->QueueMission(Mission::Unload, false);
                }
            }
        }
        foot = next;
    }
    if (!finished) return;
    bool aircraftTransport = false;
    for (int i = 0; i < Type->TaskForce->CountEntries; ++i) {
        const auto* type = Type->TaskForce->Entries[i].Type;
        if (type->WhatAmI() == AbstractType::AircraftType && type->Passengers > 0) {
            aircraftTransport = true;
            break;
        }
    }
    for (auto* foot = FirstUnit; foot;) {
        auto* next = foot->NextTeamMember;
        const bool transport =
            foot->GetTechnoType()->Passengers != 0 && (!aircraftTransport || foot->WhatAmI() == AbstractType::Aircraft);
        const int passengers = transport ? foot->GetTechnoType()->Passengers : 0;
        if (transport && passengers > 0 && Type->TransportsReturnOnUnload) {
            LiberateMember(foot, -1, 0);
            foot->SetTarget(nullptr);
            foot->SetDestination(foot->ArchiveTarget, true);
            foot->QueueMission(Mission::Move, false);
            if (foot->ReadyToNextMission()) foot->NextMission();
            foot->SetArchiveTarget(nullptr);
        } else if ((transport && passengers > 0 && (action.Argument == 2 || action.Argument == 3)) ||
                   (!transport && (action.Argument == 1 || action.Argument == 3))) {
            LiberateMember(foot, -1, 0);
            foot->SetTarget(nullptr);
            foot->SetDestination(nullptr, true);
        }
        foot = next;
    }
    StepCompleted = true;
}

void TeamClass::Unload(const ScriptActionNode&, bool) {
    bool finished = true;
    for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
        if (!foot->IsAlive) continue;
        if (foot->Health && (Unsorted::ScenarioInit || !foot->InLimbo) && !foot->IsInitiated) {
            ScriptActionNode current;
            CurrentScript->GetCurrentAction(&current);
            const int stray = current.Action == TMISSION_GATHER_AT_ENEMY || current.Action == TMISSION_GATHER_AT_BASE
                                  ? RulesClass::Instance->RelaxedStray
                                  : RulesClass::Instance->Stray;
            if (foot->DistanceFrom3D(SpawnCell) <= stray)
                foot->IsInitiated = true;
            else if (!foot->Destination) {
                foot->QueueMission(Mission::Move, false);
                foot->SetTarget(nullptr);
                foot->SetDestination(SpawnCell, true);
            }
        }
        if (!foot->IsAlive || !foot->Health || (!Unsorted::ScenarioInit && foot->InLimbo) ||
            (!foot->IsInitiated && foot->WhatAmI() != AbstractType::Aircraft))
            continue;
        const bool construction =
            foot->WhatAmI() == AbstractType::Unit && static_cast<UnitClass*>(foot)->Type->DeploysInto;
        const bool simple =
            foot->WhatAmI() == AbstractType::Unit && static_cast<UnitClass*>(foot)->Type->IsSimpleDeployer;
        // Original 0x5224E0 reads Deployer, without player-command admission.
        const bool infantry =
            foot->WhatAmI() == AbstractType::Infantry && static_cast<InfantryClass*>(foot)->Type->Deployer;
        if (construction) {
            finished = false;
            if (foot->GetCurrentMission() == Mission::Unload) continue;
            foot->Mark(MarkType::Up);
            auto* building = static_cast<UnitClass*>(foot)->Type->DeploysInto;
            if (!building->CanCreateHere(foot->GetMapCoords(), nullptr)) {
                building->CheckFoundation(foot->GetMapCoords(), foot->Owner);
                foot->Mark(MarkType::Down);
                continue;
            }
            foot->Mark(MarkType::Down);
            foot->SetDestination(nullptr, true);
            foot->SetTarget(nullptr);
            foot->QueueMission(Mission::Unload, false);
        } else if ((simple || infantry) && foot->GetCurrentMission() != Mission::Unload)
            foot->QueueMission(Mission::Unload, false);
    }
    if (finished) StepCompleted = true;
}

void TeamClass::PlayAnimation(const ScriptActionNode& action, bool first) {
    const unsigned index = unsigned(action.Argument) & 0xFFFFu;
    const int count = unsigned(action.Argument) >> 16;
    if (first && index < unsigned(AnimTypeClass::Array.Count)) {
        auto* type = AnimTypeClass::Array[index];
        for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
            const auto at = foot->GetCoords();
            if (auto* anim = GameCreate<AnimClass>(type, at, 0, count, 0x600, 0, false)) anim->SetOwnerObject(foot);
        }
    }
    StepCompleted = true;
}
