// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9 building.cpp Mark/Unlimbo/Limbo; YR
// 0x43F180/0x440580/0x445880. Ordinary-building placement shared by map loading
// and runtime placement, including the original wall-to-overlay conversion.
#include "map_world.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/ParticleSystemClass.h"
#include <algorithm>
namespace {
void adjacency(const BuildingClass& building, int amount) {
    const auto at = building.GetMapCoords();
    const int width = building.Type->GetFoundationWidth(), height = building.Type->GetFoundationHeight(false);
    for (int y = -1; y <= height; ++y)
        for (int x = -1; x <= width; ++x)
            MapClass::Instance.GetCellAt(CellStruct{short(at.X + x), short(at.Y + y)})->BlockedNeighbours += amount;
}
void ownership_lists(BuildingClass& building, bool add) {
    auto& house = *building.Owner;
    auto& type = *building.Type;
    const auto update = [&](DynamicVectorClass<BuildingClass*>& list, bool matches) {
        if (add) {
            if (matches) list.AddItem(&building);
        } else
            list.Remove(&building);
    };
    update(house.ConYards, RulesClass::Instance->BuildConst.FindItemIndex(&type) >= 0);
    update(house.UnitRepairStations, type.UnitRepair);
    update(house.Grinders, type.Grinding);
    update(house.Absorbers, type.InfantryAbsorb || type.UnitAbsorb);
    update(house.Bunkers, type.Bunker);
    update(house.Occupiables, type.CanBeOccupied && type.TechLevel > -1);
    update(house.CloningVats, type.Cloning);
    update(house.SecretLabs, type.SecretLab);
    update(house.PsychicDetectionBuildings, type.PsychicDetectionRadius > 0);
    update(house.FactoryPlants, type.FactoryPlant);
    if (add) house.CalculateCostMultipliers();
    update(house.Buildings, true);
}
void clear_smudge(CellClass& cell) {
    auto* type = SmudgeTypeClass::Array.GetItemOrDefault(cell.SmudgeTypeIndex);
    if (!type || type->Width <= 0) return;
    const CellStruct origin{short(cell.MapCoords.X - cell.SmudgeData % type->Width),
                            short(cell.MapCoords.Y - cell.SmudgeData / type->Width)};
    for (int y = 0; y < type->Height; ++y)
        for (int x = 0; x < type->Width; ++x)
            if (auto* occupied =
                    MapClass::Instance.TryGetCellAt(CellStruct{short(origin.X + x), short(origin.Y + y)})) {
                occupied->MarkForRedraw();
                occupied->SmudgeTypeIndex = -1;
                game::map_resource_changed(*occupied);
            }
}
}  // namespace
// OpenTS Can_Enter_Cell; YR 0x00449440. The already-down mobile building
// checks one cell; ordinary placement uses the type's existing virtual slot.
Move BuildingClass::IsCellOccupied(CellClass* cell, FacingType, int, CellClass*, bool) const {
    if (!cell || !Type) return Move::No;
    const bool allowed =
        Type->UndeploysInto && IsOnMap
            ? MapClass::Instance.GetCellAt(cell->MapCoords)->CanThisExistHere(Type->SpeedType, Type, Owner)
            : Type->CanCreateHere(cell->MapCoords, Owner);
    return allowed ? Move::OK : Move::No;
}
bool BuildingClass::Mark(MarkType value) {
    if (!TechnoClass::Mark(value)) return false;
    auto at = GetMapCoords();
    // Original 0x43F180 redraws foundation cells on both placement and removal.
    for (auto* part = GetFoundationData(false); part && *part != CellStruct{0x7FFF, 0x7FFF}; ++part)
        if (auto* cell = MapClass::Instance.TryGetCellAt(CellStruct{short(at.X + part->X), short(at.Y + part->Y)}))
            cell->MarkForRedraw();
    if (value == MarkType::Up)
        MapClass::Instance.RemoveContentAt(&at, this);
    else if (value == MarkType::Down || value == MarkType::ChangeRedraw) {
        MapClass::Instance.AddContentAt(&at, this);
        // 0x43F180 snaps again after Place_Down has recalculated foundation cells.
        CoordStruct source{at.X * 256 + 128, at.Y * 256 + 128, 0}, adjusted;
        SetLocation(*Type->vt_entry_6C(&adjusted, &source));
        for (int y = 0; y < Type->GetFoundationHeight(false); ++y)
            for (int x = 0; x < Type->GetFoundationWidth(); ++x)
                if (auto* cell = MapClass::Instance.TryGetCellAt(CellStruct{short(at.X + x), short(at.Y + y)}))
                    clear_smudge(*cell);
    }
    return true;
}
bool BuildingClass::DiscoveredBy(HouseClass* house) {
    if (!TechnoClass::DiscoveredBy(house)) return false;
    if (!Unsorted::ScenarioInit) {
        Owner->LastBuiltBuildingType = Type->ArrayIndex;
        Owner->IsBuiltSomething = true;
    }
    Owner->RecheckTechTree = true;
    if (!HasAnyLink() && GetCurrentMission() != Mission::Construction &&
        (Owner->IsControlledByHuman() ? QueuedMission != Mission::Construction : house == Owner))
        Place(false);
    return true;
}
bool BuildingClass::Unlimbo(const CoordStruct& where, DirType facing) {
    if (!Type || !Owner) return false;
    if (*Type->PowersUpBuilding && !INIClass::IsBlankValue(Type->PowersUpBuilding)) {
        auto* building = MapClass::Instance.GetCellAt(where)->GetBuilding();
        if (!building || !building->CanUpgrade(Type, Owner)) return false;
        const int levels = Type->PowersUpToLevel == -1 ? 1 : Type->PowersUpToLevel;
        if (levels < 1 || levels > 3 || building->UpgradeLevel >= 3) return false;
        const int first = building->UpgradeLevel;
        auto& slot = building->Type->BuildingAnim[first];
        if (_strcmpi(slot.Anim, Type->ImageFile)) std::strncpy(slot.Anim, Type->ImageFile, sizeof(slot.Anim));
        for (int i = 0; i < levels; ++i)
            building->Upgrade();
        building->Upgrades[std::min(int(building->UpgradeLevel), 3) - 1] = Type;
        Owner->RecheckPower = Owner->RecheckRadar = Owner->RecheckTechTree = true;
        UnInit();
        return true;
    }
    if (Type->Wall) {
        auto& map = MapClass::Instance;
        auto* cell = map.GetCellAt(where);
        // 0x440606: a legal replacement first removes the owner's existing wall.
        if (IsCellOccupied(cell, FacingType::None, -1, nullptr, true) == Move::OK && Type->ToOverlay &&
            cell->WallOwnerIndex == Owner->ArrayIndex && cell->OverlayTypeIndex == Type->ToOverlay->ArrayIndex) {
            auto at = cell->MapCoords;
            Owner->SellWall(at, true);
        }
        // 0x44077A..0x44086E runs before Techno::Unlimbo and building registration.
        if (IsCellOccupied(cell, FacingType::None, -1, nullptr, true) != Move::OK) return false;
        // A missing ART dependency cannot be dereferenced in the native host.
        if (!Type->ToOverlay) return false;
        auto* overlay = Type->ToOverlay->CreateObject(Owner);
        if (!overlay) return false;
        if (!overlay->Unlimbo(where, DirType::North)) {
            // Mark can already have queued deletion; terrain rejection has not.
            if (overlay->IsAlive) overlay->UnInit();
            return false;
        }
        cell->WallOwnerIndex = Owner->ArrayIndex;
        SendToFirstLink(RadioCommand::NotifyUnlink);
        auto at = where;
        map.RevealArea2(&at, Type->Sight, Owner, 0, 0, 0, 1, 0);
        map.RevealArea2(&at, Type->Sight, Owner, 0, 0, 0, 1, 1);
        UnInit();
        return true;
    }
    if (!TechnoClass::Unlimbo(where, facing)) return false;
    if (!IsAlive) return true;
    if ((!Type->UndeploysInto || !Type->ResourceGatherer) && !IsStrange()) UpdateBaseSpacing(false);
    adjacency(*this, 1);
    ownership_lists(*this, true);
    Owner->RecalculateBaseCenter();
    Owner->AddFactory(this);
    Owner->RecheckPower = true;
    IsPrimaryFactory = false;
    if (Type->Factory != AbstractType::None) SetPrimary(false);
    try {
        if (Type->LightIntensity && !LightSource) {
            LightSource = new LightSourceClass(GetCenterCoords(), Type->LightVisibility, Type->LightIntensity,
                                               {Type->LightRedTint, Type->LightGreenTint, Type->LightBlueTint});
            LightSource->Activate();
        }
        if (Type->HasSpotlight && !Spotlight) Spotlight = new BuildingLightClass(this);
    } catch (...) { /* Optional visual allocation does not undo valid placement. */
    }
    // 0x00440E76 dispatches the original discovery slot for AI/passive owners.
    if (!Owner->IsControlledByHuman()) DiscoveredBy(Owner);
    if (Type->NaturalParticleSystem && !NaturalParticleSystem)
        if (auto* memory = YRMemory::Allocate(sizeof(ParticleSystemClass)))
            NaturalParticleSystem = ::new (memory)
                ParticleSystemClass(Type->NaturalParticleSystem, Location + Type->NaturalParticleSystemLocation,
                                    MapClass::Instance.GetCellAt(Location), nullptr, CoordStruct::Empty, nullptr);
    return true;
}
bool BuildingClass::Limbo() {
    if (!InLimbo && Type) {
        if (auto* tactical = TacticalClass::Instance) {
            RectangleStruct area;
            GetRenderDimensions(&area);
            tactical->RegisterDirtyArea(area, false);
        }
        // 0x445880 updates the building counters before Techno::Limbo removes
        // ownership and cell occupation. ActuallyPlacedOnMap is not reset here.
        if (ActuallyPlacedOnMap) {
            if (Type->OrePurifier) Owner->NumOrePurifiers = std::max(0, Owner->NumOrePurifiers - 1);
            if (Type->Helipad) Owner->AirportDocks = std::max(0, Owner->AirportDocks - Type->NumberOfDocks);
            Owner->InfantrySelfHeal = std::max(0, Owner->InfantrySelfHeal - Type->InfantryGainSelfHeal);
            Owner->UnitsSelfHeal = std::max(0, Owner->UnitsSelfHeal - Type->UnitsGainSelfHeal);
        }
        if ((!Type->UndeploysInto || !Type->ResourceGatherer) && !IsStrange()) RemoveBaseSpacing();
        if (!Type->ToTile) adjacency(*this, -1);
        Owner->RemoveFactory(this);
        ownership_lists(*this, false);
        Owner->RecheckPower = Owner->RecheckTechTree = true;
        delete Spotlight;
        Spotlight = nullptr;
        delete LightSource;
        LightSource = nullptr;
    }
    if (!InLimbo && Type) {
        Owner->RecalculateBaseCenter();
        Owner->InvalidateBaseNodePosition(this);
    }
    const bool result = TechnoClass::Limbo();
    if (result) UpdateDamageFires();
    return result;
}
