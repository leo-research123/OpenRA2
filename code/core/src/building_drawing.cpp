#include "building_drawing.hpp"
#include "yrpp/BuildingClass.h"
#include "presentation/builders/building.hpp"
namespace game {
DrawingStatus with_building_drawing(BuildingDrawing& drawing, void (*call)(void*, PresentationContext&),
                                         void* argument, PresentationContext ctx) noexcept {
    ctx.building = &drawing;
    ctx.type = &drawing.context;
    drawing.status = DrawingStatus::skipped;
    try {
        if (!call) drawing.status = DrawingStatus::invalid_argument;
        else call(argument, ctx);
    } catch (...) { drawing.status = DrawingStatus::backend_failure; }
    return drawing.status;
}
DrawingStatus draw_building_parts(BuildingClass& building, const TypeDrawingContext& context, SHPStruct* depth,
                                  int intensity, Point2D point, RectangleStruct clip, bool upper,
                                  PresentationContext outer) noexcept {
    BuildingDrawing drawing{context, depth, intensity};
    try {
        auto ctx = inherit_scopes(outer);
        ctx.building = &drawing;
        ctx.type = &context;
        if (upper)
            build_building_upper(ctx, building, point, clip);
        else
            build_building(ctx, building, &point, &clip);
    } catch (...) {
        drawing.status = DrawingStatus::backend_failure;
    }
    return drawing.status;
}
}
