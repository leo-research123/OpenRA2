// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp Try_To_Deploy; YR 0x7393C0 ordinary MCV branch.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "techno_spatial.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/AnimClass.h"
#include "yrpp/MapClass.h"
#include "type_resources.hpp"
#include "x87_integer.hpp"
#include <algorithm>
#include <bit>

// YR 0x739AC0: InAir requests landing; it is not an altitude predicate.
void UnitClass::Deploy() {
    if (!Type->IsSimpleDeployer) return;
    if (GetHeight() > 0 && !GetTechnoType()->DeployToLand) return;
    if (Deployed) return;
    if (!InAir && GetHeight() > 0) InAir = true;
    if (Deploying && DeployAnim) {
        const int end = std::bit_cast<int>(unsigned(DeployAnim->Type->Start) + unsigned(DeployAnim->Type->End) - 1u);
        if (Animation.Value >= end) {
            Deployed = true;
            Deploying = false;
        }
    } else if (!InAir) {
        if (Type->DeployingAnim) {
            if (!DeployAnim) {
                // Allocation failure terminates on this side of the host boundary.
                DeployAnim = GameCreate<AnimClass>(Type->DeployingAnim, Location, 0, 1, 0x600, 0, false);
                DeployAnim->SetOwnerObject(this);
                DeployAnim->LightConvert = GetRemapColour();
            }
            Animation.Value = DeployAnim->Type->Start;
            Animation.Start(DeployAnim->Type->Rate);
            Deploying = true;
        } else
            Deployed = true;
    }
    if (GetTechnoType()->DeploySound != -1) VocClass::PlayAt(GetTechnoType()->DeploySound, Location, nullptr);
}

// YR 0x739CD0: the unit timer advances forwards while the attached anim reverses.
void UnitClass::Undeploy() {
    if (!Deployed) return;
    if (Undeploying && DeployAnim) {
        const int end = std::bit_cast<int>(unsigned(DeployAnim->Type->Start) + unsigned(DeployAnim->Type->End) - 2u);
        if (Animation.Value >= end) {
            Deployed = false;
            Undeploying = false;
            if (GetTechnoType()->DeployToLand) {
                CellStruct nearby;
                NearbyLocation(&nearby, this);
                SetDestination(MapClass::Instance.GetCellAt(nearby), true);
            }
        }
    } else if (Type->DeployingAnim) {
        if (!DeployAnim) {
            DeployAnim = GameCreate<AnimClass>(Type->DeployingAnim, Location, 0, 1, 0x600, 0, true);
            DeployAnim->SetOwnerObject(this);
            DeployAnim->LightConvert = GetRemapColour();
        }
        Animation.Value = DeployAnim->Type->Start;
        Animation.Start(DeployAnim->Type->Rate);
        Undeploying = true;
    } else
        Deployed = false;
    if (IsDisguised()) ClearDisguise();
    if (GetTechnoType()->UndeploySound != -1) VocClass::PlayAt(GetTechnoType()->UndeploySound, Location, nullptr);
}

