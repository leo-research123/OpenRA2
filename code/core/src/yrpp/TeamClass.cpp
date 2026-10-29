// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 team.cpp lifecycle/AI/TMission_MOVE. YR 0x6E8A90,
// 0x6EA500/0x6EA870/0x6E9140/0x6EC7D0/0x6EBAD0 calibrate state transitions.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TeamClass.h"
#include "techno_spatial.hpp"
#include "yrpp/ScriptClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/RulesClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/SessionClass.h"
#include <bit>
#include <cstdlib>
#include <climits>
#include <cstring>
#include "yrpp/UnitTypeClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/ThemeClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/AITriggerTypeClass.h"
#include "x87_integer.hpp"
namespace {
DynamicVectorClass<TeamClass*> teams;

// The ordinary rally-centre arm of 0x6EAEE0. Pickup action 10 has a
// separate search for allied transports and is not handled by this arm.
void update_rally_center(TeamClass& team) {
    team.SpawnCell = nullptr;
    team.ClosestMember = nullptr;
    ScriptActionNode action;
    team.CurrentScript->GetCurrentAction(&action);
    if (team.FirstUnit && action.Action == TMISSION_HOUND_DOG) {
        FootClass* best = nullptr;
        int bestDistance = -1;
        const auto search = [&](const auto& array) {
            for (int i = 0; i < array.Count; ++i) {
                auto* foot = array[i];
                if (!foot || !foot->IsAlive || !foot->Health || (!Unsorted::ScenarioInit && foot->InLimbo) ||
                    !foot->Owner->IsAlliedWith(team.Owner) || foot->Team == &team)
                    continue;
                const auto at = foot->GetCoords();
                const auto origin = team.FirstUnit->GetCoords();
                const auto delta = [](int a, int b) { return double(std::bit_cast<int>(unsigned(a) - unsigned(b))); };
                const double x = delta(origin.X, at.X), y = delta(origin.Y, at.Y), z = delta(origin.Z, at.Z);
                const int distance = game::x87_integer(Math::sqrt(x * x + z * z + y * y));
                if (bestDistance == -1 || distance < bestDistance) {
                    best = foot;
                    bestDistance = distance;
                }
            }
        };
        search(UnitClass::Array);
        search(InfantryClass::Array);
        if (best) {
            team.SpawnCell = best->TubeIndex < 0
                                 ? reinterpret_cast<CellClass*>(best)
                                 : MapClass::Instance.GetCellAt(TubeClass::Array[best->TubeIndex]->ExitCell);
            team.ClosestMember = team.FirstUnit;
        }
        return;
    }
    unsigned x = 0, y = 0;
    int count = 0, closestDistance = 0;
    FootClass* closest = nullptr;
    for (auto* foot = team.FirstUnit; foot; foot = foot->NextTeamMember) {
        if (!foot->IsAlive || !foot->Health || (!Unsorted::ScenarioInit && foot->InLimbo) ||
            (!foot->IsInitiated && foot->WhatAmI() != AbstractType::Aircraft) || !foot->IsInPlayfield)
            continue;
        x += unsigned(foot->Location.X);
        y += unsigned(foot->Location.Y);
        ++count;
        if (team.Type->GuardSlower && foot->vt_entry_4E0()) {
            x += unsigned(foot->Location.X);
            y += unsigned(foot->Location.Y);
            ++count;
        }
        if (foot->GetTechnoType()->Passengers <= 0 || !foot->GetTechnoType()->Naval) {
            int distance = 0;
            if (team.Focus) {
                CoordStruct here, there;
                foot->GetCoords(&here);
                team.Focus->GetCoords(&there);
                const unsigned dx = unsigned(here.X) - unsigned(there.X), dy = unsigned(here.Y) - unsigned(there.Y);
                distance = std::bit_cast<int>(dx * dx + dy * dy);
            }
            if (!closestDistance || distance < closestDistance) {
                closestDistance = distance;
                closest = foot;
            }
        }
    }
    if (count) {
        const CoordStruct center{std::bit_cast<int>(x) / count, std::bit_cast<int>(y) / count, 0};
        team.SpawnCell = MapClass::Instance.GetCellAt(center);
        if (!closest) closest = team.FirstUnit;
        if (closest->IsCellOccupied(team.SpawnCell, FacingType::None, -1, nullptr, true) != Move::OK)
            team.SpawnCell = reinterpret_cast<CellClass*>(closest);
    }
    team.ClosestMember = closest;
}

// 0x6EB870 keeps existing destinations and only resumes the script once
// no initiated member requires a new move order.
bool regroup_members(TeamClass& team) {
    bool complete = true;
    for (auto* foot = team.FirstUnit; foot; foot = foot->NextTeamMember) {
        if (!foot->IsAlive) continue;
        auto stray = [&team] {
            ScriptActionNode action;
            team.CurrentScript->GetCurrentAction(&action);
            return action.Action == TMISSION_GATHER_AT_ENEMY || action.Action == TMISSION_GATHER_AT_BASE
                       ? RulesClass::Instance->RelaxedStray
                       : RulesClass::Instance->Stray;
        };
        if (foot->Health && (Unsorted::ScenarioInit || !foot->InLimbo) && !foot->IsInitiated) {
            const int distance = stray();
            if (foot->DistanceFrom3D(team.SpawnCell) <= distance)
                foot->IsInitiated = true;
            else if (!foot->Destination) {
                foot->QueueMission(Mission::Move, false);
                foot->SetTarget(nullptr);
                foot->SetDestination(team.SpawnCell, true);
            }
        }
        if (foot->IsAlive && foot->Health && (Unsorted::ScenarioInit || !foot->InLimbo) &&
            (foot->IsInitiated || foot->WhatAmI() == AbstractType::Aircraft)) {
            const int distance = stray();
            if (foot->DistanceFrom3D(team.SpawnCell) <= distance ||
                (foot->GetCurrentMission() == Mission::Area_Guard && foot->Target)) {
                if (foot->GetCurrentMission() != Mission::Area_Guard) {
                    foot->QueueMission(Mission::Guard, false);
                    foot->SetDestination(nullptr, true);
                }
            } else if (!foot->Destination) {
                complete = false;
                foot->QueueMission(Mission::Move, false);
                CoordStruct where;
                team.SpawnCell->GetCoords(&where);
                const CellStruct cell{short(where.X / 256), short(where.Y / 256)};
                foot->SetDestination(MapClass::Instance.GetCellAt(cell), true);
            }
        }
    }
    if (complete) team.NeedsReGrouping = false;
    return complete;
}
}
DynamicVectorClass<TeamClass*>& TeamClass::Array = teams;
TeamClass::TeamClass(TeamTypeClass* type, HouseClass* owner, int value) noexcept
    : AbstractClass(), Type(type), CurrentScript(nullptr), Owner(owner), Target(nullptr), SpawnCell(nullptr),
      ClosestMember(nullptr), QueuedFocus(nullptr), Focus(nullptr), unknown_44(value), TotalObjects(0),
      TotalThreatValue(0), CreationFrame(Unsorted::CurrentFrame), FirstUnit(nullptr), GuardAreaTimer{}, SuspendTimer{},
      Tag(nullptr), IsTransient(false), NeedsReGrouping(false), GuardSlowerIsNotUnderStrength(false),
      IsForcedActive(false), IsHasBeen(false), IsFullStrength(false), IsUnderStrength(true), IsReforming(false),
      IsLagging(false), NeedsToDisappear(true), JustDisappeared(false), IsMoving(false), StepCompleted(true),
      TargetNotAssigned(false), IsLeavingMap(false), IsSuspended(false), AchievedGreatSuccess(false), CountObjects{} {
    GuardAreaTimer.Start(0);
    SuspendTimer.Start(0);
    Array.AddItem(this);
    TypeExpirationListeners.AddItem(this);
    TagExpirationListeners.AddItem(this);
    AbstractClass::Array.AddItem(this);
    ScriptExpirationListeners.AddItem(this);
    if (Type) {
        if (Owner && Type->IsBaseDefense) ++Owner->BaseDefenseTeamCount;
        CellStruct waypoint;
        Type->GetWaypoint(&waypoint);
        if (waypoint != CellStruct::Empty) {
            Type->GetWaypoint(&waypoint);
            SpawnCell = MapClass::Instance.GetCellAt(waypoint);
        }
        ++Type->cntInstances;
        if (Type->Tag) {
            void* storage = YRMemory::Allocate(sizeof(TagClass));
            Tag = storage ? new (storage) TagClass(Type->Tag) : nullptr;
        }
        void* storage = YRMemory::Allocate(sizeof(ScriptClass));
        CurrentScript = storage ? new (storage) ScriptClass(Type->ScriptType) : nullptr;
    }
}
TeamClass::~TeamClass() {
    for (int i = 0; i < AITriggerTypeClass::Array.Count; ++i) {
        auto* trigger = AITriggerTypeClass::Array[i];
        if (trigger->Team1 == Type) {
            if (AchievedGreatSuccess)
                trigger->RegisterSuccess();
            else
                trigger->RegisterFailure();
        }
    }
    if (Owner && Type && Type->IsBaseDefense) --Owner->BaseDefenseTeamCount;
    if (Game::IsActive && Type) {
        while (FirstUnit)
            LiberateMember(FirstUnit);
        --Type->cntInstances;
        if (Tag && !Tag->InstanceCount) Tag->Destroy();
        Tag = nullptr;
    }
    NotifyObjectExpired(true);
    ScriptExpirationListeners.Remove(this);
    AbstractClass::Array.Remove(this);
    TypeExpirationListeners.Remove(this);
    TagExpirationListeners.Remove(this);
    Array.Remove(this);
    if (IsTransient && Type) GameDelete(Type);
}

