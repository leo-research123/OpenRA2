// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 drive.cpp Process / While_Moving / Start_Of_Move.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Calibrated to YR 0x4B0500 / 0x4B0F20 / 0x4B2630. YR's raw track
// indices include slot zero; movement uses seven-lepton samples, not a
// host interpolation or the infantry locomotor.
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/AnimClass.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
CellStruct cell_of(const CoordStruct& p) { return {short(p.X/256),short(p.Y/256)}; }
bool bridge(const CellClass* c) { return (unsigned(c->Flags)&0x100u)!=0; }
int distance(const CoordStruct& a,const CoordStruct& b) {
    const double x=double(a.X)-b.X,y=double(a.Y)-b.Y,z=double(a.Z)-b.Z;
    return int(Math::sqrt(x*x+y*y+z*z));
}
CoordStruct adjacent(CoordStruct p,int facing) {
    const auto d=Unsorted::AdjacentCell[facing&7];p.X+=d.X*256;p.Y+=d.Y*256;return p;
}
void advance(FootClass* f,int count) {
    std::memmove(f->PathDirections,f->PathDirections+count,(24-count)*sizeof(int));
    std::fill(f->PathDirections+24-count,f->PathDirections+24,-1);
}
bool active(const FootClass* f) { return f&&f->IsAlive&&!f->InLimbo&&!f->IsFallingDown; }
void relocate(FootClass* f,CoordStruct p,bool floor=true) {
    auto& map=MapClass::Instance;
    auto* old=map.GetCellAt(f->Location);auto* next=map.GetCellAt(p);
    if(cell_of(p)!=cell_of(f->Location)) {
        f->Mark(MarkType::Up);f->SetLocation(p);
        if(static_cast<signed char>(next->Level)==static_cast<signed char>(old->Level)-4&&bridge(next))f->OnBridge=true;
        if(!bridge(next)&&bridge(old))f->OnBridge=false;
        if(floor)f->SetHeight(0);
        f->Mark(MarkType::Down);
    } else {
        const bool down=f->IsOnMap;f->IsOnMap=false;f->SetLocation(p);
        if(floor)f->SetHeight(0);f->IsOnMap=down;
    }
}
bool crushable(const CellClass* c) {
    return c->OverlayTypeIndex>=0&&OverlayTypeClass::Array[c->OverlayTypeIndex]->Crushable;
}
}

