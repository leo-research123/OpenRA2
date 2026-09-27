// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 object.cpp Mark/Mark_For_Redraw; YR 0x5F5850/0x5F4D10.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ObjectClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "map_world.hpp"

bool ObjectClass::Mark(MarkType mark) {
    if(InLimbo)return false;
    if(mark==MarkType::Change){
        if(!NeedsRedraw && IsOnMap){MarkForRedraw();return true;}
        return false;
    }
    // The target makes these virtual queries even though it discards the values.
    // A building's buildup suppresses them only for MARK_UP.
    if(WhatAmI()!=AbstractType::Building || !static_cast<BuildingClass*>(this)->Type->Buildup || mark!=MarkType::Up){
        if((AbstractFlags & ::AbstractFlags::Techno)!=::AbstractFlags::None){
            static_cast<TechnoClass*>(this)->GetThreatValue();
            GetOwningHouseIndex();GetMapCoords();
        }
    }
    if((mark==MarkType::Down || mark==MarkType::ChangeRedraw) && !IsOnMap){
        IsOnMap=true;MarkForRedraw();return true;
    }
    if(mark==MarkType::Up && IsOnMap){IsOnMap=false;return true;}
    return false;
}
void ObjectClass::MarkForRedraw() {
    if(!NeedsRedraw){
        NeedsRedraw=true;MapClass::Instance.MarkNeedsRedraw(0);
        // Native presentation invalidation; no map, reference or Logic mutation.
        game::map_object_changed();
    }
}
CellStruct const* ObjectClass::GetFoundationData(bool includeBib) const {
    static const CellStruct empty[]{{0x7FFF,0x7FFF}};
    if(!GetType())return empty;
    return GetType()->GetFoundationData(includeBib);
}
