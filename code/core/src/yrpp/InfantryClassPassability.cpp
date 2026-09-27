// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp::Can_Enter_Cell; calibrated to YR 0x0051BF90.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include <bit>
#include <cstdlib>

Move InfantryClass::IsCellOccupied(CellClass* cell, FacingType facing, int level,
    CellClass* source, bool) const {
    const auto delta = [](int a,int b) { return std::bit_cast<int>(static_cast<unsigned>(a)-static_cast<unsigned>(b)); };
    const int ground = static_cast<signed char>(cell->Level);
    bool bridge = (static_cast<unsigned>(cell->Flags) & 0x100u)
        && (level == -1 || std::abs(static_cast<long long>(delta(level,ground))) > 1);
    int infantry_owner = cell->InfantryOwnerIndex;
    unsigned char occupation = static_cast<unsigned char>(cell->OccupationFlags);
    bool vehicle = (cell->OccupationFlags & 0x20u) != 0;

    auto* tunnel = cell->GetTunnel();
    if (static_cast<int>(facing) == 8)
        return tunnel && tunnel->EnterCell != tunnel->ExitCell ? Move::OK : Move::No;
    if (tunnel) {
        const auto angle = std::abs(static_cast<long long>(delta(static_cast<int>(facing),tunnel->ExitFace)));
        if (angle > 2 && angle < 6 && facing != FacingType::None) return Move::No;
    }
    const auto opposite = static_cast<FacingType>((static_cast<unsigned>(facing)-4u)&7u);
    if (auto* behind = cell->GetNeighbourCell(opposite)->GetTunnel()) {
        const auto angle = std::abs(static_cast<long long>(delta(static_cast<int>(opposite),behind->ExitFace)));
        if (angle > 2 && angle < 6 && facing != FacingType::None) return Move::No;
    }
    if (delta(level,ground) > 4) return Move::OK;
    if (CanReachCell(cell,facing,level,bridge,source) == Move::No) return Move::No;
    if (level != -1 && (static_cast<unsigned>(cell->Flags)&0x100u) && level == ground+4) {
        infantry_owner = cell->AltInfantryOwnerIndex;
        occupation = static_cast<unsigned char>(cell->AltOccupationFlags);
        vehicle = (cell->AltOccupationFlags&0x20u) != 0;
    }
    if (IsInPlayfield && !Unsorted::ScenarioInit && !MapClass::Instance.IsWithinUsableArea(cell,true)
        && !vt_entry_320()) return Move::No;

    Move result = Move::OK;
    const auto promote = [&](Move obstruction) { if (result < obstruction) result = obstruction; };
    if (cell->OverlayTypeIndex != -1) {
        auto* overlay = OverlayTypeClass::Array[cell->OverlayTypeIndex];
        if (overlay->Crate && !Owner->IsControlledByHuman()) return Move::No;
        if (overlay->Wall && (cell->OverlayData >> 4) != overlay->DamageLevels) {
            if (!IsArmed() || !GetWeapon(0)->WeaponType->IsWallDestroyer()) return Move::No;
            result = Owner->IsAlliedWith(cell->WallOwnerIndex) ? Move::FriendlyDestroyable : Move::Destroyable;
        }
    }
    const auto impassable_ground = [&] {
        return !IsTether && !bridge && GroundType::Array[static_cast<int>(cell->LandType)].Cost[static_cast<int>(Type->SpeedType)] == 0.0f;
    };
    const auto driver = [](FootClass* foot) -> ILocomotion* {
        ILocomotion* value = foot->Locomotor;
        if (!value) std::abort(); // Original _com_issue_error(E_POINTER); no exception crosses the native core.
        return value;
    };
    int standing_infantry = 0;
    for (auto* object = bridge ? cell->AltObject : cell->FirstObject; object; object = object->NextObject) {
        if (object == this) continue;
        if (ParasiteImUsing && ParasiteImUsing->Victim
            && ParasiteImUsing->Victim->GetMapCoords() == object->GetMapCoords()) return Move::OK;
        if (SlaveOwner && SlaveOwner->SlaveManager && object == SlaveOwner
            && SlaveOwner->SlaveManager->IsSlaveAtCell(const_cast<InfantryClass*>(this),cell)) {
            vehicle = false;
            continue;
        }
        const auto mission = GetCurrentMission();
        if (!object->IsBeingWarpedOut() && (mission == Mission::Enter || mission == Mission::Capture
            || mission == Mission::Eaten || (mission == Mission::Sabotage && Type->C4)
            || ((mission == Mission::Area_Guard || mission == Mission::Patrol || mission == Mission::Guard) && Type->Engineer))) {
            if (object == Destination || MapClass::Instance.GetCellAt(object->Location) == Destination || object == Target) {
                if (object->IsIronCurtained() || impassable_ground()) return Move::No;
                return Move::OK;
            }
            if (auto* building = cell->GetBuilding(); building && object != building) {
                if (building == Destination || MapClass::Instance.GetCellAt(building->GetCoords()) == Destination
                    || building == Target) continue;
            }
        }
        if (mission == Mission::Area_Guard && ArchiveTarget == object && object->WhatAmI() == AbstractType::Unit)
            return Move::No;
        if (Type->VehicleThief && object->IsStrange() && Destination == object && !object->GetTechnoType()->IsTrain)
            return Move::OK;
        const auto kind = object->WhatAmI();
        if (kind == AbstractType::Building) {
            auto* building = static_cast<BuildingClass*>(object);
            const auto* type = building->Type;
            if (type->InvisibleInGame || (type->LaserFence && (building->LaserFenceFrame == 12 || building->LaserFenceFrame == 8)))
                continue;
            if (type->FirestormWall) {
                if (building->Owner->FirestormActive) return Move::No;
                continue;
            }
            if (type->Gate) {
                if (!building->IsTraversable()) {
                    if (building->Owner->IsAlliedWith(Owner)) promote(Move::ClosedGate);
                    else { if (!IsArmed()) return Move::No; promote(Move::Destroyable); }
                }
                continue;
            }
        }
        if (mission == Mission::Enter && object == Destination && IsTether) return Move::OK;
        if (Owner->IsAlliedWith(object) || Unsorted::ScenarioInit) {
            switch (kind) {
                case AbstractType::Unit: {
                    auto* unit = static_cast<FootClass*>(object);
                    if (driver(unit)->Is_Moving() || unit->Destination) {
                        if (unit->FrozenStill || driver(unit)->Will_Jump_Tracks()) promote(Move::MovingBlock);
                    } else promote(Move::Temp);
                    break;
                }
                case AbstractType::Aircraft:
                case AbstractType::Building: return Move::No;
                case AbstractType::Infantry:
                    if (!driver(static_cast<FootClass*>(object))->Is_Moving()) ++standing_infantry;
                    break;
                default: break;
            }
        } else {
            const auto* techno = (static_cast<unsigned>(object->AbstractFlags)&1u) ? static_cast<TechnoClass*>(object) : nullptr;
            if (techno && techno->CloakState == CloakState::Cloaked) promote(Move::Cloak);
            else {
                if (CombatDamage(-1) <= 0 && kind != AbstractType::Terrain) return Move::No;
                if (kind == AbstractType::Building) {
                    if (static_cast<BuildingClass*>(object)->Type->BridgeRepairHut) return Move::No;
                    promote(Move::Destroyable);
                } else if (kind == AbstractType::Infantry) {
                    if (object->IsDisguisedAs(Owner)) result = Move::Temp;
                } else if (kind != AbstractType::Terrain) promote(Move::Destroyable);
            }
        }
    }
    if (impassable_ground()) return Move::No;
    if (result == Move::OK && vehicle) return Move::MovingBlock;
    if (infantry_owner != -1) {
        if (Owner->IsAlliedWith(infantry_owner)) {
            if ((occupation&0x1Cu) == 0x1Cu && result < Move::MovingBlock)
                return standing_infantry != 3 ? Move::MovingBlock : Move::Temp;
        } else {
            if (CombatDamage(-1) <= 0) return Move::No;
            if (result < Move::Destroyable) return Move::Destroyable;
        }
    }
    return result != Move::OK ? result : (occupation&0x1Cu) == 0x1Cu ? Move::No : Move::OK;
}
