// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 anim.cpp::Draw_It.
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// YR 0x00422CA0: temporal flags, hardware ring, building tint, -50 tiled Z,
// -3 flat Z and the separate Shadow branch are calibrated to the fixed EXE.
#include "yrpp/AnimClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/BuildingClass.h"
#include "sprite_drawing.hpp"
#include <algorithm>
#include <bit>

namespace {
int add(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
int sub(int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); }
bool beyond_fifth(int frame, int end, int fifths) {
    // YR keeps End * double(0.2/0.4/0.6) in x87 extended precision.
    // At an exact fifth, 0.6 is slightly low and 0.2/0.4 slightly high.
    // Integer comparison preserves that boundary on SSE and ARM hosts too.
    const auto left=std::int64_t(frame)*5, right=std::int64_t(end)*fifths;
    if(left!=right)return left>right;
    return fifths==3 ? end>0 : end<0;
}
float truncate_float(double value) {
    const float rounded=float(value);
    // The original render path stores x87 results with rounding toward zero.
    if ((value>0 && double(rounded)>value) || (value<0 && double(rounded)<value))
        return std::bit_cast<float>(std::bit_cast<unsigned>(rounded)-1u);
    return rounded;
}
unsigned color_mask(const unsigned char* c, int format) {
    const unsigned r=c[0], g=c[1], b=c[2];
    // The original falls through all three conversions and ORs the results.
    unsigned value = unsigned(format - (format == 1 ? 1 : 2)) & 0xFFFF0000u;
    if (format == 2) value |= (r << 11) | (g << 5) | b;
    if (format == 1 || format == 2) value |= (r << 11) | ((g >> 1) << 6) | b;
    return value | (r << 10) | ((g >> 1) << 5) | b;
}
void ring(const AnimClass& a, game::SpriteDrawing& d, const Point2D& point) {
    int remaining=a.Animation.Timer.TimeLeft;
    if (a.Animation.Timer.StartTime != -1) {
        const int elapsed=sub(d.frame,a.Animation.Timer.StartTime);
        remaining=elapsed >= remaining ? 0 : sub(remaining,elapsed);
    }
    const int progress=sub(add(a.Animation.Rate, std::bit_cast<int>(unsigned(a.Animation.Value)*unsigned(a.Animation.Rate))), remaining);
    const int duration=std::bit_cast<int>(unsigned(a.Animation.Rate)*unsigned(a.GetEnd()));
    const int third=duration/3, two_thirds=std::bit_cast<int>(2u*unsigned(duration))/3;
    if (!duration || !third || !two_thirds || duration==third || duration==two_thirds) {
        d.status=game::DrawingStatus::invalid_argument; return;
    }
    const auto scale=[](int n){return std::bit_cast<int>(unsigned(n)<<8);};
    const int remaining_scaled=scale(sub(duration,progress));
    const int red=std::clamp(remaining_scaled/duration,0,255);
    const int green=2*std::clamp(progress<third?scale(progress)/third:remaining_scaled/(duration-third),0,255)/3;
    const int blue=std::clamp(progress<two_thirds?scale(progress)/two_thirds:remaining_scaled/(duration-two_thirds),0,255);
    const int radius=add(progress,8);
    const int depth=add(sub(sub(add(a.ZAdjust,a.Type->YDrawOffset),d.height(a.GetZ())),2),
        int(static_cast<unsigned short>(d.depth_origin-point.Y-d.tactical_rect.Y)));
    const float top_z=truncate_float(double(static_cast<unsigned short>(add(depth,radius)))*double(std::bit_cast<float>(0x37800080u)));
    const float bottom_z=truncate_float(double(static_cast<unsigned short>(sub(depth,radius)))*double(std::bit_cast<float>(0x37800080u)));
    const float left=truncate_float(sub(point.X,add(radius,radius))), right=truncate_float(add(point.X,add(radius,radius)));
    const float top=truncate_float(sub(point.Y,radius)), bottom=truncate_float(add(point.Y,radius));
    const unsigned color=0xFF000000u | (unsigned(red)<<16) | (unsigned(green)<<8) | unsigned(blue);
    const game::SpriteTriangleVertex tl{left,top,top_z,1,color,0xFF000000u,0,0};
    const game::SpriteTriangleVertex bl{left,bottom,bottom_z,1,color,0xFF000000u,0,1};
    const game::SpriteTriangleVertex br{right,bottom,bottom_z,1,color,0xFF000000u,1,1};
    const game::SpriteTriangleVertex tr{right,top,top_z,1,color,0xFF000000u,1,0};
    if (!d.triangle) { d.status=game::DrawingStatus::unavailable; return; }
    d.status=d.triangle(d.context,game::SpriteTriangle{{tl,bl,br}});
    if (game::sprite_drawing_complete(d.status)) d.status=d.triangle(d.context,game::SpriteTriangle{{tl,br,tr}});
}
}

