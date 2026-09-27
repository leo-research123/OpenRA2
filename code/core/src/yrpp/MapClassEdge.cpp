// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 display.cpp Calculated_Cell; YR 0x4AA440/0x4AAB30.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <vector>

CellStruct* MapClass::PickCellOnEdge(CellStruct& output,::Edge edge,const CellStruct& current,const CellStruct& fallback,
        SpeedType speed,bool,MovementZone) const {
    output=CellStruct::Empty;
    // The aircraft branch of Good_Reinforcement_Cell returns immediately.
    // Ground zone/occupation searches remain unsupported by this native entry.
    if(speed!=SpeedType::Winged||VisibleRect.Width<=0||VisibleRect.Height<=0)return &output;
    const auto toCell=[&](int x,int y){
        x+=VisibleRect.X;y+=VisibleRect.Y;
        return CellStruct{short(x+((y+1)>>1)),short((y>>1)+MapRect.Width-x)};
    };
    const auto origin=current!=CellStruct::Empty?current:fallback;
    int direction=int(edge);if(direction==-1)direction=0;
    int x=0,y=0;bool vertical=false;
    auto& random=ScenarioClass::Instance->Random;
    if(origin==CellStruct::Empty){
        switch(direction){
        case 1:vertical=true;x=VisibleRect.Width;y=random.RandomRanged(1,2*VisibleRect.Height)-1;break;
        case 2:y=2*VisibleRect.Height+2;x=random.RandomRanged(1,VisibleRect.Width)-1;break;
        case 3:vertical=true;y=random.RandomRanged(0,2*VisibleRect.Height)-1;break;
        default:x=random.RandomRanged(1,VisibleRect.Width)-1;break;
        }
    }else{
        const int px=MapRect.Width/2+((origin.X-origin.Y+(MapRect.Width&1))>>1)-VisibleRect.X;
        const int py=origin.X+origin.Y-MapRect.Width-VisibleRect.Y;
        const int dx=px-VisibleRect.X;
        const int dy=py-2*VisibleRect.Y;
        if(2*std::min(dx,VisibleRect.Width-dx)<std::min(dy,2*VisibleRect.Height-dy)){
            vertical=true;direction=dx<VisibleRect.Width/2?3:1;
            x=direction==3?-1:VisibleRect.Width;y=py-VisibleRect.Y;
        }else{direction=dy<VisibleRect.Height?0:2;x=dx;y=direction==0?-1:2*VisibleRect.Height;}
    }
    output=toCell(1,VisibleRect.Width/2);
    if(vertical){
        for(int i=0;i<2*VisibleRect.Height;++i){const auto at=toCell(x,(y+i)%(2*VisibleRect.Height));
            if(!IsWithinUsableArea(at,true)){output=at;break;}}
    }else if(direction==2){
        std::vector<CellStruct> candidates;
        for(int ix=0;ix<VisibleRect.Width;++ix)for(int iy=0;iy<15;++iy){
            const auto at=toCell(ix,2*VisibleRect.Height+iy);
            if(!IsWithinUsableArea(at,true)){candidates.push_back(at);break;}
        }
        output=CellStruct::Empty;
        if(!candidates.empty()){
            if(origin==CellStruct::Empty)output=candidates[random.RandomRanged(0,int(candidates.size())-1)];
            else{int closest=-1;for(auto at:candidates){
                const int dx=short(origin.X-at.X),dy=short(origin.Y-at.Y);
                const int distance=game::x87_integer(Math::sqrt(double(dx)*dx+double(dy)*dy));
                if(closest==-1||distance<closest){closest=distance;output=at;}
            }}
        }
    }else{
        for(int i=0;i<VisibleRect.Width;++i){const auto at=toCell((x+i)%VisibleRect.Width,y);
            if(!IsWithinUsableArea(at,true)){output=at;break;}}
    }
    return &output;
}