bool UnitClass::TryToDeploy() {
    if (!CanDeploySlashUnload()) {
        IsDeploying = false;
        return false;
    }
    if (Destination || !Locomotor || Locomotor->Is_Moving() || !Type->DeploysInto) return false;
    auto* buildingType = Type->DeploysInto;
    Mark(MarkType::Up);
    Locomotor->Mark_All_Occupation_Bits(MarkType::Up);
    auto cell = GetMapCoords();
    if (buildingType->GetFoundationWidth() > 2 || buildingType->GetFoundationHeight(false) > 2) {
        --cell.X;
        --cell.Y;
    }
    const bool allowed = buildingType->CanCreateHere(cell, nullptr);
    if (!allowed) {
        if (Owner->IsControlledByCurrentPlayer() && !Type->DeployToFire && !game::type_resources().audio_unavailable)
            VoxClass::Play("EVA_CannotDeployHere", -1, -1);
        Locomotor->Mark_All_Occupation_Bits(MarkType::Down);
        Mark(MarkType::Down);
        IsDeploying = false;
        QueueMission(Mission::Guard, false);
        return false;
    }
    Locomotor->Mark_All_Occupation_Bits(MarkType::Down);
    Mark(MarkType::Down);
    // 0x465D70 is a getter for DeployFacing (already in 256-direction units).
    const int facing = buildingType->DeployFacing;
    if (PrimaryFacing.Current().GetValue<8>() != facing) {
        if (!Locomotor->Is_Moving_Now()) Locomotor->Do_Turn(DirStruct(short((facing & 0xFF) << 8)));
        SendToFirstLink(RadioCommand::NotifyUnlink);
        IsDeploying = true;
        return true;
    }
    const bool selected = IsSelected;
    Mark(MarkType::Up);
    Locomotor->Mark_All_Occupation_Bits(MarkType::Up);
    auto* building = static_cast<BuildingClass*>(buildingType->CreateObject(Owner));
    if (building) {
        building->QueueMission(Mission::Construction, false);
        const CoordStruct where{cell.X * 256 + 128, cell.Y * 256 + 128, 0};
        if (!building->Unlimbo(where, DirType::North)) {
            GameDelete(building);
            building = nullptr;
        }
    }
    if (!building) {
        Locomotor->Mark_All_Occupation_Bits(MarkType::Down);
        Mark(MarkType::Down);
        IsDeploying = false;
        return false;
    }
    SendToFirstLink(RadioCommand::NotifyUnlink);
    for (auto* techno : TechnoClass::Array) {
        if (techno->Target != this || !techno->IsAlive || techno == this || techno == building) continue;
        if (buildingType->ConstructionYard && techno->WhatAmI() == AbstractType::Infantry &&
            static_cast<InfantryClass*>(techno)->Type->C4)
            techno->SetTarget(nullptr);
        else
            techno->SetTarget(building);
    }
    building->Group = Group;
    building->Veterancy = Veterancy;
    if (!Owner->IsControlledByCurrentPlayer()) building->BeingProduced = building->ShouldRebuild = true;
    if (buildingType->Artillary || buildingType->TickTank) building->PrimaryFacing.SetCurrent(DirStruct(short(0x4000)));
    building->SetTarget(Target);
    building->DiscoveredBy(Owner);
    building->IsReadyToCommence = true;
    if (!Owner->IsControlledByHuman() && buildingType->ConstructionYard &&
        SessionClass::Instance.GameMode != GameMode::Campaign) {
        const CellStruct base{short(building->Location.X / 256), short(building->Location.Y / 256)};
        Owner->BaseSpawnCell = base;
        Owner->EnsureBasePlan();
        if (Owner->Base.BaseNodes.Count) Owner->Base.BaseNodes[0].MapCoords = base;
        Owner->Base.Center = base;
        Owner->Production = Owner->AITriggersActive = Owner->AutoBaseBuilding = true;
        Owner->GatherUnitsAtBase();
    }
    building->Health = building->EstimatedHealth =
        std::max(1, game::x87_integer(GetHealthPercentage() * buildingType->Strength));
    game::techno_spatial_dirty(building);
    if (SlaveManager) {
        SlaveManager->Deploy();
        SlaveManager->SetOwner(building);
    }
    building->QueuedVoiceSoundController = QueuedVoiceSoundController;
    building->QueuedVoiceIndex = QueuedVoiceIndex;
    building->unknown_4F4 = unknown_4F4;
    QueuedVoiceSoundController.SetEvent(nullptr, nullptr);
    QueuedVoiceIndex = unknown_4F4 = 0xFFFFFFFFu;
    const bool feedback = Unsorted::MoveFeedback;
    Unsorted::MoveFeedback = false;
    if (selected) building->Select();
    Unsorted::MoveFeedback = feedback;
    Stun();
    if (AttachedTag) {
        building->AttachTrigger(AttachedTag);
        --AttachedTag->InstanceCount;
        AttachedTag = nullptr;
    }
    if (Type->DeploySound != -1 && !game::type_resources().audio_unavailable)
        VocClass::PlayAt(Type->DeploySound, Location, nullptr);
    UnInit();
    return true;
}

