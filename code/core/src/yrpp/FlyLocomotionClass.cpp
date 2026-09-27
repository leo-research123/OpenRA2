// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 fly.cpp; YR 0x4CC9A0, 0x4CCB40, 0x4CD600,
// 0x4CEFB0, 0x4CF610. Ordinary waypoint flight and its rendering matrices.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FlyLocomotionClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/AircraftTrackerClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/YRMath.h"
#include "yrpp/Drawing.h"
#include "yrpp/VocClass.h"
#include "type_resources.hpp"
#include "map_world.hpp"
#include "x87_integer.hpp"
#include "Matrix3DArithmetic.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace {
AircraftClass* aircraft(FootClass* foot){return foot&&foot->WhatAmI()==AbstractType::Aircraft?static_cast<AircraftClass*>(foot):nullptr;}
DirStruct direction(const CoordStruct& from,const CoordStruct& to){
    return DirStruct(game::x87_integer((Math::atan2(double(from.Y)-to.Y,double(to.X)-from.X)-Math::HalfPi)*-10430.060040584269));
}
}
FlyLocomotionClass::FlyLocomotionClass():LocomotionClass(),AirportBound(false),MovingDestination(CoordStruct::Empty),XYZ2(CoordStruct::Empty),
 HasMoveOrder(false),FlightLevel(0),TargetSpeed(0),CurrentSpeed(0),IsTakingOff(0),IsLanding(false),WasLanding(false),unknown_bool_53(false),
 unknown_54(0),unknown_58(0),IsElevating(false),unknown_bool_5D(false),unknown_bool_5E(false),unknown_bool_5F(false){}
