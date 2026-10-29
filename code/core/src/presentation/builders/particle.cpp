// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 particle.cpp::Draw_It.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// YR 0x62CEC0 spark/railgun pixel branch; EA terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/FPSCounter.h"
#include "map_runtime.hpp"
#include "type_drawing.hpp"
#include "yrpp/RulesClassReaders.hpp"
#include <bit>

#include "particle_shape.hpp"
#include "presentation/builders/particle.hpp"
namespace game {
void build_particle(PresentationContext& ctx, const ParticleClass& self, Point2D* position, RectangleStruct* bounds) {
    const auto* context = ctx.type;
    if (!context || !self.Type || !bounds) return;
    const auto minimum = Detail::GetMinFrameRate();
    if (FPSCounter::CurrentFrameRate < minimum && !self.Type->Damage) return;
    if (!GameOptionsClass::Instance.DetailLevel &&
        (self.Type->BehavesLike == ParticleBehavior::Smoke || self.Type->BehavesLike == ParticleBehavior::Spark))
        return;
    const auto& runtime = game::map_runtime();
    if (!(runtime.debug_map && *runtime.debug_map) && runtime.has_window && runtime.has_window() &&
        ScenarioClass::Instance->SpecialFlags.FogOfWar && MapClass::Instance.IsLocationFogged(self.Location))
        return;
    if (self.Type->BehavesLike != ParticleBehavior::Spark && self.Type->BehavesLike != ParticleBehavior::Railgun) {
        const int depth = -15 - TacticalClass::AdjustForZ(self.GetZ());
        auto* image = self.GetImage();
        if (!image || !position) return;
        // 0x62D015 selects ANIM_PAL for every SHP particle, independent of its behavior.
        game::ShapeDrawingRequest r;
        r.target = context->target;
        if (!game::resolve_drawing_palette(ctx, game::DrawingPaletteKind::animation, -1, r.palette)) return;
        r.image = image;
        r.frame = particle_shape_number(self);
        r.position = *position;
        r.clip = *bounds;
        r.flags = 0x2E00;
        r.gradient = 2;
        r.intensity = 1000;
        if (GameOptionsClass::Instance.DetailLevel == 2) {
            if (self.Translucency == 25)
                r.flags |= 2;
            else if (self.Translucency == 50)
                r.flags |= 4;
            else if (static_cast<signed char>(self.Translucency) >= 75)
                r.flags |= 6;
        }
        r.depth_adjustment = depth;
        game::record_type_drawing_result(ctx, game::submit_type_shape(*context, r));
        return;
    }
    if (self.Type->BehavesLike != ParticleBehavior::Spark && self.Type->BehavesLike != ParticleBehavior::Railgun) {
        game::record_type_drawing_result(ctx, game::DrawingStatus::unsupported);
        return;
    }
    if (self.ColorIndex < 0 || self.ColorIndex + 1 >= self.Type->ColorList.Count) {
        game::record_type_drawing_result(ctx, game::DrawingStatus::invalid_argument);
        return;
    }
    Point2D point{};
    if (!TacticalClass::CoordsToClient(self.Location, TacticalClass::Instance->TacticalPos, *bounds, point)) return;
    const auto a = self.ColorIndex ? self.Type->ColorList[self.ColorIndex] : self.Color, b = self.Type->ColorList[self.ColorIndex + 1];
    const int red = rule_integer(a.Red * (1.0 - self.ColorAccum) + b.Red * self.ColorAccum);
    const int green = rule_integer(a.Green * (1.0 - self.ColorAccum) + b.Green * self.ColorAccum);
    const int blue = rule_integer(a.Blue * (1.0 - self.ColorAccum) + b.Blue * self.ColorAccum);
    game::RasterDrawingRequest r;
    r.target = context->target;
    r.position = {point.X + bounds->X, point.Y + bounds->Y};
    r.clip = *bounds;
    r.width = r.height = 1;
    r.blend_mode = game::RasterBlendMode::particle;
    r.line_rgb = unsigned(red) | (unsigned(green) << 8) | (unsigned(blue) << 16);
    r.line_z = -TacticalClass::AdjustForZ(self.Location.Z) - 50;
    game::record_type_drawing_result(ctx, game::submit_type_raster(*context, r));
}
}
