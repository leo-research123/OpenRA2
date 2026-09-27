// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp::Draw_Overlays; YR 0x0043E7B0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/HouseClass.h"
#include "building_selection.hpp"

void BuildingClass::DrawInfoTipAndSpiedSelection(Point2D* point,RectangleStruct* bounds) const {
    CellStruct cell;GetMapCoords(&cell); // Original query occurs even when unselected.
    auto* player=HouseClass::CurrentPlayer;
    if(!point||!bounds||!player||!player->Type)return;
    const bool spied=DisplayProductionTo.Contains(unsigned(player->Type->ArrayIndex)&31u);
    if(IsSelected&&((Owner&&Owner->IsAlliedWith(player))||spied)) {
        const Point2D origin{point->X-10,point->Y+10};
        DrawExtraInfo(origin,*point,*bounds);
    }
    if(!spied||!IsSelected||!Owner||!Type)return;
    auto* factory=Owner->IsControlledByHuman()?Owner->GetPrimaryFactory(Type->Factory,Type->Naval,BuildCat::DontCare):Factory;
    auto* object=factory?factory->Object:nullptr;
    if(!object)return;
    auto* frame=game::building_health_drawing();if(!frame)return;
    auto* image=object->GetType()->GetCameo();
    game::ShapeDrawingRequest request;request.target=frame->drawing.target;request.palette=frame->palette;
    request.image=image;request.position=*point;request.clip=*bounds;request.flags=0xE00;
    const auto result=game::submit_type_shape(frame->drawing,request);
    if(result!=game::DrawingStatus::skipped)frame->status=result;
}
