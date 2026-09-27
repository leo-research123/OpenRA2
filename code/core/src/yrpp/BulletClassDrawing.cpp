// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/bullet.cpp Draw_It; YR 0x468090 adds Shadow,
// SpawnNextAnim, FirersPalette and the original 32-facing frame lookup.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BulletClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/HouseClass.h"
#include "type_drawing.hpp"

void BulletClass::DrawIt(Point2D* point,RectangleStruct* clip) const {
    const auto* context=game::active_type_drawing();if(!context)return;
    if(!point||!clip){game::record_type_drawing_result(game::DrawingStatus::invalid_argument);return;}
    if(!Type||Type->Inviso||SpawnNextAnim)return;
    auto* cell=GetCell();
    if(ScenarioClass::Instance&&ScenarioClass::Instance->SpecialFlags.FogOfWar&&cell->IsFogged())return;
    // Voxel projectile resources still have an explicit unavailable load
    // contract. Never draw an unrelated SHP in place of the voxel branch.
    if(Type->Voxel){game::record_type_drawing_result(game::DrawingStatus::unsupported);return;}
    auto* image=GetImage();if(!image)return;
    const int frame=GetAnimFrame();
    int height=GetHeight(),floor=MapClass::Instance.GetCellFloorHeight(Location);
    if(!OnBridge&&(unsigned(cell->Flags)&0x100)&&height>=CellClass::BridgeHeight){height-=CellClass::BridgeHeight;floor+=CellClass::BridgeHeight;}
    game::ShapeDrawingRequest r;r.target=context->target;r.palette=context->palette;
    r.image=image;r.frame=frame;r.position=*point;r.clip=*clip;r.gradient=0;r.intensity=1000;
    if(height>0&&Type->Shadow){
        // 0x00468374 always uses NormalDrawer, independently of the body.
        if(!game::resolve_drawing_palette(game::DrawingPaletteKind::normal,-1,r.palette))return;
        r.position.Y+=TacticalClass::AdjustForZ(height);r.flags=0x2601;
        r.depth_adjustment=-10-TacticalClass::AdjustForZ(floor);
        const auto status=game::submit_type_shape(*context,r);
        game::record_type_drawing_result(status);if(!game::drawing_completed(status))return;
    }
    // 0x0046837F..0x004683D1: AnimPalette takes precedence. InheritedColor
    // indexes ColorScheme, not House; only -1 selects the current player's index.
    auto palette=game::DrawingPaletteKind::normal;int color=-1;
    if(Type->AnimPalette)palette=game::DrawingPaletteKind::animation;
    else if(Type->FirersPalette){
        palette=game::DrawingPaletteKind::color_scheme;color=InheritedColor;
        if(color==-1){
            if(!HouseClass::CurrentPlayer){game::record_type_drawing_result(game::DrawingStatus::unavailable);return;}
            color=HouseClass::CurrentPlayer->ColorSchemeIndex;
        }
    }
    if(!game::resolve_drawing_palette(palette,color,r.palette))return;
    r.position=*point;r.flags=0x2E00;r.depth_adjustment=-30-TacticalClass::AdjustForZ(Location.Z);
    game::record_type_drawing_result(game::submit_type_shape(*context,r));
}
