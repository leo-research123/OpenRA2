// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 techno.cpp::Techno_Draw_Object; YR 0x00705E00.
#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/AirstrikeClass.h"
#include "techno_drawing.hpp"
#include <algorithm>
#include <bit>

void TechnoClass::DrawObject(SHPStruct* image,int frame,Point2D* location,RectangleStruct* bounds,
        int,int,int z,ZGradient gradient,int write_depth,int intensity,int tint,
        SHPStruct* depth_image,int depth_frame,int depth_x,int depth_y,int excluded) {
    auto* d=game::techno_drawing();if(!d||!image)return;
    if(!location||!bounds||!d->height){game::record_techno_drawing(*d,game::DrawingStatus::invalid_argument);return;}
    const auto visual=VisualCharacter(false,nullptr);
    unsigned flags=0;
    if(!excluded){
        switch(visual){
        case VisualType::Hidden:return;
        case VisualType::Indistinct:flags=2;break;
        case VisualType::Darken:case VisualType::Shadowy:flags=4;break;
        case VisualType::Ripple:flags=CloakProgress.Value?4:2;break;
        default:break;
        }
    }
    const auto kind=WhatAmI();
    const auto* building=kind==AbstractType::Building?static_cast<const BuildingClass*>(this):nullptr;
    const auto* unit=kind==AbstractType::Unit?static_cast<const UnitClass*>(this):nullptr;
    if(IsBeingWarpedOut()||IsWarpingIn())flags|=building&&building->Type->DoubleThick?6u:4u;
    auto palette=game::TechnoPalette::house;auto* house=Owner;CellClass* cell=nullptr;
    if(building&&building->Type->TerrainPalette){
        cell=game::techno_drawing_cell(*d,*this);if(!cell){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
        if(!cell->LightConvert){const auto status=d->initialize_light?d->initialize_light(d->context,*cell):game::DrawingStatus::unavailable;
            if(!game::techno_drawing_complete(status)){game::record_techno_drawing(*d,status);return;}}
        palette=game::TechnoPalette::cell;intensity=std::bit_cast<short>(cell->Intensity_Terrain);
    }else if(unit&&unit->TerrainPalette)palette=game::TechnoPalette::eight_bit;
    bool shadow=d->draw_shadows;
    auto point=*location;
    if(kind==AbstractType::Unit||kind==AbstractType::Infantry){
        if(unit&&(unit->Type->SmallVisceroid||unit->Type->LargeVisceroid))shadow=false;
        if(GetHeight()){z-=d->height(GetZ());shadow=false;}else z+=GetZAdjustment();
    }else if(kind==AbstractType::Aircraft){point.Y-=d->height(GetZ());z-=d->height(GetZ());}
    if(GetTechnoType()->NoShadow)shadow=false;
    if(!IsClearlyVisibleTo(d->player)){
        auto* disguise=GetDisguise(true);
        if(disguise&&(disguise->WhatAmI()==AbstractType::TerrainType||disguise->WhatAmI()==AbstractType::OverlayType)){
            cell=GetCell();palette=game::TechnoPalette::cell;
            if(!cell){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
            if(!cell->LightConvert){const auto status=d->initialize_light?d->initialize_light(d->context,*cell):game::DrawingStatus::unavailable;
                if(!game::techno_drawing_complete(status)){game::record_techno_drawing(*d,status);return;}}
            intensity=std::bit_cast<short>(cell->Intensity_Terrain);
        }else {house=GetDisguiseHouse(true);palette=game::TechnoPalette::house;}
        if(!house&&palette==game::TechnoPalette::house)house=d->player;
    }
    if(gradient!=ZGradient::None)flags|=0x2000;
    if(write_depth)flags|=0x4000;
    flags|=0x800;
    if(unit&&unit->TerrainPalette){flags|=0x20;shadow=false;}
    flags&=~unsigned(excluded);
    auto* shape=d->shape_data?d->shape_data(image):image;
    if(!shape){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
    if(unit&&unit->TerrainPalette){
        if(!d->frame_bounds){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
        const auto frame_bounds=d->frame_bounds(shape,frame);
        const RectangleStruct r{point.X+frame_bounds.X-shape->Width/2,point.Y+frame_bounds.Y-shape->Height/2,frame_bounds.Width,frame_bounds.Height};
        auto& out=d->composite_rect;
        if(out.Width<=0||out.Height<=0)out=r;
        else if(r.Width>0&&r.Height>0){const int right=out.X+out.Width,bottom=out.Y+out.Height;out.X=std::min(out.X,r.X);out.Y=std::min(out.Y,r.Y);out.Width=(right<r.X+r.Width?r.X+r.Width+1:right)-out.X;out.Height=(bottom<r.Y+r.Height?r.Y+r.Height+1:bottom)-out.Y;}
    }
    if(Owner&&Owner->IsControlledByCurrentPlayer()&&IsDisguised())flags=GetDisguiseFlags(flags);
    intensity=GetFlashingIntensity(intensity);
    if(IsIronCurtained()||(building&&building->Airstrike&&building->Airstrike->Target==building))
        intensity=GetEffectTintIntensity(intensity);
    if(auto* at=game::techno_drawing_cell(*d,*this);at&&d->shrouded&&d->shrouded(*at))tint=0;
    if(visual>VisualType::Ripple)return;
    game::ShapeDrawingRequest r;r.image=image;r.frame=frame;r.position=point;r.clip=*bounds;
    r.flags=flags|0x600;r.depth_adjustment=z-2;r.gradient=int(gradient);r.intensity=intensity;r.tint=unsigned(tint);
    r.depth_image=depth_image;r.depth_frame=depth_frame;r.depth_offset={depth_x,depth_y};
    if(!game::techno_submit_shape(*d,*this,palette,cell,house,r))return;
    if(shadow&&(visual==VisualType::Normal||visual==VisualType::Indistinct)){
        r.frame=frame+shape->Frames/2;r.flags=visual==VisualType::Normal?(flags&0xFFFFF9F8u)|0x601u:(flags&0xFFFFFFF9u)|0x601u;
        r.depth_adjustment=-d->height(GetZ())-4;r.gradient=0;r.intensity=1000;r.tint=0;
        if(IsOnCarryall)r.position.Y-=14;
        r.depth_image=nullptr;r.depth_frame=0;r.depth_offset={};
        game::techno_submit_shape(*d,*this,palette,cell,house,r);
    }
}
