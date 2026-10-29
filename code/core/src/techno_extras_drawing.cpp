#include "techno_extras_drawing.hpp"
#include "presentation/builders/extras.hpp"
#include "presentation/context.hpp"
#include "type_drawing.hpp"
#include "yrpp/BuildingClass.h"

namespace game {


void collect_line_status(const PresentationContext& ctx, TechnoExtrasDrawing& frame) noexcept {
    const auto lines = type_drawing_status(ctx);
    if (lines != DrawingStatus::skipped && drawing_completed(frame.status)) frame.status = lines;
}

DrawingStatus with_techno_extras_drawing(TechnoExtrasDrawing& frame, void (*call)(void*, PresentationContext&),
                                         void* argument, PresentationContext ctx) noexcept {
    ctx.extras = &frame;
    frame.status = DrawingStatus::skipped;
    try {
        if (!call) frame.status = DrawingStatus::invalid_argument;
        else {
            struct Call { void (*operation)(void*, PresentationContext&); void* argument; } forward{call, argument};
            const auto lines = with_type_drawing(frame.drawing, [](void* p, PresentationContext& scoped) {
                auto& c = *static_cast<Call*>(p);
                c.operation(c.argument, scoped);
            }, &forward, ctx);
            if (lines != DrawingStatus::skipped && drawing_completed(frame.status)) frame.status = lines;
        }
    } catch (...) { frame.status = DrawingStatus::backend_failure; }
    return frame.status;
}

DrawingStatus draw_building_health(const BuildingClass& building, const TypeDrawingContext& drawing, SHPStruct* pips,
                                   const DrawingPaletteHandle* palette, Point2D point,
                                   RectangleStruct bounds) noexcept {
    TechnoExtrasDrawing frame{drawing, pips, palette};
    struct Call {
        const BuildingClass& building;
        Point2D point;
        RectangleStruct bounds;
    } call{building, point, bounds};
    return with_techno_extras_drawing(
        frame,
        [](void* p, game::PresentationContext& ctx) {
            auto& c = *static_cast<Call*>(p);
            build_health_bar(ctx, c.building, &c.point, &c.bounds, false);
        },
        &call);
}

DrawingStatus draw_techno_extras(const TechnoClass& techno, TechnoExtrasDrawing& frame, Point2D point,
                                 RectangleStruct bounds) noexcept {
    struct Call {
        const TechnoClass& techno;
        Point2D point;
        RectangleStruct bounds;
    } call{techno, point, bounds};
    return with_techno_extras_drawing(
        frame,
        [](void* p, game::PresentationContext& ctx) {
            auto& c = *static_cast<Call*>(p);
            build_techno_extras(ctx, c.techno, &c.point, &c.bounds);
        },
        &call);
}

unsigned selection_color(const TechnoExtrasDrawing& frame, int index) noexcept {
    const auto c = frame.selection_palette->Entries[index];
    return unsigned((c.R >> 3) << 11 | (c.G >> 2) << 5 | (c.B >> 3));
}
}
