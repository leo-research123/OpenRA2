// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 walk.cpp Movement_AI; calibrated to YR 0x75AEC0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/WalkLocomotionClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/VocClass.h"
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {
CellStruct cell_of(const CoordStruct& coord) { return {short(coord.X/256),short(coord.Y/256)}; }
CoordStruct cell_center(CellStruct cell) { return {cell.X*256+128,cell.Y*256+128,0}; }
bool under_bridge(const CellClass* cell) { return (static_cast<unsigned>(cell->Flags)&0x100u)!=0; }
int distance(const CoordStruct& a,const CoordStruct& b,bool xy=false) {
    const double x=double(a.X)-b.X,y=double(a.Y)-b.Y,z=xy?0:double(a.Z)-b.Z;
    return int(Math::sqrt(x*x+y*y+z*z));
}
DirStruct direction(const CoordStruct& from,const CoordStruct& to) {
    return DirStruct(int((Math::atan2(double(from.Y)-to.Y,double(to.X)-from.X)
        -1.5707963267948966)*-10430.060040584269));
}
void advance_path(FootClass* foot) {
    std::memmove(foot->PathDirections,foot->PathDirections+1,23*sizeof(int));
    foot->PathDirections[23]=-1;
}
}

bool YRPP_STDCALL WalkLocomotionClass::Process() {
    InProcessing=true;
    Movement_AI(true);
    InProcessing=false;
    return Is_Moving();
}

