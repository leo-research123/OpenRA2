// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 astar.cpp path straightening; YR 0x42B210..0x42BE20.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/AStarClass.h"
#include "yrpp/MapClass.h"
#include <algorithm>
#include <cstdlib>
namespace {
CellStruct adjacent(CellStruct cell,int direction) {
    constexpr CellStruct offsets[]{{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};
    const auto offset=offsets[direction&7];return {short(cell.X+offset.X),short(cell.Y+offset.Y)};
}
CellStruct subtract(CellStruct a,CellStruct b){return {short(a.X-b.X),short(a.Y-b.Y)};}
int radius(CellStruct cell){return std::max(std::abs(int(cell.X)),std::abs(int(cell.Y)));}
int next_level(int level,const CellClass* cell) {
    const int height=static_cast<signed char>(cell->Level);
    return level-height==4 && (static_cast<unsigned>(cell->Flags)&0x100u)?height+4:height;
}
bool predicted(const CellClass* cell){return (static_cast<unsigned>(cell->Flags)&0x40000u)!=0;}
}

void AStarClass::CutCorners(PathFinderData* path,FootClass* foot) {
    const int maximum=path->PathLength-1;
    auto* moves=path->Directions;auto* levels=path->Levels;
    int first_start=0,second_start=0,first_length=0,second_length=0,first_direction=-1,second_direction=-1;
    CellStruct start=path->StartCell,cursor=start;bool corner=false;
    while(first_start+first_length<maximum && second_start+second_length<maximum) {
        if(!corner) {
            const int direction=moves[first_start+first_length],turn=(direction-first_direction)&7;
            if(direction==first_direction)++first_length;
            else if((turn==2 || turn==6) && first_direction!=-1 && first_direction!=8 && direction!=8) {
                corner=true;second_direction=direction;second_length=1;second_start=first_start+first_length;
            } else {first_start+=first_length;first_length=1;first_direction=(direction&1)?direction:-1;start=cursor;}
            FollowPath(&cursor,&cursor,1,&direction);
        } else if(moves[second_start+second_length]==second_direction)++second_length;
        else {
            first_start+=TryDiagonalShortcut(foot,moves+first_start,levels+first_start,first_length,second_length,start);
            first_length=1;corner=false;cursor=adjacent(start,moves[first_start]);first_direction=moves[first_start];
        }
    }
    if(corner)TryDiagonalShortcut(foot,moves+first_start,levels+first_start,first_length,second_length,start);
}

int YRPP_STDCALL AStarClass::TryDiagonalShortcut(FootClass* foot,int* moves,int* levels,int first,int second,CellStruct& cell) {
    const int first_direction=moves[0],second_direction=moves[first];
    int diagonal=(first_direction+second_direction)>>1;
    if(diagonal+1!=first_direction && diagonal+1!=second_direction)diagonal=0;
    if(first_direction==8 || second_direction==8){FollowPath(&cell,&cell,first+second,moves);return first+second;}
    auto start=cell;second=std::min(first,second);
    if(second<first)FollowPath(&start,&start,first-second,moves);
    const double avoidance=foot->ThreatAvoidanceValue();
    while(second>0) {
        int remaining=2*second;bool blocked=false;
        auto scan=adjacent(start,diagonal);auto* scan_cell=MapClass::Instance.GetCellAt(scan);
        int level=levels[first-second];
        while(!blocked && remaining>0) {
            blocked=foot->IsCellOccupied(scan_cell,static_cast<FacingType>(diagonal),level,nullptr,true)!=Move::OK
                || predicted(scan_cell) || MapClass::Instance.GetThreatPosed(start,foot->Owner)*avoidance>=1.0;
            --remaining;scan=adjacent(scan,diagonal);scan_cell=MapClass::Instance.GetCellAt(scan);level=next_level(level,scan_cell);
        }
        if(!blocked){std::fill_n(moves+first-second,2*second,diagonal);FollowPath(&cell,&cell,first-second,moves);return first-second;}
        start=adjacent(start,first_direction);--second;
    }
    FollowPath(&cell,&cell,first,moves);return first;
}

void AStarClass::OptimizeMoves(PathFinderData* path,FootClass* foot) {
    const int maximum=path->PathLength-1;auto* moves=path->Directions;auto* levels=path->Levels;
    auto cursor=path->StartCell;CellStruct offset{0,0},candidate{0,0},last_turn{0,0};
    int scan=0,previous_turn=0,last_turn_index=0,extent=0,max_x=0,max_y=0;
    while(scan<maximum && scan<20) {
        const int move=moves[scan];
        if(move==8){cursor=adjacent(cursor,0);candidate=offset=last_turn={0,0};++scan;
            previous_turn=last_turn_index=scan;extent=max_x=max_y=0;continue;}
        if(move==-2){++scan;continue;}
        const auto next_offset=adjacent(offset,move),next_candidate=adjacent(candidate,move);
        if(std::abs(int(next_candidate.X))>=max_x && std::abs(int(next_candidate.Y))>=max_y) {
            candidate=next_candidate;max_x=std::abs(int(candidate.X));max_y=std::abs(int(candidate.Y));
            cursor=adjacent(cursor,move);const int distance=radius(next_offset);
            if(extent<distance)extent=distance;
            else {int splice;auto start=cursor;SplicePath(moves,scan,previous_turn,splice,start);
                PlotStraightLine(moves+splice,scan-splice+1,start,subtract(cursor,start),foot,levels[splice],false);}
            offset=next_offset;++scan;
        } else {
            if(last_turn!=CellStruct{0,0}){previous_turn=last_turn_index;offset=subtract(last_turn,cursor);extent=radius(offset);}
            max_x=max_y=0;candidate={0,0};last_turn=cursor;last_turn_index=scan;
        }
    }
    if(last_turn!=CellStruct{0,0} && scan-last_turn_index-1>radius(subtract(cursor,last_turn))) {
        --scan;int splice;auto start=cursor;SplicePath(moves,scan,last_turn_index,splice,start);
        PlotStraightLine(moves+splice,scan-splice+1,start,subtract(cursor,start),foot,levels[splice],true);
    }
    int length=0;
    for(int index=0;index<maximum && moves[index]!=-1;++index)if(moves[index]!=-2)moves[length++]=moves[index];
    // Original writes one extra terminator. Caller requires PathLength + 1 slots.
    std::fill(moves+length,moves+path->PathLength+1,-1);path->PathLength=length+1;
}

void YRPP_STDCALL AStarClass::SplicePath(int* moves,int start,int end,int& splice,CellStruct& cell) {
    int maximum=0;CellStruct displacement{0,0},base=cell;bool found=false;
    for(int index=start;index>=end;--index) {
        if(moves[index]==-2)continue;
        const int backward=(moves[index]-4)&7;
        displacement=adjacent(displacement,backward);base=adjacent(base,backward);
        const int distance=radius(displacement);
        if(distance>maximum){if(found){splice=index+1;cell=adjacent(base,backward-4);return;}maximum=distance;}
        else found=true;
    }
    splice=end;cell=base;
}

bool YRPP_STDCALL AStarClass::PlotStraightLine(int* moves,int count,const CellStruct& from,const CellStruct& delta,FootClass* foot,int level,bool fearless) {
    int first_direction=delta.X<0?(delta.Y<0?7:5):(delta.Y<0?1:3);
    const int difference=delta.X-delta.Y,sum=delta.X+delta.Y;
    int second_direction=difference>0?(sum>0?2:0):(sum>0?4:6);
    int first=std::min(std::abs(int(delta.X)),std::abs(int(delta.Y))),second=radius(delta)-first;
    const double avoidance=foot->ThreatAvoidanceValue();const bool check_threat=avoidance>0.00001;
    for(int attempt=0;attempt<2;++attempt) {
        bool blocked=false;int threats=0,current_level=level;auto cell=from;
        if(first)for(int leg=0;leg<2 && !blocked;++leg) {
            const int length=leg?second:first,direction=leg?second_direction:first_direction;
            for(int i=0;i<length && !blocked;++i) {
                cell=adjacent(cell,direction);auto* tile=MapClass::Instance.GetCellAt(cell);
                if(check_threat && MapClass::Instance.GetThreatPosed(cell,foot->Owner)*avoidance>=0.01)++threats;
                blocked=foot->IsCellOccupied(tile,static_cast<FacingType>(direction),current_level,nullptr,true)!=Move::OK
                    || predicted(tile) || threats>3 || (!fearless && threats>0);
                current_level=next_level(current_level,tile);
            }
        }
        if(!first || blocked){std::swap(first_direction,second_direction);std::swap(first,second);continue;}
        std::fill_n(moves,first,first_direction);std::fill_n(moves+first,second,second_direction);
        if(count>first+second)std::fill_n(moves+first+second,count-first-second,-2);
        return true;
    }
    return false;
}
