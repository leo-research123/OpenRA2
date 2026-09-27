// Supplied 005FE5A0: original coordinate adjustment, frame and flag selection.
#include "yrpp/OverlayTypeClass.h"
#include "type_drawing.hpp"
#include <cstdint>
#include <limits>

game::DrawingStatus OverlayTypeClass::Draw(const game::TypeDrawingContext& context,
        const Point2D& point, const RectangleStruct& clip, int frame) noexcept {
    try {
        const auto x = std::int64_t(point.X) + context.overlay_offset.X + 30;
        const auto y = std::int64_t(point.Y) + context.overlay_offset.Y + 15;
        if (x < std::numeric_limits<int>::min() || x > std::numeric_limits<int>::max() ||
            y < std::numeric_limits<int>::min() || y > std::numeric_limits<int>::max())
            return game::DrawingStatus::invalid_argument;
        game::ShapeDrawingRequest request;
        request.target = context.target;
        request.palette = context.palette;
        // The modern path must not enter the EXE through an original vtable.
        // Demand-loading resources are not available without a calibrated host.
        request.image = OverlayTypeClass::GetImage();
        if (!request.image) return ImageLoaded ? game::DrawingStatus::unavailable : game::DrawingStatus::skipped;
        request.frame = frame;
        request.position = {static_cast<int>(x), static_cast<int>(y)};
        request.clip = clip;
        request.flags = 0x600;
        request.intensity = 1000;
        return game::submit_type_shape(context, request);
    } catch (...) { return game::DrawingStatus::backend_failure; }
}
void OverlayTypeClass::Draw(Point2D* point, RectangleStruct* clip, int frame) {
    const auto* context = game::active_type_drawing();
    if (!context) { DrawOriginal(point, clip, frame); return; }
    game::record_type_drawing_result(point && clip ? Draw(*context, *point, *clip, frame)
        : game::DrawingStatus::invalid_argument);
}
