// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Update_Radar_Position / Is_Radar_Visible.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR 0x70CC90/0x70CCC0/0x70CCF0, 0x70D990, 0x70D1D0/0x70D420/0x70D460.
#include "yrpp/TechnoClass.h"
#include "scenario_runtime.hpp"
#include "yrpp/RadarClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/Surface.h"
#include <bit>
void TechnoClass::RadarTrackingStart() {
    RadarClass::Instance.TrackObject(this,RadarPosition.X,RadarPosition.Y);
    IsRadarTracked=true;
}
void TechnoClass::RadarTrackingStop() {
    RadarClass::Instance.UntrackObject(this,RadarPosition.X,RadarPosition.Y);
    IsRadarTracked=false;
}
void TechnoClass::RadarTrackingFlash() {
    RadarClass::Instance.RefreshCrd(&RadarPosition);
}

bool TechnoClass::IsSensorVisibleToPlayer() const {
    if(!HouseClass::CurrentPlayer)return false;
    return MapClass::Instance.GetCellAt(GetCoords())->Sensors_InclHouse(HouseClass::CurrentPlayer->ArrayIndex);
}
bool TechnoClass::IsSensorVisibleToHouse(HouseClass* house) const {
    if(!house)return false;
    return MapClass::Instance.GetCellAt(GetCoords())->Sensors_InclHouse(house->ArrayIndex);
}
bool TechnoClass::IsRadarVisible(int* detection) const {
    if(GetTechnoType()->Invisible || IsSinking || !IsAlive || InLimbo)return false;
    if(Owner->IsControlledByCurrentPlayer())return DiscoveredByCurrentPlayer;
    const auto at=GetCoords();
    if(!MapClass::Instance.IsWithinUsableArea(CellStruct{short(at.X/256),short(at.Y/256)},true))return false;
    const int height=GetHeight();
    const bool invisible=HasAbility(Ability::RadarInvisible);
    // Native equivalent of YR's hWnd presentation guard: a configured game
    // window rectangle. No Win32 handle is fabricated or dereferenced here.
    const bool window=DSurface::WindowBounds.Width>0 && DSurface::WindowBounds.Height>0;
    const bool shrouded=MapClass::Instance.IsLocationShrouded(Location) && window;
    const bool fogged=ScenarioClass::Instance->SpecialFlags.FogOfWar && MapClass::Instance.IsLocationFogged(Location);
    if(!fogged && CloakState!=::CloakState::Cloaked && height>=-20 && !invisible && !shrouded)return true;
    if(!IsSensorVisibleToPlayer())return false;
    if(!HouseClass::CurrentPlayer->IsAlliedWith(Owner) && !invisible && !fogged && !shrouded)*detection=height<-20?2:1;
    return true;
}
void TechnoClass::RadarTrackingUpdate(bool force) {
    const auto& runtime=game::scenario_runtime();
    if(!DiscoveredByCurrentPlayer && runtime.session_mode(runtime.context)==static_cast<int>(GameMode::Campaign))
        DiscoveredByCurrentPlayer=!MapClass::Instance.IsLocationShrouded(GetCoords());
    auto& radar=RadarClass::Instance;
    Point2D position=RadarPosition;
    if(WhatAmI()!=AbstractType::Building || force) {
        auto at=Location;
        // No native radar image means no available projection, not pixel zero.
        if(!radar.GetCrdOnRadar(&position,&at,false))return;
    }
    int detection=0;
    bool visible=IsRadarVisible(&detection);
    // The native radar stores its image extent directly (the same extent used
    // by TrackObject). YR obtains it from the radar BSurface's GetRect.
    const RectangleStruct bounds{0,0,radar.unknown_rect_149C.Width,radar.unknown_rect_149C.Height};
    auto* foot=(AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None?static_cast<FootClass*>(this):nullptr;
    if(detection && visible && (!foot || static_cast<signed char>(foot->TubeIndex)<0))
        RadarEventClass::Create(RadarEventType::EnemySensed,CellStruct{short(Location.X/256),short(Location.Y/256)});
    if(position.X<bounds.X || position.X>=bounds.X+bounds.Width || position.Y<bounds.Y || position.Y>=bounds.Y+bounds.Height) {
        if(visible) {
            visible=MapClass::Instance.IsWithinUsableArea(GetCellAgain(),true);
            if(visible){auto at=Location;if(!radar.GetCrdOnRadar(&position,&at,true))return;}
        }
    }
    if(InLimbo && visible)visible=false;
    if(IsRadarTracked && (position!=RadarPosition || !visible))RadarTrackingStop();
    RadarPosition=position;
    if(!IsRadarTracked && visible)RadarTrackingStart();
    if(Owner==HouseClass::CurrentPlayer) {
        int remaining=RadarFlashTimer.TimeLeft;
        if(RadarFlashTimer.StartTime!=-1) {
            const int elapsed=std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame)-static_cast<unsigned>(RadarFlashTimer.StartTime));
            remaining=elapsed>=remaining?0:std::bit_cast<int>(static_cast<unsigned>(remaining)-static_cast<unsigned>(elapsed));
        }
        if(remaining>0 && remaining%RulesClass::Instance->FlashFrameTime==0)RadarTrackingFlash();
    }
}
