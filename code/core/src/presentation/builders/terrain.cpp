// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 terrain.cpp::Draw_It.
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// YR 0x0071C1B0: no TS YDrawFudge; animated flags 0x2E00; shadow Z bias -3. 0x0071C360 DrawAgain.
#include "presentation/builders/terrain.hpp"
#include "yrpp/TerrainClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Drawing.h"
#include "sprite_drawing.hpp"
#include "tactical_cells.hpp"
#include <algorithm>
#include <bit>

namespace game {
void build_terrain_again(PresentationContext& ctx, const TerrainClass& self, const Point2D& point, const RectangleStruct& clip) {
    auto* drawing = ctx.sprite;
    if (!drawing) return;
    if (!self.Type || !drawing->cell_at || !drawing->height || !drawing->shape_data) {
        drawing->status = game::DrawingStatus::unavailable;
        return;
    }
    const auto coords = self.GetMapCoords();
    auto* cell = drawing->cell_at(drawing->context, coords);
    auto* image = self.GetImage();
    if (!image) return;
    if (!cell || !game::sprite_initialize_light(*drawing, *cell)) return;
    const int depth = -drawing->height(self.GetZ());
    game::ShapeDrawingRequest request;
    request.image = image;
    request.frame = self.IsCrumbling ? int(self.IsCrumbling) + self.Animation.Value : 0;
    request.position = point;
    request.clip = clip;
    request.flags = 0xE00;
    request.depth_adjustment = depth - 4;
    request.gradient = 2;
    request.intensity = std::bit_cast<short>(self.Type->SpawnsTiberium ? cell->Intensity_Normal : cell->Intensity_Terrain);
    if (self.Type->SpawnsTiberium) request.position.Y -= 16;
    if (!game::sprite_submit(*drawing, self.Type->SpawnsTiberium ? game::SpritePalette::tiberium : game::SpritePalette::cell,
                             nullptr, cell, request)) return;
    if (drawing->draw_shadows) {
        auto* shape = drawing->shape_data(image);
        if (!shape) {
            drawing->status = game::DrawingStatus::unavailable;
            return;
        }
        request.frame += shape->Frames / 2;
        request.flags = 0x601;
        request.depth_adjustment = depth - 2;
        request.gradient = 0;
        request.intensity = 1000;
        game::sprite_submit(*drawing, game::SpritePalette::cell, nullptr, cell, request);
    }
}


void build_terrain(PresentationContext& ctx, const TerrainClass& self, Point2D* point, RectangleStruct* clip) {
    auto* d = ctx.sprite;
    if (!d) return;
    if (!point || !clip || !self.Type) {
        d->status = game::DrawingStatus::invalid_argument;
        return;
    }
    if (!d->cell_at || !d->height || !d->shape_data) {
        d->status = game::DrawingStatus::unavailable;
        return;
    }
    CellStruct coords;
    auto* cell = d->cell_at(d->context, *self.GetMapCoords(&coords));
    auto* image = self.GetImage();
    if (!image) return;
    if (!cell) {
        d->status = game::DrawingStatus::unavailable;
        return;
    }
    int frame = self.Type->IsAnimated ? self.Animation.Value : self.IsCrumbling ? self.Animation.Value + 1 : self.Health < 2;
    const int z = -d->height(self.GetZ());
    if (!game::sprite_initialize_light(*d, *cell)) return;
    game::ShapeDrawingRequest r;
    r.image = image;
    r.frame = frame;
    r.position = *point;
    r.clip = *clip;
    r.flags = self.Type->IsAnimated || self.IsCrumbling ? 0x2E00 : 0x4E00;
    r.depth_adjustment = z - 12;
    r.gradient = 2;
    r.intensity = std::bit_cast<short>(self.Type->SpawnsTiberium ? cell->Intensity_Normal : cell->Intensity_Terrain);
    if (self.Type->SpawnsTiberium) r.position.Y -= 16;
    if (!game::sprite_submit(*d, self.Type->SpawnsTiberium ? game::SpritePalette::tiberium : game::SpritePalette::cell,
                             nullptr, cell, r))
        return;
    if (d->draw_shadows) {
        auto* shape = d->shape_data(image);
        if (!shape) {
            d->status = game::DrawingStatus::unavailable;
            return;
        }
        r.frame += shape->Frames / 2;
        r.flags |= 1;
        r.depth_adjustment = z - 3;
        r.gradient = 0;
        r.intensity = 1000;
        game::sprite_submit(*d, game::SpritePalette::cell, nullptr, cell, r);
    }
}


void build_terrain_objects(PresentationContext& ctx, bool forced, const RectangleStruct& area, const RectangleStruct& clip) {
    try {
        const auto intersection = Drawing::Intersect(area, clip), bounds = tactical_pass_viewport(ctx);
        auto& layer = MapClass::ObjectsInLayers[int(Layer::Ground)];
        for (int i = layer.Count - 1; i >= 0 && tactical_pass_complete(ctx); --i) {
            auto* object = layer[i];
            if (object->WhatAmI() != AbstractType::Terrain || object->IsDead()) continue;
            auto* terrain = static_cast<TerrainClass*>(object);
            if (!terrain->Type || terrain->Type->IsAnimated || terrain->IsCrumbling) continue;
            RectangleStruct dimensions;
            object->GetRenderDimensions(&dimensions);
            dimensions.X += bounds.X;
            dimensions.Y += bounds.Y;
            if (!tactical_rects_overlap(dimensions, intersection)) continue;
            game::record_tactical_drawing(ctx, game::draw_tactical_object(ctx, *object, forced, clip, false));
        }
    } catch (...) {
        game::record_tactical_drawing(ctx, game::DrawingStatus::backend_failure);
    }
}
}
