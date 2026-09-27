// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 map.cpp Set_Local_Dimensions.
// Electronic Arts / OpenTS; additional terms: third_party/opents/LICENSE.md.
// YR 0x00567230, including original object re-entry and virtual See dispatch.
#include "yrpp/MapClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Drawing.h"
#include <algorithm>
#include <bit>
#if !defined(RA2_YRPP_GAME)
void MapClass::SetVisibleRect(const RectangleStruct& rect) {
    VisibleRect=Drawing::Intersect(rect,MapRect);
    VisibleRect.X=std::max(VisibleRect.X,2);
    VisibleRect.Y=std::max(VisibleRect.Y,2);
    const auto sub=[](int a,int b){return std::bit_cast<int>(unsigned(a)-unsigned(b));};
    VisibleRect.Width=std::min(VisibleRect.Width,sub(sub(MapRect.Width,VisibleRect.X),2));
    VisibleRect.Height=std::min(VisibleRect.Height,sub(sub(MapRect.Height,VisibleRect.Y),6));
    GScreenClass::Instance.MarkNeedsRedraw(2);
    for(int i=0;i<TechnoClass::Array.Count;++i){
        auto* object=TechnoClass::Array[i];
        const bool was_inside=object->IsInPlayfield;
        object->IsInPlayfield=IsWithinUsableArea(object->GetMapCoords(),true);
        if(!was_inside && object->IsInPlayfield && object->Owner->IsControlledByCurrentPlayer() &&
            object->WhatAmI()!=AbstractType::Building && object->IsAlive && !object->InLimbo)
            object->See(0,0);
    }
}
#endif
