#include "sprite_drawing.hpp"
#include "yrpp/ObjectClass.h"
#include "yrpp/CellClass.h"
#include "presentation/object_draw.hpp"
namespace game {
bool sprite_drawing_complete(DrawingStatus s) noexcept {
    return s == DrawingStatus::drawn || s == DrawingStatus::skipped;
}
DrawingStatus with_sprite_drawing(SpriteDrawing& drawing, void (*call)(void*, PresentationContext&),
                                         void* argument, PresentationContext ctx) noexcept {
    ctx.sprite = &drawing;
    ctx.type = &drawing.types;
    drawing.status = DrawingStatus::skipped;
    try {
        if (!call) drawing.status = DrawingStatus::invalid_argument;
        else call(argument, ctx);
    } catch (...) { drawing.status = DrawingStatus::backend_failure; }
    return drawing.status;
}
DrawingStatus draw_object_sprite(const ObjectClass& object, SpriteDrawing& drawing, Point2D point,
                                 RectangleStruct clip, bool force_draw, PresentationContext outer) noexcept {
    struct Call {
        const ObjectClass& object;
        Point2D point;
        RectangleStruct clip;
        bool force_draw;
    } c{object, point, clip, force_draw};
    return with_sprite_drawing(
        drawing,
        [](void* p, game::PresentationContext& ctx) {
            auto& c = *static_cast<Call*>(p);
            draw_it(ctx, c.object, &c.point, &c.clip, c.force_draw);
        },
        &c, outer);
}
bool sprite_initialize_light(SpriteDrawing& d, CellClass& cell) noexcept {
    if (cell.LightConvert) return true;
    const auto s = d.initialize_light ? d.initialize_light(d.context, cell) : DrawingStatus::unavailable;
    if (!sprite_drawing_complete(s)) d.status = s;
    return sprite_drawing_complete(s);
}
bool sprite_submit(SpriteDrawing& d, SpritePalette palette, const AnimClass* anim, CellClass* cell,
                   ShapeDrawingRequest& r) noexcept {
    if (!sprite_drawing_complete(d.status)) return false;
    r.target = d.types.target;
    auto s = d.palette ? d.palette(d.context, palette, anim, cell, r.palette) : DrawingStatus::unavailable;
    if (s == DrawingStatus::drawn) s = submit_type_shape(d.types, r);
    if (s != DrawingStatus::skipped) d.status = s;
    return sprite_drawing_complete(s);
}
}
