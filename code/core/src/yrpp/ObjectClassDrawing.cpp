// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 object.cpp::Render; YR 0x005F4B10.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ObjectClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Drawing.h"
#include "map_runtime.hpp"
#include <bit>

bool ObjectClass::DrawIfVisible(RectangleStruct* bounds,bool forced,DWORD) const {
    const auto& runtime=game::map_runtime();
    if(!bounds || !runtime.drawing_bounds || !TacticalClass::Instance)return false;
    if(!(runtime.debug_map && *runtime.debug_map) && runtime.has_window && runtime.has_window()
        && ((!forced && !NeedsRedraw) || InLimbo))return false;
    const_cast<ObjectClass*>(this)->NeedsRedraw=false;
    CoordStruct coordinates;Point2D point{};
    if(!TacticalClass::Instance->CoordsToClient(GetRenderCoords(&coordinates),&point)
        && WhatAmI()!=AbstractType::ParticleSystem)return false;
    const auto& tactical=*runtime.drawing_bounds;
    Drawing::Intersect(bounds,*bounds,tactical);
    if(bounds->X>tactical.X)point.X=std::bit_cast<int>(unsigned(point.X)+unsigned(tactical.X)-unsigned(bounds->X));
    if(bounds->Y>tactical.Y)point.Y=std::bit_cast<int>(unsigned(point.Y)+unsigned(tactical.Y)-unsigned(bounds->Y));
    // Even an empty intersection reaches DrawIt and returns true in the EXE.
    DrawIt(&point,bounds);
    return true;
}
