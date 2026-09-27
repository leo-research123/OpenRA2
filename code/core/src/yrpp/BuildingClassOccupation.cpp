// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp::Set_Occupy_Bit / Clear_Occupy_Bit.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR 0x453D60 / 0x453DC0: ground occupation byte, per foundation cell.
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/Unsorted.h"

// YR garrison additions (no equivalent OpenTS building garrison routine).
// 0x457DE0: the two arguments are booleans, not a source-unit pointer.
void BuildingClass::UnloadOccupants(bool scatter,bool force) {
    FiringOccupantIndex=0;
    if(!GetOccupantCount())return;
    const auto origin=GetMapCoords();
    const int width=Type->GetFoundationWidth(),height=Type->GetFoundationHeight(false);
    auto* first=Occupants[0];CellStruct exit=CellStruct::Empty;bool found=false;
    const auto consider=[&](int x,int y){
        const CellStruct at{short(x),short(y)};
        if(first->IsCellOccupied(MapClass::Instance.GetCellAt(at),FacingType(-1),-1,nullptr,true)==Move::OK){exit=at;found=true;}
    };
    // Preserve the original four perimeter scans and corner visit order.
    consider(origin.X+width,origin.Y+height);
    for(int y=origin.Y+height-1;y>=origin.Y-1&&!found;--y)consider(origin.X+width,y);
    for(int x=origin.X+width-1;x>=origin.X-1&&!found;--x)consider(x,origin.Y+height);
    for(int x=origin.X;x<=origin.X+width&&!found;++x)consider(x,origin.Y-1);
    for(int y=origin.Y;y<=origin.Y+height&&!found;++y)consider(origin.X-1,y);
    if(!found){
        if(!force){KillOccupants(nullptr);return;}
        exit={short(origin.X+width-1),short(origin.Y+height-1)};
    }
    const auto at=MapClass::Instance.GetCellAt(exit)->GetCoords();
    ++Unsorted::ScenarioInit;
    for(int i=Occupants.Count-1;i>=0;--i){
        auto* occupant=Occupants[i];
        if(occupant->Unlimbo(at,DirType::North)){
            if(occupant->Owner->IsHumanPlayer)occupant->ShouldEnterOccupiable=occupant->ShouldGarrisonStructure=false;
            occupant->SetTarget(nullptr);
            occupant->Scatter(GetCoords(),true,true);
            if(scatter){
                if(occupant->Team)occupant->Team->LiberateMember(occupant,-1,0);
                occupant->QueueMission(Mission::Area_Guard,false);
            }
        }else occupant->UnInit();
    }
    --Unsorted::ScenarioInit;
    const int capacity=Occupants.Capacity;Occupants.Clear();Occupants.SetCapacity(capacity);
    UpdateThreatInCell(GetCell());
}

// YR 0x4585C0: no escape cell, or an assaulter kills the garrison.
void BuildingClass::KillOccupants(TechnoClass* assaulter) {
    FiringOccupantIndex=0;
    for(int i=Occupants.Count-1;i>=0;--i){
        auto* occupant=Occupants[i];
        if(assaulter){
            Point2D offset;
#if defined(RA2_YRPP_GAME)
            TacticalClass::Instance->ApplyMatrix_Pixel(&offset,&Type->MuzzleFlash[i]);
#else
            offset=TacticalClass::Instance->ApplyMatrix_Pixel(Type->MuzzleFlash[i]);
#endif
            const auto origin=GetRenderCoords();const CoordStruct at{origin.X+offset.X,origin.Y+offset.Y,origin.Z};
            if(void* memory=YRMemory::Allocate(sizeof(AnimClass))){
                auto* animation=::new(memory) AnimClass(assaulter->GetWeapon(0)->WeaponType->AssaultAnim,at,0,1,0x600,0,false);
                animation->ZAdjust=-200;
            }
            assaulter->RegisterDestruction(occupant);
        }
        occupant->UnInit();
    }
    const int capacity=Occupants.Capacity;Occupants.Clear();Occupants.SetCapacity(capacity);
    UpdateThreatInCell(GetCell());
}

void BuildingClass::MarkAllOccupationBits(const CoordStruct& coords) {
    MapClass::Instance.GetCellAt(coords)->OccupationFlags|=0x80u;
}
void BuildingClass::UnmarkAllOccupationBits(const CoordStruct& coords) {
    MapClass::Instance.GetCellAt(coords)->OccupationFlags&=~0x80u;
}