void TeamClass::PointerExpired(AbstractClass* object, bool removed) {
    if (Tag == object) Tag = nullptr;
    if (FirstUnit == object && removed) FirstUnit = FirstUnit->NextTeamMember;
    if (Owner == object) Owner = nullptr;
    if (CurrentScript == object) CurrentScript = nullptr;
    if (Focus == object) Focus = nullptr;
    if (QueuedFocus == object) QueuedFocus = nullptr;
    if (SpawnCell == object) SpawnCell = nullptr;
    if (Type == object) Type = nullptr;
    if (ClosestMember == object) ClosestMember = nullptr;
    if (Target == object) Target = nullptr;
    if (static_cast<std::uintptr_t>(unknown_44) == reinterpret_cast<std::uintptr_t>(object)) unknown_44 = 0;
}
bool TeamClass::AddMember(FootClass* foot, bool force) {
    int index = 0;
    if (!CanAddMember(foot, &index, force)) return false;
    if (foot->Team) foot->Team->LiberateMember(foot);
    if (!force) ++CountObjects[index];
    foot->IsInitiated = FirstUnit == nullptr;
    foot->NextTeamMember = FirstUnit;
    FirstUnit = foot;
    foot->Team = this;
    foot->Group = Type->Group == -1 && Type->TaskForce ? Type->TaskForce->Group : Type->Group;
    if (Tag && (!Type->OnTransOnly || foot->GetTechnoType()->Passengers > 0)) foot->AttachTrigger(Tag);
    ++TotalObjects;
    TotalThreatValue += foot->GetThreatValue();
    if (!SpawnCell) update_rally_center(*this);
    NeedsToDisappear = JustDisappeared = true;
    foot->RecruitableB = Type->AreTeamMembersRecruitable;
    return true;
}
bool TeamClass::CanAddMember(FootClass* foot, int* index, bool force) const {
    if (!foot || foot->Team == this || !Type || !Type->TaskForce || !foot->IsAlive || !foot->Health ||
        (!Unsorted::ScenarioInit && foot->InLimbo) || foot->Owner != Owner)
        return false;
    if (foot->HasAnyLink() &&
        (foot->WhatAmI() != AbstractType::Aircraft || !static_cast<AircraftClass*>(foot)->Type->AirportBound))
        return false;
    *index = 0;
    while (*index < Type->TaskForce->CountEntries && Type->TaskForce->Entries[*index].Type != foot->GetType())
        ++*index;
    if (*index == Type->TaskForce->CountEntries && !force) return false;
    if (foot->GetCurrentMission() != Mission::None && !MissionClass::IsRecruitableMission(foot->GetCurrentMission()))
        return false;
    if (!foot->RecruitableA && !Type->Autocreate) return false;
    ScriptActionNode action{TMISSION_NONE, 0};
    if (CurrentScript) CurrentScript->GetCurrentAction(&action);
    bool permitted = action.Action == TMISSION_UNLOAD;
    if (foot->WhatAmI() == AbstractType::Aircraft) permitted = true;
    if (!foot->RecruitableB && Type->Autocreate && !permitted) return false;
    if (foot->ShouldEnterAbsorber || foot->ShouldEnterOccupiable || foot->ShouldGarrisonStructure ||
        foot->DrainTarget || foot->BunkerLinkedItem)
        return false;
    if (foot->Team && foot->Team->Type->Priority >= Type->Priority) return false;
    if (foot->WhatAmI() == AbstractType::Aircraft && foot->GetWeapon(0)->WeaponType && !foot->Ammo) return false;
    if (!(CountObjects[*index] < Type->TaskForce->Entries[*index].Amount || force)) return false;
    return !foot->BunkerLinkedItem && !foot->LocomotorSource;
}
void TeamClass::LiberateMember(FootClass* foot, int index, byte count) {
    if (!foot) return;
    foot->unknown_bool_6B8 = true;
    if (foot->Team != this) return;
    if (foot->AttachedTag == Tag && foot->Owner && !foot->Owner->IsControlledByHuman()) foot->AttachTrigger(nullptr);
    if (index == -1 && Type && Type->TaskForce)
        for (index = 0; index < Type->TaskForce->CountEntries; ++index)
            if (Type->TaskForce->Entries[index].Type == foot->GetType()) break;
    if (Type && Type->TaskForce && unsigned(index) < unsigned(Type->TaskForce->CountEntries)) --CountObjects[index];
    FootClass** next = &FirstUnit;
    while (*next && *next != foot)
        next = &(*next)->NextTeamMember;
    if (*next) {
        *next = foot->NextTeamMember;
        foot->Team = nullptr;
        foot->NextTeamMember = nullptr;
        foot->SuspendedMission = Mission::None;
        foot->LastDestination = nullptr;
        foot->LastTarget = nullptr;
        --TotalObjects;
        TotalThreatValue -= foot->GetThreatValue();
    }
    foot->Team = nullptr;
    if (!Unsorted::ScenarioInit && foot->IsAlive && !foot->IsCrashing && !foot->InLimbo && !count)
        foot->EnterIdleMode(false, true);
    bool initiated = false;
    for (auto* member = FirstUnit; member; member = member->NextTeamMember)
        initiated |= member->IsInitiated;
    if (!initiated && FirstUnit) {
        FirstUnit->IsInitiated = true;
        SpawnCell = nullptr;
    }
    NeedsToDisappear = JustDisappeared = true;
}
FootClass* TeamClass::FetchALeader() const {
    FootClass* result = FirstUnit;
    int score = -1;
    for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
        const int rating = foot->GetTechnoType()->LeadershipRating;
        // Scenario initialization selects leaders before their placement.
        if (foot->IsAlive && foot->Health && (Unsorted::ScenarioInit || !foot->InLimbo) &&
            (foot->IsInitiated || foot->WhatAmI() == AbstractType::Aircraft) && rating > score) {
            result = foot;
            score = rating;
        }
    }
    return result;
}
void TeamClass::AssignMissionTarget(AbstractClass* target) {
    if (target != QueuedFocus && QueuedFocus)
        for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
            const bool destination = foot->Destination == QueuedFocus, attack = foot->Target == QueuedFocus;
            if (destination || attack) {
                foot->QueueMission(Mission::Guard, false);
                if (destination) foot->SetDestination(nullptr, true);
                if (attack) foot->SetTarget(nullptr);
            }
        }
    if (!Focus || Focus == QueuedFocus) Focus = target;
    QueuedFocus = target;
    if (target && target->WhatAmI() == AbstractType::Cell) {
        IsLeavingMap = !MapClass::Instance.IsWithinUsableArea(static_cast<CellClass*>(target), true);
        if (IsLeavingMap)
            for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember)
                foot->SetDestination(nullptr, true);
    }
}
int TeamClass::Recruit(int entry) {
    CoordStruct origin = SpawnCell ? SpawnCell->GetCoords() : CoordStruct::Empty;
    CellStruct waypoint;
    Type->GetWaypoint(&waypoint);
    if (waypoint != CellStruct::Empty) {
        Type->GetWaypoint(&waypoint);
        origin = {int(waypoint.X) * 256 + 128, int(waypoint.Y) * 256 + 128, 0};
    }
    if (Type->TaskForce->Entries[entry].Amount <= CountObjects[entry]) return 0;
    const int group = Type->GetGroup();
    FootClass* best = nullptr;
    int bestDistance = -1;
    const auto search = [&](const auto& array, bool vehicles) {
        for (int i = 0; i < array.Count; ++i) {
            auto* foot = array[i];
            if (group != -2 && foot->Group != group && !Type->Recruiter) continue;
            const auto at = foot->GetCoords();
            // 0x5F6560 is squared XY distance with 32-bit wraparound.
            const unsigned dx = unsigned(at.X) - unsigned(origin.X), dy = unsigned(at.Y) - unsigned(origin.Y);
            const int distance = std::bit_cast<int>(dx * dx + dy * dy + (foot->Group != group ? 12800u : 0u));
            if (vehicles && (foot->Owner != Owner || static_cast<UnitClass*>(static_cast<FootClass*>(foot))->Type !=
                                                         Type->TaskForce->Entries[entry].Type))
                continue;
            if ((distance < bestDistance || bestDistance == -1) && CanAddMember(foot, &entry, false)) {
                best = foot;
                bestDistance = distance;
            }
        }
    };
    const auto kind = Type->TaskForce->Entries[entry].Type->WhatAmI();
    switch (kind) {
        case AbstractType::Infantry:
        case AbstractType::InfantryType:
            search(InfantryClass::Array, false);
            break;
        case AbstractType::Aircraft:
        case AbstractType::AircraftType:
            search(AircraftClass::Array, false);
            break;
        case AbstractType::Unit:
        case AbstractType::UnitType:
            search(UnitClass::Array, true);
            break;
        default:
            return 0;
    }
    if (!best) return 0;
    best->SetTarget(nullptr);
    AddMember(best, false);
    if (kind == AbstractType::Unit || kind == AbstractType::UnitType) {
        auto* passenger = best->Passengers.GetFirstPassenger();
        while (passenger) {
            AddMember(passenger, false);
            auto* next = passenger->NextObject;
            if (!next || (unsigned(next->AbstractFlags) & 4u) == 0) break;
            passenger = static_cast<FootClass*>(next);
        }
    }
    return 1;
}

