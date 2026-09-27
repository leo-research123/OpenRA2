// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp::Render; YR 0x0043CEA0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Drawing.h"
#include "map_runtime.hpp"
#include <bit>

bool BuildingClass::DrawIfVisible(RectangleStruct* bounds,bool forced,DWORD extrasOnly) const {
    const auto& runtime=game::map_runtime();
    if(!bounds || !runtime.drawing_bounds || !TacticalClass::Instance)return false;
    if(!(runtime.debug_map && *runtime.debug_map) && runtime.has_window && runtime.has_window()
        && ((!forced && !NeedsRedraw) || !IsOnMap || InLimbo))return false;
    auto& self=*const_cast<BuildingClass*>(this);
    self.NeedsRedraw=false;
    const auto& tactical=*runtime.drawing_bounds;
    Drawing::Intersect(bounds,*bounds,tactical);
    RectangleStruct storage;
    const auto* dimensions=self.GetRenderDimensions(&storage);
    const auto add=[](int a,int b){return std::bit_cast<int>(unsigned(a)+unsigned(b));};
    const int x=add(tactical.X,dimensions->X),y=add(tactical.Y,dimensions->Y);
    if(bounds->X>=add(x,dimensions->Width) || bounds->Y>=add(y,dimensions->Height)
        || add(bounds->X,bounds->Width)<=x || add(bounds->Y,bounds->Height)<=y)return false;
    CoordStruct coordinates;Point2D point{};
    TacticalClass::Instance->CoordsToClient(GetRenderCoords(&coordinates),&point);
    if(bounds->X>tactical.X)point.X=std::bit_cast<int>(unsigned(point.X)+unsigned(tactical.X)-unsigned(bounds->X));
    if(bounds->Y>tactical.Y)point.Y=std::bit_cast<int>(unsigned(point.Y)+unsigned(tactical.Y)-unsigned(bounds->Y));
    // 0x0043CFCD consumes only the low byte of the existing DWORD ABI argument.
    if(static_cast<BYTE>(extrasOnly)){if(!IsFogged)self.Draw(point,*bounds);}
    else DrawIt(&point,bounds);
    return true;
}