bool YRPP_STDCALL DriveLocomotionClass::Process() {
    Set_Slope(LinkedTo->GetCell()->SlopeIndex);
    if(TrackNumber!=-1&&IsDriving) {
        if(While_Moving()||!active(LinkedTo))return false;
        if(TrackNumber==-1&&(Is_Moving()||LinkedTo->PathDirections[0]!=-1)
            &&(LinkedTo->WhatAmI()!=AbstractType::Unit||!static_cast<UnitClass*>(LinkedTo)->Unloading)) {
            bool stop=false;Start_Of_Move(stop);
            if(stop||!active(LinkedTo))return false;
            While_Moving(true);if(!active(LinkedTo))return false;
        }
    } else {
        if(LinkedTo->Destination&&LinkedTo->Destination->WhatAmI()==AbstractType::Cell
            // YR 0x4B0500 calls GetMapCoords (virtual slot 0x1B8).
            // CurrentMapCoords is the planned track endpoint, not our position.
            &&LinkedTo->GetMapCoords()==cell_of(LinkedTo->Destination->GetCoords())) {
            Abandon_Navigation();return false;
        }
        if(LinkedTo->GetCurrentMission()==Mission::Guard&&!IsDriving
            &&DestinationCoord!=CoordStruct::Empty&&LinkedTo->Location==DestinationCoord) {
            Abandon_Navigation();return false;
        }
        if(LinkedTo->PrimaryFacing.IsRotating())IsRotating=true;
        else {
            if(IsRotating) {
                IsRotating=false;LinkedTo->UpdatePosition(PCPType::Rotation);
                if(!active(LinkedTo))return false;
            }
            if((LinkedTo->GetCurrentMission()!=Mission::Guard||Is_Moving())&&LinkedTo->GetCurrentMission()!=Mission::Unload) {
                if(Is_Moving()||LinkedTo->PathDirections[0]!=-1) {
                    if(LinkedTo->IsInPlayfield&&LinkedTo->GetCurrentMission()!=Mission::Enter&&Is_Moving()
                        &&!LinkedTo->IsInSameZoneAsCoords(DestinationCoord)) {
                        Stop_Driver();if(Abandon_Navigation())return false;
                    } else {
                        bool stop=false;Start_Of_Move(stop);
                        if(stop||!active(LinkedTo))return false;
                        While_Moving();if(!active(LinkedTo))return false;
                    }
                } else if(LinkedTo->IsSinking) { Stop_Driver();movementspeed_50=0; }
                else if(LinkedTo->Destination)Move_To(LinkedTo->Destination->GetDestination(LinkedTo));
            }
        }
    }
    if(Is_Moving_Now()&&Unsorted::CurrentFrame%10==0&&!LinkedTo->OnBridge
        &&LinkedTo->GetCell()->LandType==LandType::Water&&RulesClass::Instance->Wake)
        GameCreate<AnimClass>(RulesClass::Instance->Wake,LinkedTo->Location);
    if(DestinationCoord==CoordStruct::Empty&&HeadToCoord==CoordStruct::Empty
        &&LinkedTo->PathDirections[0]==-1&&LinkedTo->SpeedPercentage>0)LinkedTo->SetSpeedPercentage(0);
    return Is_Moving();
}

