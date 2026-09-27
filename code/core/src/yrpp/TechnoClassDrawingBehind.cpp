// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::Draw_Pre_Render; YR 0x006F60D0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "building_selection.hpp"
#include "type_drawing.hpp"
#include <bit>
#include <utility>

void TechnoClass::DrawBehind(Point2D*,RectangleStruct* bounds) const {
    // YR has no TS limpet-color branch. The second RTTI query and the
    // dimensions/center virtuals are retained in their original order.
    if(WhatAmI()!=AbstractType::Building||!IsSelected||WhatAmI()==AbstractType::Infantry)return;
    auto* frame=game::building_health_drawing();
    if(!frame||!bounds)return;
    if(!frame->selection_palette){frame->status=game::DrawingStatus::unavailable;return;}
    game::BuildingSelectionGeometry geometry;
    geometry.palette_index=GetHeight()<-4?12:15;
    CoordStruct dimensions;GetType()->Dimension2(&dimensions);
    const int x=dimensions.X/2,y=dimensions.Y/2,z=dimensions.Z;
    const auto center=GetCoords();
    const auto sum=[](int a,int b){return std::bit_cast<int>(unsigned(a)+unsigned(b));};
    const auto at=[&](int dx,int dy,int dz){return CoordStruct{sum(center.X,dx),sum(center.Y,dy),sum(center.Z,dz)};};
    const auto quarter=[](int a,int b){return std::bit_cast<int>(3u*unsigned(a)+unsigned(b))/4;};
    const auto end=[&](CoordStruct a,CoordStruct b){return CoordStruct{quarter(a.X,b.X),quarter(a.Y,b.Y),quarter(a.Z,b.Z)};};
    const auto edge=[&](CoordStruct a,CoordStruct b){if(a.Z<=b.Z)std::swap(a,b);geometry.edges[geometry.count++]={a,b};};
    const auto corners=[&](CoordStruct a,CoordStruct b){edge(a,end(a,b));edge(b,end(b,a));};
    corners(at(-x,-y,0),at(-x,-y,z));
    corners(at(-x,-y,0),at(x,-y,0));
    corners(at(-x,-y,0),at(-x,y,0));
    corners(at(-x,-y,z),at(-x,y,z));
    corners(at(-x,-y,z),at(x,-y,z));
    const auto result=game::draw_building_selection(frame->drawing,*bounds,frame->camera,geometry,*frame->selection_palette);
    if(result!=game::DrawingStatus::skipped)frame->status=result;
}
