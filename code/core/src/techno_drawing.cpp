#include "techno_drawing.hpp"
#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/CellClass.h"
#include "building_drawing.hpp"
#include "presentation/object_draw.hpp"
#include "debug/debug_state.hpp"
#include "yrpp/MapClass.h"
#include <bit>
namespace game {
void accumulate_unit_voxel_bounds(RectangleStruct& out, const RectangleStruct& r) noexcept {
    if (out.Width <= 0 || out.Height <= 0) {
        out = r;
        return;
    }
    if (r.Width <= 0 || r.Height <= 0) return;
    if (out.X > r.X) {
        out.Width += out.X - r.X;
        out.X = r.X;
    }
    if (out.Y > r.Y) {
        out.Height += out.Y - r.Y;
        out.Y = r.Y;
    }
    if (out.X + out.Width < r.X + r.Width) out.Width = r.X - out.X + r.Width + 1;
    if (out.Y + out.Height < r.Y + r.Height) out.Height = r.Y - out.Y + r.Height + 1;
}
unsigned techno_color_mask(const unsigned char* color, int format) noexcept {
    const unsigned r = color[0], g = color[1], b = color[2];
    unsigned value = 0;
    if (format == 2) value |= (r << 11) | (g << 5) | b;
    if (format == 1 || format == 2) value |= (r << 11) | ((g >> 1) << 6) | b;
    return value | (r << 10) | ((g >> 1) << 5) | b;
}
bool techno_drawing_complete(DrawingStatus s) noexcept {
    return s == DrawingStatus::drawn || s == DrawingStatus::skipped;
}
void record_techno_drawing(TechnoDrawing& d, DrawingStatus s) noexcept {
    if (techno_drawing_complete(d.status) && s != DrawingStatus::skipped) d.status = s;
}
DrawingStatus with_techno_drawing(TechnoDrawing& drawing, void (*call)(void*, PresentationContext&),
                                         void* argument, PresentationContext ctx) noexcept {
    ctx.techno = &drawing;
    ctx.type = &drawing.types;
    drawing.status = DrawingStatus::skipped;
    try {
        if (!call) drawing.status = DrawingStatus::invalid_argument;
        else call(argument, ctx);
    } catch (...) { drawing.status = DrawingStatus::backend_failure; }
    return drawing.status;
}
DrawingStatus draw_techno_object(const TechnoClass& object, TechnoDrawing& d, Point2D point,
                                 RectangleStruct clip, PresentationContext outer) noexcept {
    struct Call {
        const TechnoClass& object;
        Point2D point;
        RectangleStruct clip;
    } c{object, point, clip};
    return with_techno_drawing(
        d,
        [](void* p, game::PresentationContext& ctx) {
            auto& c = *static_cast<Call*>(p);
            if (c.object.WhatAmI() == AbstractType::Building) {
                auto& d = *ctx.techno;
                auto& building = const_cast<BuildingClass&>(static_cast<const BuildingClass&>(c.object));
                auto* cell = techno_drawing_cell(c.object);
                const int intensity = cell ? std::bit_cast<short>(cell->Intensity_Normal) : 1000;
                for (bool upper : {false, true}) {
                    const auto s =
                        draw_building_parts(building, d.types, d.building_depth, intensity, c.point, c.clip, upper, inherit_scopes(ctx));
                    record_techno_drawing(d, s);
                    if (!techno_drawing_complete(d.status)) break;
                }
            } else
                {
                    draw_it(ctx, c.object, &c.point, &c.clip);
                }
        },
        &c, outer);
}
CellClass* techno_drawing_cell(const TechnoClass& object) {
    CoordStruct at;
    object.GetCoords(&at);
    return MapClass::Instance.TryGetCellAt(CellStruct{short(at.X / 256), short(at.Y / 256)});
}
bool techno_cell_shrouded(CellClass& cell) noexcept { return !debug_fog_hidden() && cell.IsShrouded(); }
bool techno_location_fogged(const CoordStruct& at) noexcept { return debug_player_location_fogged(at); }
bool techno_drawing_palette(TechnoDrawing& d, const TechnoClass& object, TechnoPalette kind, CellClass* cell,
                            HouseClass* house, const DrawingPaletteHandle*& result) noexcept {
    if (!techno_drawing_complete(d.status)) return false;
    const auto s = d.palette ? d.palette(d.context, object, kind, cell, house, result) : DrawingStatus::unavailable;
    if (!techno_drawing_complete(s)) record_techno_drawing(d, s);
    return s == DrawingStatus::drawn;
}
bool techno_submit_shape(TechnoDrawing& d, const TechnoClass& object, TechnoPalette kind, CellClass* cell,
                         HouseClass* house, ShapeDrawingRequest& r) noexcept {
    r.target = d.types.target;
    if (!techno_drawing_palette(d, object, kind, cell, house, r.palette)) return false;
    const auto* previous = d.submitting_object;
    d.submitting_object = &object;
    const auto s = submit_type_shape(d.types, r);
    d.submitting_object = previous;
    record_techno_drawing(d, s);
    return techno_drawing_complete(s);
}
}
