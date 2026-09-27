// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 astar.cpp: step costs, predicted paths and moving blockers.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR calibration: 0x429780 / 0x429830 / 0x42ACF0 / 0x42B080.
#include "yrpp/AStarClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TubeClass.h"
#include <cstdlib>

namespace {
constexpr unsigned Bridge = 0x100u, Predicted = 0x40000u;
int height(const CellClass* cell) { return static_cast<signed char>(cell->Level); }
bool on_bridge(const CellClass* cell) { return (static_cast<unsigned>(cell->Flags) & Bridge) != 0; }
int facing(const FootClass* foot) { return static_cast<int>(foot->PrimaryFacing.Current().GetFacing<8>()); }
CellStruct adjacent(CellStruct cell, int direction) {
    // Original AdjacentCell (0x0089F688), initialized by 0x0049F2F0.
    constexpr CellStruct offsets[]{{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};
    const auto offset=offsets[direction&7];
    return {static_cast<short>(cell.X+offset.X),static_cast<short>(cell.Y+offset.Y)};
}
void toggle(CellClass* cell) { cell->Flags=static_cast<CellFlags>(static_cast<unsigned>(cell->Flags)^Predicted); }
}

CellStruct* YRPP_FASTCALL AStarClass::FollowPath(CellStruct* output, const CellStruct* start, int count, const int* directions) {
    auto cell=*start;
    for (int i=0;i<count;++i) {
        if (directions[i]==8) {
            const auto* tube=MapClass::Instance.GetCellAt(cell)->GetTunnel();
            cell=tube?tube->ExitCell:CellStruct{0,0};
        } else cell=adjacent(cell,directions[i]);
    }
    *output=cell;return output;
}

double AStarClass::GetMovementCost(CellClass** from, CellClass** to, bool bridge, Move move, FootClass*) {
    constexpr float costs[]{1,1000,1,1,60,20,8,10000};
    float cost=costs[static_cast<int>(move)];
    auto* destination=*to;
    if (move==Move::MovingBlock) {
        auto* occupier=bridge?destination->AltObject:destination->FirstObject;
        bool clear=false;
        if (!FindMode) {
            for(int depth=0;depth<10;++depth) {
                if (!occupier) {clear=true;break;}
                if (!(static_cast<unsigned>(occupier->AbstractFlags)&4u)) break;
                auto* blocker=static_cast<FootClass*>(occupier);
                int direction=blocker->SpeedPercentage==0.0?blocker->PathDirections[0]:facing(blocker);
                if (blocker->SpeedPercentage==0.0 && direction==-1) {clear=true;break;}
                // Unlike this OpenTS revision, YR masks the tunnel pseudo-facing
                // to North in THIS traffic lookahead; FollowPath still follows tubes.
                auto* cell=MapClass::Instance.GetCellAt(adjacent(blocker->GetMapCoords(),direction));
                const bool alt=on_bridge(cell) && (blocker->OnBridge || height(blocker->GetCell())-height(cell)>2);
                occupier=alt?cell->AltObject:cell->FirstObject;
            }
        }
        if (!clear || FindMode) cost=4;
        if (FindMode==2) cost=1000;
    }
    if (static_cast<unsigned>(destination->Flags)&Predicted) cost*=4;
    if (!bridge || !FindBridgeDir) return cost;
    constexpr int directions[3][3]{{7,0,1},{6,-1,2},{5,4,3}};
    constexpr int across[]{-2,-2,0,1,1,1,0,-2};
    constexpr int along[]{0,-1024,-1024,-1024,0,512,512,512};
    const int dx=destination->MapCoords.X-(*from)->MapCoords.X,dy=destination->MapCoords.Y-(*from)->MapCoords.Y;
    const int direction=directions[dy+1][dx+1];
    const auto* offsets=(static_cast<unsigned>(destination->Flags)&0x800u)?along:across;
    if (!on_bridge(to[offsets[direction]])) return 10.0*cost;
    return (on_bridge(to[offsets[(direction-4)&7]])?2.0:1.0)*cost;
}

FootClass* YRPP_STDCALL AStarClass::FindMovingBlocker(const CellStruct& cell, int level) {
    const CoordStruct coord{cell.X*256+128,cell.Y*256+128,level*Unsorted::LevelHeight};
    for (int y=-2;y<3;++y) for(int x=-2;x<3;++x) {
        auto* neighbour=MapClass::Instance.GetCellAt(CellStruct{static_cast<short>(cell.X+x),static_cast<short>(cell.Y+y)});
        const bool alt=on_bridge(neighbour) && std::abs(height(neighbour)-level)>2;
        for(auto* object=alt?neighbour->AltObject:neighbour->FirstObject;object;object=object->NextObject) {
            if (!(static_cast<unsigned>(object->AbstractFlags)&4u)) continue;
            auto* foot=static_cast<FootClass*>(object);
            ILocomotion* driver=foot->Locomotor;
            if (!driver) std::abort(); // Original E_POINTER; no COM exception crosses the native boundary.
            if (driver->Is_Moving_Here(coord)) return foot;
        }
    }
    return nullptr;
}

void AStarClass::ApplyPathCollisionAvoidance(FootClass* foot) {
    if (!CanFindPath) return; // Original flag is IsAvoidPathCollision, not path success.
    const auto current=foot->GetMapCoords();
    auto* current_cell=MapClass::Instance.GetCellAt(current);
    auto* next=MapClass::Instance.GetCellAt(adjacent(current,facing(foot)));
    const bool alt=on_bridge(next) && (std::abs(height(current_cell)-height(next))>3 || foot->OnBridge);
    auto* blocker=alt?next->AltObject:next->FirstObject;
    if (!blocker) blocker=FindMovingBlocker(next->MapCoords,height(next)+(alt?4:0));
    const auto* type=foot->GetTechnoType();
    bool marked=false;
    for (;blocker;blocker=blocker->NextObject) {
        const auto kind=blocker->WhatAmI();
        if (kind!=AbstractType::Unit && kind!=AbstractType::Infantry) continue;
        auto* mobile=static_cast<FootClass*>(blocker);
        auto cell=mobile->CurrentMapCoords;
        const auto* blocker_type=mobile->GetTechnoType();
        if (FindMode!=2 && !(type!=blocker_type && type->Speed>blocker_type->Speed && MapClass::Instance.IsWithinUsableArea(cell,true))) continue;
        if (mobile->PathDirections[0]==-1 || mobile->PathDirections[1]==-1
            || (kind==AbstractType::Infantry && mobile->PathDirections[2]==-1)) continue;
        marked=true;
        for (int i=0;i<24 && mobile->PathDirections[i]!=-1;++i) {
            FollowPath(&cell,&cell,1,mobile->PathDirections+i);
            toggle(MapClass::Instance.GetCellAt(cell));
        }
    }
    if (!marked && FindMode==1) {FindMode=0;return;}
    for (int x=-2;x<3;++x) for(int y=-2;y<3;++y) {
        auto* cell=MapClass::Instance.GetCellAt(CellStruct{static_cast<short>(next->MapCoords.X+x),static_cast<short>(next->MapCoords.Y+y)});
        if (static_cast<unsigned char>(cell->OccupationFlags) && cell->MapCoords!=current) toggle(cell);
    }
    toggle(next);
}