HRESULT YRPP_STDCALL FlyLocomotionClass::Link_To_Object(void* object){
    const auto result=LocomotionClass::Link_To_Object(object);
    auto* air=aircraft(LinkedTo);AirportBound=air&&air->Type->AirportBound;return result;
}
bool YRPP_STDCALL FlyLocomotionClass::Is_Moving(){return HasMoveOrder||(LinkedTo&&LinkedTo->PitchAngle>0);}
bool YRPP_STDCALL FlyLocomotionClass::Is_Moving_Now(){return CurrentSpeed!=0;}
CoordStruct YRPP_STDCALL FlyLocomotionClass::Destination(){return Is_Moving()?MovingDestination:CoordStruct::Empty;}
Layer YRPP_STDCALL FlyLocomotionClass::In_Which_Layer(){return LinkedTo&&LinkedTo->GetHeight()>0?Layer::Top:Layer::Ground;}
int YRPP_STDCALL FlyLocomotionClass::Apparent_Speed(){return game::x87_integer(LinkedTo->GetTechnoType()->Speed*CurrentSpeed);}
void YRPP_STDCALL FlyLocomotionClass::Do_Turn(DirStruct dir){LinkedTo->SecondaryFacing.SetDesired(dir);}
void YRPP_STDCALL FlyLocomotionClass::Mark_All_Occupation_Bits(MarkType){}
void YRPP_STDCALL FlyLocomotionClass::Limbo(){}
HRESULT YRPP_STDCALL FlyLocomotionClass::GetClassID(CLSID* output){if(!output)return static_cast<HRESULT>(0x80004003u);*output=CLSIDs::Fly;return 0;}
int FlyLocomotionClass::Size(){return sizeof(*this);}
void YRPP_STDCALL FlyLocomotionClass::Move_To(CoordStruct to){
    if(!LinkedTo||LinkedTo->IsBeingWarpedOut()||LinkedTo->IsWarpingIn()||!Is_Powered())return;
    if(CellClass::Coord2Cell(to)==CellClass::Coord2Cell(MovingDestination)&&IsLanding)return;
    auto* air=aircraft(LinkedTo);const int landing=air?air->Landing_Altitude():0;
    if(to==CoordStruct::Empty){
        if((LinkedTo->GetHeight()>landing||IsTakingOff)&&!IsLanding){MovingDestination=LinkedTo->Location;IsTakingOff=false;IsLanding=true;WasLanding=false;FlightLevel=0;}
        else MovingDestination=CoordStruct::Empty;
        IsElevating=false;
        return;
    }
    MovingDestination=to;
    if(LinkedTo->Target&&LinkedTo->Ammo)MovingDestination.Z=MapClass::Instance.GetCellFloorHeight(to)+LinkedTo->GetTechnoType()->GetFlightLevel();
    HasMoveOrder=true;
    if(LinkedTo->Health>0&&!IsTakingOff&&(IsLanding||LinkedTo->GetHeight()<=landing)){
        IsLanding=false;IsTakingOff=true;FlightLevel=LinkedTo->GetTechnoType()->GetFlightLevel();
        if(!LinkedTo->GetHeight())LinkedTo->PrimaryFacing.SetCurrent(LinkedTo->SecondaryFacing.Desired());
    }
    IsElevating=MovingDestination.Z>MapClass::Instance.GetCellFloorHeight(MovingDestination)+120
        ||(LinkedTo->Target&&LinkedTo->Ammo)||(air&&(air->Is_Locked()||!air->Type->Landable));
}
void YRPP_STDCALL FlyLocomotionClass::Stop_Moving(){
    // 0x4CCFD0 first tests Is_Moving. In particular, clearing navigation after
    // landing must not start another takeoff from the current coordinate.
    if(Is_Moving())Move_To(LinkedTo->Location);
}
bool YRPP_STDCALL FlyLocomotionClass::Process(){
    if(!LinkedTo||!LinkedTo->IsAlive)return false;
    auto& foot=*LinkedTo;const auto& type=*foot.GetTechnoType();auto* air=aircraft(&foot);
    if(!IsLanding&&!IsTakingOff&&TargetSpeed>=1.0&&!FlightLevel)FlightLevel=type.GetFlightLevel();
    if(foot.Health>0&&!IsLanding&&!IsTakingOff&&foot.ReadyToNextMission())foot.NextMission();
    // YR Movement_AI 0x4CD600 updates the spatial index before advancing XY.
    const auto cell=foot.GetMapCoords();
    if(cell!=foot.GetLastFlightMapCoords()&&Is_Moving_Now())
        AircraftTrackerClass::Instance.Update(&foot,foot.GetLastFlightMapCoords(),cell);
    // YR 0x4CD600 zero-health branch: accelerate the fall by 1 before
    // ordinary XY movement. Impact removes the tracker and the object.
    if(!foot.Health&&foot.GetHeight()>0) {
        ++unknown_58;
        auto falling=foot.Location;falling.Z-=unknown_58;
        foot.Mark(MarkType::Up);foot.SetLocation(falling);foot.Mark(MarkType::Down);
        if(foot.GetHeight()<=0&&!foot.Health) {
            AircraftTrackerClass::Instance.Remove(&foot);foot.SetHeight(0);
            foot.FireDeathWeapon(0);
            const bool water=foot.GetCell()->LandType==LandType::Water;
            int sound=water?type.ImpactWaterSound:type.ImpactLandSound;
            if(sound==-1)sound=water?RulesClass::Instance->ImpactWaterSound:RulesClass::Instance->ImpactLandSound;
            if(sound!=-1&&!game::type_resources().audio_unavailable)VocClass::PlayAt(sound,foot.Location);
            foot.UnInit();return false;
        }
    }
    if(!Is_Moving()||!Is_Powered())return false;
    foot.Mark(MarkType::Up);
    auto at=foot.Location;
    const int speed=Apparent_Speed();
    if(speed>0){
        const auto angle=(std::bit_cast<short>(foot.PrimaryFacing.Current().Raw)-0x3FFF)*-0.00009587672516830327;
        at.Y=game::x87_integer(double(at.Y)-Math::sin(angle)*speed);
        at.X=game::x87_integer(double(at.X)+Math::cos(angle)*speed);
        if(MapClass::Instance.CoordinatesLegal(at))foot.SetLocation(at);
    }
    int height=foot.GetHeight();
    if(height<FlightLevel&&foot.Health>0){foot.SetHeight(height+std::min(FlightLevel-height,air&&air->Is_Loaded()?10:20));foot.OnBridge=false;}
    if(height>FlightLevel||!foot.Health)foot.SetHeight(std::max(foot.Health?0:1,height-std::clamp((height-FlightLevel)/20,20,50)));
    const auto here=foot.Location;
    const int distance=game::x87_integer(Math::sqrt(double(MovingDestination.X-here.X)*(MovingDestination.X-here.X)+double(MovingDestination.Y-here.Y)*(MovingDestination.Y-here.Y)));
    if(foot.Health>0&&!IsLanding&&(!IsTakingOff||height>=FlightLevel/2)&&MovingDestination!=CoordStruct::Empty){
        const bool land=!(air&&air->Is_Locked())&&(IsLanding||!IsElevating||(!(air&&air->Type->FlyBy)&&(!(air&&(air->Is_Strafe()||air->Is_Fighter()))||!foot.Ammo)));
        TargetSpeed=land?std::min(1.0,double(distance)/std::max(type.SlowdownDistance,1)):1.0;
        if(land&&TargetSpeed<0.1){if(distance>85)TargetSpeed=0.1;else{TargetSpeed=0;CurrentSpeed*=0.5;}}
        if(land&&distance<CurrentSpeed)CurrentSpeed=distance;
        if(land&&!TargetSpeed&&!CurrentSpeed&&distance>0)CurrentSpeed=0.05;
    }
    if(CurrentSpeed<TargetSpeed)CurrentSpeed=std::min(CurrentSpeed+0.1,TargetSpeed);
    else if(CurrentSpeed>TargetSpeed)CurrentSpeed=std::max(CurrentSpeed-0.1,TargetSpeed);
    foot.Mark(MarkType::Down);
    if(foot.Health>0&&MovingDestination!=CoordStruct::Empty&&!IsLanding&&!IsTakingOff&&foot.GetHeight()>0){
        const auto facing=direction(foot.Location,MovingDestination);
        if(!air||!air->Is_Locked()){
            foot.PrimaryFacing.SetDesired(facing);
            foot.SecondaryFacing.SetDesired(distance<256?DirStruct((air?air->Landing_Direction():0)<<13):facing);
        }
        FlightLevel=IsElevating&&distance<768?MovingDestination.Z-MapClass::Instance.GetCellFloorHeight(MovingDestination):type.GetFlightLevel();
        const bool land=!(air&&air->Is_Locked())&&(IsLanding||!IsElevating||(!(air&&air->Type->FlyBy)&&(!(air&&(air->Is_Strafe()||air->Is_Fighter()))||!foot.Ammo)));
        if(land&&distance<768)TargetSpeed=distance<128?0.0:distance<512?0.5:0.75;
        if(!IsElevating&&distance<128&&CurrentSpeed<0.05){IsLanding=true;WasLanding=false;FlightLevel=0;}
        if(!IsElevating&&!TargetSpeed&&CellClass::Coord2Cell(MovingDestination)==foot.GetMapCoords()){
            IsLanding=true;WasLanding=false;FlightLevel=0;
        }
    }
    // 0x4CD2A0 keeps non-landable aircraft at flight altitude even after
    // SetDestination(nullptr) on the transition from overfly to retreat.
    if(air&&!air->Type->Landable){IsElevating=true;IsLanding=false;IsTakingOff=false;FlightLevel=type.GetFlightLevel();}
    if(IsTakingOff){
        // YR 0x4CE680 clears both flags on this call, unlike OpenTS which
        // waits until full altitude. Keep the target's two facing thresholds.
        const int landing=air?air->Landing_Altitude():0;
        const int relative=foot.GetHeight()-landing,level=FlightLevel-landing;
        IsTakingOff=false;IsLanding=false;
        if(relative>level-level/3)foot.SecondaryFacing.SetDesired(foot.PrimaryFacing.Desired());
        else if(relative>level/2){foot.PrimaryFacing.SetDesired(direction(foot.Location,MovingDestination));TargetSpeed=1.0;}
    }
    if(IsLanding&&foot.GetHeight()<=0){
        // Landing transfers the original adjacency contribution from the
        // departure cell. It is not a count of current airborne XY positions.
        if(foot.LastMapCoords!=CellStruct{})for(int i=0;i<8;++i){const auto delta=Unsorted::AdjacentCell[i];
            --MapClass::Instance.GetCellAt(CellStruct{short(foot.LastMapCoords.X+delta.X),short(foot.LastMapCoords.Y+delta.Y)})->BlockedNeighbours;
        }
        foot.LastMapCoords=foot.GetMapCoords();
        for(int i=0;i<8;++i){const auto delta=Unsorted::AdjacentCell[i];
            ++MapClass::Instance.GetCellAt(CellStruct{short(foot.LastMapCoords.X+delta.X),short(foot.LastMapCoords.Y+delta.Y)})->BlockedNeighbours;
        }
        foot.SetSpeedPercentage(0);
        IsLanding=false;HasMoveOrder=false;MovingDestination=CoordStruct::Empty;TargetSpeed=CurrentSpeed=0;
        foot.SetDestination(nullptr,true);
    }
    if(MapClass::Instance.IsWithinUsableArea(foot.GetMapCoords(),true))foot.IsInPlayfield=true;
    else if(foot.GetCurrentMission()==Mission::Retreat){foot.UnInit();return false;}
    game::map_object_changed();
    return Is_Moving();
}
Matrix3D YRPP_STDCALL FlyLocomotionClass::Draw_Matrix(VoxelIndexKey* key){
    auto matrix=Matrix3D::GetIdentity();const auto facing=LinkedTo->SecondaryFacing.Current().GetValue<5>();
    if(key)key->Value|=int(facing<<3);
    matrix.RotateZ(matrix_store_float((int(facing)-8)*-0.1963495408493621));
    const auto& type=*LinkedTo->GetTechnoType();
    if(LinkedTo->GetHeight()>0&&LinkedTo->IsCrashing){
        matrix.RotateX(LinkedTo->AngleRotatedSideways);
        matrix.RotateY(matrix_store_float(LinkedTo->AngleRotatedForwards+(CurrentSpeed>type.PitchSpeed?type.PitchAngle:0.0)));
        if(key)key->Invalidate();return matrix;
    }
    if(LinkedTo->GetHeight()>0&&CurrentSpeed>type.PitchSpeed){
        if(type.PitchAngle!=0){matrix.RotateY(matrix_store_float(type.PitchAngle));if(key)key->Value|=4;}
        if(type.RollAngle!=0){
            if(LinkedTo->SecondaryFacing.IsRotatingCW()){matrix.RotateX(matrix_store_float(type.RollAngle));if(key)key->Value|=1;}
            else if(LinkedTo->SecondaryFacing.IsRotatingCCW()){matrix.RotateX(matrix_store_float(-type.RollAngle));if(key)key->Value|=2;}
        }
    }
    return matrix;
}
Matrix3D YRPP_STDCALL FlyLocomotionClass::Shadow_Matrix(VoxelIndexKey* key){
    const auto ramp=LinkedTo->GetCell()->SlopeIndex;auto matrix=Matrix3D::VoxelRampMatrix[ramp];
    const auto facing=LinkedTo->SecondaryFacing.Current().GetValue<5>();
    matrix.RotateZ(matrix_store_float((int(facing)-8)*-0.1963495408493621));
    if(key&&key->Is_Valid_Key())key->Value=int(32*(ramp+(unsigned(key->Value)<<6))|facing);
    return matrix;
}
Point2D YRPP_STDCALL FlyLocomotionClass::Draw_Point(){
    auto* air=aircraft(LinkedTo);const int landing=air?air->Landing_Altitude():0;
    return {0,!IsLanding&&!IsTakingOff&&LinkedTo->GetHeight()>landing?game::x87_integer(Math::sin((Unsorted::CurrentFrame%20)*0.3141592653589793)*1.5+0.5):0};
}
Point2D YRPP_STDCALL FlyLocomotionClass::Shadow_Point(){return Point2D::Empty;}
