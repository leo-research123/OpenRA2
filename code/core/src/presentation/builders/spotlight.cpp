// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 OpenTS contributors; adaptations Copyright 2026 OpenRA2.
// OpenTS ovrlight.cpp/xsurface.cpp, 44fac744f70235e0d5ddca107364a68f95132ce9.
// YR 0x005FF850 pool lookup, clipping and RGB565 destination brightening.
#include "yrpp/SpotlightClass.h"
#include "presentation/builders/spotlight.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "type_drawing.hpp"
#include "images/drawing_resources.hpp"
#include <array>
#include <algorithm>
#include <memory>
namespace game {
namespace {
void circle(std::array<std::uint16_t, 256 * 128>& pixels, int radius, int color) {
    if (radius < 0) return;
    auto row = [&](int y, int x1, int x2) {
        y += 128;
        if (y < 0 || y >= 255 || (y & 1)) return;
        for (int x = std::max(0, 128 + x1); x <= std::min(254, 128 + x2); ++x)
            pixels[(y / 2) * 256 + x] = std::uint16_t(color);
    };
    int x = radius, y = 0, d = 3 - 2 * radius;
    do {
        row(y, -x, x);
        row(x, -y, y);
        row(-y, -x, x);
        row(-x, -y, y);
        if (d < 0)
            d += 4 * y + 6;
        else {
            d += 4 * (y - x) + 10;
            --x;
        }
        ++y;
    } while (x >= y);
}
struct SpotlightMask {
    std::array<std::uint16_t, 256 * 128> pixels{};
    const std::uint64_t identity = game::allocate_raster_content_identity();
    std::uint64_t version = 1;
    int radius = 0;
};
const SpotlightMask& mask(int index, int radius) {
    // There are 74 original pool entries. Only requested masks are materialized.
    static std::array<std::unique_ptr<SpotlightMask>, 74> cache;
    auto& entry = cache[index];
    if (!entry) {
        entry = std::make_unique<SpotlightMask>();
        entry->radius = radius;
        if (index < 64) {
            for (int r = 2 * index + 1, color = -2; r > 0; r -= 2, color += 4)
                circle(entry->pixels, r, std::max(0, color));
        } else
            circle(entry->pixels, radius, 128 - (index - 64) * 6);
    } else if (index >= 64 && entry->radius != radius) {
        entry->pixels.fill(0);
        entry->radius = radius;
        ++entry->version;
        circle(entry->pixels, radius, 128 - (index - 64) * 6);
    }
    return *entry;
}
}
void build_spotlight(PresentationContext& ctx, const CoordStruct& Coords, int Size, int MovementRadius, SpotlightFlags DisableFlags) {
    const auto* context = ctx.type;
    auto* tactical = TacticalClass::Instance;
    if (!context || !tactical || !RulesClass::Instance) return;
    if (ScenarioClass::Instance && ScenarioClass::Instance->SpecialFlags.FogOfWar) {
        auto* cell = MapClass::Instance.TryGetCellAt(Coords);
        if (!cell || cell->IsFogged()) return;
    }
    Point2D point{};
    if (!TacticalClass::CoordsToClient(Coords, tactical->TacticalPos, TacticalClass::ViewBounds, point)) return;
    static constexpr int indices[90] = {5,  10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 61, 62, 63, 63, 63, 62,
                                        61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44,
                                        43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26,
                                        25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9,  8,
                                        7,  6,  5,  4,  3,  2,  1,  0,  64, 65, 66, 67, 68, 69, 70, 71, 72, 73};
    if (MovementRadius < 0 || MovementRadius >= 90) return;
    int index = indices[MovementRadius];
    if (index < 64) index = index * Size / 64;
    if (index < 0 || index >= 74) return;
    try {
        const int radius = index < 64 ? 0 : index - 64 - int((30 * RulesClass::Instance->SpotlightRadius) / -358.4);
        const auto& source = mask(index, radius);
        const auto bounds = TacticalClass::ViewBounds;
        game::RasterDrawingRequest r;
        r.target = context->target;
        r.position = {point.X + bounds.X - 128, point.Y + bounds.Y - 64};
        r.clip = bounds;
        r.width = 255;
        r.height = 127;
        r.pitch = 256;
        r.pixels = source.pixels.data();
        r.pixel_count = source.pixels.size();
        r.blend_mode = game::RasterBlendMode::spotlight;
        r.spotlight_flags = unsigned(DisableFlags);
        r.content_identity = source.identity;
        r.content_version = source.version;
        game::record_type_drawing_result(ctx, game::submit_type_raster(*context, r));
    } catch (...) {
        game::record_type_drawing_result(ctx, game::DrawingStatus::backend_failure);
    }
}
void build_spotlights(PresentationContext& ctx) {
    for (int i=SpotlightClass::Array.Count-1;i>=0;--i) {
        const auto& light=*SpotlightClass::Array[i];
        build_spotlight(ctx, light.Coords, light.Size, light.MovementRadius, light.DisableFlags);
    }
}
}
