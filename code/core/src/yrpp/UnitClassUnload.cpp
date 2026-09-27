// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp Do_MISSION_UNLOAD / Desired_Load_Dir.
// YR 0x73D630 / 0x740B60: passenger branch, including YR's two-cell exits.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/Unsorted.h"
#include "RulesClassReaders.hpp"
#include <cstdlib>

namespace {
CellStruct adjacent_cell(CellStruct cell,int facing){
    const auto delta=Unsorted::AdjacentCell[facing&7];
    return {short(cell.X+delta.X),short(cell.Y+delta.Y)};
}
}

FacingType UnitClass::DesiredLoadDir(FootClass* passenger,CellStruct& output) const {
    DirStruct desired;
    if(passenger)GetDirectionTo(&desired,passenger);
    else desired=DirStruct(PrimaryFacing.Current().Raw+0x7FFF);
    const int desired256=static_cast<signed char>(desired.GetValue<8>());
    int best=-1,bestFacing=0;
    for(int facing=0;facing<8;++facing){
        const auto at=adjacent_cell(GetMapCoords(),facing);
        auto* cell=MapClass::Instance.GetCellAt(at);
        bool free=false;
        if(passenger)free=passenger->IsCellOccupied(cell,FacingType(facing),GetCellLevel(),nullptr,true)==Move::OK||passenger->GetMapCoords()==at;
        else if(GroundType::Array[int(cell->LandType)].Cost[int(SpeedType::Foot)]!=0.0f
            &&!(cell->OccupationFlags&0xE0u)&&(cell->OccupationFlags&0x1Fu)!=0x1Fu){
            const auto* occupant=cell->FindTechnoNearestTo(Point2D{0,0},false,nullptr);
            free=!occupant||Owner->IsAlliedWith(occupant);
        }
        int score=(free?128:-128)-std::abs(int(static_cast<signed char>(facing*32))-desired256);
        if(facing==4)score-=100;
        if(best==-1||score>best){best=score;bestFacing=facing;}
    }
    output=CellStruct::Empty;auto facing=FacingType::South;
    if(best>0){output=adjacent_cell(GetMapCoords(),bestFacing);facing=FacingType((bestFacing+4)&7);}
    return Type->IsTrain?FacingType(PrimaryFacing.Current().GetValue<3>()):facing;
}

int UnitClass::Mission_Unload(){
    if(Type->Passengers<=0&&Type->Harvester)return MissionHarvestUnload();
    // Harvester, MCV and simple-deployer state machines remain separate native
    // dependencies. This ports the shared passenger path, not an IFV shortcut.
    if(Type->Passengers<=0)return FootClass::Mission_Unload();
    switch(MissionStatus){
    case 0:{
        if(Locomotor->Is_Moving())return 10;
        if(!Destination&&GetCell()->LandType==LandType::Water){
            CellStruct shore;
            MapClass::Instance.NearByLocation(shore,GetMapCoords(),SpeedType::Foot,-1,MovementZone::Normal,
                false,1,1,false,false,false,true,CellStruct{0,0},false,false);
            SetDestination(MapClass::Instance.GetCellAt(shore),true);return 10;
        }
        CellStruct exit;const auto facing=DesiredLoadDir(nullptr,exit);
        if(!Passengers.NumPassengers||exit==CellStruct::Empty){QueueMission(Mission::Guard,false);break;}
        if(Type->TurretCount>0)NonPassengerCount=Passengers.NumPassengers!=1;
        Locomotor->Do_Turn(DirStruct(int(facing)<<13));MissionStatus=1;return 1;
    }
    case 1:
        if(!unknown_bool_6AF){MissionStatus=3;return 1;}
        break;
    case 3:{
        if(Passengers.NumPassengers<=NonPassengerCount){MissionStatus=4;break;}
        auto* passenger=RemoveFirstPassenger();if(!passenger)break;
        const int first=int(DirStruct(PrimaryFacing.Current().Raw+0x7FFF).GetValue<3>());
        bool requireTwo=true,placed=false;CellStruct destination=CellStruct::Empty;
        for(int index=0;index<8;++index){
            const auto facing=FacingType((first+index)&7);
            const auto exitCell=adjacent_cell(GetMapCoords(),int(facing));
            const auto walkCell=adjacent_cell(exitCell,int(facing));
            auto* nearCell=MapClass::Instance.GetCellAt(exitCell);
            const bool farFree=passenger->IsCellOccupied(MapClass::Instance.GetCellAt(walkCell),facing,GetCellLevel(),nullptr,true)==Move::OK;
            const bool nearFree=passenger->IsCellOccupied(nearCell,facing,GetCellLevel(),nullptr,true)==Move::OK;
            if(!nearFree||(requireTwo&&!farFree)){
                // 0x73DA15: the fallback restarts at index 1 (not 0).
                if(requireTwo&&index==7){index=0;requireTwo=false;}
                continue;
            }
            if(unsigned(nearCell->Flags)&0x100u)continue;
            CoordStruct where{int(exitCell.X)*256+128,int(exitCell.Y)*256+128,0};
            ++Unsorted::ScenarioInit;
            if(passenger->WhatAmI()==AbstractType::Infantry)MapClass::PickInfantrySublocation(where,where,false);
            else {
                CellStruct cell;
                MapClass::Instance.NearByLocation(cell,exitCell,passenger->GetTechnoType()->SpeedType,-1,MovementZone::Normal,
                    false,1,1,false,false,false,true,CellStruct{0,0},false,false);
                where={int(cell.X)*256+128,int(cell.Y)*256+128,0};
            }
            placed=passenger->Unlimbo(where,DirType(int(facing)*32));
            --Unsorted::ScenarioInit;
            destination=requireTwo?walkCell:exitCell;break;
        }
        if(!placed){
            // Use Cargo directly: AddPassenger would replay the entry sound.
            Passengers.AddPassenger(passenger);
            if(Type->Gunner)ReceiveGunner(passenger);
            passenger->Undiscover();break;
        }
        if(Type->OpenTopped){
            // ExitedOpenTopped 0x7104A0 has only this state change.
            passenger->InOpenToppedTransport=false;
            if(Owner!=passenger->Owner)passenger->SetTarget(nullptr);
        }
        passenger->Transporter=nullptr;
        passenger->QueueMission(Mission::Move,false);
        passenger->SetDestination(MapClass::Instance.GetCellAt(destination),true);
        if(Team)Team->AddMember(passenger,false);
        if(Type->LeaveTransportSound!=-1)VocClass::PlayIndexAtPos(Type->LeaveTransportSound,GetCoords(),0);
        break;
    }
    case 4:
        QueueMission(Mission::Guard,false);unknown_bool_B8=true;break;
    default:break;
    }
    return rule_integer(CurrentMissionControl()->Rate*900.0)+ScenarioClass::Instance->Random.RandomRanged(0,2);
}
