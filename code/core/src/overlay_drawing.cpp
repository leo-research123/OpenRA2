#include "overlay_drawing.hpp"
#include "yrpp/CellClass.h"
#include "presentation/builders/cell.hpp"
namespace game {
DrawingStatus with_overlay_drawing(OverlayDrawing& drawing, void (*call)(void*, PresentationContext&),
                                         void* argument, PresentationContext ctx) noexcept {
    ctx.overlay = &drawing;
    if (!ctx.type) ctx.type = &drawing.types;
    drawing.status = DrawingStatus::skipped;
    try {
        if (!call) drawing.status = DrawingStatus::invalid_argument;
        else call(argument, ctx);
    } catch (...) { drawing.status = DrawingStatus::backend_failure; }
    return drawing.status;
}
DrawingStatus draw_cell_overlay(CellClass& cell, OverlayDrawing& drawing, const Point2D& point,
                                const RectangleStruct& clip, bool shadow, PresentationContext outer) noexcept {
    struct Call {
        CellClass& cell;
        const Point2D& point;
        const RectangleStruct& clip;
        bool shadow;
    } call{cell, point, clip, shadow};
    return with_overlay_drawing(
        drawing,
        [](void* p, game::PresentationContext& ctx) {
            auto& c = *static_cast<Call*>(p);
            if (c.shadow)
                build_cell_overlay_shadow(ctx, c.cell, c.point, c.clip);
            else
                build_cell_overlay(ctx, c.cell, c.point, c.clip);
        },
        &call, outer);
}
}
