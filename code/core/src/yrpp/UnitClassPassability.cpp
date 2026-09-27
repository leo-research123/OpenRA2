// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp Can_Enter_Cell, YR 0x73F0A0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/TubeClass.h"
#include <cmath>

Move UnitClass::IsCellOccupied(CellClass* cell,FacingType facing,int level,CellClass* source,bool checkLocomotor) const {
    const int ground=static_cast<signed char>(cell->Level);
    bool bridge=(unsigned(cell->Flags)&0x100u)&&(level==-1||std::abs(level-ground)>1);
    unsigned flags=cell->OccupationFlags;int infantryOwner=cell->InfantryOwnerIndex;
    if(Type->MovementRestrictedTo!=LandType::None&&cell->LandType!=Type->MovementRestrictedTo&&cell->LandType!=LandType::Tunnel)return Move::No;
    auto* tunnel=cell->GetTunnel();
    if(int(facing)==8)return tunnel&&tunnel->ExitCell!=CellStruct::Empty?Move::OK:Move::No;
    const auto opposite=static_cast<FacingType>((unsigned(facing)-4)&7);
    if(tunnel) {const int d=std::abs(int(facing)-tunnel->ExitFace);if(d>2&&d<6&&facing!=FacingType::None)return Move::No;}
    if(auto* back=cell->GetNeighbourCell(opposite)->GetTunnel()) {
        const int d=std::abs(int(opposite)-back->ExitFace);if(d>2&&d<6&&facing!=FacingType::None)return Move::No;
    }
    if(CanReachCell(cell,facing,level,bridge,source)==Move::No)return Move::No;
    // YR 0x73F0A0 selects the layer AFTER CanReachCell updates level.
    // In particular a FacingType::None / level -1 query resolves to the
    // bridge deck; using the previous level misses its reserved vehicle bit.
    if(level!=-1&&(unsigned(cell->Flags)&0x100u)&&level==ground+4){flags=cell->AltOccupationFlags;infantryOwner=cell->AltInfantryOwnerIndex;}
    bool vehicle=(flags&0x20)!=0;
    if(IsInPlayfield&&!Unsorted::ScenarioInit&&!MapClass::Instance.IsWithinUsableArea(cell,true)&&!vt_entry_320())return Move::No;
    Move result=FootClass::IsCellOccupied(cell,facing,level,source,checkLocomotor);
    if(result==Move::No)return result;
    const auto promote=[&](Move value){if(result<value)result=value;};
    const bool crusher=Type->Crusher||HasAbility(Ability::Crusher);
    if(cell->OverlayTypeIndex>=0) {
        auto* overlay=OverlayTypeClass::Array[cell->OverlayTypeIndex];
        if(overlay->Crate&&!Owner->IsControlledByHuman())return Move::No;
        if(overlay->Wall) {
            if(overlay->Crushable&&crusher){if(Owner->IsAlliedWith(cell->WallOwnerIndex))promote(Move::FriendlyDestroyable);}
            else if(IsArmed()&&GetWeapon(0)->WeaponType->IsWallDestroyer())
                promote(Owner->IsAlliedWith(cell->WallOwnerIndex)?Move::FriendlyDestroyable:Move::Destroyable);
            else return Move::No;
        }
    }
    bool canCrush=false;
    for(auto* object=bridge?cell->AltObject:cell->FirstObject;object;object=object->NextObject) {
        if(object==this){vehicle=false;flags&=~0x20u;continue;}
        // YR 0x73F0A0 checks the linked building's impassable rows even
        // before tethering. Refinery docking pads are inside the foundation.
        if(object==GetNthLink()&&object->WhatAmI()==AbstractType::Building
            &&!static_cast<BuildingClass*>(object)->IsCellImpassable(cell))continue;
        const auto kind=object->WhatAmI();
        if(kind==AbstractType::Building) {
            auto* b=static_cast<BuildingClass*>(object);auto* t=b->Type;
            if(t->Gate) {
                if(!b->IsTraversable()) {
                    if(Owner->IsAlliedWith(b->Owner))promote(Move::ClosedGate);
                    else {if(!IsArmed())return Move::No;promote(Move::Destroyable);}
                }
                continue;
            }
            if(t->InvisibleInGame||(t->LaserFence&&(b->LaserFenceFrame==12||b->LaserFenceFrame==8)))continue;
            if(t->FirestormWall){if(b->Owner->FirestormActive)return Move::No;continue;}
        }
        if(GetCurrentMission()==Mission::Enter&&object==Destination&&kind==AbstractType::Unit)return Move::OK;
        if(GetCurrentMission()==Mission::Area_Guard&&ArchiveTarget==object)return Move::No;
        auto* foot=(object->AbstractFlags&AbstractFlags::Foot)!=AbstractFlags::None?static_cast<FootClass*>(object):nullptr;
        const bool moving=foot&&(foot->Destination||foot->PrimaryFacing.IsRotating()||(foot->Locomotor&&foot->Locomotor->Is_Moving()));
        if(Owner->IsAlliedWith(object)) {
            if(moving) {
                const int face=PrimaryFacing.Current().GetValue<3>();
                const int reverse=(foot->PrimaryFacing.Current().GetValue<3>()+4)&7;
                const auto delta=object->Location-Location;
                const DirStruct toward(int((Math::atan2(-double(delta.Y),double(delta.X))-1.5707963267948966)*-10430.060040584269));
                if(face==reverse&&delta.Magnitude()<=511&&toward.GetValue<3>()==face)return Move::No;
                if((foot->FrozenStill&&kind!=AbstractType::Infantry)||(foot->Locomotor&&foot->Locomotor->Will_Jump_Tracks()))promote(Move::MovingBlock);
            } else {if(kind==AbstractType::Building)return Move::No;promote(Move::Temp);}
        } else {
            auto* techno=(object->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None?static_cast<TechnoClass*>(object):nullptr;
            if(techno&&techno->CloakState==CloakState::Cloaked){promote(Move::Cloak);continue;}
            if(crusher&&object->GetType()->Crushable)canCrush=true;
            else {
                if(!IsArmed()&&!Type->IsTrain)return Move::No;
                if(kind==AbstractType::Terrain) {
                    auto* weapon=GetWeapon(SelectWeapon(object))->WeaponType;
                    if(!weapon||!weapon->Warhead||!weapon->Warhead->Wood||object->GetType()->Immune)return Move::No;
                }
                if(kind==AbstractType::Building&&static_cast<BuildingClass*>(object)->Type->BridgeRepairHut)return Move::No;
                promote(Move::Destroyable);
            }
        }
    }
    if(!bridge&&GroundType::Array[int(cell->LandType)].Cost[int(Type->SpeedType)]==0)return Move::No;
    if(result==Move::OK&&!canCrush&&(flags&0x3F)) {
        if(vehicle||(infantryOwner!=-1&&Owner->IsAlliedWith(infantryOwner)))result=Move::MovingBlock;
        else if(!crusher) {
            auto* weapon=GetWeapon(0)->WeaponType;
            if(Type->IsTrain||(weapon&&weapon->Projectile&&weapon->Projectile->AG))result=Move::Destroyable;
            else return Move::No;
        }
    }
    if(result==Move::OK&&canCrush&&vehicle) {
        auto* unit=cell->GetUnit(bridge);if(!unit||!unit->Type->Crushable)return Move::MovingBlock;
    }
    return result;
}
