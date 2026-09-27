// YR 0x006B55F0. World lookup/projection and the renderer are separate inputs.
#include "yrpp/SmudgeTypeClass.h"
#include "type_drawing.hpp"
#include <cstdint>
#include <limits>

game::DrawingStatus SmudgeTypeClass::DrawIt(const game::TypeDrawingContext& context,
        const Point2D& point, const RectangleStruct& clip, int data, int height,
        const CellStruct& cell) noexcept {
    try {
        auto* image = ObjectTypeClass::GetImage();
        if (!image) return game::DrawingStatus::skipped;
        if (!context.smudge_cell) return game::DrawingStatus::unavailable;
        if (data && Width <= 0) return game::DrawingStatus::invalid_argument;
        std::int64_t x = point.X, y = point.Y;
        if (data) {
            const auto row = std::int64_t(data) / Width, column = std::int64_t(data) % Width;
            x += 30 * (row - column);
            y -= 15 * (row + column);
        }
        if (x < std::numeric_limits<int>::min() || x > std::numeric_limits<int>::max() ||
            y < std::numeric_limits<int>::min() || y > std::numeric_limits<int>::max())
            return game::DrawingStatus::invalid_argument;
        game::ShapeDrawingRequest request;
        request.target = context.target;
        auto status = context.smudge_cell(context.world_context, cell, request.palette, request.intensity);
        if (status != game::DrawingStatus::drawn) return status;
        int projected = 0;
        if (height) {
            if (!context.height_to_pixels) return game::DrawingStatus::unavailable;
            status = context.height_to_pixels(context.world_context, height, projected);
            if (status != game::DrawingStatus::drawn) return status;
        }
        request.image = image;
        request.frame = 0;
        request.position = {static_cast<int>(x), static_cast<int>(y)};
        request.clip = clip;
        request.flags = 0xE00;
        const auto depth_adjustment = -1ll - projected;
        if (depth_adjustment < std::numeric_limits<int>::min() || depth_adjustment > std::numeric_limits<int>::max())
            return game::DrawingStatus::invalid_argument;
        request.depth_adjustment = static_cast<int>(depth_adjustment);
        request.gradient = 0;
        return game::submit_type_shape(context, request);
    } catch (...) { return game::DrawingStatus::backend_failure; }
}
void SmudgeTypeClass::DrawIt(const Point2D& point, const RectangleStruct& clip,
        int data, int height, const CellStruct& cell) {
    const auto* context = game::active_type_drawing();
    if (!context) { DrawItOriginal(point, clip, data, height, cell); return; }
    game::record_type_drawing_result(DrawIt(*context, point, clip, data, height, cell));
}
