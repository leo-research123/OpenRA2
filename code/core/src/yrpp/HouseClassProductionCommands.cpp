// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 house.cpp Begin/Suspend/Abandon_Production, Place_Object.
// YR 0x4FA350 / 0x4FA910 / 0x4FAA10 / 0x4FB0E0 / 0x4FB6B0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "type_resources.hpp"
#include "map_world.hpp"

void HouseClass::Update_FactoriesQueues(AbstractType type, bool naval, BuildCat category) const {
    switch (type) {
        case AbstractType::AircraftType:
        case AbstractType::BuildingType:
        case AbstractType::InfantryType:
        case AbstractType::UnitType:
            break;
        default:
            return;
    }
    auto* factory = GetPrimaryFactory(type, naval, category);
    if (!factory) return;
    for (int i = factory->QueuedObjects.Count - 1; i >= 0; --i)
        if (!factory->QueuedObjects[i]->FindFactory(true, false, true, this)) factory->QueuedObjects.RemoveItem(i);
    if (auto* object = factory->Object) {
        if (auto* product = object->GetTechnoType(); product->FindFactory(true, false, true, this)) {
            if (factory->Object->GetTechnoType()->FindFactory(true, true, true, this)) {
                if (factory->IsSuspended && !factory->IsManual) factory->Unsuspend(false);
            } else {
                factory->Suspend(false);
                if (this == CurrentPlayer) {
                    // Original 0x006ABCD0 maps this factory category to a tab.
                    const int tab = type == AbstractType::BuildingType   ? (category == BuildCat::Combat ? 1 : 0)
                                    : type == AbstractType::InfantryType ? 2
                                                                         : 3;
                    SidebarClass::Instance.RepaintSidebar(tab);
                }
            }
        } else {
            factory->AbandonProduction();
            factory->StartProduction();
        }
    }
    if (!factory->Object && !factory->QueuedObjects.Count) GameDelete(factory);
}