// 0x738D30: the construction-yard branch reserves room for a shipyard.
bool UnitClass::GotoClearSpot() {
    if (!Type->DeploysInto) return true;
    Mark(MarkType::Up);
    if ((RulesClass::Instance->BuildConst.FindItemIndex(Type->DeploysInto) >= 0) && GetOwningHouse()->ShipYardConst_1F1) {
        if (!Destination) {
            auto* shipyard = Owner->FirstBuildableFromArray(RulesClass::Instance->Shipyard);
            if (!shipyard) { Mark(MarkType::Down); return false; }
            const int width = shipyard->GetFoundationWidth() + 1;
            shipyard = Owner->FirstBuildableFromArray(RulesClass::Instance->Shipyard);
            if (!shipyard) { Mark(MarkType::Down); return false; }
            const int height = shipyard->GetFoundationHeight(false) + 1;
            CellStruct at;
            MapClass::Instance.NearByLocation(at, GetMapCoords(), SpeedType::Float, -1, MovementZone::Normal,
                false, width, height, false, false, false, true, CellStruct{0, 0}, false, false);
            auto* yard = Owner->FirstBuildableFromArray(RulesClass::Instance->BuildConst);
            if (!yard) { Mark(MarkType::Down); return false; }
            const int yardWidth = yard->GetFoundationWidth() + 1;
            yard = Owner->FirstBuildableFromArray(RulesClass::Instance->BuildConst);
            if (!yard) { Mark(MarkType::Down); return false; }
            const int yardHeight = yard->GetFoundationHeight(false) + 1;
            MapClass::Instance.NearByLocation(at, at, SpeedType::Track, -1, MovementZone::Normal,
                false, yardWidth, yardHeight, false, false, false, true, CellStruct{0, 0}, false, false);
            if (at != CellStruct{0, 0}) SetDestination(MapClass::Instance.GetCellAt(at), true);
            GetOwningHouse()->ShipYardConst_1F1 = false;
        }
    } else if (!Destination) {
        const auto coords = GetCoords();
        const CellStruct anchor{short(coords.X / 256 - 1), short(coords.Y / 256 - 1)};
        if (Type->DeploysInto->CanCreateHere(anchor, nullptr)) { Mark(MarkType::Down); return true; }
        if (!Destination) {
            static constexpr CellStruct offsets[] = {
                {0,-1},{0,-2},{1,-2},{-1,-2},{0,-3},{1,-3},{-1,-3},{2,-3},{-2,-3},
                {0,-4},{1,-4},{-1,-4},{2,-4},{-2,-4},
                {0,1},{0,2},{1,2},{-1,2},{0,3},{1,3},{-1,3},{2,3},{-2,3},
                {0,4},{1,4},{-1,4},{2,4},{-2,4},{-1,0},{-2,0},{-3,0},{-4,0},
                {1,0},{2,0},{3,0},{4,0}};
            for (const auto& offset : offsets) {
                const auto current = GetMapCoords();
                const CellStruct at{short(current.X + offset.X), short(current.Y + offset.Y)};
                const CellStruct candidate{short(at.X - 1), short(at.Y - 1)};
                if (Type->DeploysInto->CanCreateHere(candidate, nullptr)) {
                    SetDestination(MapClass::Instance.GetCellAt(at), true);
                    break;
                }
            }
        }
    }
    Mark(MarkType::Down);
    if (!Destination && !Owner->IsControlledByHuman()) Scatter(CoordStruct{0, 0, 0}, false, false);
    return false;
}

bool UnitClass::FlagAttach(int index) {
    if (index == -1 || FlagHouseIndex != -1) return false;
    FlagHouseIndex = index;
    Mark(MarkType::Change);
    return true;
}
bool UnitClass::FlagRemove() {
    if (FlagHouseIndex == -1) return false;
    FlagHouseIndex = -1;
    Mark(MarkType::Change);
    return true;
}