bool TeamClass::RecalculateStrength() {
    const bool wasUnderStrength = IsUnderStrength;
    int desired = 0;
    for (int i = 0; i < Type->TaskForce->CountEntries; ++i)
        desired = std::bit_cast<int>(unsigned(desired) + unsigned(Type->TaskForce->Entries[i].Amount));
    if (TotalObjects > 0) {
        IsFullStrength = TotalObjects == desired;
        if (IsFullStrength) IsHasBeen = true;
        IsUnderStrength =
            Type->Reinforce ? (desired > 2 ? TotalObjects <= desired / 3 : TotalObjects < desired) : !IsHasBeen;
        if (Type->GuardSlower) GuardSlowerIsNotUnderStrength = !IsUnderStrength;
        JustDisappeared = NeedsToDisappear = false;
    } else {
        GuardSlowerIsNotUnderStrength = false;
        IsUnderStrength = true;
        IsFullStrength = false;
        SpawnCell = nullptr;
        if (IsHasBeen) {
            if (IsLeavingMap)
                for (int i = LogicClass::PendingTags.Count - 1; i >= 0; --i)
                    if (LogicClass::PendingTags[i]->RaiseEvent(static_cast<TriggerEvent>(23), nullptr,
                                                               CellStruct::Empty) &&
                        !LogicClass::PendingTags.Count)
                        break;
            delete this;
            return false;
        }
    }
    if (wasUnderStrength != IsUnderStrength) IsReforming = true;
    return true;
}

