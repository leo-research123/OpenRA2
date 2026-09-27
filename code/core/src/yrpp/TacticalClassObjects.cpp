// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tactical.cpp::Draw_Objects; calibrated to YR 0x006D8DB0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR adds world-space rejection, IsVisible, and a separate second Extras pass.
#include "yrpp/TacticalClass.h"
#include "type_drawing.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/ScenarioClass.h"
#include "map_runtime.hpp"
#include "tactical_drawing.hpp"
#include <bit>

void TacticalClass::DrawObjects(bool forced) {
    const auto& runtime=game::map_runtime();
    if(!runtime.drawing_bounds)return;
    auto bounds=*runtime.drawing_bounds;
    const auto wrap=[](unsigned value){return std::bit_cast<int>(value);};
    const auto top=ApplyMatrix_Pixel({wrap(unsigned(TacticalPos.X)+unsigned(bounds.Width/2)),wrap(unsigned(TacticalPos.Y)-unsigned(bounds.Height))});
    const auto right=ApplyMatrix_Pixel({wrap(unsigned(TacticalPos.X)+2u*unsigned(bounds.Width)),wrap(unsigned(TacticalPos.Y)+unsigned(bounds.Height/2))});
    const auto left=ApplyMatrix_Pixel({wrap(unsigned(TacticalPos.X)-unsigned(bounds.Width)),wrap(unsigned(TacticalPos.Y)+unsigned(bounds.Height/2))});
    const auto end_x=wrap(unsigned(top.X)+2u*(unsigned(right.X)-unsigned(top.X)));
    const auto end_y=wrap(unsigned(top.Y)+2u*(unsigned(left.Y)-unsigned(top.Y)));
    const auto inside=[&](CoordStruct at){return at.X>=top.X&&at.X<end_x&&at.Y>=top.Y&&at.Y<end_y;};
    const auto fog=[](const CoordStruct& at){return MapClass::Instance.IsLocationFogged(at);};
    const auto memory_fog=[](){return ScenarioClass::Instance&&ScenarioClass::Instance->SpecialFlags.FogOfWar;};
    const auto invoke=[&](ObjectClass& object,game::TacticalObjectPass pass,Point2D* point,RectangleStruct* clip){
        const auto result=game::dispatch_tactical_object(object,pass,point,clip,forced);
        game::record_tactical_drawing(result);return game::drawing_completed(result);
    };
    for(int layer=0;layer<5;++layer) {
        auto& objects=MapClass::ObjectsInLayers[layer];
        for(int index=0;index<objects.Count;++index) {
            auto* object=objects[index];if(!object)continue;
            object->IsVisible=false;
            Point2D point;CoordStruct ignored;
            if((object->AbstractFlags&AbstractFlags::Foot)!=AbstractFlags::None) {
                const auto at=object->Location;if(!inside(at))continue;
                object->GetDestination(&ignored,nullptr); // Original query even without memory fog.
                if(memory_fog()&&fog(at))continue;
                const auto center=object->GetCoords();
                if(CoordsToClient(&center,&point)) {
                    object->IsVisible=true;
                    if(!invoke(*object,game::TacticalObjectPass::behind,&point,&bounds)
                        ||!invoke(*object,game::TacticalObjectPass::body,nullptr,&bounds))return;
                }
            } else if((object->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None) {
                if(!inside(object->Location))continue;
                object->IsVisible=true;
                const auto center=object->GetCoords();
                if(CoordsToClient(&center,&point)) {
                    if(!invoke(*object,game::TacticalObjectPass::behind,&point,&bounds)
                        ||!invoke(*object,game::TacticalObjectPass::extras,&point,&bounds))return;
                }
                if(!invoke(*object,game::TacticalObjectPass::building_upper,nullptr,&bounds))return;
            } else {
              const auto kind=object->WhatAmI();
              if(kind==AbstractType::Anim) {
                auto& anim=static_cast<AnimClass&>(*object);const auto at=anim.GetCoords();
                if(!inside(at))continue;
                anim.IsVisible=true;
                if((anim.IsBuildingAnim||!anim.Type->ShouldFogRemove||!fog(anim.GetCoords()))
                    &&!invoke(anim,game::TacticalObjectPass::body,nullptr,&bounds))return;
              } else if(kind==AbstractType::Terrain) {
                auto& terrain=static_cast<TerrainClass&>(*object);
                if(!(terrain.Type->IsAnimated||terrain.IsCrumbling)||!inside(terrain.Location)||fog(terrain.Location))continue;
                terrain.IsVisible=true;
                if(!invoke(terrain,game::TacticalObjectPass::body,nullptr,&bounds))return;
              } else if(inside(object->Location)) {
                const auto center=object->GetCoords();
                if(CoordsToClient(&center,&point)) {
                    object->IsVisible=true;
                    if(!invoke(*object,game::TacticalObjectPass::behind,&point,&bounds)
                        ||!invoke(*object,game::TacticalObjectPass::body,nullptr,&bounds))return;
                }
              }
            }
        }
        if(layer==int(Layer::Ground))for(int n=0;n<BuildingClass::Array.Count;++n) {
            auto* building=BuildingClass::Array[n];
            if(!building->IsOnMap||!building->IsVisible)continue;
            const auto at=building->GetRenderCoords();Point2D point;
            if(CoordsToClient(&at,&point)) {
                auto clip=*runtime.drawing_bounds;
                if(!invoke(*building,game::TacticalObjectPass::building_info,&point,&clip))return;
            }
        }
    }
    // YR performs this separate pass over all five layers. Buildings therefore
    // receive both their first-pass Extras and this call; do not deduplicate.
    for(int layer=0;layer<5;++layer) {
        auto& objects=MapClass::ObjectsInLayers[layer];const int count=objects.Count;
        for(int index=0;index<count;++index) {
            auto* object=objects[index];
            if(!object||!object->IsVisible||(object->AbstractFlags&AbstractFlags::Techno)==AbstractFlags::None)continue;
            if((object->AbstractFlags&AbstractFlags::Foot)!=AbstractFlags::None) {
                CoordStruct ignored;object->GetDestination(&ignored,nullptr);
                if(runtime.has_window&&runtime.has_window()&&!(runtime.debug_map&&*runtime.debug_map)
                    &&memory_fog()&&fog(object->Location))continue;
            }
            const auto center=object->GetCoords();Point2D point;
            if(CoordsToClient(&center,&point)&&!invoke(*object,game::TacticalObjectPass::extras,&point,&bounds))return;
        }
    }
}
