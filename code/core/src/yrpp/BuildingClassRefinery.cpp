// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Grand_Opening, calibrated to YR 0x445F80.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/ParticleSystemClass.h"
#include <algorithm>

bool BuildingClass::IsCellImpassable(CellClass* cell) const {
    if (!cell || cell->GetBuilding() != this) return false;
    return Type->NumberImpassableRows == -1 || (Type->Bunker && BunkerLinkedItem) ||
           GetMapCoords().X + Type->NumberImpassableRows > cell->MapCoords.X;
}

void BuildingClass::Place(bool captured) {
    if (ActuallyPlacedOnMap && !captured) return;
    if (Type->ProduceCashDelay) CashProductionTimer.Start(Type->ProduceCashDelay);
    if (!ActuallyPlacedOnMap) {
        if (Type->OrePurifier) ++Owner->NumOrePurifiers;
        Owner->InfantrySelfHeal += Type->InfantryGainSelfHeal;
        Owner->UnitsSelfHeal += Type->UnitsGainSelfHeal;
        if (Type->Helipad) Owner->AirportDocks += Type->NumberOfDocks;
        if (Type->Refinery) {
            StorageFilledSlots = Type->Storage ? int(Tiberium.GetTotalAmount()) * 4 / Type->Storage : 0;
            PlayNthAnim(static_cast<BuildingAnimSlot>(3 + std::clamp(int(StorageFilledSlots), 0, 3)), !IsGreenHP(),
                        false);
        } else
            for (int slot : {18, 3, 4, 5, 6})
                PlayNthAnim(static_cast<BuildingAnimSlot>(slot), !IsGreenHP(), GetOccupantCount() > 0);
        // 0x4463F1..0x446579: the initial super animation uses the owner's timer.
        if (Type->SuperWeapon != -1)
            for (auto* super : Owner->Supers) {
                if (int(super->Type->Type) != Type->SuperWeapon) continue;
                const int seconds = super->RechargeTimer.GetTimeLeft() / 15;
                const int scaled = std::bit_cast<int>(unsigned(seconds) * 60u);
                PlayNthAnim(static_cast<BuildingAnimSlot>(scaled > 4 ? 14 : 16), !IsGreenHP(), GetOccupantCount() > 0);
            }
        if (HasTurret() && !Type->TurretAnimIsVoxel) PlayNthAnim(BuildingAnimSlot::Turret, !IsGreenHP(), false);
        IsReadyToCommence = true;
    }
    ActuallyPlacedOnMap = true;
    Owner->RecheckPower = Owner->RecheckRadar = Owner->RecheckTechTree = true;
    if (captured || Unsorted::ScenarioInit || Unsorted::ArmageddonMode || !Type->FreeUnit ||
        (Owner->IsControlledByHuman() && Value && Value <= Type->GetCost()))
        return;
    // Grant once at construction completion. Scenario loading and capture have
    // already been excluded; repeated activation cannot create more miners.
    // 0x446B68..0x446B97 branches on allocation failure; 0x446EB9..0x446EDD
    // refunds the unit's Soylent. This allocation must remain recoverable.
    void* storage = YRMemory::Allocate(sizeof(UnitClass));
    UnitClass* unit = storage ? ::new (storage) UnitClass(Type->FreeUnit, Owner) : nullptr;
    if (!unit) {
        Owner->GiveMoney(Type->FreeUnit->GetRefund(Owner, true));
        return;
    }
    if (!unit->InitializeLocomotor()) {
        Owner->GiveMoney(Type->FreeUnit->GetRefund(Owner, true));
        GameDelete(unit);
        return;
    }
    auto& map = MapClass::Instance;
    const auto center = CellClass::Coord2Cell(GetCoords());
    const auto delta = Unsorted::AdjacentCell[static_cast<int>(FacingType::South)];
    const CellStruct first{short(center.X + delta.X), short(center.Y + delta.Y)};
    const auto place = [&](CellStruct at, DirType facing) {
        auto* cell = map.TryGetCellAt(at);
        return cell && map.IsWithinUsableArea(at, true) && unit->Unlimbo(cell->GetCoords(), facing);
    };
    bool placed = place(first, DirType::West);
    for (bool requireEmpty : {true, false})
        if (!placed) {
            const int zone = map.GetMovementZoneType(GetMapCoords(), unit->Type->MovementZone, false);
            const auto at = map.NearByLocation(GetMapCoords(), SpeedType::Wheel, zone, unit->Type->MovementZone, false,
                                               1, 1, requireEmpty, true, false, false, CellStruct::Empty, false, false);
            if (at != CellStruct::Empty) placed = place(at, DirType::SouthWest);
        }
    if (!placed) {
        Owner->GiveMoney(Type->FreeUnit->GetRefund(Owner, true));
        GameDelete(unit);
        return;
    }
    unit->QueueMission(Mission::Harvest, false);
    unit->NextMission();
    unit->UpdateTimer.Start(0);
    if (LogicClass::Instance.FindItemIndex(unit) < 0) LogicClass::Instance.AddObject(unit, false);
}

// YR adds this hook to the OpenTS harvester unloading sequence (0x73E37E).
void BuildingClass::UpdateRefinerySmokeSystems() {
    if (!Type->RefinerySmokeParticleSystem) return;
    for (const auto offset : {Type->RefinerySmokeOffsetOne, Type->RefinerySmokeOffsetTwo,
                              Type->RefinerySmokeOffsetThree, Type->RefinerySmokeOffsetFour}) {
        // Original skips both empty coordinates and cell-centre sentinel.
        if (offset == CoordStruct::Empty || offset == CoordStruct{128, 128, 0}) continue;
        auto* system = GameCreate<ParticleSystemClass>(Type->RefinerySmokeParticleSystem, Location + offset, nullptr,
                                                       this, CoordStruct::Empty, nullptr);
        if (system) system->Lifetime = Type->RefinerySmokeFrames;  // 0x006301F0
    }
}
