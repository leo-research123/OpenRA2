// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 bullet.cpp AI, missile.cpp Projectile_Motion and
// velocity.h; YR 0x4666E0 / 0x468BB0 / 0x5B20F0 / 0x4E11F0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BulletClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/Matrix3D.h"
#include "RulesClassReaders.hpp"
#include "projectile_diagnostics.hpp"
#include <algorithm>
#include <bit>

namespace {
int difference(int a,int b){return std::bit_cast<int>(unsigned(a)-unsigned(b));}
int add(int a,int b){return std::bit_cast<int>(unsigned(a)+unsigned(b));}
double speed(const BulletVelocity& v){return Math::sqrt(v.X*v.X+v.Y*v.Y+v.Z*v.Z);}
double horizontal(const BulletVelocity& v){return Math::sqrt(v.X*v.X+v.Y*v.Y);}
int distance(CoordStruct a,CoordStruct b) {
    const double x=difference(a.X,b.X),y=difference(a.Y,b.Y),z=difference(a.Z,b.Z);
    return rule_integer(Math::sqrt(x*x+y*y+z*z));
}
unsigned short direction(double angle){return static_cast<unsigned short>(rule_integer((angle-1.5707963267948966)*-10430.060040584269));}
double radians(unsigned short value){return (int(std::bit_cast<short>(value))-0x3FFF)*-0.00009587672516830327;}
unsigned short pitch(const BulletVelocity& v){return direction(Math::atan2(v.Z,horizontal(v)));}
unsigned short turn(unsigned short from,unsigned short to,unsigned short rate) {
    const auto delta=std::bit_cast<short>(static_cast<unsigned short>(to-from));
    if(std::abs(int(delta))<=int(rate))return to;
    return static_cast<unsigned short>(int(from)+(delta>=0?int(rate):-int(rate)));
}
void set_speed(BulletVelocity& v,double magnitude) {
    if(v.X==0 && v.Y==0 && v.Z==0)v.X=100;
    const double scale=magnitude/speed(v);v.X*=scale;v.Y*=scale;v.Z*=scale;
}
void set_pitch(BulletVelocity& v,unsigned short value) {
    const double original=radians(pitch(v)),magnitude=speed(v),next=radians(value);
    if(original!=0){v.X/=Math::cos(original);v.Y/=Math::cos(original);}
    v.X*=Math::cos(next);v.Y*=Math::cos(next);v.Z=magnitude*Math::sin(next);
}
void advance(CoordStruct& at,const BulletVelocity& v) {
    at.X=add(at.X,rule_integer(v.X));at.Y=add(at.Y,rule_integer(v.Y));at.Z=add(at.Z,rule_integer(v.Z));
}
int projectile_motion(CoordStruct& at,BulletVelocity& v,const CoordStruct& target,unsigned short rate,
        bool aircraft,bool airburst,bool high,bool level) {
    if(target==CoordStruct::Empty) {
        // YR differs from OpenTS: pitch 0x2000, then an actual position step
        // and velocity length; not TS's pitch 0x4200 and constant return 1.
        set_pitch(v,turn(pitch(v),0x2000,rate));advance(at,v);return rule_integer(speed(v));
    }
    advance(at,v);
    const int targetDistance=distance(at,target);
    CoordStruct planarTarget=target;planarTarget.Z=at.Z;
    const int planar=distance(at,planarTarget);
    CoordStruct delta{difference(target.X,at.X),difference(target.Y,at.Y),difference(target.Z,at.Z)};
    const BulletVelocity towards{double(delta.X),double(delta.Y),double(delta.Z)};
    const auto yaw=turn(direction(Math::atan2(-v.Y,v.X)),direction(Math::atan2(-towards.Y,towards.X)),rate);
    const double magnitude=horizontal(v);v.X=Math::cos(radians(yaw))*magnitude;v.Y=-Math::sin(radians(yaw))*magnitude;
    auto desired=pitch(v);
    if(!aircraft && (airburst || planar>((high?6:3)<<8)) && (((unsigned(rate)>>7)+1)>>1 & 0xFFu)>1) {
        CoordStruct ahead{add(at.X,6*rule_integer(v.X)),add(at.Y,6*rule_integer(v.Y)),add(at.Z,6*rule_integer(v.Z))};
        int floor=MapClass::Instance.GetCellFloorHeight(ahead);
        if(MapClass::Instance.GetCellAt(ahead)->ContainsBridgeEx())floor+=CellClass::BridgeHeight;
        const int cells=airburst || high?10:std::min(targetDistance/256,5);
        if(!level) {
            const int error=at.Z-Unsorted::LevelHeight*cells-floor;
            if(error<-20)at.Z+=18;else if(error>20)at.Z-=18;
            desired=turn(desired,error<-Unsorted::LevelHeight/2?0x2000:error>Unsorted::LevelHeight/2?0x4800:0x4000,
                static_cast<unsigned short>(std::bit_cast<short>(rate)/2));
        }
    }else if(!level)desired=turn(desired,pitch(towards),static_cast<unsigned short>(std::bit_cast<short>(static_cast<unsigned short>(rate+256))/2));
    set_pitch(v,desired);
    delta.Z=airburst?0:delta.Z/4;
    return distance(delta,CoordStruct::Empty);
}
int fuse_check(BulletData& fuse,const CoordStruct& at) {
    if(fuse.ArmTimer.GetTimeLeft())return 0;
    const int current=distance(at,fuse.Location)/2;
    if(current<32)return 1;
    if(current<256 && current>fuse.Distance)return 2;
    fuse.Distance=current;return 0;
}
}