void TeamClass::Update() {
    if (IsSuspended) {
        if (SuspendTimer.GetTimeLeft()) return;
        IsSuspended = false;
    }
    if (!Type || !CurrentScript || !CurrentScript->Type) return;
    if (NeedsToDisappear && !RecalculateStrength()) return;
    if (IsMoving && IsUnderStrength) Retreat();
    if (!IsMoving && (IsFullStrength || IsForcedActive)) {
        IsMoving = IsHasBeen = true;
        IsUnderStrength = false;
        if (IsReforming || IsForcedActive)
            for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember)
                foot->IsInitiated = true;
        CurrentScript->CurrentMission = -1;
        StepCompleted = true;
    }
    if (IsReforming || IsMoving || !SpawnCell || !ClosestMember) {
        ScriptActionNode action;
        CurrentScript->GetCurrentAction(&action);
        update_rally_center(*this);
    }
    // 0x6E9227: recruit before checking for an empty team. Each task-force
    // entry admits at most one selected member per update (plus its cargo).
    if ((!IsMoving || (!IsFullStrength && Type->Reinforce)) && (!Owner->IsControlledByHuman() || !IsHasBeen)) {
        for (int entry = 0; entry < Type->TaskForce->CountEntries; ++entry)
            if (CountObjects[entry] < Type->TaskForce->Entries[entry].Amount) Recruit(entry);
    }
    if (!FirstUnit) {
        if (IsHasBeen || (SessionClass::Instance.GameMode != GameMode::Campaign &&
                          std::bit_cast<int>(unsigned(Unsorted::CurrentFrame) - unsigned(CreationFrame)) >
                              RulesClass::Instance->DissolveUnfilledTeamDelay)) {
            if (IsLeavingMap)
                for (int i = 0; i < LogicClass::PendingTags.Count; ++i)
                    if (LogicClass::PendingTags[i]->RaiseEvent(static_cast<TriggerEvent>(23), nullptr,
                                                               CellStruct::Empty)) {
                        --i;
                        if (!LogicClass::PendingTags.Count) break;
                    }
            delete this;
            return;
        }
    }
    const bool warping = FirstUnit && FirstUnit->IsWarpingIn();
    if (!IsMoving) {
        CoordinateMove();
        return;
    }
    if (IsReforming || IsUnderStrength || warping) {
        IsReforming = !regroup_members(*this);
        return;
    }
    bool next = false;
    if (StepCompleted) {
        next = true;
        StepCompleted = false;
        CurrentScript->NextMission();
        for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember)
            if (!Type->TransportsReturnOnUnload || foot->GetTechnoType()->Passengers <= 0)
                foot->ArchiveTarget = nullptr;
        if (!CurrentScript->HasCurrentMission()) {
            delete this;
            return;
        }
        AssignMissionTarget(nullptr);
        Focus = nullptr;
    } else if (!Focus)
        Focus = QueuedFocus;
    if (TargetNotAssigned) {
        next = true;
        TargetNotAssigned = false;
        AssignMissionTarget(nullptr);
        Focus = nullptr;
    }
    ScriptActionNode action;
    CurrentScript->GetCurrentAction(&action);
    switch (action.Action) {
        case TMISSION_ATT_WAYPT:
            AttackWaypoint(action, next);
            return;
        case TMISSION_GARRISON_WAYPOINT:
            GarrisonWaypoint(action, next);
            return;
        case TMISSION_SPY:
            Spy(action, next);
            return;
        case TMISSION_PATROL:
            Patrol(action, next);
            return;
        case TMISSION_DO:
            DoMission(action, next);
            return;
        case TMISSION_FORCE_FACING:
            ForceFacing(action, next);
            return;
        case TMISSION_ATTACK_BUILDING_WITH_PROPERTY:
            AttackBuilding(action, next);
            return;
        case TMISSION_MOVETO_BUILDING_WITH_PROPERTY:
            MoveToBuilding(action, next);
            return;
        case TMISSION_MOVETO_FRIENDLY_BUILDING:
            MoveToFriendlyBuilding(action, next);
            return;
        case TMISSION_SCRIPT: {
            GameDelete(CurrentScript);
            auto* storage = YRMemory::Allocate(sizeof(ScriptClass));
            CurrentScript = storage ? new (storage) ScriptClass(ScriptTypeClass::Array[action.Argument]) : nullptr;
            // 0x6E976F clears the pointer before a terminal null access. Native
            // execution terminates explicitly rather than invoking undefined behavior.
            if (!CurrentScript) std::abort();
            CurrentScript->ClearMission();
            return;
        }
        case TMISSION_TEAMCHANGE: {
            auto* storage = YRMemory::Allocate(sizeof(TeamClass));
            auto* team = storage ? new (storage) TeamClass(TeamTypeClass::Array[action.Argument], Owner, 0) : nullptr;
            while (FirstUnit) {
                auto* foot = FirstUnit;
                LiberateMember(foot, -1, 0);
                // With a failed allocation, original CanAddMember rejects the
                // detached member because both its Team and the receiver are null.
                if (team) team->AddMember(foot, false);
            }
            delete this;
            return;
        }
        case TMISSION_PLAY_SOUND:
            VocClass::PlayGlobal(action.Argument, 0x2000, 1.0f, nullptr);
            StepCompleted = true;
            return;
        case TMISSION_PLAY_MUSIC:
            ThemeClass::Instance.Queue(action.Argument);
            StepCompleted = true;
            return;
        case TMISSION_PLAY_MOVIE:
            Game::PlayMovie(action.Argument, -1, 1, 1, 1);
            StepCompleted = true;
            return;
        case TMISSION_REDUCE_TIBERIUM:
            for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
                const auto at = foot->GetCoords();
                MapClass::AreaReduceTiberium(CellStruct{short(at.X / 256), short(at.Y / 256)});
            }
            StepCompleted = true;
            return;
        case TMISSION_CENTER_VIEWPOINT: {
            auto at = SpawnCell->GetCoords();
            at.Z = MapClass::Instance.GetZPos(&at);
            auto* cell = MapClass::Instance.GetCellAt(at);
            if ((cell->Flags & CellFlags::BridgeHead) != CellFlags{} ||
                (MapClass::Instance.GetCellAt(at)->Flags & CellFlags::BridgeBody) != CellFlags{})
                at.Z += CellClass::BridgeHeight;
            TacticalClass::Instance->FocusOn(&at, action.Argument);
            StepCompleted = true;
            return;
        }
        case TMISSION_RESHROUD:
            MapClass::Instance.ShroudMap(nullptr);
            StepCompleted = true;
            return;
        case TMISSION_REVEAL:
            MapClass::Instance.RevealShroud(nullptr);
            StepCompleted = true;
            return;
        case TMISSION_PLAY_ANIM:
            PlayAnimation(action, next);
            return;
        case TMISSION_HOUND_DOG: {
            update_rally_center(*this);
            Focus = SpawnCell;
            CoordinateMove();
            TechnoClass* leader = nullptr;
            if (SpawnCell) {
                switch (SpawnCell->WhatAmI()) {
                    case AbstractType::Unit:
                    case AbstractType::Aircraft:
                    case AbstractType::Building:
                    case AbstractType::Infantry:
                        leader = reinterpret_cast<TechnoClass*>(SpawnCell);
                        break;
                    default:
                        break;
                }
            }
            if (leader)
                for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
                    if (!foot->IsAlive || !foot->Health || (!Unsorted::ScenarioInit && foot->InLimbo) ||
                        (!foot->IsInitiated && foot->WhatAmI() != AbstractType::Aircraft))
                        continue;
                    if (foot->Target && foot->Target != leader->Target) foot->SetTarget(nullptr);
                    if (leader->Target && !foot->Locomotor->Is_Moving() && foot->IsArmed() &&
                        foot->GetCurrentMission() == Mission::Guard) {
                        foot->QueueMission(Mission::Attack, false);
                        foot->SetTarget(leader->Target);
                    }
                }
            return;
        }
        case TMISSION_SCOUT:
            Scout(action, next);
            return;
        case TMISSION_GATHER_AT_ENEMY:
            GatherAtEnemy(action, next);
            return;
        case TMISSION_GATHER_AT_BASE:
            GatherAtBase(action, next);
            return;
        case TMISSION_ASK_FOR_SW:
            AskForSW(action, next);
            return;
        case TMISSION_CHRONOSHIFT_TO_BUILDING:
            ChronoshiftToBuilding(action, next);
            return;
        case TMISSION_CHRONOSHIFT_TO_TARGET:
            ChronoshiftToTarget(action, next);
            return;
        case TMISSION_ION_STORM_START:
            if (!LightningStorm::IsActive())
                LightningStorm::Start(action.Argument, RulesClass::Instance->LightningDeferment, CellStruct::Empty,
                                      nullptr);
            StepCompleted = true;
            return;
        case TMISSION_ION_STORM_END:
            if (LightningStorm::IsActive()) LightningStorm::RequestStop();
            StepCompleted = true;
            return;
        case TMISSION_TALK_BUBBLE:
            if (next) {
                TechnoClass::SetTalkBubble(FirstUnit, action.Argument);
                StepCompleted = true;
            }
            return;
        case TMISSION_MOVECELL:
            if (next)
                AssignMissionTarget(MapClass::Instance.GetCellAt(
                    CellStruct{short(action.Argument % 128), short(action.Argument / 128)}));
            CoordinateMove();
            return;
        case TMISSION_GUARD:
            if (next) {
                GuardAreaTimer.Start(std::bit_cast<int>(15u * unsigned(action.Argument)));
                // 0x6E941D also stores the script argument in the clock's ABI slot.
                std::memcpy(reinterpret_cast<byte*>(&GuardAreaTimer) + 4, &action.Argument, 4);
            }
            regroup_members(*this);
            if (!GuardAreaTimer.GetTimeLeft()) StepCompleted = true;
            return;
        case TMISSION_WIN:
        case TMISSION_LOSE:
            if (action.Action == TMISSION_WIN)
                HouseClass::CurrentPlayer->Win(false);
            else
                HouseClass::CurrentPlayer->Lose(false);
            StepCompleted = true;
            return;
        case TMISSION_SET_GLOBAL:
        case TMISSION_CLEAR_GLOBAL:
            ScenarioClass::Instance->SetGlobal(action.Argument, action.Action == TMISSION_SET_GLOBAL);
            StepCompleted = true;
            return;
        case TMISSION_IDLE_ANIM:
            for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember)
                foot->PlayIdleAnim(action.Argument);
            StepCompleted = true;
            return;
        case TMISSION_PANIC:
        case TMISSION_UNPANIC:
            for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
                if (action.Action == TMISSION_PANIC)
                    foot->Panic();
                else
                    foot->UnPanic();
            }
            StepCompleted = true;
            return;
        case TMISSION_SCATTER:
            for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember)
                foot->Scatter(CoordStruct::Empty, true, false);
            StepCompleted = true;
            return;
        case TMISSION_GOTO_SHROUD:
            if (next)
                AssignMissionTarget(MapClass::Instance.FindShroudedCell(
                    SpawnCell ? static_cast<AbstractClass*>(SpawnCell) : FirstUnit));
            CoordinateMove();
            return;
        case TMISSION_SELF_DESTRUCT: {
            bool damaged;
            do {
                damaged = false;
                for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
                    if (foot->Health > 0 && foot->IsAlive && foot->IsOnMap && !foot->InLimbo) {
                        int damage = foot->Health;
                        foot->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, false, nullptr);
                        damaged = true;
                    }
                }
            } while (damaged);
            StepCompleted = true;
            return;
        }
        case TMISSION_FULLY_LOADED:
            for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember)
                if (foot->GetTechnoType()->Passengers > foot->Passengers.NumPassengers) return;
            StepCompleted = true;
            return;
        case TMISSION_UNLOAD_TRUCK:
        case TMISSION_LOAD_TRUCK:
            for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
                if (foot->WhatAmI() != AbstractType::Unit) continue;
                const bool unload = action.Action == TMISSION_UNLOAD_TRUCK;
                if (!_strcmpi(foot->GetTechnoType()->ID, unload ? "TRUCKB" : "TRUCKA"))
                    static_cast<UnitClass*>(foot)->Type =
                        UnitTypeClass::Array[UnitTypeClass::FindIndex(unload ? "TRUCKA" : "TRUCKB")];
                game::techno_spatial_dirty(foot);
            }
            StepCompleted = true;
            return;
        case TMISSION_ENTER_GRINDER:
        case TMISSION_ENTER_TANK_BUNKER:
        case TMISSION_ENTER_BIO_REACTOR:
        case TMISSION_ENTER_BATTLE_BUNKER:
        case TMISSION_ENTER_NEUTRAL_BUILDING:
            for (auto* foot = FirstUnit; foot;) {
                bool entered = false;
                switch (action.Action) {
                    case TMISSION_ENTER_GRINDER:
                        entered = foot->EnterGrinder();
                        break;
                    case TMISSION_ENTER_TANK_BUNKER:
                        entered = foot->EnterTankBunker();
                        break;
                    case TMISSION_ENTER_BIO_REACTOR:
                        entered = foot->EnterBioReactor();
                        break;
                    case TMISSION_ENTER_BATTLE_BUNKER:
                        entered = foot->EnterBattleBunker();
                        break;
                    default:
                        entered = foot->GarrisonStructure();
                        break;
                }
                auto* nextMember = foot->NextTeamMember;
                if (entered) LiberateMember(foot, -1, 1);
                foot = nextMember;
            }
            StepCompleted = true;
            return;
        default:
            break;
    }
    // OpenTS TMission_ATTACK / Coordinate_Attack; YR 0x6ED090 /
    // 0x6EB490, quarry masks from 0x645BB0. Preserve in-flight cargo teams.
    if (action.Action == TMISSION_ATTACK) {
        if (!QueuedFocus) {
            auto* leader = FetchALeader();
            if (!leader) return;
            ThreatType mask = ThreatType::Normal;
            switch (static_cast<QuarryType>(action.Argument)) {
                case QuarryType::Buildings:
                    mask = ThreatType::Buildings;
                    break;
                case QuarryType::Harvesters:
                    mask = ThreatType::Tiberium;
                    break;
                case QuarryType::Infantry:
                    mask = ThreatType::Infantry;
                    break;
                case QuarryType::Vehicles:
                    mask = ThreatType::Vehicles;
                    break;
                case QuarryType::Factories:
                    mask = ThreatType::Factories;
                    break;
                case QuarryType::Defenses:
                    mask = ThreatType::BaseDefense;
                    break;
                case QuarryType::Power:
                    mask = ThreatType::Power;
                    break;
                case QuarryType::OccupiableBuildings:
                    mask = ThreatType::OccupiableBuildings;
                    break;
                case QuarryType::TechBuildings:
                    mask = ThreatType::TechBuildings;
                    break;
                default:
                    break;
            }
            auto at = leader->Location;
            AssignMissionTarget(leader->GreatestThreat(mask, &at, Type->OnlyTargetHouseEnemy));
            if (QueuedFocus) {
                bool armed = false;
                for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember)
                    if (foot->GetTechnoType()->Ammo <= 0 || foot->Ammo > 0) {
                        armed = true;
                        break;
                    }
                if (!armed) StepCompleted = true;
            } else
                StepCompleted = true;
        }
        if (!QueuedFocus)
            StepCompleted = true;
        else {
            bool armed = false;
            for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember)
                if (foot->GetTechnoType()->Ammo <= 0 || foot->Ammo > 0) {
                    armed = true;
                    break;
                }
            if (!armed) StepCompleted = true;
        }
        CoordinateAttack();
        return;
    }
    if (action.Action == TMISSION_LOOP) {
        CurrentScript->CurrentMission = std::bit_cast<int>(unsigned(action.Argument) - 2u);
        StepCompleted = true;
        return;
    }
    if (action.Action == TMISSION_UNLOAD) {
        TransportUnload(action, next);
        return;
    }
    if (action.Action == TMISSION_DEPLOY) {
        Unload(action, next);
        return;
    }
    if (action.Action == TMISSION_LOAD) {
        TMission_LOAD(action, next);
        return;
    }
    if (action.Action == TMISSION_CHANGE_HOUSE) {
        // 0x6E9683: country index, not the runtime house-array index.
        for (auto* foot = FirstUnit; foot;) {
            auto* nextMember = foot->NextTeamMember;
            foot->SetOwningHouse(HouseClass::FindByCountryIndex(action.Argument), true);
            foot = nextMember;
        }
        StepCompleted = true;
        return;
    }
    if (action.Action == TMISSION_DESTROY_MEMBERS) {
        // 0x6E954D: only members still owned by this team disappear.
        for (auto* foot = FirstUnit; foot;) {
            auto* nextMember = foot->NextTeamMember;
            foot->Limbo();
            foot->UnInit();
            foot = nextMember;
        }
        StepCompleted = true;
        return;
    }
    if (action.Action == TMISSION_PLAY_SPEECH) {
        StepCompleted = true;
        return;
    }
    if (action.Action == TMISSION_BEGIN_PRODUCTION) {
        Owner->Production = true;
        StepCompleted = true;
        return;
    }
    if (action.Action == TMISSION_FIRE_SALE) {
        Owner->AIMode = AIMode::SellAll;
        StepCompleted = true;
        return;
    }
    // 0x6E9BA1 / 0x6E9BBE: even an unchanged or invalid local completes
    // this step. The following script action starts on the next update.
    if (action.Action == TMISSION_SET_LOCAL || action.Action == TMISSION_CLEAR_LOCAL) {
        ScenarioClass::Instance->SetLocal(action.Argument, action.Action == TMISSION_SET_LOCAL);
        StepCompleted = true;
        return;
    }
    if (action.Action == TMISSION_SUCCESS) {
        StepCompleted = true;
        AchievedGreatSuccess = true;
        return;
    }
    // 0x6E9CF9: write once on entry; normal object AI owns the countdown.
    if (action.Action == TMISSION_FLASH) {
        if (next) {
            for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember)
                foot->Flashing.DurationRemaining = action.Argument;
            StepCompleted = true;
        }
        return;
    }
    if (action.Action == TMISSION_MOVE) MoveToWaypoint(action, next);
}

