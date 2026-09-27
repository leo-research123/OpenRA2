#include "sprite_drawing.hpp"
#include "yrpp/ObjectClass.h"
#include "yrpp/CellClass.h"
namespace game {
namespace { thread_local SpriteDrawing* current = nullptr; }
SpriteDrawing* sprite_drawing() noexcept { return current; }
bool sprite_drawing_complete(DrawingStatus s) noexcept {
    return s == DrawingStatus::drawn || s == DrawingStatus::skipped;
}
DrawingStatus with_sprite_drawing(SpriteDrawing& drawing,void(*call)(void*),void* context) noexcept {
    auto* previous = current;
    current = &drawing;
    drawing.status = DrawingStatus::skipped;
    try { if(call)call(context);else drawing.status=DrawingStatus::invalid_argument; }
    catch (...) { drawing.status = DrawingStatus::backend_failure; }
    current = previous;
    return drawing.status;
}
DrawingStatus draw_object_sprite(const ObjectClass& object, SpriteDrawing& drawing,
        Point2D point, RectangleStruct clip) noexcept {
    struct Call{const ObjectClass& object;Point2D point;RectangleStruct clip;} c{object,point,clip};
    return with_sprite_drawing(drawing,[](void* p){auto& c=*static_cast<Call*>(p);c.object.DrawIt(&c.point,&c.clip);},&c);
}
bool sprite_initialize_light(SpriteDrawing& d, CellClass& cell) noexcept {
    if (cell.LightConvert) return true;
    const auto s = d.initialize_light ? d.initialize_light(d.context, cell) : DrawingStatus::unavailable;
    if (!sprite_drawing_complete(s)) d.status = s;
    return sprite_drawing_complete(s);
}
bool sprite_submit(SpriteDrawing& d, SpritePalette palette, const AnimClass* anim,
        CellClass* cell, ShapeDrawingRequest& r) noexcept {
    if (!sprite_drawing_complete(d.status)) return false;
    r.target = d.types.target;
    auto s = d.palette ? d.palette(d.context, palette, anim, cell, r.palette) : DrawingStatus::unavailable;
    if (s == DrawingStatus::drawn) s = submit_type_shape(d.types, r);
    if (s != DrawingStatus::skipped) d.status = s;
    return sprite_drawing_complete(s);
}
}