bool BulletClass::IsForcedToExplode(CoordStruct* at) const {
    *at=Location;auto& map=MapClass::Instance;auto* cell=map.GetCellAt(*at);
    if((Type->SubjectToCliffs || Type->SubjectToWalls)
        && TrajectoryHelper::GetObstacle(map.GetCellAt(SourceCoords),map.GetCellAt(TargetCoords),map.GetCellAt(LastMapCoords),
            *at,Type,Owner?Owner->Owner:nullptr))return true;
    if(GetHeight()<=-4*Unsorted::LevelHeight)return true;
    if(Target && Type->FlakScatter && Location.Z<Target->GetCenterCoords().Z && GetHeight()<0)return true;
    if(Type->Level && cell->IsOnFloor())return true;
    return Type->AA && Target && Target->IsInAir() && DistanceFrom3D(Target)<128;
}

void BulletClass::Update() {
    game::projectile_log_event(*this,"update_enter");
    ObjectClass::Update();if(!IsAlive)return;
    if(SpawnNextAnim) {
        if(!NextAnim){NextAnimBullets.Remove(this);SpawnNextAnim=false;Explode(false);UnInit();}
        return;
    }
    bool forced=Type->Dropping && !IsFallingDown,collided=false;
    if(Type->AnimLow || Type->AnimHigh)if(!--AnimRateCounter) {
        AnimRateCounter=Type->AnimRate;if(++AnimFrame>Type->AnimHigh)AnimFrame=Type->AnimLow;
    }
    auto at=Location;auto& map=MapClass::Instance;auto& rules=*RulesClass::Instance;
    if(Type->Trailer && !(Unsorted::CurrentFrame%(Type->ScaledSpawnDelay?Type->ScaledSpawnDelay:Type->SpawnDelay)))
        if(auto* memory=YRMemory::Allocate(sizeof(AnimClass)))::new(memory) AnimClass(Type->Trailer,at,1,1,0x600,0,false);
    int impact=0;
    if(Type->ROT>0) {
        double magnitude=speed(Velocity);
        if(Type->CourseLockDuration) {
            if(CourseLockCounter<Type->CourseLockDuration && ++CourseLockCounter>=Type->CourseLockDuration)CourseLock=false;
        }else if(Speed>=40 || magnitude+0.5>=Speed)CourseLock=false;
        const int acceleration=CourseLock && !Type->CourseLockDuration?int(Unsorted::CurrentFrame%2==0):Type->Acceleration;
        if(magnitude<Speed)set_speed(Velocity,std::min(magnitude+acceleration,double(Speed)));
        else if(magnitude>Speed)set_speed(Velocity,std::max(magnitude-acceleration/2,0.0));
        auto target=Target?Target->GetCenterCoords():CoordStruct::Empty;
        if(Target && (Target->AbstractFlags & AbstractFlags::Object)!=AbstractFlags::None)static_cast<ObjectClass*>(Target)->GetTargetCoords(&target);
        int rot=rule_integer((Math::sin(double((Unsorted::CurrentFrame+Fetch_ID())%15)/15.0*6.283185307179586)*rules.MissileROTVar+rules.MissileROTVar+1)*Type->ROT);
        if(distance(GetCoords(),target)<256)rot=rule_integer(rot*1.5);
        const auto previous=at;
        const int left=projectile_motion(at,Velocity,target,static_cast<unsigned short>((CourseLock?0:rot&0xFF)<<8),
            Target && Target->WhatAmI()==AbstractType::Aircraft,Type->Airburst,Type->VeryHigh,Type->Level);
        if(left<=speed(Velocity)/2.0 || GetHeight()<=0) {
            if(!Type->Airburst && GetHeight()>0 && target!=CoordStruct::Empty)at=target;
            impact=1;forced=true;
        }
        if(target==CoordStruct::Empty && GetHeight()>=rules.CruiseHeight){impact=1;forced=true;}
        else if(target!=CoordStruct::Empty && !CourseLock) {
            const int closure=left-distance(at,target);
            if(unknown_118<60){++unknown_118;unknown_120+=closure;}
            else {
                unknown_120=unknown_120*0.9833333333333333+closure;
                if(unknown_120>=0 && unknown_120<60 && !Type->Airburst && !Type->VeryHigh){forced=true;impact=1;}
            }
        }
        if(!impact && (map.GetCellAt(at)->ContainsBridgeEx() || map.GetCellAt(previous)->ContainsBridgeEx())) {
            const int height=map.GetCellFloorHeight(at)+CellClass::BridgeHeight;
            if((at.Z>height && previous.Z<height) || (at.Z<height && previous.Z>height)){at.Z=height;forced=true;impact=1;}
        }
    }else {
        auto velocity=Velocity;
        impact=speed(velocity)<8?1:0;
        const auto previous=at;
        if(Type->Vertical) {
            const int magnitude=rule_integer(speed(Velocity));
            if(magnitude<Speed)set_speed(Velocity,double(magnitude+Type->Acceleration));
            advance(at,Velocity);
            const int bridge=map.GetCellFloorHeight(at)+CellClass::BridgeHeight;
            if(at.Z>Type->DetonationAltitude || GetHeight()<0){forced=true;impact=1;}
            else if((map.GetCellAt(at)->ContainsBridgeEx() || map.GetCellAt(previous)->ContainsBridgeEx())
                && ((at.Z<bridge && previous.Z>=bridge) || (at.Z>=bridge && previous.Z<bridge))){forced=true;impact=1;}
        }else {
            velocity.Z-=double(rules.Gravity)*(Type->Floater?0.5:1.0);
            BulletVelocity position{double(at.X)+velocity.X,double(at.Y)+velocity.Y,double(at.Z)+velocity.Z};
            CoordStruct next{rule_integer(position.X),rule_integer(position.Y),rule_integer(position.Z)};
            const int floor=map.GetCellFloorHeight(next),deck=floor+CellClass::BridgeHeight;
            auto* cell=map.GetCellAt(next);
            bool fell=false,rose=false,obstacle=false;
            if(cell->ContainsBridgeEx() || map.GetCellAt(previous)->ContainsBridgeEx()) {
                fell=next.Z<deck && previous.Z>=deck;rose=next.Z>=deck && previous.Z<deck;
            }
            if(!fell && !rose && position.Z>=floor && position.Z-150.0<floor) {
                auto* building=cell->GetBuilding();obstacle=building || cell->ConnectsToOverlay(-1,-1);
                if(building && (building==Owner || (building->Type->LaserFence && building->LaserFenceFrame>=8)
                    || building->IsStrange() || (Owner && Owner->Owner->IsAlliedWith(building))))obstacle=false;
            }
            if(position.Z<floor || fell || rose || obstacle) {
                game::projectile_log_contact(*this,next.X,next.Y,next.Z,floor,cell->SlopeIndex,fell,rose,obstacle);
                if(Owner) {
                    if(fell)position.Z=deck;else if(rose)position.Z=deck-20;
                    else goto position_ready; // original firer-present ground branch skips bounce
                }else {
                    if(fell)position.Z=deck;else if(rose)position.Z=deck-20;else if(position.Z>floor-100)position.Z=floor;
                    const auto& matrix=Matrix3D::VoxelRampMatrix[cell->SlopeIndex];
                    auto inverse=Matrix3D::GetIdentity();
                    for(int row=0;row<3;++row)for(int column=0;column<3;++column)inverse.row[row][column]=matrix.row[column][row];
                    auto reflected=inverse.RotateVector({float(velocity.X),float(-velocity.Y),float(velocity.Z)});
                    const float elasticity=float(Type->Elasticity);
                    reflected.X*=elasticity;reflected.Y*=elasticity;reflected.Z*=-elasticity;
                    reflected=matrix.RotateVector(reflected);velocity={reflected.X,-reflected.Y,reflected.Z};
                }
                impact=1;forced=true;
            }
position_ready:
            at={rule_integer(position.X),rule_integer(position.Y),rule_integer(position.Z)};
        }
        auto* cell=map.GetCellAt(at);auto* building=cell->GetBuilding();
        if(!Type->Vertical && GetHeight()<2*Unsorted::LevelHeight
            && (CellClass::Coord2Cell(at)==CellClass::Coord2Cell(TargetCoords)
                || (building && building==map.GetCellAt(TargetCoords)->GetBuilding()))) {
            impact=1;forced=true;collided=true;
        }else {
            auto* object=cell->FindTechnoNearestTo({0,0},false,nullptr);
            if(object && object!=Owner && (!Owner || !Owner->Owner->IsAlliedWith(object)) && distance(at,object->Location)<128) {
                impact=1;forced=true;if(!Type->Inaccurate)at=object->Location;
            }else if(!map.IsWithinUsableArea2D(CellClass::Coord2Cell(at))) {impact=2;forced=true;at=Location;}
            else {
                if(!Type->Vertical)Velocity=velocity;
                if(speed(Velocity)<10 && GetHeight()<10){impact=1;forced=true;}
            }
        }
    }
    Mark(MarkType::Change);
    if(impact==2)UnInit();
    else {
        SetLocation(at);
        if(!forced){auto checked=Location;forced=IsForcedToExplode(&checked);SetLocation(checked);}
        if(forced && GetHeight()<0)SetHeight(0);
        int fuse=Type->ROT>0 || Type->Ranged?fuse_check(Data,at):0;
        if(Owner && Owner->GetTechnoType()->JumpJet && fuse==2)fuse=1;
        if(!forced && (Type->Dropping || !fuse)) {if(Type->Degenerates && Health>5)--Health;}
        else {
            if(Target && (fuse==1 || collided) && !Type->Airburst && !Type->Inaccurate) {
                const auto center=Target->GetCenterCoords();auto midpoint=at;midpoint.Z=add(at.Z,center.Z)/2;
                int error=distance(midpoint,center);if(collided)error/=3;
                if(fuse==1 || error<=std::max(128.0,2*speed(Velocity)))SetLocation(Target->GetCoords());
            }
            bool deferred=false;
            if(!_strcmpi(WH->ID,"NUKE")) {
                if(GetHeight()<0)SetHeight(0);
                NukeFlash::FadeIn();RadarEventClass::Create(static_cast<RadarEventType>(0),GetMapCoords());
                if(auto* type=AnimTypeClass::Find("NUKEBALL")) {
                    auto* memory=YRMemory::Allocate(sizeof(AnimClass));
                    NextAnim=memory?::new(memory) AnimClass(type,Location,0,1,0x2600,-15,false):nullptr;
                    SpawnNextAnim=true;NextAnimBullets.AddItem(this);deferred=true;
                }
            }
            if(!deferred){Explode(forced);UnInit();}
        }
    }
    LastMapCoords=CellClass::Coord2Cell(at);
}
