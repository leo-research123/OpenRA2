// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
// OpenTS 44fac744 cell.cpp Draw_Shroud_And_Fog / Draw_Shadow_Cast.
// YR 0x004801F0 / 0x004802A0. Actual ABuffer writes use the typed backend.
#include "yrpp/CellClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/HouseClass.h"
#include "tactical_drawing.hpp"
#include "debug/debug_state.hpp"


#include "debug/debug_state.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/AlphaShapeClass.h"
#include "tactical_cells.hpp"
#include "map_runtime.hpp"
#include <algorithm>
#include "presentation/builders/shroud.hpp"
#include "presentation/builders/alpha.hpp"
namespace game {
void build_cell_shroud(PresentationContext& ctx, const CellClass& self, const Point2D& point, const RectangleStruct& clip) {
    if (game::debug_fog_hidden()) return;
    if (!TacticalClass::Instance) return;
    const char visibility = TacticalClass::Instance->GetOcclusion(self.MapCoords, false);
    const bool fog = ScenarioClass::Instance && ScenarioClass::Instance->SpecialFlags.FogOfWar;
    const auto index = [](char state) { return state == -2 ? 15 : state == -1 ? 0 : int(state); };
    // YR also submits frame zero: shroud zero pixels are valid ABuffer writes.
    {
        auto* image = static_cast<SHPStruct*>(FileSystem::LoadFile(fog ? "FOG.SHP" : "SHROUD.SHP", true));
        game::record_tactical_drawing(ctx, 
            game::draw_lighting_shape(ctx, image, index(visibility), point, clip, game::RasterBlendMode::shroud));
    }
    // YR updates fog state after submitting the shroud draw.
    const char foggedness = TacticalClass::Instance->GetOcclusion(self.MapCoords, true);
    // YR checks CurrentPlayer->Defeated (+0x1F5), unlike OpenTS Session.ObiWan.
    if (fog && (!HouseClass::CurrentPlayer || !HouseClass::CurrentPlayer->Defeated)) {
        auto* image = static_cast<SHPStruct*>(FileSystem::LoadFile("FOG.SHP", true));
        game::record_tactical_drawing(ctx, 
            game::draw_lighting_shape(ctx, image, index(foggedness), point, clip, game::RasterBlendMode::fog));
    }
}

void build_shroud(PresentationContext& ctx, TacticalClass& tactical, const RectangleStruct& area) {
    try {
        const auto bounds = game::tactical_pass_viewport(ctx);
        // The same requests in the same order; a host may resolve the batch per pixel.
        const bool batched = game::begin_lighting_batch(ctx);
        game::tactical_walk_cells(ctx, tactical, area, 17, 4, 0, true, [&](CellClass& cell, const Point2D& at) {
            const RectangleStruct footprint{at.X, bounds.Y + at.Y, 60, 30};
            const auto clip = Drawing::Intersect(bounds, Drawing::Intersect(area, footprint));
            if (clip.Width > 0 && clip.Height > 0) {
                if (batched) game::lighting_batch_cell(ctx, cell.MapCoords);
                build_cell_shroud(ctx, cell, {at.X + bounds.X, at.Y + bounds.Y}, clip);
            }
        });
        if (batched) game::lighting_batch_cell(ctx, {-1, -1});
        if (game::tactical_pass_complete(ctx)) build_alpha_shapes(ctx, area);
        if (batched) {
            if (game::tactical_pass_complete(ctx))
                game::record_tactical_drawing(ctx, game::end_lighting_batch(ctx, bounds));
            else
                game::cancel_lighting_batch(ctx);
        }
    } catch (...) {
        game::cancel_lighting_batch(ctx);
        game::record_tactical_drawing(ctx, game::DrawingStatus::backend_failure);
    }
}

}