void TeamClass::MoveToWaypoint(const ScriptActionNode& action, bool first) {
    if (first) {
        auto* leader = FetchALeader();
        if (!leader) return;
        auto at = ScenarioClass::Instance->GetWaypointCoords(action.Argument);
        AssignMissionTarget(MapClass::Instance.GetCellAt(at));
        ScriptActionNode current;
        CurrentScript->GetCurrentAction(&current);
        if (!IsMoving || !CurrentScript->HasCurrentMission() || current.Action != TMISSION_MOVE ||
            MapClass::Instance.IsWithinUsableArea(ScenarioClass::Instance->GetWaypointCoords(current.Argument), true)) {
            if (leader->IsCellOccupied(MapClass::Instance.GetCellAt(at), FacingType::None, -1, nullptr, false) !=
                ::Move::OK)
                at = MapClass::Instance.NearByLocation(at, leader->GetTechnoType()->SpeedType, -1, MovementZone::Normal,
                                                       false, 1, 1, false, false, false, true, CellStruct{0, 0}, false,
                                                       false);
        }
        AssignMissionTarget(at == CellStruct::Empty ? nullptr : MapClass::Instance.GetCellAt(at));
    }
    CoordinateMove();
}

void TeamClass::CoordinateMove() {
    if (!FirstUnit) return;
    if (!Focus) Focus = QueuedFocus;
    if (!Focus) return;
    const auto active = [](FootClass* foot) {
        return foot->IsAlive && foot->Health && (Unsorted::ScenarioInit || !foot->InLimbo);
    };
    const auto stray = [this] {
        ScriptActionNode action;
        CurrentScript->GetCurrentAction(&action);
        return action.Action == TMISSION_GATHER_AT_ENEMY || action.Action == TMISSION_GATHER_AT_BASE
                   ? RulesClass::Instance->RelaxedStray
                   : RulesClass::Instance->Stray;
    };
    // 0x6EBF50: hold the leading members while a lagging member catches up.
    if (IsLagging) {
        bool lagging = false;
        for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
            if (!active(foot) || (!foot->IsInitiated && foot->WhatAmI() != AbstractType::Aircraft)) continue;
            int distance = stray();
            if (foot->WhatAmI() == AbstractType::Aircraft) distance *= 3;
            if (Type->GuardSlower && !foot->vt_entry_4E0()) distance /= 3;
            const auto anchor = ClosestMember->GetCoords();
            const auto position = foot->GetCoords();
            const auto delta = [](int a, int b) { return double(std::bit_cast<int>(unsigned(a) - unsigned(b))); };
            const double x = delta(position.X, anchor.X), y = delta(position.Y, anchor.Y),
                         z = delta(position.Z, anchor.Z);
            if (game::x87_integer(Math::sqrt(x * x + y * y + z * z)) <= distance) {
                if (foot->GetCurrentMission() != Mission::Guard) {
                    foot->QueueMission(Mission::Guard, false);
                    foot->SetDestination(nullptr, true);
                }
            } else {
                if (!foot->Destination) {
                    foot->QueueMission(Mission::Move, false);
                    foot->SetDestination(ClosestMember, true);
                }
                lagging = true;
            }
        }
        IsLagging = lagging;
        if (lagging) return;
    }
    bool complete = true, processed = false;
    const auto cell_of = [](FootClass* foot) {
        const auto at = foot->GetCoords();
        return MapClass::Instance.GetCellAt(at);
    };
    for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
        if (active(foot) && !foot->IsInitiated) {
            if (foot->DistanceFrom3D(SpawnCell) <= stray())
                foot->IsInitiated = true;
            else {
                if (!foot->Destination) {
                    foot->QueueMission(Mission::Move, false);
                    foot->SetTarget(nullptr);
                    foot->SetDestination(SpawnCell, true);
                }
                complete = false;
            }
        }
        if (foot->GetCurrentMission() == Mission::Unload || foot->QueuedMission == Mission::Unload) complete = false;
        if (!active(foot) || (!foot->IsInitiated && foot->WhatAmI() != AbstractType::Aircraft) ||
            foot->GetCurrentMission() == Mission::Unload || foot->QueuedMission == Mission::Unload)
            continue;
        processed = true;
        const int limit = stray() * (foot->IsInAir() ? 2 : 1);
        const int distance = foot->DistanceFrom3D(Focus);
        ScriptActionNode next;
        CurrentScript->GetNextAction(&next);
        const bool arrived = distance <= limit && (foot->GetHeight() >= 0 || next.Action == TMISSION_MOVE) &&
                             (foot->WhatAmI() != AbstractType::Aircraft || foot->GetZ() <= 0 ||
                              cell_of(foot) == Focus || next.Action == TMISSION_MOVE);
        if (arrived) {
            if (foot->GetCurrentMission() == Mission::Move &&
                (!foot->Destination || (foot->DistanceFrom3D(foot->Destination) <= RulesClass::Instance->CloseEnough &&
                                        !foot->Locomotor->Is_Moving())) &&
                !foot->Target) {
                foot->SetDestination(nullptr, true);
                foot->EnterIdleMode(false, true);
            }
        } else {
            if (Type->Aggressive && foot->Target && !foot->unknown_bool_68E) continue;
            if (Type->Aggressive && foot->Target) foot->SetTarget(nullptr);
            if (foot->GetCurrentMission() != Mission::Move) {
                foot->QueueMission(Mission::Move, false);
                if (foot->ReadyToNextMission()) foot->NextMission();
            }
            if (!foot->Destination) foot->SetDestination(Focus, true);
            if (foot->Destination != Focus &&
                (foot->GetTechnoType()->BalloonHover ||
                 (foot->WhatAmI() == AbstractType::Aircraft && foot->Destination == foot->GetCell())))
                foot->SetDestination(Focus, true);
            complete = false;
        }
        const bool docking = foot->WhatAmI() == AbstractType::Aircraft && foot->Destination == foot->GetCell();
        const bool hovering =
            (unsigned(foot->AbstractFlags) & 1) && foot->GetTechnoType()->BalloonHover && distance < limit;
        if (foot->Destination && !docking && !hovering) complete = false;
    }
    if (processed && complete && IsMoving) StepCompleted = true;
}

