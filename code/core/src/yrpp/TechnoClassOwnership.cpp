// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::Captured; calibrated to YR 0x7014A0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "techno_spatial.hpp"
#include "yrpp/HouseClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/CaptureManagerClass.h"

HouseClass* TechnoClass::GetOriginalOwner() const noexcept {
    if (MindControlledBy) return MindControlledBy->CaptureManager->GetOriginalOwner(const_cast<TechnoClass*>(this));
    if (MindControlledByHouse) return OriginallyOwnedByHouse;
    return Owner;
}

void TechnoClass::RemoveFromTargeting() { RemoveFromTargeting(this); }
void YRPP_FASTCALL TechnoClass::RemoveFromTargeting(AbstractClass* target) {
    auto* techno = target && (target->AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None
                       ? static_cast<TechnoClass*>(target)
                       : nullptr;
    for (int i = Array.Count - 1; i >= 0; --i) {
        auto* actor = Array[i];
        // 0x70D4D0 reads Techno::Airstrike (0x294), then Airstrike::Target (0x50).
        const bool keep =
            techno && actor->Airstrike && actor->Airstrike->Target == target && techno->Health > 0 && techno->IsAlive;
        if (actor->Target == target && !keep) {
            actor->Mission_Revert();
            if (actor->WhatAmI() == AbstractType::Aircraft && actor->CurrentMission == Mission::Patrol) {
                actor->MissionStatus = 0;
                static_cast<AircraftClass*>(actor)->IsLocked = false;
            }
            if (actor->Target == target) actor->SetTarget(nullptr);
        }
    }
    // 0x70D555 traverses TeamClass::Array, clearing both focus fields.
    for (int i = TeamClass::Array.Count - 1; i >= 0; --i) {
        auto* team = TeamClass::Array[i];
        if (team->QueuedFocus == target) team->QueuedFocus = nullptr;
        if (team->Focus == target) team->Focus = nullptr;
    }
}

bool TechnoClass::SetOwningHouse(HouseClass* house, bool) {
    if (house == Owner) return false;
    auto* old = Owner;
    auto* type = GetTechnoType();
    if (old == HouseClass::CurrentPlayer && IsSelected) Deselect();
    SetTarget(nullptr);
    SetDestination(nullptr, true);
    const auto* unit = WhatAmI() == AbstractType::Unit ? static_cast<UnitClass*>(this) : nullptr;
    if (!unit || !unit->IsDeployingOrUndeploying()) ArchiveTarget = nullptr;
    if (CurrentMission != Mission::Selling && (!unit || !unit->Type->Harvester || CurrentMission != Mission::Unload))
        QueueMission(Mission::Guard, true);
    if (SpawnManager) SpawnManager->ResetTarget();
    if (PlanningToken) ClearPlanningTokens(nullptr);
    if (!InLimbo) old->RegisterLoss(this, false);
    RegisterDestruction(nullptr);
    house->PointTotal += type->GetActualCost(old);
    old->RemoveTracking(this);
    house->AddTracking(this);
    if (!type->DontScore && old->ArrayIndex >= 0 && old->ArrayIndex < 20) {
        if (WhatAmI() == AbstractType::Building)
            ++house->KilledBuildingsOfHouses[old->ArrayIndex];
        else if (WhatAmI() == AbstractType::Infantry || WhatAmI() == AbstractType::Unit ||
                 WhatAmI() == AbstractType::Aircraft)
            ++house->KilledUnitsOfHouses[old->ArrayIndex];
    }
    // 0x0070164F reads the country selector at HouseTypeClass + 0xB8.
    old->WhoLastHurtMe = house->Type->ArrayIndex2;
    if (!InLimbo) Mark(MarkType::Up);
    RemoveFromTargeting();
    if (!InLimbo) Mark(MarkType::ChangeRedraw);
    CellClass* cell = nullptr;
    if (!InLimbo) {
        cell = (AbstractFlags & ::AbstractFlags::Foot) != ::AbstractFlags::None
                   ? MapClass::Instance.GetCellAt(static_cast<FootClass*>(this)->LastMapCoords)
                   : GetCell();
        if (cell) {
            cell->UpdateThreat(old->ArrayIndex, -int(ThreatPosed));
            ThreatPosed = 0;
        }
        vt_entry_48C(false, 0, false, nullptr);
    }
    Owner = house;
    game::techno_spatial_dirty(this);
    IsOwnedByCurrentPlayer = house == HouseClass::CurrentPlayer && HouseClass::CurrentPlayer;
    if (!InLimbo) {
        if (cell) AddThreatToCell(cell);
        house->RegisterGain(this, true);
    }
    // 0x007017A0 clears the stored original house at TechnoClass + 0x2E0.
    OriginallyOwnedByHouse = nullptr;
    if (CurrentMission == Mission::Rescue || QueuedMission == Mission::Rescue) ForceMission(Mission::Guard);
    if (!InLimbo) {
        const auto* building = WhatAmI() == AbstractType::Building ? static_cast<BuildingClass*>(this) : nullptr;
        auto* link = GetNthLink();
        if ((!building || CurrentMission != Mission::Unload || !building->Type->WeaponsFactory) &&
            (!link || link->WhatAmI() != AbstractType::Building ||
             !static_cast<BuildingClass*>(link)->Type->WeaponsFactory)) {
            SetDestination(nullptr, true);
            SetTarget(nullptr);
            EnterIdleMode(false, true);
        }
        if (IsRadarTracked) {
            RadarTrackingStop();
            RadarTrackingStart();
        }
        UpdateSight(0, 0, 0, 0, 0);
    }
    return true;
}
