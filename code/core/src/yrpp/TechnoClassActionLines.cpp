// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 actionline.cpp Draw_Action_Line_Segment and xsurface.cpp line rasterizers.
// Calibrated to YR 0x007049C0 / 0x007BA610 / 0x007BA8C0: marker order/size,
// Y-only viewport offset, endpoint exclusion on diagonals, dash direction/phase.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include "map_runtime.hpp"
#include "type_drawing.hpp"
#include <algorithm>
#include <cstdlib>
#include <bit>

namespace {
WORD pixel(ColorStruct c){return WORD((c.R>>3<<11)|(c.G>>2<<5)|(c.B>>3));}
game::DrawingStatus fill(const game::TypeDrawingContext& context,const RectangleStruct& clip,
    Point2D point,WORD color){
    const int x=std::max(point.X-2,clip.X),y=std::max(point.Y-2,clip.Y);
    const int right=std::min(point.X+1,clip.X+clip.Width),bottom=std::min(point.Y+1,clip.Y+clip.Height);
    if(right<=x||bottom<=y)return game::DrawingStatus::skipped;
    game::RasterDrawingRequest r;r.target=context.target;r.clip=clip;r.position={x,y};
    r.width=right-x;r.height=bottom-y;r.color=color;
    return game::submit_type_raster(context,r);
}
game::DrawingStatus line(const game::TypeDrawingContext& context,RectangleStruct clip,
    Point2D a,Point2D b,WORD color,bool dashed){
    if(!Line_In_Bounds(&a,&b,&clip))return game::DrawingStatus::skipped;
    int phase=std::bit_cast<int>(0x7FFFFFFFu-unsigned(Unsorted::CurrentFrame))%15,increment=1;
    if(a.X>b.X){
        std::swap(a,b);
        phase=(phase+std::max(std::abs(a.X-b.X),std::abs(a.Y-b.Y)+1))%16;
        increment=-1;
    }
    const int dx=b.X-a.X,dy=std::abs(b.Y-a.Y),sy=b.Y>=a.Y?1:-1;
    const bool horizontal=dx>dy;
    // Original XSurface includes the last pixel only on axis-aligned lines.
    const int count=std::max(dx,dy)+int(dx==0||dy==0);
    int error=horizontal?2*dy-dx:2*dx-dy;
    auto result=game::DrawingStatus::skipped;
    for(int i=0;i<count;++i){
        phase=(phase+16)%16;
        if(!dashed||phase%8<5){
            game::RasterDrawingRequest r;r.target=context.target;r.clip=clip;
            r.position=a;r.width=r.height=1;r.color=color;
            const auto status=game::submit_type_raster(context,r);
            if(!game::drawing_completed(status))return status;
            if(status==game::DrawingStatus::drawn)result=status;
        }
        phase+=increment;
        if(horizontal){if(error>0){a.Y+=sy;error-=2*dx;}error+=2*dy;++a.X;}
        else{if(error>0){++a.X;error-=2*dy;}error+=2*dx;a.Y+=sy;}
    }
    return result;
}
}
void TechnoClass::DrawActionLine(CoordStruct from,CoordStruct to,ColorStruct color,bool dashed,bool shadow){
    const auto* context=game::active_type_drawing();
    if(!context){game::record_type_drawing_result(game::DrawingStatus::unavailable);return;}
    Point2D a{},b{};
    if(!TacticalClass::Instance||!game::map_runtime().view_bounds){
        game::record_type_drawing_result(game::DrawingStatus::unavailable);return;
    }
    TacticalClass::Instance->CoordsToClient(&from,&a);
    TacticalClass::Instance->CoordsToClient(&to,&b);
    const auto clip=DSurface::ViewBounds;a.Y+=clip.Y;b.Y+=clip.Y;
    const auto submit=[&](game::DrawingStatus status){game::record_type_drawing_result(status);return game::drawing_completed(status);};
    if(shadow){
        const auto* palette=game::map_runtime().action_line_palette;
        if(!palette){submit(game::DrawingStatus::unavailable);return;}
        const Point2D sa{a.X,a.Y+1},sb{b.X,b.Y+1};const auto black=pixel(palette->Entries[0]);
        if(!submit(fill(*context,clip,sa,black))||!submit(fill(*context,clip,sb,black))||
            !submit(line(*context,clip,sa,sb,black,false)))return;
    }
    const auto rgb=pixel(color);
    if(!submit(fill(*context,clip,a,rgb))||!submit(fill(*context,clip,b,rgb)))return;
    submit(line(*context,clip,a,b,rgb,dashed));
}

// YR-only 0x00704E40. Reuses the pinned OpenTS XSurface line rasterizer
// above; the 32-segment arc and moving color phase are YR binary behavior.
void TechnoClass::DrawMindControlLine(CoordStruct from,CoordStruct to,ColorStruct color){
    const auto* context=game::active_type_drawing();
    if(!context||!TacticalClass::Instance||!game::map_runtime().view_bounds){
        game::record_type_drawing_result(game::DrawingStatus::unavailable);return;
    }
    const auto clip=DSurface::ViewBounds;
    const auto project=[&](const CoordStruct& at){
        Point2D out{};TacticalClass::Instance->CoordsToClient(&at,&out);out.Y+=clip.Y;return out;
    };
    const auto submit=[](game::DrawingStatus status){game::record_type_drawing_result(status);return game::drawing_completed(status);};
    const auto rgb=pixel(color);
    if(!submit(fill(*context,clip,project(from),rgb))||!submit(fill(*context,clip,project(to),rgb)))return;
    // SystemTimer uses the original timeGetTime() >> 4 tick, independent of
    // simulation CurrentFrame (including when the game is paused).
    const unsigned phase=8u*SystemTimer::GetTime();
    const auto interpolate=[](int a,int b,int step){
        return std::bit_cast<int>(unsigned(a)*unsigned(32-step)+unsigned(b)*unsigned(step))/32;
    };
    auto previous=from;
    for(int i=1;i<=32;++i){
        const double u=(double(i)-16.0)*0.0625;
        const CoordStruct next{interpolate(from.X,to.X,i),interpolate(from.Y,to.Y,i),
            int(double(interpolate(from.Z,to.Z,i))+(1.0-u*u)*512.0)};
        auto segmentColor=color;
        const unsigned brightness=(unsigned(8*i)-phase)&0x1F8u;
        if(brightness>0&&brightness<=0xF8u){
            // RGBClass::Adjust (0x006612C0), interpolation divisor is 256.
            segmentColor.R=byte(color.R+int(brightness)*(255-color.R)/256);
            segmentColor.G=byte(color.G+int(brightness)*(255-color.G)/256);
            segmentColor.B=byte(color.B+int(brightness)*(255-color.B)/256);
        }
        if(!submit(line(*context,clip,project(previous),project(next),pixel(segmentColor),false)))return;
        previous=next;
    }
}
