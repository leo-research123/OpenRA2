// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
// OpenTS 44fac744 alphashp.cpp Draw_All; YR 0x00421350.
#include "yrpp/AlphaShapeClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/ObjectTypeClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Drawing.h"
#include "tactical_drawing.hpp"
#include "presentation/builders/alpha.hpp"
#include "map_world_internal.hpp"
namespace game {
SHPStruct* alpha_presentation_image(PresentationState* state, const AlphaShapeClass& shape) {
    if (shape.AlphaImage) return shape.AlphaImage;
    if (state) {
        const auto found = state->alpha_images.find(&shape);
        if (found != state->alpha_images.end()) return found->second;
    }
    const auto* type = shape.AttachedTo ? shape.AttachedTo->GetType() : nullptr;
    auto* image = type ? type->AlphaImage : nullptr;
    if (image && state) state->alpha_images.emplace(&shape, image);
    return image;
}
void build_alpha_shapes(PresentationContext& ctx, const RectangleStruct& clip) {
    auto* frame = ctx.frame;
    if (!frame || !TacticalClass::Instance) return;
    try {
        const auto camera = TacticalClass::Instance->TacticalPos;
        for (auto* shape : AlphaShapeClass::Array) {
            auto rect = shape->Rect;
            rect.X += frame->bounds.X - camera.X;
            rect.Y += frame->bounds.Y - camera.Y;
            const auto intersection = Drawing::Intersect(rect, clip);
            if (intersection.Width <= 0 || intersection.Height <= 0) continue;
            auto* state = frame->world ? &frame->world->impl->presentation : nullptr;
            auto* image = alpha_presentation_image(state, *shape);
            const auto status = game::draw_lighting_shape(ctx, image, 0, {rect.X, rect.Y}, clip,
                                                          game::RasterBlendMode::alpha_shape);
            game::record_tactical_drawing(ctx, status);
            if (status != game::DrawingStatus::drawn && status != game::DrawingStatus::skipped) return;
        }
    } catch (...) {
        game::record_tactical_drawing(ctx, game::DrawingStatus::backend_failure);
    }
}

}
