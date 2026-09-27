// Existing placement virtual, YR 0x00464A70 -> Map 0x00578080 -> Cell 0x0047B3A0.
// Preserve X/Y; floor height includes the subcell ramp, not only Cell::Level.
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/MapClass.h"
#include <bit>

// OpenTS 44fac744 builtype.cpp Get_Draw_Rect; YR 0x465570.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
RectangleStruct* BuildingTypeClass::GetDrawRect(RectangleStruct* output) {
    RectangleStruct rect{unknown_1538,unknown_153C,unknown_1540,unknown_1544};
    if(rect.X||rect.Y||rect.Width||rect.Height){*output=rect;return output;}
    auto* shape=GetImage();if(!shape){*output={};return output;}
    const int width=shape->Width,height=shape->Height;
    const auto add=[](int a,int b){return std::bit_cast<int>(unsigned(a)+unsigned(b));};
    const auto merge=[&](RectangleStruct next){
        if(rect.Width<=0||rect.Height<=0){rect=next;return;}
        if(next.Width<=0||next.Height<=0)return;
        if(rect.X>next.X){rect.Width=add(add(rect.X,rect.Width),-next.X);rect.X=next.X;}
        if(rect.Y>next.Y){rect.Height=add(rect.Height,add(rect.Y,-next.Y));rect.Y=next.Y;}
        // Preserve the original Union's extra pixel on right/bottom expansion.
        if(add(rect.X,rect.Width)<add(next.X,next.Width))rect.Width=add(add(add(next.X,-rect.X),next.Width),1);
        if(add(rect.Y,rect.Height)<add(next.Y,next.Height))rect.Height=add(add(add(next.Y,-rect.Y),next.Height),1);
    };
    for(int i=0;i<shape->Frames;++i)merge(shape->GetFrameBounds(i));
    // Native BuildingType INI loading already owns the buildup allocation.
    // The EXE path retains its demand-load/free policy at 0x465570.
    if(Buildup)for(int i=0;i<Buildup->Frames;++i)merge(Buildup->GetFrameBounds(i));
    if(BibShape)merge(BibShape->GetFrameBounds(0));
    rect.X=add(rect.X,-width/2);rect.Y=add(rect.Y,-height/2);
    unknown_1538=rect.X;unknown_153C=rect.Y;unknown_1540=rect.Width;unknown_1544=rect.Height;
    *output=rect;return output;
}

CoordStruct* BuildingTypeClass::vt_entry_6C(CoordStruct* dest, CoordStruct* source) const {
    if (!dest || !source) return nullptr;
    const auto location = *source;
    const auto* cell = MapClass::Instance.GetCellAt(location);
    *dest = {location.X, location.Y, cell->GetFloorHeight({location.X, location.Y})};
    return dest;
}
