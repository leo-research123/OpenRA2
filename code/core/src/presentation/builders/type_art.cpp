// Original type-class drawing on an explicit context (declared in
// api/type_drawing.hpp): the tile of IsometricTileTypeClass::DrawTMP (YR 0x00547CF0), OverlayTypeClass::Draw
// (0x005FE5A0) and SmudgeTypeClass::DrawIt (0x006B55F0). Resource and flag selection follow the original functions
// named there. The cell builder calls the tile and smudge functions; the class methods keep traps.
#include "type_drawing_context.hpp"
#include "type_drawing.hpp"
#include "verify/draw_coverage.hpp"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include <cstdint>
#include <limits>

namespace game {
DrawingStatus draw_isometric_tile(IsometricTileTypeClass& tile, const TypeDrawingContext& context, int subTile, int x,
                                  int y, RectangleStruct clip, int level, int intensity, bool useZ, int alternate,
                                  bool flat, bool flag16, bool flag17, int color) noexcept {
    draw_coverage_instance(DrawCategory::tile);
    if (subTile < 0 || alternate < 0) return DrawingStatus::invalid_argument;
    if (clip.Width <= 0 || clip.Height <= 0) return DrawingStatus::skipped;
    try {
        auto* owner = &tile;
        if (alternate) {
            if (tile.unk_2F0 <= 0) return DrawingStatus::invalid_argument;
            alternate %= tile.unk_2F0;
            while (alternate-- && owner)
                owner = owner->NextVariant;
            if (!owner) return DrawingStatus::invalid_argument;
        }
        const auto* tmp = reinterpret_cast<const TMPStruct*>(owner->IsometricTileTypeClass::GetImage());
        if (!tmp) return DrawingStatus::skipped;
        if (tmp->Width != 60 || tmp->Height != 30 || tmp->Columns <= 0 || tmp->Rows <= 0 || tmp->Columns > 255 ||
            tmp->Rows > 255)
            return DrawingStatus::invalid_argument;
        TileDrawingRequest request;
        request.sub_tile = subTile % (tmp->Columns * tmp->Rows);
        if (!tmp->GetSubTile(request.sub_tile, request.image)) return DrawingStatus::skipped;
        request.target = context.target;
        request.palette = context.palette;
        request.resource = tmp;
        request.position = {x, y};
        request.clip = clip;
        request.level = level;
        request.intensity = intensity;
        request.use_depth = useZ;
        request.flat = flat;
        request.flag16 = flag16;
        request.flag17 = flag17;
        request.color = color;
        request.buffer_offset_y = context.buffer_offset_y;
        request.translucent = context.translucent;
        return submit_type_tile(context, request);
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus draw_overlay_type(OverlayTypeClass& type, const TypeDrawingContext& context, const Point2D& point,
                                const RectangleStruct& clip, int frame) noexcept {
    draw_coverage_instance(DrawCategory::overlay);
    try {
        const auto x = std::int64_t(point.X) + context.overlay_offset.X + 30;
        const auto y = std::int64_t(point.Y) + context.overlay_offset.Y + 15;
        if (x < std::numeric_limits<int>::min() || x > std::numeric_limits<int>::max() ||
            y < std::numeric_limits<int>::min() || y > std::numeric_limits<int>::max())
            return DrawingStatus::invalid_argument;
        ShapeDrawingRequest request;
        request.target = context.target;
        request.palette = context.palette;
        // The modern path must not enter the EXE through an original vtable.
        // Demand-loading resources are not available without a calibrated host.
        request.image = type.OverlayTypeClass::GetImage();
        if (!request.image) return type.ImageLoaded ? DrawingStatus::unavailable : DrawingStatus::skipped;
        request.frame = frame;
        request.position = {static_cast<int>(x), static_cast<int>(y)};
        request.clip = clip;
        request.flags = 0x600;
        request.intensity = 1000;
        return submit_type_shape(context, request);
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus draw_smudge_type(SmudgeTypeClass& type, const TypeDrawingContext& context, const Point2D& point,
                               const RectangleStruct& clip, int data, int height, const CellStruct& cell) noexcept {
    draw_coverage_instance(DrawCategory::smudge);
    try {
        auto* image = type.ObjectTypeClass::GetImage();
        if (!image) return DrawingStatus::skipped;
        if (!context.smudge_cell) return DrawingStatus::unavailable;
        if (data && type.Width <= 0) return DrawingStatus::invalid_argument;
        std::int64_t x = point.X, y = point.Y;
        if (data) {
            const auto row = std::int64_t(data) / type.Width, column = std::int64_t(data) % type.Width;
            x += 30 * (row - column);
            y -= 15 * (row + column);
        }
        if (x < std::numeric_limits<int>::min() || x > std::numeric_limits<int>::max() ||
            y < std::numeric_limits<int>::min() || y > std::numeric_limits<int>::max())
            return DrawingStatus::invalid_argument;
        ShapeDrawingRequest request;
        request.target = context.target;
        auto status = context.smudge_cell(context.world_context, cell, request.palette, request.intensity);
        if (status != DrawingStatus::drawn) return status;
        int projected = 0;
        if (height) {
            if (!context.height_to_pixels) return DrawingStatus::unavailable;
            status = context.height_to_pixels(context.world_context, height, projected);
            if (status != DrawingStatus::drawn) return status;
        }
        request.image = image;
        request.frame = 0;
        request.position = {static_cast<int>(x), static_cast<int>(y)};
        request.clip = clip;
        request.flags = 0xE00;
        const auto depth_adjustment = -1ll - projected;
        if (depth_adjustment < std::numeric_limits<int>::min() || depth_adjustment > std::numeric_limits<int>::max())
            return DrawingStatus::invalid_argument;
        request.depth_adjustment = static_cast<int>(depth_adjustment);
        request.gradient = 0;
        return submit_type_shape(context, request);
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
}