void HouseClass::SetPrimaryFactory(FactoryClass* factory, AbstractType type, bool naval, BuildCat category) {
    switch (type) {
        case AbstractType::Infantry:
        case AbstractType::InfantryType:
            Primary_ForInfantry = factory;
            break;
        case AbstractType::Unit:
        case AbstractType::UnitType:
            (naval ? Primary_ForShips : Primary_ForVehicles) = factory;
            break;
        case AbstractType::Aircraft:
        case AbstractType::AircraftType:
            Primary_ForAircraft = factory;
            break;
        case AbstractType::Building:
        case AbstractType::BuildingType:
            (category == BuildCat::Combat ? Primary_ForDefenses : Primary_ForBuildings) = factory;
            break;
        default:
            break;
    }
}
int HouseClass::BeginProduction(AbstractType type, int index, bool naval, bool resume) {
    auto* product = TechnoTypeClass::GetByTypeAndIndex(type, index);
    if (!product) return 3;
    if (product->WhatAmI() == AbstractType::BuildingType) {
        const auto* building = static_cast<BuildingTypeClass*>(product);
        // Native scope guard: reject unsupported placement protocols before payment.
        if (building->ToTile || building->PowersUpBuilding[0] || building->Gate || building->LaserFence ||
            building->LaserFencePost || building->FirestormWall)
            return 3;
    }
    bool hold = false;
    if (!product->FindFactory(false, true, true, this)) {
        if (!resume || !product->FindFactory(true, false, true, this)) return 3;
        hold = true;
    }
    const auto category = product->WhatAmI() == AbstractType::BuildingType
                              ? static_cast<BuildingTypeClass*>(product)->BuildCat
                              : BuildCat(0);
    auto* factory = GetPrimaryFactory(type, naval, category);
    if (!factory) {
        factory = GameCreate<FactoryClass>();
        if (!factory) return 3;
    }
    if (factory->Production.Rate && !factory->IsSuspended && type == AbstractType::BuildingType) return 3;
    SetPrimaryFactory(factory, type, naval, category);
    const bool held = factory->IsSuspended && factory->Object && factory->Object->GetTechnoType() == product;
    if (held || factory->DemandProduction(product, this, resume)) {
        if (!factory->QueuedObjects.Count || resume || held) factory->Unsuspend(hold);
        if (this == CurrentPlayer) SidebarClass::Instance.LinkFactory(factory, type, index);
        return 0;
    }
    if (!factory->QueuedObjects.Count && !factory->Object) {
        SetPrimaryFactory(nullptr, type, naval, category);
        GameDelete(factory);
    }
    return 3;
}
int HouseClass::SuspendProduction(AbstractType type, int index, bool naval) {
    auto* product = TechnoTypeClass::GetByTypeAndIndex(type, index);
    const auto category = product && product->WhatAmI() == AbstractType::BuildingType
                              ? static_cast<BuildingTypeClass*>(product)->BuildCat
                              : BuildCat(0);
    auto* factory = GetPrimaryFactory(type, naval, category);
    if (!factory) return 3;
    factory->Suspend(true);
    if (this == CurrentPlayer) SidebarClass::Instance.RepaintSidebar(SidebarClass::GetObjectTabIdx(type, index, 0));
    return 0;
}
void HouseClass::AbandonProduction(AbstractType type, int index, bool naval, bool all) {
    auto* product = TechnoTypeClass::GetByTypeAndIndex(type, index);
    const auto category = product && product->WhatAmI() == AbstractType::BuildingType
                              ? static_cast<BuildingTypeClass*>(product)->BuildCat
                              : BuildCat(0);
    auto* factory = GetPrimaryFactory(type, naval, category);
    if (!factory) return;
    if (factory->QueuedObjects.Count && index >= 0 && category == BuildCat(0)) {
        const bool removed = factory->RemoveOneFromQueue(product);
        if (all)
            while (factory->RemoveOneFromQueue(product)) {
            }
        if (removed && this == CurrentPlayer)
            SidebarClass::Instance.RepaintSidebar(SidebarClass::GetObjectTabIdx(type, index, 0));
        if (removed && !all) return;
    }
    // A nonzero building category permits the completed (empty) factory,
    // but an existing product must still match the requested index.
    if (index != -1 &&
        (factory->Object ? factory->Object->GetTechnoType()->GetArrayIndex() != index : category == BuildCat(0)))
        return;
    if (this == CurrentPlayer) SidebarClass::UnlinkFactory(type, index, factory);
    if ((type == AbstractType::Building || type == AbstractType::BuildingType) && this == CurrentPlayer) {
        auto& display = DisplayClass::Instance;
        display.CurrentBuilding = nullptr;
        display.CurrentBuildingType = nullptr;
        display.CurrentBuildingOwnerArrayIndex = -1;
        display.SetActiveFoundation(nullptr);
    }
    factory->AbandonProduction();
    if (factory->QueuedObjects.Count) {
        factory->StartProduction();
        return;
    }
    SetPrimaryFactory(nullptr, type, naval, category);
    GameDelete(factory);
}
bool HouseClass::PlaceObject(AbstractType type, int index, bool naval, const CellStruct& cell) {
    auto* requested = TechnoTypeClass::GetByTypeAndIndex(type, index);
    const auto category = requested && requested->WhatAmI() == AbstractType::BuildingType
                              ? static_cast<BuildingTypeClass*>(requested)->BuildCat
                              : BuildCat(0);
    auto* factory = GetPrimaryFactory(type, naval, category);
    if (!factory || !factory->IsDone() || !factory->Object) return false;
    auto* product = factory->Object;
    if (cell != CellStruct::Empty && cell != CellStruct{-1, -1}) {
        if (product->WhatAmI() != AbstractType::Building) return false;
        auto* building = static_cast<BuildingClass*>(product);
        // These use other placement protocols; they are not ordinary foundations.
        if (building->Type->ToTile || building->Type->PowersUpBuilding[0]) return false;
        auto* builder = product->FindFactory(false, false);
        if (!builder) return false;
        builder->SendCommand(RadioCommand::RequestLink, product);
        const bool placed = product->Unlimbo({cell.X * 256 + 128, cell.Y * 256 + 128, 0}, DirType::North);
        auto& display = DisplayClass::Instance;
        if (placed) {
            // 0x4FB27A: gap filling follows the first segment's Unlimbo and
            // precedes factory completion. The temporary product is deferred.
            if (building->Type->ToOverlay && building->Type->ToOverlay->Wall)
                MapClass::Instance.BuildingToWall(cell, this, building->Type);
            factory->CompletedProduction();
            product->SendCommand(RadioCommand::RequestEndProduction, builder);
            AbandonProduction(type, index, naval, false);
            if (this == CurrentPlayer) {
                if (!game::type_resources().audio_unavailable)
                    VocClass::PlayGlobal(RulesClass::Instance->BuildingSlam, 0x2000, 1.0f, nullptr);
                if (!Unsorted::ArmageddonMode) {
                    display.SetActiveFoundationCopy(nullptr);
                    display.CurrentBuildingCopy = nullptr;
                    display.CurrentBuildingTypeCopy = nullptr;
                }
                display.SetActiveFoundation(nullptr);
                display.CurrentBuilding = nullptr;
                display.CurrentBuildingType = nullptr;
                display.CurrentBuildingOwnerArrayIndex = -1;
            }
        } else if (this == CurrentPlayer) {
            if (!game::type_resources().audio_unavailable) VoxClass::Play("EVA_CannotDeployHere", -1, -1);
            if (!Unsorted::ArmageddonMode) {
                display.CurrentBuilding = display.CurrentBuildingCopy;
                display.CurrentBuildingType = display.CurrentBuildingTypeCopy;
                display.CurrentBuildingOwnerArrayIndex = display.CurrentBuildingOwnerArrayIndexCopy;
                display.CurrentFoundation_CenterCell = display.CurrentFoundationCopy_CenterCell;
                display.CurrentFoundation_TopLeftOffset = display.CurrentFoundationCopy_TopLeftOffset;
                display.CurrentFoundation_Data = display.CurrentFoundationCopy_Data;
                if (display.CurrentBuildingType) {
                    display.SetActiveFoundation(display.CurrentBuildingType->GetFoundationData(true));
                    // 0x4FB0E0 resolves the pointer again when a queued placement fails.
                    CellStruct current, previous;
                    if (game::current_map_pointer_cell(current)) display.MoveFoundation(&previous, &current);
                }
                display.SetActiveFoundationCopy(nullptr);
                display.CurrentBuildingCopy = nullptr;
                display.CurrentBuildingTypeCopy = nullptr;
            }
        }
        builder->SendToFirstLink(RadioCommand::NotifyUnlink);
        if (placed) {
            JustBuilt(product);
            if (!game::type_resources().audio_unavailable && builder->Type->CreateUnitSound != -1)
                VocClass::PlayIndexAtPos(builder->Type->CreateUnitSound, product->GetCoords(), 0);
        }
        return placed;
    }
    // 0x4FB532 resolves the exit through the product's original virtual chain.
    auto* building = product->FindFactory(false, true);
    if (!building && product->WhatAmI() == AbstractType::Aircraft) building = product->FindFactory(true, true);
    if (!building) return false;
    const auto result = building->KickOutUnit(product, CellStruct::Empty);
    if (result != KickOutResult::Succeeded && !(result == KickOutResult::Busy && building->Factory)) {
        AbandonProduction(type, index, naval, false);
        return false;
    }
    if (IsControlledByCurrentPlayer() && !game::type_resources().audio_unavailable) VoxClass::Play("EVA_UnitReady");
    factory->CompletedProduction();
    AbandonProduction(type, index, naval, false);
    JustBuilt(product);
    if (auto* builder = product->FindFactory(false, true))
        if (!game::type_resources().audio_unavailable && builder->Type->CreateUnitSound != -1)
            VocClass::PlayIndexAtPos(builder->Type->CreateUnitSound, product->GetCoords(), 0);
    return true;
}
void HouseClass::JustBuilt(TechnoClass* object) {
    IsBuiltSomething = true;
    auto* type = object->GetTechnoType();
    const int index = type->GetArrayIndex();
    int sound = -1;
    switch (object->WhatAmI()) {
        case AbstractType::Infantry:
            LastBuiltInfantryType = index;
            if (!type->DontScore) FactoryProducedInfantryTypes.Increment(index);
            sound = RulesClass::Instance->CreateInfantrySound;
            break;
        // The original header names at 0x274/0x278 are reversed; preserve layout.
        case AbstractType::Unit:
            LastBuiltAircraftType = index;
            if (!type->DontScore) FactoryProducedUnitTypes.Increment(index);
            sound = RulesClass::Instance->CreateUnitSound;
            break;
        case AbstractType::Aircraft:
            LastBuiltVehicleType = index;
            if (!type->DontScore) FactoryProducedAircraftTypes.Increment(index);
            sound = RulesClass::Instance->CreateAircraftSound;
            break;
        case AbstractType::Building:
            LastBuiltBuildingType = index;
            if (!type->DontScore) FactoryProducedBuildingTypes.Increment(index);
            break;
        default:
            break;
    }
    if (type->CreateSound != -1) sound = type->CreateSound;
    if (sound != -1 && !game::type_resources().audio_unavailable) VocClass::PlayAt(sound, object->Location, nullptr);
    RecheckTechTree = true;
}

bool HouseClass::EnterPlacement(BuildingClass* builder, BuildingClass* product) {
    auto& display = DisplayClass::Instance;
    if (this != CurrentPlayer || display.CurrentBuildingType || !builder || !product) return false;
    if (product->Type->ToTile || product->Type->PowersUpBuilding[0]) return false;
    MapClass::UnselectAll();
    display.SetRepairMode(0);
    display.SetSellMode(0);
    display.PowerToggleMode = display.PlanningMode = display.PlaceBeaconMode = false;
    display.CurrentSWTypeIndex = -1;
    display.CurrentBuilding = product;
    display.CurrentBuildingType = product->Type;
    display.CurrentBuildingOwnerArrayIndex = ArrayIndex;
    display.SetActiveFoundation(product->GetFoundationData(true));
    CellStruct previous;
    const auto at = builder->GetMapCoords();
    display.MoveFoundation(&previous, &at);
    builder->Mark(MarkType::Change);
    return true;
}