void WalkLocomotionClass::Movement_AI(bool firstPass) {
    auto& map=MapClass::Instance;
    auto& rules=*RulesClass::Instance;
    if(HeadToCoord!=CoordStruct::Empty) {
        IsReallyMoving=true;
        if(distance(LinkedTo->GetCoords(),HeadToCoord,true)<17) {
            LinkedTo->Mark(MarkType::Up);
            if(LinkedTo->PathDirections[0]==-1)LinkedTo->PathDirections[1]=-1;
            advance_path(LinkedTo);
            LinkedTo->SetLocation(HeadToCoord);
            LinkedTo->CurrentMapCoords=cell_of(HeadToCoord);
            LinkedTo->SetHeight(0);
            LinkedTo->IsWaitingBlockagePath=false;
            Mark_Head_To(CoordStruct::Empty);
            if(LinkedTo->PathDirections[0]==-1)LinkedTo->SetSpeedPercentage(0.0);
            LinkedTo->UpdatePosition(PCPType::End);
            // This callback is part of the original Infantry execution chain;
            // it must not be replaced by a host-side arrival notification.
            if(LinkedTo->IsAlive && !LinkedTo->InLimbo && !LinkedTo->IsFallingDown) {
                if(DestinationCoord==CoordStruct::Empty
                    || (cell_of(LinkedTo->GetDestination())==cell_of(DestinationCoord)
                        && std::abs(LinkedTo->GetDestination().Z-DestinationCoord.Z)<2*Unsorted::LevelHeight)) {
                    LinkedTo->SetDestination(nullptr,true);
                    LinkedTo->SetSpeedPercentage(0.0);
                    HeadToCoord=CoordStruct::Empty;
                    Stop_Moving();
                    IsReallyMoving=false;
                }
                LinkedTo->Mark(MarkType::Down);
                LinkedTo->ShouldScanForTarget=false;
            }
            return;
        }
        if(LinkedTo->IsUnderEMP()) {
            IsReallyMoving=false;
            LinkedTo->ShouldScanForTarget=false;
            return;
        }
        LinkedTo->SetSpeedPercentage(1.0);
        const int speed=LinkedTo->GetCurrentSpeed();
        LinkedTo->IsWaitingBlockagePath=false;
        const auto facing=direction(LinkedTo->GetCoords(),HeadToCoord);
        Do_Turn(facing);
        const auto oldCell=LinkedTo->GetMapCoords();
        const auto here=LinkedTo->Location;
        // 0x75C067 is MOVSX ECX,SI: use the SIGNED 16-bit direction.
        const double angle=(int(std::bit_cast<std::int16_t>(facing.Raw))-0x3FFF)*-0.00009587672516830327;
        const CoordStruct next{int(double(here.X)+Math::cos(angle)*speed),
            int(double(here.Y)-Math::sin(angle)*speed),here.Z};
        const auto newCell=cell_of(next);
        if(oldCell!=newCell) {
            LinkedTo->Mark(MarkType::Up);
            LinkedTo->SetLocation(next);
            const auto* oldTile=map.GetCellAt(oldCell);
            const auto* newTile=map.GetCellAt(newCell);
            if(static_cast<signed char>(newTile->Level)==static_cast<signed char>(oldTile->Level)-4 && under_bridge(newTile))
                LinkedTo->OnBridge=true;
            if(!under_bridge(newTile) && under_bridge(oldTile))LinkedTo->OnBridge=false;
            LinkedTo->SetHeight(0);
            LinkedTo->Mark(MarkType::Down);
            map.GetCellAt(LinkedTo->Location)->ActivateVeins();
        } else {
            const bool down=LinkedTo->IsOnMap;
            LinkedTo->IsOnMap=false;
            LinkedTo->SetLocation(next);
            LinkedTo->SetHeight(0);
            LinkedTo->IsOnMap=down;
        }
        LinkedTo->ShouldScanForTarget=false;
        return;
    }
    if(DestinationCoord==CoordStruct::Empty) {
        if(LinkedTo->SpeedPercentage>0.0)LinkedTo->SetSpeedPercentage(0.0);
        LinkedTo->ShouldScanForTarget=false;
        return;
    }
    if(LinkedTo->PathDirections[0]==-1) {
        if(LinkedTo->PathDelayTimer.GetTimeLeft()) { LinkedTo->vt_entry_548();return; }
        LinkedTo->PathDelayTimer.Start(int(rules.PathDelay*900.0));
        if(!LinkedTo->UpdatePathfinding(cell_of(DestinationCoord),0,0)) {
            IsReallyMoving=false;
            if(!LinkedTo->IsInSameZoneAsCoords(DestinationCoord)) { LinkedTo->SetDestination(nullptr,true);return; }
            LinkedTo->vt_entry_4F4();
            if(distance(LinkedTo->GetCoords(),DestinationCoord)<rules.CloseEnough && !LinkedTo->IsTether) {
                LinkedTo->SetDestination(nullptr,true);
            } else if(LinkedTo->PathWaitTimes) {
                --LinkedTo->PathWaitTimes;
            } else {
                if(LinkedTo->ShouldScanForTarget)VocClass::PlayGlobal(rules.ScoldSound,0x2000,1.0f);
                LinkedTo->ShouldScanForTarget=false;
                const auto destination=cell_of(DestinationCoord);
                if(LinkedTo->IsInPlayfield && !MapClass::IsSameCellZone(cell_of(LinkedTo->GetDestination()),destination,
                    LinkedTo->GetTechnoType()->MovementZone,false,under_bridge(map.GetCellAt(destination)),LinkedTo->vt_entry_320()))
                    LinkedTo->SetDestination(nullptr,true);
                if(LinkedTo->Target) {
                    const auto target=cell_of(LinkedTo->Target->GetDestination(LinkedTo));
                    // YR 0x75B27E passes ESI (the Foot pointer), not OnBridge.
                    // IsSameCellZone consumes its low byte; preserve that quirk.
                    const bool fromBridge=(reinterpret_cast<std::uintptr_t>(LinkedTo)&0xFFu)!=0;
                    if(LinkedTo->IsInPlayfield && LinkedTo->Target && !MapClass::IsSameCellZone(
                        cell_of(LinkedTo->GetDestination(LinkedTo)),target,LinkedTo->GetTechnoType()->MovementZone,
                        fromBridge,under_bridge(map.GetCellAt(target)),LinkedTo->vt_entry_320()))LinkedTo->SetTarget(nullptr);
                }
            }
            LinkedTo->SetSpeedPercentage(0.0);
            Stop_Moving();
            return;
        }
        LinkedTo->PathWaitTimes=10;
        if(LinkedTo->vt_entry_4F8())return;
    }
    const int facing=LinkedTo->PathDirections[0];
    if(facing==8) {
        const int index=map.GetCellAt(LinkedTo->Location)->TubeIndex;
        if(index>=0 && index<TubeClass::Array.Count) {
            LinkedTo->Mark(MarkType::Up);
            LinkedTo->UnmarkAllOccupationBits(LinkedTo->Location);
            const auto* tube=TubeClass::Array[index];
            HeadToCoord=cell_center(tube->ExitCell);
            advance_path(LinkedTo);
            LinkedTo->TubeIndex=static_cast<signed char>(index);
            LinkedTo->TubeFaceIndex=0;
            const auto enter=cell_center(tube->EnterCell);
            const auto visual=map.GetCellAt(tube->EnterCell)->GetNeighbourCell(static_cast<FacingType>(tube->Faces[0]&7))->GetCellCoords();
            LinkedTo->CurrentTunnelCoords=LinkedTo->Location+(visual-enter);
            const int floor=map.GetCellFloorHeight(LinkedTo->Location);
            LinkedTo->CurrentTunnelCoords.Z=floor+(map.GetCellFloorHeight(cell_center(tube->ExitCell))-floor)/tube->FaceCount;
            return;
        }
        LinkedTo->PathDirections[0]=-1;
        HeadToCoord=CoordStruct::Empty;
        Stop_Moving();
        LinkedTo->SetDestination(nullptr,true);
        return;
    }
    const auto offset=Unsorted::AdjacentCell[static_cast<unsigned>(facing)&7u];
    const CoordStruct coord{LinkedTo->Location.X+offset.X*256,LinkedTo->Location.Y+offset.Y*256,LinkedTo->Location.Z};
    auto* cell=map.GetCellAt(coord);
    if(LinkedTo->OnBridge!=under_bridge(cell))LinkedTo->unknown_bool_68B=true;
    const auto move=LinkedTo->IsCellOccupied(cell,static_cast<FacingType>(facing),LinkedTo->GetCellLevel(),nullptr,true);
    if(move!=Move::OK) {
        IsReallyMoving=false;
        LinkedTo->vt_entry_548();
        const auto retry=[&] {
            LinkedTo->PathDirections[0]=-1;HeadToCoord=CoordStruct::Empty;
            LinkedTo->PathDelayTimer.Start(0);Movement_AI(false);
        };
        if(move==Move::Temp) {
            if(firstPass)retry();
            else if(distance(LinkedTo->GetCoords(),DestinationCoord)<rules.CloseEnough && !LinkedTo->HasAnyLink()
                && std::abs(DestinationCoord.Z-LinkedTo->Location.Z)<2*Unsorted::LevelHeight
                && map.GetCellAt(LinkedTo->Location)->LandType!=LandType::Tunnel) {
                HeadToCoord=CoordStruct::Empty;Stop_Moving();LinkedTo->SetSpeedPercentage(0.0);
                LinkedTo->SetDestination(nullptr,true);LinkedTo->PathDirections[0]=-1;
            } else cell->ScatterContent(CoordStruct::Empty,true,true,
                under_bridge(cell) && std::abs(LinkedTo->Location.Z/Unsorted::LevelHeight-static_cast<signed char>(cell->Level))>2);
            return;
        }
        if(move==Move::MovingBlock) {
            if(!LinkedTo->IsWaitingBlockagePath) {
                LinkedTo->IsWaitingBlockagePath=true;LinkedTo->BlockagePathTimer.Start(rules.BlockagePathDelay);
            }
            if(LinkedTo->PathDelayTimer.GetTimeLeft())return;
            const bool blocked=LinkedTo->IsWaitingBlockagePath && !LinkedTo->BlockagePathTimer.GetTimeLeft();
            const bool found=LinkedTo->UpdatePathfinding(cell_of(DestinationCoord),0,blocked?2:1);
            LinkedTo->PathDelayTimer.Start(int(rules.PathDelay*900.0));
            if(found)LinkedTo->vt_entry_4F8();
            else if(LinkedTo->IsInSameZoneAsCoords(DestinationCoord))LinkedTo->vt_entry_4F4();
            else LinkedTo->SetDestination(nullptr,true);
            return;
        }
        if(move==Move::ClosedGate) { map.MakeTraversable(LinkedTo,cell->MapCoords);LinkedTo->PathWaitTimes=10;return; }
        if(move==Move::Destroyable || move==Move::FriendlyDestroyable) {
            auto* object=cell->GetSomeObject({0,0},false);
            if(firstPass) { retry();return; }
            if(object) {
                if(!LinkedTo->Owner->IsAlliedWith(object))LinkedTo->Override_Mission(Mission::Attack,object,nullptr);
            } else if(cell->OverlayTypeIndex!=-1 && OverlayTypeClass::Array[cell->OverlayTypeIndex]->Wall)
                LinkedTo->Override_Mission(Mission::Attack,cell,nullptr);
        }
        if(move==Move::Cloak)cell->RevealCellObjects();
        else if(move!=Move::No) {
            LinkedTo->PathDirections[0]=-1;LinkedTo->SetSpeedPercentage(0.0);Stop_Moving();
            LinkedTo->ShouldScanForTarget=false;return;
        }
        if(firstPass)retry();
        return;
    }
    if(Mark_Head_To(coord)) {
        IsReallyMoving=true;
        if(LinkedTo->IsAlive) {
            Do_Turn(direction(LinkedTo->GetCoords(),HeadToCoord));
            LinkedTo->SetSpeedPercentage(1.0);LinkedTo->ShouldScanForTarget=false;
        }
    } else { LinkedTo->SetSpeedPercentage(0.0);LinkedTo->ShouldScanForTarget=false; }
}