int AnimClass::GetZ() const { return add(Location.Z, OwnerObject ? OwnerObject->Location.Z : 0); }
int AnimClass::GetEnd() const { return Type->End; }

void AnimClass::DrawIt(Point2D* point, RectangleStruct* clip) const {
    auto* d=game::sprite_drawing(); if (!d) return;
    if (!point || !clip || !Type) { d->status=game::DrawingStatus::invalid_argument; return; }
    if (!d->height || !d->shape_data) { d->status=game::DrawingStatus::unavailable; return; }
    if (d->hardware && d->depth_available && d->is_ring && d->is_ring(*this)) { ring(*this,*d,*point); return; }
    if (d->reduce_effects && d->reduce_effects(d->context) && Type->DetailLevel>1) return;
    if (Invisible || Type->DetailLevel>d->detail_level || (IsFogged && Type->ShouldFogRemove)) return;
    auto* image=GetImage(); if (!image) return;
    unsigned flags=static_cast<unsigned>(AnimFlags);
    const int frame=add(Animation.Value,Type->Start);
    if (UnderTemporal) flags |= Type->DoubleThick ? 6u : 4u;
    if (Type->TranslucencyDetailLevel<=d->detail_level) {
        // x86 cmp/jge tests this byte as signed, including values 0x80..0xFF.
        const int level=std::bit_cast<signed char>(TranslucencyLevel);
        if (Type->Translucent) {
            if (level>=15) return;
            if (beyond_fifth(Animation.Value,Type->End,3)) flags|=6;
            else if (beyond_fifth(Animation.Value,Type->End,2)) flags|=4;
            else if (beyond_fifth(Animation.Value,Type->End,1)) flags|=2;
        } else if (Type->Translucency>0) {
            if (level>=15) return;
            if (Type->Translucency==25) flags|=2;
            else if (Type->Translucency==50) flags|=4;
            else if (Type->Translucency==75) flags|=6;
        } else if (level) {
            if (level>15) return;
            flags |= level>5 ? 4u : 2u;
        }
    }
    if (!(flags&1)) flags|=0x800;
    const int height=GetHeight();
    int intensity=1000;
    CellClass* cell=nullptr;
    auto palette=game::SpritePalette::animation;
    const auto render_cell=[&](bool world_lookup) {
        CoordStruct at; GetRenderCoords(&at);
        if (world_lookup) return d->cell_at_world ? d->cell_at_world(d->context,at) : nullptr;
        const CellStruct pos{short(at.X/256),short(at.Y/256)};
        return d->cell_at ? d->cell_at(d->context,pos) : nullptr;
    };
    if (Type->IsVeins) {
        palette=game::SpritePalette::player;
        if (!Type->UseNormalLight) { cell=render_cell(true); if(cell)intensity=std::bit_cast<short>(cell->Intensity_Terrain); }
    } else if (UseCellLightConvert) {
        cell=render_cell(false);
        if (!cell || !game::sprite_initialize_light(*d,*cell)) { if(!cell)d->status=game::DrawingStatus::unavailable; return; }
        palette=game::SpritePalette::cell;
        if (!Type->UseNormalLight) intensity=std::bit_cast<short>(cell->Intensity_Terrain);
    } else if (LightConvert || d->alternative_palette) {
        palette=game::SpritePalette::alternative;
        if (!Type->UseNormalLight) intensity=d->alternative_palette ? d->alternative_intensity : TintColor;
    } else {
        if (Type->AltPalette) palette=game::SpritePalette::neutral;
        if (!Type->UseNormalLight) { cell=render_cell(false); if(cell)intensity=std::bit_cast<short>(cell->Intensity_Normal); }
    }
    if (!Type->UseNormalLight && (palette==game::SpritePalette::animation || palette==game::SpritePalette::neutral || palette==game::SpritePalette::player) && !cell) {
        d->status=game::DrawingStatus::unavailable; return;
    }
    game::ShapeDrawingRequest r;
    r.image=image; r.frame=frame; r.clip=*clip; r.position=*point;
    if (HasExtras) {
        r.position.Y=add(add(point->Y,Type->YDrawOffset),d->height(height));
        r.flags=0x2601; r.depth_adjustment=sub(Type->YDrawOffset,d->height(GetZ())); r.intensity=1000;
        if (!game::sprite_submit(*d,palette,this,cell,r)) return;
    }
    unsigned tint=0;
    if (IsBuildingAnim) {
        CoordStruct at; GetCoords(&at);
        auto* c=d->cell_at_world ? d->cell_at_world(d->context,at) : nullptr;
        auto* building=c && d->building ? d->building(*c) : nullptr;
        if (building) {
            if (building->Airstrike) {
                if (!d->laser_color || !d->pixel_format) { d->status=game::DrawingStatus::unavailable; return; }
                tint|=color_mask(d->laser_color,d->pixel_format());
            }
            if (building->IsIronCurtained() && building->ForceShielded==1) {
                if (!d->shield_color || !d->pixel_format) { d->status=game::DrawingStatus::unavailable; return; }
                tint|=color_mask(d->shield_color,d->pixel_format());
            }
        }
    }
    CoordStruct at; GetCoords(&at);
    auto* center=d->cell_at ? d->cell_at(d->context,CellStruct{short(at.X/256),short(at.Y/256)}) : nullptr;
    if (!center || !d->shrouded) { d->status=game::DrawingStatus::unavailable; return; }
    if (d->shrouded(*center)) tint=0;
    r.tint=std::bit_cast<int>(tint); r.intensity=intensity; r.flags=flags|0x2000;
    r.position=*point; r.gradient=Type->Flat?0:2;
    if (Type->Tiled) {
        if (!d->frame_bounds) { d->status=game::DrawingStatus::unavailable; return; }
        const int step=d->frame_bounds(image,0).Height;
        if (step<=0) { d->status=game::DrawingStatus::invalid_argument; return; }
        int y=point->Y-step/2;
        r.depth_adjustment=sub(sub(add(ZAdjust,Type->YDrawOffset),d->height(GetZ())),50);
        r.clip=d->tactical_rect; r.gradient=2;
        for (;;) {
            r.position.Y=add(y,Type->YDrawOffset);
            if (!game::sprite_submit(*d,game::SpritePalette::animation,this,cell,r)) return;
            if (y<0) break;
            y=sub(y,step); r.depth_adjustment=sub(r.depth_adjustment,step+step/2);
        }
    } else {
        r.position.Y=add(point->Y,Type->YDrawOffset);
        r.depth_adjustment=sub(sub(add(ZAdjust,Type->YDrawOffset),d->height(GetZ())),Type->Flat?3:2);
        if (!game::sprite_submit(*d,palette,this,cell,r)) return;
        if (!Type->Flat && Type->Shadow) {
            auto* shape=d->shape_data(image); if(!shape){d->status=game::DrawingStatus::unavailable;return;}
            r.frame=add(frame,shape->Frames/2); r.position=*point; r.flags=(flags&0xFFFFF9F8u)|0x601;
            r.depth_adjustment=sub(-2,d->height(GetZ())); r.gradient=0; r.intensity=1000; r.tint=0;
            game::sprite_submit(*d,palette,this,cell,r);
        }
    }
}
