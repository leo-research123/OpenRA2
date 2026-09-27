// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp: Draw_Health_Bar; YR 0x006F64A0 positions/frames.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Building and infantry bars. Other pip groups remain explicit dependencies.
#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TacticalClass.h"
#include "building_selection.hpp"
#include "RulesClassReaders.hpp"
#include <algorithm>
#include <vector>

void TechnoClass::DrawHealthBar(Point2D* location,RectangleStruct* bounds,bool) const {
    auto* frame=game::building_health_drawing();
    if(!frame)return;
    if(!location||!bounds){frame->status=game::DrawingStatus::invalid_argument;return;}
    if(WhatAmI()==AbstractType::Infantry||WhatAmI()==AbstractType::Unit||WhatAmI()==AbstractType::Aircraft){
        const bool infantry=WhatAmI()==AbstractType::Infantry;
        const auto* type=GetTechnoType();if(!type || type->Strength<=0 || Health<=0)return;
        if(!frame->pips || !frame->palette || (IsSelected && !frame->pip_border)){frame->status=game::DrawingStatus::unavailable;return;}
        const auto shape=[&](SHPStruct* image,int index,Point2D at,unsigned flags){
            game::ShapeDrawingRequest r;r.target=frame->drawing.target;r.palette=frame->palette;r.image=image;
            r.frame=index;r.position=at;r.clip=*bounds;r.flags=flags;r.intensity=1000;
            const auto status=game::submit_type_shape(frame->drawing,r);
            if(status!=game::DrawingStatus::skipped)frame->status=status;
            return status==game::DrawingStatus::drawn || status==game::DrawingStatus::skipped;
        };
        const int delta=type->PixelSelectionBracketDelta;
        // 0x5F76B0 loads PIPBRD.SHP into 0xAC1478. SELECT.SHP is the
        // OpenTS resource and has different geometry despite a valid frame 1.
        if(IsSelected && !shape(frame->pip_border,infantry?1:0,{location->X+(infantry?11:1),location->Y+delta-(infantry?25:26)},0xE00))return;
        const double ratio=GetHealthPercentage();
        const int count=infantry?8:17;
        const int filled=std::clamp(int(std::min(ratio,1.0)*count),1,count);
        const int index=ratio<=RulesClass::Instance->ConditionRed?18:ratio<=RulesClass::Instance->ConditionYellow?17:16;
        for(int i=0;i<filled;++i)if(!shape(frame->pips,index,{location->X-(infantry?5:15)+2*i,location->Y+delta-(infantry?24:25)},0x600))return;
        const auto* player=HouseClass::CurrentPlayer;
        const int country=player && player->Type?player->Type->ArrayIndex:-1;
        if((Owner && player && Owner->IsAlliedWith(player)) || (country>=0 && country<32 && DisplayProductionTo.Contains(country))){
            Point2D anchor{location->X-10,location->Y+10};DrawPipScalePips(&anchor,location,bounds);
        }
        return;
    }
    if(WhatAmI()!=AbstractType::Building){frame->status=game::DrawingStatus::unsupported;return;}
    const auto* type=static_cast<const BuildingClass*>(this)->Type;
    if(!type||type->Strength<=0||Health<=0)return;
    if(!frame->pips||!frame->palette){frame->status=game::DrawingStatus::unavailable;return;}
    const int width=type->GetFoundationWidth(),length=type->GetFoundationHeight(false);
    if(int(type->Foundation)<0||int(type->Foundation)>=22||width<0||width>64||length<0||length>64||type->Height<0||type->Height>256){
        frame->status=game::DrawingStatus::invalid_argument;return;
    }
    // Dimension2 (0x00464AF0), then two original integer projections. SHP
    // crop, Bib and PixelSelectionBracketDelta do not place a building bar.
    const auto left=TacticalClass::CoordsToScreen({-width*128,length*128,type->Height*104});
    const auto back=TacticalClass::CoordsToScreen({-width*128,-length*128,type->Height*104});
    const int count=(left.Y-back.Y)/2;
    const int filled=count>0?std::clamp(static_cast<int>(std::min(GetHealthPercentage(),1.0)*count),1,count):0;
    const int health_frame=IsYellowHP()?2:IsRedHP()?4:1;
    for(int i=0;i<count;++i){
        game::ShapeDrawingRequest request;
        request.target=frame->drawing.target;request.palette=frame->palette;request.image=frame->pips;
        request.frame=i<filled?health_frame:0;
        request.position={location->X+left.X+4*count+3-4*i,location->Y+left.Y+4-2*count+2*i};
        request.clip=*bounds;request.flags=0x600;request.intensity=1000;
        const auto result=game::submit_type_shape(frame->drawing,request);
        if(result!=game::DrawingStatus::drawn&&result!=game::DrawingStatus::skipped){frame->status=result;return;}
        if(result==game::DrawingStatus::drawn)frame->status=result;
    }
    // 0x006F64A0: occupiable structures expose capacity even to opponents.
    // Other buildings require alliance, the original country discovery bit,
    // or the type's PipsDrawForAll flag.
    const auto* player=HouseClass::CurrentPlayer;
    const bool allied=Owner&&player&&Owner->IsAlliedWith(player);
    const int country=player&&player->Type?player->Type->ArrayIndex:-1;
    const bool discovered=country>=0&&country<32&&DisplayProductionTo.Contains(country);
    if(type->CanBeOccupied||type->PipsDrawForAll||allied||discovered){
        const auto ground=TacticalClass::CoordsToScreen({-width*128,length*128,0});
        Point2D anchor{location->X+ground.X,location->Y+ground.Y};
        DrawPipScalePips(&anchor,location,bounds);
    }
}

