// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 object.cpp Limbo/Unlimbo/Detach_All; YR
// 0x5F4D30/0x5F4EC0/0x5F5280. EA Section 7 terms: third_party/opents/LICENSE.md.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
#include "yrpp/ObjectClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/AlphaShapeClass.h"
#include "yrpp/LineTrail.h"
#include "yrpp/BombClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Unsorted.h"
#include "scenario_runtime.hpp"

namespace {
bool participates_in_logic(const ObjectClass& object) {
    const auto& runtime=game::scenario_runtime();
    const auto mode=static_cast<GameMode>(runtime.session_mode(runtime.context));
    return mode==GameMode::Campaign || mode==GameMode::Skirmish || object.Fetch_ID()!=-2;
}
Point2D alpha_position(const ObjectClass& object) {
    Point2D point{};
    const auto at=object.GetCoords();
    TacticalClass::Instance->CoordsToClient(&at,&point);
    const auto* shape=object.GetType()->AlphaImage;
    point.X-=shape->Width/2;point.Y-=shape->Height/2;
    return point;
}
}

bool ObjectClass::Limbo() {
    if(!Game::IsActive || InLimbo)return false;
    Deselect();Disappear(true);Mark(MarkType::Up);
    DisplayClass::Remove(this);
    AmbientSoundController.Stop();CustomSoundController.End();
    if(GetType() && GetType()->IsLogic && participates_in_logic(*this))LogicClass::Instance.RemoveObject(this);
    if(GetType() && GetType()->AlphaImage) {
        const auto point=alpha_position(*this);
        const auto* shape=GetType()->AlphaImage;
        TacticalClass::Instance->RegisterDirtyArea({point.X,point.Y,shape->Width,shape->Height},true);
    }
    Undiscover();InLimbo=true;NeedsRedraw=false;
    return true;
}

bool ObjectClass::Unlimbo(const CoordStruct& where,DirType) {
    if(where==CoordStruct::Empty || !Game::IsActive || !InLimbo || IsOnMap)return false;
    if(!Unsorted::ScenarioInit && IsCellOccupied(MapClass::Instance.GetCellAt(where),FacingType::None,-1,nullptr,false)!=Move::OK)return false;
    const auto previousLocation=Location;
    InLimbo=false;NeedsRedraw=false;
    auto* type=GetType();auto at=where;
    if(type){auto source=where;CoordStruct adjusted;at=*type->vt_entry_6C(&adjusted,&source);}
    SetLocation(at);
    if(!Mark(MarkType::Down)){InLimbo=true;return false;}
    if(!IsAlive)return true;
    if(InWhichLayer()!=Layer::None)DisplayClass::Submit(this);
    if(type) {
        const bool terrainLogic=WhatAmI()!=AbstractType::Terrain || static_cast<TerrainTypeClass*>(type)->SpawnsTiberium;
        if(type->IsLogic && terrainLogic && participates_in_logic(*this)
            && !LogicClass::Instance.AddObject(this,false)) {
            // Native allocation failure: undo only the base placement already
            // performed. Derived Unlimbo has not registered ownership/adjacency.
            DisplayClass::Remove(this);Mark(MarkType::Up);
            InLimbo=true;Location=previousLocation;return false;
        }
        if(type->AlphaImage) {
            const auto point=alpha_position(*this);
            const auto camera=TacticalClass::Instance->TacticalPos;
            if(auto* storage=YRMemory::Allocate(sizeof(AlphaShapeClass)))
                ::new(storage) AlphaShapeClass(this,point.X+camera.X,point.Y+camera.Y);
            if(!Unsorted::ScenarioInit)
                TacticalClass::Instance->RegisterDirtyArea({point.X,point.Y,type->AlphaImage->Width,type->AlphaImage->Height},true);
        }
    }
    // YR moved line-trail creation here, before projectile placement.
    if(GetType()->UseLineTrail) {
        auto* storage=YRMemory::Allocate(sizeof(LineTrail));
        LineTrailer=storage?::new(storage) LineTrail():nullptr;
        if(LineTrailer) {
            const auto color=RulesClass::Instance->LineTrailColorOverride;
            LineTrailer->Color=(color.R || color.G || color.B)?color:GetType()->LineTrailColor;
            LineTrailer->SetDecrement(GetType()->LineTrailColorDecrement);
            LineTrailer->Owner=this;
        }
    }
    return true;
}

void ObjectClass::Disappear(bool permanently) {
    if(LineTrailer)LineTrailer->Detach();
    auto* owner=GetOwningHouse();
    const bool sensed=!permanently && (AbstractFlags & ::AbstractFlags::Techno)!=::AbstractFlags::None
        && static_cast<TechnoClass*>(this)->IsSensorVisibleToPlayer();
    if(permanently || !owner || (!owner->IsControlledByCurrentPlayer() && !sensed))Deselect();
    auto& display=DisplayClass::Instance;
    if(display.FollowObject && display.ObjectToFollow==this){display.ObjectToFollow=nullptr;display.FollowObject=false;}
    NotifyObjectExpired(permanently);
}

// OpenTS ObjectClass::Delete_This, calibrated to YR's deferred destruction.
void ObjectClass::UnInit() {
    if(AttachedBomb)AttachedBomb->Disarm();
    if((AbstractFlags & ::AbstractFlags::Techno)!=::AbstractFlags::None)
        static_cast<TechnoClass*>(this)->KillPassengers(nullptr);
    // This entry is an ObjectClass: use the original mixed-object listener
    // arm, including bullets and attached animations, before changing Limbo.
    NotifyObjectExpired(true);
    Limbo();
    IsAlive=false;
    PendingDeletes.AddItem(this);
}
