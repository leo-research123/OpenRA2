// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tactical.cpp: selectable registration/query, rubber band and Select_These.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR: fixed 500-entry table, inclusive band corners, current-camera subtraction.
#include "yrpp/TacticalClass.h"
#include "yrpp/TechnoClass.h"

// OpenTS Reset_Action_Line_Timer; YR 0x0070D150.
void TacticalClass::StartDrawActionLineTimer() { TechnoClass::ActionLineTimer.Start(25); }
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <bit>

namespace {
TacticalSelectableStruct selectables[500]{};
RectangleStruct band_rect(const LTRBStruct& band) {
    return {std::min(band.Left,band.Right),std::min(band.Top,band.Bottom),
        std::abs(band.Right-band.Left)+1,std::abs(band.Bottom-band.Top)+1};
}
bool contains(const RectangleStruct& rect,Point2D at) {
    return at.X>=rect.X && at.X<rect.X+rect.Width && at.Y>=rect.Y && at.Y<rect.Y+rect.Height;
}
}
TacticalSelectableStruct (&TacticalClass::SelectableObjects)[500]=selectables;

bool TacticalClass::AddSelectable(ObjectClass* object,int x,int y) {
    if(SelectableCount>=500 || x<-32 || x>ViewBounds.Width+32 || y<-32 || y>ViewBounds.Height+32)return false;
    SelectableObjects[SelectableCount++]={object,x+TacticalPos.X,y+TacticalPos.Y};return true;
}
void TacticalClass::AddBuildingsToSelectables(RectangleStruct bounds) {
    const int left=bounds.X+TacticalPos.X,top=bounds.Y+TacticalPos.Y;
    const int right=left+bounds.Width,bottom=top+bounds.Height;
    for(auto* building:BuildingClass::Array) {
        if(!building->IsAlive || !building->IsOnMap || building->Type->InvisibleInGame)continue;
        // 0x006D9CE0 uses Location, not GetRenderCoords or a frame's crop.
        const auto at=CoordsToScreen(building->Location);
        if(at.X>=left && at.X<=right && at.Y>=top && at.Y<=bottom)
            AddSelectable(building,at.X-TacticalPos.X,at.Y-TacticalPos.Y);
    }
}
ObjectClass* TacticalClass::GetSelectableObject(const Point2D& point) {
    ObjectClass* best=nullptr;unsigned bestDistance=~0u;
    const Point2D pixel{point.X+TacticalPos.X,point.Y+TacticalPos.Y};
    for(int i=0;i<SelectableCount;++i) {
        const auto& candidate=SelectableObjects[i];auto* object=candidate.Object;
        if(!object)continue;
        switch(object->WhatAmI()) {
        case AbstractType::Unit:case AbstractType::Aircraft:
        case AbstractType::Building:case AbstractType::Infantry: {
            auto* techno=static_cast<TechnoClass*>(object);
            if(!object->IsAlive || object->InLimbo)continue;
            if(!techno->IsOwnedByCurrentPlayer && techno->CloakState==CloakState::Cloaked) {
                auto* cell=MapClass::Instance.GetCellAt(techno->GetCoords());
                if(!cell->Sensors_InclHouse(HouseClass::CurrentPlayer->ArrayIndex))continue;
            }
            break;
        }
        default: {
            auto* type=object->GetType();
            if(!type || type->WhatAmI()!=AbstractType::TerrainType
                || !static_cast<TerrainTypeClass*>(type)->IsVeinhole)continue;
            break;
        }
        }
        // Original 32-bit IMUL squares, then x87 truncation of dx²+dy²/2.
        const auto dx=unsigned(candidate.X)-unsigned(pixel.X),dy=unsigned(candidate.Y)-unsigned(pixel.Y);
        const double value=double(std::bit_cast<int>(dx*dx))+double(std::bit_cast<int>(dy*dy))*0.5;
        const unsigned distance=static_cast<unsigned>(static_cast<std::int64_t>(value));
        if(distance<bestDistance && distance<200){best=object;bestDistance=distance;}
    }
    if(best)return best;
    CellStruct cell;
    if(!PickTerrainCell(point,{0,0,ViewBounds.Width,ViewBounds.Height},cell))return nullptr;
    // 0x006DA4FB returns FirstObject, not a nearest-object or shadow query.
    return MapClass::Instance.GetCellAt(cell)->FirstObject;
}
void TacticalClass::StartRubberBand(const Point2D& point) {
    if(!Band.Left && !Band.Top)Band={point.X,point.Y,point.X,point.Y};
}
void TacticalClass::ModifyRubberBand(const Point2D& point) {
    if(Band.Left || Band.Top){Band.Right=point.X;Band.Bottom=point.Y;}
}
void TacticalClass::EndRubberBand() {Band={};}
bool TacticalClass::HasBandObjects() const {
    const auto rect=band_rect(Band);
    for(int i=0;i<SelectableCount;++i){const auto& candidate=SelectableObjects[i];
        if(candidate.Object && candidate.Object->IsAlive
            && contains(rect,{candidate.X-TacticalPos.X,candidate.Y-TacticalPos.Y}))return true;
    }
    return false;
}
void TacticalClass::SelectRubberBand(SelectionCallback callback) {
    if(Band.Left || Band.Top){SelectThese(band_rect(Band),callback);Band.Left=Band.Top=0;}
}
void TacticalClass::SelectThese(const RectangleStruct& rect,SelectionCallback callback) {
    Unsorted::MoveFeedback=true;
    if(rect.Width>0 && rect.Height>0)for(int i=0;i<SelectableCount;++i){
        const auto candidate=SelectableObjects[i];auto* object=candidate.Object;
        if(!object || !object->IsAlive || !contains(rect,{candidate.X-TacticalPos.X,candidate.Y-TacticalPos.Y}))continue;
        if(Game::IsTypeSelecting()) {Game::UICommands_TypeSelect_7327D0(object->GetType()->ID);continue;}
        if(callback){callback(object);continue;}
        bool buildingAllowed=false;
        if(object->WhatAmI()==AbstractType::Building){auto* type=static_cast<BuildingClass*>(object)->Type;
            buildingAllowed=type->UndeploysInto && type->IsVehicle();
        }
        auto* owner=object->GetOwningHouse();
        if(owner && owner->IsControlledByCurrentPlayer() && object->CanBeSelected()
            && (object->WhatAmI()!=AbstractType::Building || buildingAllowed) && object->Select())Unsorted::MoveFeedback=false;
    }
    Unsorted::MoveFeedback=true;
}