void TeamClass::ScanLimit() {
    AssignMissionTarget(nullptr);
    for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember) {
        foot->SetTarget(nullptr);
        foot->IsScanLimited = true;
    }
}

void TeamClass::Retreat() {
    IsMoving = false;
    CurrentScript->ClearMission();
    if (TotalObjects <= 0) {
        SpawnCell = nullptr;
        return;
    }
    update_rally_center(*this);
    if (!SpawnCell) {
        if (!ClosestMember) return;
        SpawnCell = reinterpret_cast<CellClass*>(ClosestMember);
    }
    auto at = SpawnCell->GetCoords();
    CellStruct destination{short(at.X / 256), short(at.Y / 256)};
    int best = INT_MAX;
    const int count = Owner->Buildings.Count;
    for (int i = 0; i < count; ++i) {
        auto* building = Owner->Buildings[i];
        if (!building || building->InLimbo || building->GetWeapon(0)->WeaponType) continue;
        at = building->GetCoords();
        const CellStruct cell{short(at.X / 256), short(at.Y / 256)};
        const auto origin = SpawnCell->GetCoords();
        at = building->GetCoords();
        const auto delta = [](int a, int b) { return double(std::bit_cast<int>(unsigned(a) - unsigned(b))); };
        const double x = delta(at.X, origin.X), y = delta(at.Y, origin.Y), z = delta(at.Z, origin.Z);
        int score = std::bit_cast<int>(unsigned(game::x87_integer(Math::sqrt(x * x + z * z + y * y))) *
                                       (unsigned(MapClass::Instance.GetThreatPosed(cell, Owner)) + 1u));
        if (building->Type->BaseNormal) score /= 2;
        if (score >= best) continue;
        at = SpawnCell->GetCoords();
        const CellStruct current{short(at.X / 256), short(at.Y / 256)};
        CellStruct result;
        FetchALeader()->FindRetreatCell(&result, current, cell, 2, 4);
        if (result != CellStruct::Empty) {
            best = score;
            destination = result;
        }
    }
    Focus = MapClass::Instance.GetCellAt(destination);
    CoordinateMove();
}

// OpenTS Took_Damage; YR 0x6EB380 retains regrouping but omits the TS
// assignment of the attacker to the team's focus at the end of this branch.
void TeamClass::MemberTookDamage(FootClass*, DamageState state, TechnoClass* source) {
    if (!Type || !source || state == DamageState::Unaffected || Type->Suicide) return;
    if (IsMoving) {
        for (auto* foot = FirstUnit; foot; foot = foot->NextTeamMember)
            if (foot == source) return;
        if (!FirstUnit || FirstUnit->WhatAmI() == AbstractType::Aircraft || !FirstUnit->GetWeapon(0)->WeaponType ||
            Focus == source || !Type->Annoyance)
            return;
    }
    SpawnCell = nullptr;
    NeedsReGrouping = true;
    IsReforming = true;
}
