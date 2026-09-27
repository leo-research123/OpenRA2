// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 tactical.cpp::Draw_Rubber_Band / Render. YR calibration:
// 0x006DA180 and Render's 0x006D46DD..0x006D47F6 action/link phase.
#include "yrpp/TacticalClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "type_drawing.hpp"
#include <algorithm>
#include <cstdlib>

void TacticalClass::DrawRubberBand() {
    // 0x006DA195 tests the original band state, not a host mouse-drag flag.
    if(!Band.Left&&!Band.Top)return;
    const auto* drawing=game::active_type_drawing();
    const auto* resources=game::active_drawing_resources();
    if(!drawing||!resources||!resources->normal_palette){
        game::record_type_drawing_result(game::DrawingStatus::unavailable);return;
    }
    const auto& clip=resources->clip;
    const int left=std::min(Band.Left,Band.Right)+clip.X,top=std::min(Band.Top,Band.Bottom)+clip.Y;
    const int width=std::abs(Band.Right-Band.Left)+1,height=std::abs(Band.Bottom-Band.Top)+1;
    // NormalDrawer->Convert_Pixel(15), RGB565 host target. The four fills are
    // the backend decomposition of the original Surface::Draw_Rect call.
    const auto color=resources->normal_palette->Entries[15];
    for(const auto rect:{RectangleStruct{left,top,width,1},RectangleStruct{left,top+height-1,width,1},
            RectangleStruct{left,top,1,height},RectangleStruct{left+width-1,top,1,height}}){
        game::RasterDrawingRequest r;r.target=drawing->target;r.clip=clip;
        r.position={rect.X,rect.Y};r.width=rect.Width;r.height=rect.Height;
        r.color=std::uint16_t((color.R>>3)<<11|(color.G>>2)<<5|(color.B>>3));
        const auto status=game::submit_type_raster(*drawing,r);
        game::record_type_drawing_result(status);if(!game::drawing_completed(status))return;
    }
}

void TacticalClass::DrawActionLinesAndLinks() {
    const bool planning=PlanningNodeClass::PlanningModeActive;
    const int count=TechnoClass::Array.Count;
    for(int i=0;i<count;++i){auto* techno=TechnoClass::Array[i];
        if(techno->Owner->IsControlledByCurrentPlayer()&&!planning&&techno->IsSelected&&TechnoClass::ActionLines)
            techno->DrawActionLines(false,0);
        // Do not deduplicate managers: the controller and controlled-object
        // entries independently visit the same link drawer in original order.
        if(!planning&&techno->Health){
            if(techno->CaptureManager&&techno->CaptureManager->NeedsToDrawLinks())
                techno->CaptureManager->DrawLinks();
            if(techno->Health&&techno->MindControlledBy&&techno->MindControlledBy->CaptureManager
                &&techno->MindControlledBy->CaptureManager->NeedsToDrawLinks())
                techno->MindControlledBy->CaptureManager->DrawLinks();
        }
    }
}