bool DriveLocomotionClass::While_Moving(bool justStarted) {
    auto* type=LinkedTo->GetTechnoType();
    if(((!IsDriving||TrackNumber==-1)&&LinkedTo->PathDirections[0]!=8)||(IsRotating&&!type->Turret)) {
        SpeedAccum=0;return false;
    }
    if(type->Accelerates) {
        if(TrackNumber<64&&(LinkedTo->WhatAmI()!=AbstractType::Unit||!static_cast<UnitClass*>(LinkedTo)->Type->Passive)) {
            auto dest=DestinationCoord;
            dest.Z=MapClass::Instance.GetCellFloorHeight(dest)+(bridge(MapClass::Instance.GetCellAt(dest))?CellClass::BridgeHeight:0);
            double speed=LinkedTo->SpeedPercentage;const int maximum=LinkedTo->GetDefaultSpeed();bool forced=false;
            if(distance(LinkedTo->Location,dest)<type->SlowdownDistance) {
                forced=true;speed=std::max(double(0.3f),speed-maximum*type->DecelerationFactor);
            } else if(LinkedTo->IsSinking) { forced=true;speed=std::max(double(0.1f),speed-maximum*double(0.0015f)); }
            if(LinkedTo->IsCrushingSomething) { movementspeed_50=std::min(movementspeed_50,0.2);speed=movementspeed_50; }
            else if(!forced) {
                if(speed<movementspeed_50)speed=std::min(movementspeed_50,speed+type->AccelerationFactor);
                else if(speed>movementspeed_50)speed=std::max(movementspeed_50,speed-maximum*type->DecelerationFactor);
            }
            LinkedTo->SetSpeedPercentage(speed);
            if(LinkedTo->WhatAmI()==AbstractType::Unit)for(auto* f=static_cast<UnitClass*>(LinkedTo)->FollowerCar;f;) {
                f->SetSpeedPercentage(speed);f=f->FollowerCar;if(f&&f==f->FollowerCar)break;
            }
        }
    } else LinkedTo->SetSpeedPercentage(movementspeed_50);
    int actual=SpeedAccum+(justStarted?0:LinkedTo->GetCurrentSpeed());
    int nextface=LinkedTo->PathDirections[0];
    if(nextface==8&&TrackNumber==-1) {
        LinkedTo->Mark(MarkType::Up);Stop_Driver();
        const int index=LinkedTo->GetCell()->TubeIndex;
        if(index>=0&&index<TubeClass::Array.Count) {
            auto* tube=TubeClass::Array[index];
            HeadToCoord={tube->ExitCell.X*256+128,tube->ExitCell.Y*256+128,0};
            advance(LinkedTo,1);LinkedTo->TubeIndex=static_cast<signed char>(index);LinkedTo->TubeFaceIndex=0;
            LinkedTo->CurrentTunnelCoords=MapClass::Instance.GetCellAt(tube->EnterCell)->GetNeighbourCell(static_cast<FacingType>(tube->Faces[0]&7))->GetCellCoords();
            const int z=MapClass::Instance.GetCellFloorHeight(LinkedTo->Location);
            LinkedTo->CurrentTunnelCoords.Z=z+(MapClass::Instance.GetCellFloorHeight(HeadToCoord)-z)/tube->FaceCount;
            IsDriving=true;
        } else { LinkedTo->PathDirections[0]=-1;HeadToCoord=CoordStruct::Empty;Stop_Driver(); }
        return false;
    }
    if(TrackNumber<0)return false;
    auto rawIndex=[&]{const auto& c=TurnTrack[TrackNumber];return IsOnShortTrack?c.ShortTrackStructIndex:c.NormalTrackStructIndex;};
    if(nextface< -1||nextface>8){LinkedTo->PathDirections[0]=-1;return false;}
    while(actual>7) {
        actual-=7;const auto& raw=RawTrack[rawIndex()];const auto& point=raw.TrackPoint[TrackIndex];
        if(point.Point!=Point2D{0,0}||!TrackIndex) {
            if(LinkedTo->IsStandingStill()) {
                LinkedTo->UnmarkAllOccupationBits(LinkedTo->Location);
                LinkedTo->FrozenStill=false;LinkedTo->IsWaitingBlockagePath=false;
            }
            int dir=point.Face;
            const auto pos=Smooth_Turn(point.Point,dir);
            relocate(LinkedTo,{pos.X,pos.Y,0});
            if(!active(LinkedTo))return false;
            LinkedTo->PrimaryFacing.SetCurrent(DirStruct(static_cast<unsigned short>(unsigned(dir)<<8)));
            if(TrackIndex&&raw.CellIndex==TrackIndex)LinkedTo->UnmarkAllOccupationBits(LinkedTo->Location);
            const int face=((unsigned(TurnTrack[TrackNumber].Face)+16)>>5)&7;
            if(nextface>=0&&nextface<8&&face!=nextface&&raw.JumpIndex==TrackIndex&&TrackIndex) {
                const int number=face*8+nextface,nextRaw=TurnTrack[number].NormalTrackStructIndex;
                if(nextRaw&&RawTrack[nextRaw].EntryIndex) {
                    const double oldSpeed=LinkedTo->SpeedPercentage;const auto target=adjacent(HeadToCoord,nextface);
                    auto* cell=MapClass::Instance.GetCellAt(target);
                    const auto move=LinkedTo->IsCellOccupied(cell,static_cast<FacingType>(nextface),LinkedTo->GetCellLevel(),nullptr,true);
                    // Preserve the target's Passive branch, even though it differs
                    // from the usual interpretation of the OpenTS comment.
                    if((move==Move::OK||move==Move::MovingBlock)
                        &&(LinkedTo->WhatAmI()!=AbstractType::Unit||static_cast<UnitClass*>(LinkedTo)->Type->Passive)) {
                        IsOnShortTrack=false;TrackNumber=number;TrackIndex=RawTrack[nextRaw].EntryIndex-1;
                        Stop_Driver();IsDriving=true;LinkedTo->UpdatePosition(PCPType::End);IsDriving=false;
                        if(!active(LinkedTo))return false;
                        if(Start_Driver(target)){LinkedTo->SetSpeedPercentage(oldSpeed);advance(LinkedTo,1);}
                    } else if(move==Move::Cloak)cell->RevealCellObjects();
                    else if(move==Move::ClosedGate)MapClass::Instance.MakeTraversable(LinkedTo,cell->MapCoords);
                    else if(move==Move::Temp)cell->ScatterContent(CoordStruct::Empty,true,true,
                        bridge(cell)&&std::abs(LinkedTo->Location.Z/Unsorted::LevelHeight-static_cast<signed char>(cell->Level))>2);
                }
            }
            ++TrackIndex;
        } else {
            const int d=std::abs(HeadToCoord.X-LinkedTo->Location.X)+std::abs(HeadToCoord.Y-LinkedTo->Location.Y);
            actual+=int((1.0-double(d)/11.0)*7.0);
            LinkedTo->FrozenStill=true;LinkedTo->IsWaitingBlockagePath=false;
            relocate(LinkedTo,HeadToCoord);Stop_Driver();TrackNumber=-1;TrackIndex=0;
            const bool arrived=LinkedTo->Destination&&cell_of(LinkedTo->Location)==cell_of(LinkedTo->Destination->GetDestination(LinkedTo))
                &&std::abs(LinkedTo->GetDestination().Z-DestinationCoord.Z)<2*Unsorted::LevelHeight;
            if(arrived){DestinationCoord=CoordStruct::Empty;Stop_Driver();IsDriving=false;}
            LinkedTo->UpdatePosition(PCPType::End);if(!active(LinkedTo))return true;
            if(arrived) {
                LinkedTo->Destination=nullptr;LinkedTo->PathDirections[0]=-1;
                if(LinkedTo->GetCurrentMission()==Mission::Move&&LinkedTo->EnterIdleMode(false,true))return true;
            }
            break;
        }
    }
    SpeedAccum=actual;
    if(actual>0&&TrackNumber>=0) {
        const auto& point=RawTrack[rawIndex()].TrackPoint[TrackIndex];
        if(point.Point!=Point2D{0,0}||!TrackIndex) {
            int dir=point.Face;const auto p=Smooth_Turn(point.Point,dir);
            const auto old=LinkedTo->Location;
            const CoordStruct step{p.X,p.Y,old.Z},partial{old.X+int((p.X-old.X)*(actual/7.0)),old.Y+int((p.Y-old.Y)*(actual/7.0)),old.Z};
            auto coord=old;
            if(cell_of(partial)!=cell_of(step)&&cell_of(partial)!=cell_of(old)){if(actual>3)coord=step;}
            else coord=partial;
            relocate(LinkedTo,coord,false);
        }
    }
    return false;
}

