// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::Detach; YR 0x007077C0 branches and ordering.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/TemporalClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/WaveClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"

void TechnoClass::PointerExpired(AbstractClass* object, bool removed) {
    // A null expiration is outside the original call contract.
    if (!object) return;
    RadioClass::PointerExpired(object, removed);
    if (removed) Passengers.RemovePassenger(reinterpret_cast<FootClass*>(object));
    if (SpawnOwner == object) SpawnOwner = nullptr;
    if (OriginallyOwnedByHouse == object) OriginallyOwnedByHouse = nullptr;
    if (MindControlledByHouse == object) MindControlledByHouse = nullptr;
    if (TemporalTargetingMe == object) TemporalTargetingMe = nullptr;
    if (Airstrike == object) Airstrike = nullptr;
    if (BeingManipulatedBy == object) BeingManipulatedBy = nullptr;
    if (BunkerLinkedItem == object && Unsorted::ScenarioInit) BunkerLinkedItem = nullptr;
    if (DrainTarget == object) {
        if (DrainAnim) { DrainAnim->UnInit(); DrainAnim = nullptr; }
        if (DrainTarget) {
            DrainTarget->DrainingMe = nullptr;
            if (DrainTarget->Owner) DrainTarget->Owner->RecheckPower = true;
            DrainTarget = nullptr;
        }
    }
    if (Transporter == object && Transporter &&
        (!Transporter->IsAlive || !Transporter->Health || Unsorted::ScenarioInit))
        Transporter = nullptr;
    if (DrainingMe == object) {
        auto* drainer = DrainingMe;
        if (drainer->DrainAnim) { drainer->DrainAnim->UnInit(); drainer->DrainAnim = nullptr; }
        if (drainer->DrainTarget) {
            drainer->DrainTarget->DrainingMe = nullptr;
            if (drainer->DrainTarget->Owner) drainer->DrainTarget->Owner->RecheckPower = true;
            drainer->DrainTarget = nullptr;
        }
        DrainingMe = nullptr;
    }
    if (DrainAnim == object) DrainAnim = nullptr;
    if (MindControlRingAnim == object) MindControlRingAnim = nullptr;
    if (DeployAnim == object) DeployAnim = nullptr;

    bool clear_target = true;
    if (!removed && (object->AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None && Owner) {
        auto* cell = MapClass::Instance.TryGetCellAt(object->GetCoords());
        const int index = Owner->ArrayIndex;
        if (cell && index >= 0 && index < 0x18 && cell->Sensors_InclHouse(index)) clear_target = false;
    }
    if (Target == object && clear_target && (removed || object->GetOwningHouse() != Owner)) {
        if (TargetingTimer.GetTimeLeft() > 10 && !Unsorted::ScenarioInit && ScenarioClass::Instance)
            TargetingTimer.Start(ScenarioClass::Instance->Random.RandomRanged(4, 8));
        SetTarget(nullptr);
        if (MissionIsOverriden()) {
            Mission_Revert();
            if (WhatAmI() == AbstractType::Aircraft && CurrentMission == Mission::Patrol) {
                MissionStatus = 0;
                static_cast<AircraftClass*>(this)->IsLocked = false;
            }
        }
    }
    if (LastTarget == object && clear_target) LastTarget = nullptr;
    if (Owner == object) { Owner = nullptr; DiscoveredByCurrentPlayer = false; }
    if (IsDisguised() && GetDisguise(true) == object) ClearDisguise();
    // Explicit fields: pointer arithmetic across distinct C++ members is not portable.
    ParticleSystemClass** particles[]{&FireParticleSystem, &SparkParticleSystem, &NaturalParticleSystem,
        &DamageParticleSystem, &RailgunParticleSystem, &unk1ParticleSystem, &unk2ParticleSystem, &FiringParticleSystem};
    for (auto** slot : particles) if (*slot == object) *slot = nullptr;
    if (Wave == object) Wave = nullptr;
    if (removed) {
        if (QueueUpToEnter == object) QueueUpToEnter = nullptr;
        if (ArchiveTarget == object) ArchiveTarget = nullptr;
        if (CaptureManager) CaptureManager->UnlinkPointer(object);
    }
    if (SpawnManager) SpawnManager->UnlinkPointer(object);
    if (TemporalImUsing) TemporalImUsing->UnlinkPointer(object);
    if (Airstrike && Airstrike->Owner == this) Airstrike->InvalidatePointer(object);
    if (DirectRockerLinkedUnit == object) {
        DirectRockerLinkedUnit->DirectRockerLinkedUnit = nullptr;
        DirectRockerLinkedUnit = nullptr;
    }
    if (LocomotorTarget == object) {
        ReleaseLocomotor(true);
        if (LocomotorTarget) { LocomotorTarget->LocomotorSource = nullptr; LocomotorTarget = nullptr; }
    }
    if (LocomotorSource == object) {
        ReleaseLocomotor(true);
        if (LocomotorSource) { LocomotorSource->LocomotorTarget = nullptr; LocomotorSource = nullptr; }
    }
    if (OldTeam == object) OldTeam = nullptr;
    if (BehindAnim == object) BehindAnim = nullptr;
    const int index = CurrentTargets.FindItemIndex(object);
    if (index >= 0) {
        CurrentTargetThreatValues.RemoveItem(index);
        CurrentTargets.Remove(object); // first only: preserve the YR duplicate semantics
    }
    AttackedTargets.Remove(object);
}