void TechnoClass::DrawPipScalePips(Point2D* location,Point2D*,RectangleStruct* bounds) const {
    auto* frame=game::building_health_drawing();
    if(!frame)return;
    if(!location||!bounds){frame->status=game::DrawingStatus::invalid_argument;return;}
    if(WhatAmI()!=AbstractType::Building){
        const auto* type=GetTechnoType();
        if(type->PipScale==PipScale::None)return;
        // 0x70A357..0x70A4EC: aircraft and vehicles share the mobile ammo
        // group. PipWrap changes one-frame-per-round into stacked columns.
        if(type->PipScale==PipScale::Ammo){
            if(!frame->mobile_pips||!frame->palette){frame->status=game::DrawingStatus::unavailable;return;}
            const int maximum=type->GetPipMax(),wrap=type->PipWrap;
            const int columns=wrap?wrap:maximum;
            for(int i=0;i<columns;++i){
                int index=13;
                if(wrap){
                    int level=maximum/wrap-1;
                    while(level>=0&&i+wrap*level>=Ammo)--level;
                    index=level<0?14:level+15;
                }else if(i>=Ammo)continue;
                game::ShapeDrawingRequest r;r.target=frame->drawing.target;r.palette=frame->palette;r.image=frame->mobile_pips;
                r.frame=index;r.position={location->X-5+4*i+(WhatAmI()==AbstractType::Infantry?11:0),location->Y-3};
                r.clip=*bounds;r.flags=0x600;r.intensity=1000;
                const auto status=game::submit_type_shape(frame->drawing,r);
                if(status!=game::DrawingStatus::skipped)frame->status=status;
                if(status!=game::DrawingStatus::drawn&&status!=game::DrawingStatus::skipped)return;
            }
            return;
        }
        if(type->PipScale==PipScale::Tiberium&&type->Storage>0){
            if(!frame->mobile_pips||!frame->palette){frame->status=game::DrawingStatus::unavailable;return;}
            const int count=type->GetPipMax();
            // 0x709F78: gems first (frame 5), then ore (frame 2); each
            // quantity is rounded to its own share of the full capacity.
            int ore=rule_integer((double(Tiberium.GetAmount(0))+Tiberium.GetAmount(2)+Tiberium.GetAmount(3))/type->Storage*count+0.5);
            int gems=rule_integer(double(Tiberium.GetAmount(1))/type->Storage*count+0.5);
            for(int i=0;i<count;++i){
                game::ShapeDrawingRequest r;r.target=frame->drawing.target;r.palette=frame->palette;r.image=frame->mobile_pips;
                r.frame=gems>0?(--gems,5):ore>0?(--ore,2):0;
                r.position={location->X-5+4*i+(WhatAmI()==AbstractType::Infantry?11:0),location->Y};
                r.clip=*bounds;r.flags=0x600;r.intensity=1000;
                const auto status=game::submit_type_shape(frame->drawing,r);
                if(status!=game::DrawingStatus::skipped)frame->status=status;
                if(status!=game::DrawingStatus::drawn&&status!=game::DrawingStatus::skipped)return;
            }
            return;
        }
        // OpenTS Draw_Pips, YR 0x709A90: mobile passengers use PIPS2,
        // fill hold slots in reverse cargo order, and reserve a gunner gap.
        if(type->PipScale==PipScale::Passengers){
            // OpenTS Draw_Pips gates the cargo branch on Max_Passengers()>0.
            // YR 0x00709D28..0x00709D32 does the same; the non-cargo path
            // only draws MindControl/Tiberium/Ammo scales. Civilian PTRUCK
            // keeps PipScale=Passengers with Passengers=0 in RULESMD.INI,
            // so its health bar has no cargo pips. This is a valid no-op,
            // not an unsupported drawing mode that should abort the frame.
            if(type->Passengers<=0)return;
            if(!frame->mobile_pips||!frame->palette){frame->status=game::DrawingStatus::unavailable;return;}
            if(type->Passengers>1024){frame->status=game::DrawingStatus::invalid_argument;return;}
            std::vector<int> pips(type->Passengers,0);
            int slot=Passengers.GetTotalSize()-1;
            if(slot>=type->Passengers)return;
            auto* passenger=Passengers.FirstPassenger;
            for(int i=0;i<Passengers.NumPassengers&&passenger&&slot>=0;++i){
                const auto kind=passenger->WhatAmI();
                if(kind==AbstractType::Infantry||kind==AbstractType::Aircraft){
                    for(int extra=int(passenger->GetTechnoType()->Size-1);extra>0&&slot>=0;--extra)pips[slot--]=3;
                }
                if(slot>=0)pips[slot--]=kind==AbstractType::Infantry?int(static_cast<InfantryClass*>(passenger)->Type->Pip):kind==AbstractType::Aircraft?5:1;
                passenger=static_cast<FootClass*>(passenger->NextObject);
            }
            for(int i=0;i<type->Passengers;++i){
                game::ShapeDrawingRequest r;r.target=frame->drawing.target;r.palette=frame->palette;r.image=frame->mobile_pips;
                r.frame=pips[i];r.position={location->X-5+4*i+(type->Gunner&&i>0?8:0),location->Y};
                r.clip=*bounds;r.flags=0x600;r.intensity=1000;
                const auto status=game::submit_type_shape(frame->drawing,r);
                if(status!=game::DrawingStatus::skipped)frame->status=status;
                if(status!=game::DrawingStatus::drawn&&status!=game::DrawingStatus::skipped)return;
            }
            return;
        }
        frame->status=game::DrawingStatus::unsupported;return;
    }
    const auto& building=*static_cast<const BuildingClass*>(this);
    const auto* type=building.Type;
    // The original tiberium-storage branch bypasses this whole pip group.
    if(!type||type->PipScale==PipScale::Tiberium||!type->ShowOccupantPips)return;
    if(!frame->pips||!frame->palette){frame->status=game::DrawingStatus::unavailable;return;}
    const int occupied=building.GetOccupantCount();
    for(int i=0;i<type->MaxNumberOccupants;++i){
        const auto* occupant=i<occupied?building.Occupants.GetItemOrDefault(i):nullptr;
        game::ShapeDrawingRequest request;
        request.target=frame->drawing.target;request.palette=frame->palette;request.image=frame->pips;
        request.frame=occupant&&occupant->Type?static_cast<int>(occupant->Type->OccupyPip):6;
        request.position={location->X+6+4*i,location->Y-1+2*i};
        request.clip=*bounds;request.flags=0x600;request.intensity=1000;
        const auto result=game::submit_type_shape(frame->drawing,request);
        if(result!=game::DrawingStatus::drawn&&result!=game::DrawingStatus::skipped){frame->status=result;return;}
        if(result==game::DrawingStatus::drawn)frame->status=result;
    }
}
