// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 aircraft.cpp Paradrop_Cargo and reinf.cpp; YR adds
// approach/overfly missions 0x4158E0/0x415960 and alternates cargo positions
// in 0x415C60. Retreat is 0x415A50.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/AircraftClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/YRMath.h"
#include "type_resources.hpp"
#include "x87_integer.hpp"
#include <bit>

int AircraftClass::Mission_ParaDropApproach() {
    if(!Target){SetDestination(nullptr,true);QueueMission(Mission::Retreat,false);}
    else if(!Destination)SetDestination(Target,true);
    else if(DistanceFrom(Target)<=RulesClass::Instance->ParadropRadius){
        QueueMission(Mission::ParadropOverfly,false);--NumParadropsLeft;
    }
    return 3;
}
int AircraftClass::Mission_ParaDropOverfly() {
    IsLocked=true;
    if(!Target||!Passengers.NumPassengers){
        IsLocked=false;SetTarget(nullptr);SetDestination(nullptr,true);QueueMission(Mission::Retreat,false);
    }else if(DistanceFrom(Target)>RulesClass::Instance->ParadropRadius){
        IsLocked=false;
        if(NumParadropsLeft>0)QueueMission(Mission::ParadropApproach,false);
        else{SetTarget(nullptr);SetDestination(nullptr,true);QueueMission(Mission::Retreat,false);}
    }else if(MapClass::Instance.IsWithinUsableArea(Location))ParadropCargo();
    return 5;
}
int AircraftClass::ParadropCargo() {
    auto* passenger=Passengers.RemoveFirstPassenger();if(!passenger)return 0;
    --Ammo;
    const auto yaw=static_cast<unsigned short>(PrimaryFacing.Current().Raw+((Ammo&1)?-0x3FFF:0x3FFF));
    const double angle=(int(std::bit_cast<short>(yaw))-0x3FFF)*-0.00009587672516830327;
    auto at=GetCoords();
    at.X=game::x87_integer(double(at.X)+Math::cos(angle)*128.0);
    at.Y=game::x87_integer(double(at.Y)-Math::sin(angle)*128.0);
    auto* cell=MapClass::Instance.GetCellAt(at);
    bool placed=false;
    if(passenger->IsCellOccupied(cell,FacingType::None,-1,nullptr,true)==Move::OK){
        const auto sub=cell->FindInfantrySubposition(at,false,false,false);
        if(sub!=CoordStruct::Empty){at.X=sub.X;at.Y=sub.Y;placed=passenger->SpawnParachuted(at);}
    }
    if(!placed){Passengers.AddPassenger(passenger);passenger->Mark(MarkType::Change);++Ammo;return 0;}
    if(!game::type_resources().audio_unavailable)VocClass::PlayAt(RulesClass::Instance->ChuteSound,Location);
    passenger->LastMapCoords=CellClass::Coord2Cell(at);
    if(Team)Team->LiberateMember(passenger);
    NumParadropsLeft=5;
    RearmTimer.Start(0);
    return 0;
}
int AircraftClass::Mission_Retreat() {
    if(Destination){if(Destination==GetCell())SetDestination(nullptr,true);}
    else{
        auto edge=Owner->StartingEdge;
        if(unsigned(edge)>=4)edge=unsigned(Owner->Edge)<4?Owner->Edge:Edge::North;
        const auto at=MapClass::Instance.PickCellOnEdge(edge,CellStruct::Empty,CellStruct::Empty,SpeedType::Winged,true,MovementZone::Normal);
        if(at!=CellStruct::Empty)SetDestination(MapClass::Instance.GetCellAt(at),true);
    }
    return 3;
}