bool DriveLocomotionClass::Start_Of_Move(bool& stopProcessing,bool retry,bool forceStraight) {
    int facing=LinkedTo->PathDirections[0];auto& map=MapClass::Instance;auto& rules=*RulesClass::Instance;
    auto* type=LinkedTo->GetTechnoType();
    if(!Is_Moving()&&facing==-1) {
        IsTurretLockedDown=false;Stop_Driver();
        if(LinkedTo->GetCurrentMission()==Mission::Move)stopProcessing=LinkedTo->EnterIdleMode(false,true);
        return false;
    }
    if(DestinationCoord==CoordStruct::Empty||LinkedTo->IsBeingWarpedOut()||LinkedTo->IsWarpingIn())return false;
    if(LinkedTo->IsUnderEMP()||LinkedTo->IsParalyzed())return true;
    if(facing!=-1&&LinkedTo->Destination&&(LinkedTo->Destination->AbstractFlags&AbstractFlags::Foot)!=AbstractFlags::None) {
        const int length=distance(LinkedTo->GetCoords(),DestinationCoord)/256;
        if(length<24){LinkedTo->PathDirections[length]=-1;facing=LinkedTo->PathDirections[0];}
    }
    if(facing==-1) {
        if(LinkedTo->PathDelayTimer.GetTimeLeft())return false;
        LinkedTo->PathDelayTimer.Start(int(rules.PathDelay*900.0));
        if(!LinkedTo->UpdatePathfinding(cell_of(DestinationCoord),0,0)) {
            if(!LinkedTo->IsInSameZoneAsCoords(DestinationCoord)){LinkedTo->SetDestination(nullptr,true);return false;}
            if(distance(LinkedTo->GetCoords(),DestinationCoord)<rules.CloseEnough
                &&(LinkedTo->GetCurrentMission()==Mission::Move||LinkedTo->GetCurrentMission()==Mission::Area_Guard)) {
                Stop_Driver();if(Abandon_Navigation())return true;
            } else if(LinkedTo->PathWaitTimes>0)--LinkedTo->PathWaitTimes;
            else {Stop_Driver();if(Abandon_Navigation())return true;LinkedTo->ShouldScanForTarget=false;}
            Stop_Driver();TrackNumber=-1;IsTurretLockedDown=false;return false;
        }
        LinkedTo->PathWaitTimes=10;facing=LinkedTo->PathDirections[0];
    }
    if(facing<0||facing>=8)return false;
    auto dest=adjacent(LinkedTo->Location,facing);auto* cell=map.GetCellAt(dest);
    const int level=static_cast<signed char>(LinkedTo->GetCell()->Level)+(LinkedTo->OnBridge?4:0);
    if(LinkedTo->OnBridge!=bridge(cell))LinkedTo->unknown_bool_68B=true;
    if(!map.MakeTraversable(LinkedTo,cell->MapCoords))return true;
    const auto dir=DirStruct(static_cast<unsigned short>(facing*0x2000));
    if(LinkedTo->PrimaryFacing.Current().Raw!=dir.Raw){Do_Turn(dir);return true;}
    LinkedTo->Mark(MarkType::Up);
    auto move=LinkedTo->IsCellOccupied(cell,static_cast<FacingType>(facing),level,nullptr,true);
    LinkedTo->Mark(MarkType::Down);
    const auto tryAgain=[&](bool straight=false){LinkedTo->PathDirections[0]=-1;LinkedTo->PathDelayTimer.Start(0);return Start_Of_Move(stopProcessing,false,straight);};
    const auto blocked=[&](CellClass* tile,Move reason) {
        if(reason==Move::ClosedGate)map.MakeTraversable(LinkedTo,tile->MapCoords);
        if(reason==Move::Cloak)tile->RevealCellObjects();
        if(reason==Move::Temp&&!type->IsTrain) {
            if(distance(LinkedTo->Location,DestinationCoord)>=rules.CloseEnough
                ||std::abs(DestinationCoord.Z-LinkedTo->Location.Z)>=2*Unsorted::LevelHeight
                ||LinkedTo->GetCell()->LandType==LandType::Tunnel)
                tile->ScatterContent(CoordStruct::Empty,true,true,bridge(tile)&&std::abs(LinkedTo->Location.Z/Unsorted::LevelHeight-static_cast<signed char>(tile->Level))>2);
            else {Stop_Driver();return Abandon_Navigation();}
        }
        Stop_Driver();return false;
    };
    if(move!=Move::OK) {
        if(retry&&(move==Move::Temp||move==Move::Cloak||move==Move::No||move==Move::Destroyable||move==Move::FriendlyDestroyable))return tryAgain();
        if(blocked(cell,move))return true;
        if(move==Move::MovingBlock) {
            if(!LinkedTo->IsWaitingBlockagePath){LinkedTo->IsWaitingBlockagePath=true;LinkedTo->BlockagePathTimer.Start(rules.BlockagePathDelay);}
            if(!LinkedTo->PathDelayTimer.GetTimeLeft()) {
                const bool around=LinkedTo->IsWaitingBlockagePath&&!LinkedTo->BlockagePathTimer.GetTimeLeft();
                if(LinkedTo->UpdatePathfinding(cell_of(DestinationCoord),0,around?2:1)||LinkedTo->IsInSameZoneAsCoords(DestinationCoord)) {
                    LinkedTo->PathDelayTimer.Start(int(rules.PathDelay*900.0));return true;
                }
                LinkedTo->SetDestination(nullptr,true);return false;
            }
        }
        if(move==Move::No||move==Move::Cloak)return Abandon_Navigation();
        LinkedTo->ShouldScanForTarget=false;TrackNumber=-1;return true;
    }
    const bool groundLevel=std::abs(level-static_cast<signed char>(cell->Level))<2;
    const auto land=groundLevel?cell->LandType:LandType::Road;
    // YR 0x4B2630 advances the entry level to the first cell before checking
    // the second cell of a turn. Keeping the starting level rejects a legal
    // two-cell ascent as a two-level cliff and abandons the movement order.
    const int nextLevel=groundLevel?static_cast<signed char>(cell->Level):level;
    double speed=std::min(1.0,double(GroundType::Array[int(land)].Cost[int(type->SpeedType)]));
    const int height=map.GetCellFloorHeight(cell->GetCellCoords()),oldHeight=map.GetCellFloorHeight(LinkedTo->Location);
    if(height>oldHeight)speed*=type->SpeedType==SpeedType::Track?rules.TrackedUphill:rules.WheeledUphill;
    else if(height<oldHeight)speed*=type->SpeedType==SpeedType::Track?rules.TrackedDownhill:rules.WheeledDownhill;
    if(speed==0)speed=0.5;
    if(LinkedTo->GetHealthPercentage()<=rules.ConditionYellow)speed*=0.75;
    if(TrackNumber<64)movementspeed_50=speed;else LinkedTo->SetSpeedPercentage(speed);
    int next=LinkedTo->PathDirections[1];
    if(next==-1&&distance(LinkedTo->GetCoords(),DestinationCoord)>512) {
        if(!LinkedTo->UpdatePathfinding(cell_of(DestinationCoord),type->IsTrain?1:0,0)
            &&!LinkedTo->IsInSameZoneAsCoords(DestinationCoord))LinkedTo->SetDestination(nullptr,true);
        next=LinkedTo->PathDirections[1];
    }
    if(next<0||next>=8||forceStraight)next=facing;
    IsRocking=crushable(cell)||crushable(cell->GetNeighbourCell(static_cast<FacingType>(next)));
    if(IsRocking)next=facing;
    IsOnShortTrack=false;TrackNumber=facing*8+next;
    if(!TurnTrack[TrackNumber].NormalTrackStructIndex)TrackNumber=facing*9;
    if(TurnTrack[TrackNumber].Flag&8) {
        Move nextMove=Move::No;
        if(cell->CollectCrate(LinkedTo)||LinkedTo->InLimbo) {
            if(!active(LinkedTo))return false;
            dest=adjacent(dest,next);cell=map.GetCellAt(dest);
            nextMove=LinkedTo->IsCellOccupied(cell,static_cast<FacingType>(next),nextLevel,nullptr,true);
        }
        if(nextMove!=Move::OK) {
            if(nextMove==Move::MovingBlock)return Start_Of_Move(stopProcessing,retry,true);
            if(retry&&(nextMove==Move::Temp||nextMove==Move::Cloak||nextMove==Move::No))return tryAgain();
            if(blocked(cell,nextMove))return true;
            if(nextMove==Move::No||nextMove==Move::Cloak)return Abandon_Navigation();
            LinkedTo->PathDirections[0]=-1;TrackNumber=-1;dest=CoordStruct::Empty;
        } else {advance(LinkedTo,2);LinkedTo->unknown_bool_68B=true;}
    } else advance(LinkedTo,1);
    LinkedTo->CurrentMapCoords=cell_of(dest);LinkedTo->ShouldScanForTarget=false;TrackIndex=0;
    if(!Start_Driver(dest)){TrackNumber=-1;LinkedTo->PathDirections[0]=-1;LinkedTo->SetSpeedPercentage(0);}
    return false;
}
