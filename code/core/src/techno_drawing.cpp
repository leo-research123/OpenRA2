#include "techno_drawing.hpp"
#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/CellClass.h"
#include "building_drawing.hpp"
#include <bit>
namespace game {
namespace { thread_local TechnoDrawing* current = nullptr; }
TechnoDrawing* techno_drawing() noexcept { return current; }
unsigned techno_color_mask(const unsigned char* color,int format) noexcept {
    const unsigned r=color[0],g=color[1],b=color[2];unsigned value=0;
    if(format==2)value|=(r<<11)|(g<<5)|b;
    if(format==1||format==2)value|=(r<<11)|((g>>1)<<6)|b;
    return value|(r<<10)|((g>>1)<<5)|b;
}
bool techno_drawing_complete(DrawingStatus s) noexcept {
    return s == DrawingStatus::drawn || s == DrawingStatus::skipped;
}
void record_techno_drawing(TechnoDrawing& d, DrawingStatus s) noexcept {
    if (techno_drawing_complete(d.status) && s != DrawingStatus::skipped) d.status = s;
}
DrawingStatus with_techno_drawing(TechnoDrawing& d, void(*call)(void*), void* context) noexcept {
    auto* previous = current; current = &d; d.status = DrawingStatus::skipped;
    try { if (call) call(context); else d.status = DrawingStatus::invalid_argument; }
    catch (...) { d.status = DrawingStatus::backend_failure; }
    current = previous; return d.status;
}
DrawingStatus draw_techno_object(const TechnoClass& object, TechnoDrawing& d,
        Point2D point, RectangleStruct clip) noexcept {
    struct Call {const TechnoClass& object; Point2D point; RectangleStruct clip;} c{object,point,clip};
    return with_techno_drawing(d, [](void* p) {
        auto& c = *static_cast<Call*>(p);
        if(c.object.WhatAmI()==AbstractType::Building){
            auto& d=*techno_drawing();auto& building=const_cast<BuildingClass&>(static_cast<const BuildingClass&>(c.object));
            auto* cell=techno_drawing_cell(d,c.object);const int intensity=cell?std::bit_cast<short>(cell->Intensity_Normal):1000;
            for(bool upper:{false,true}){
                const auto s=draw_building_parts(building,d.types,d.building_depth,intensity,c.point,c.clip,upper);
                record_techno_drawing(d,s);if(!techno_drawing_complete(d.status))break;
            }
        }else c.object.DrawIt(&c.point,&c.clip);
    }, &c);
}
CellClass* techno_drawing_cell(TechnoDrawing& d, const TechnoClass& object) {
    CoordStruct at;object.GetCoords(&at);
    return d.cell_at ? d.cell_at(d.context,{short(at.X/256),short(at.Y/256)}) : nullptr;
}
int techno_drawing_height(const TechnoDrawing& d, int z) noexcept { return d.height ? d.height(z) : 0; }
bool techno_drawing_palette(TechnoDrawing& d, const TechnoClass& object, TechnoPalette kind,
        CellClass* cell, HouseClass* house, const DrawingPaletteHandle*& result) noexcept {
    if (!techno_drawing_complete(d.status)) return false;
    if (kind == TechnoPalette::cell && cell && !cell->LightConvert) {
        const auto s = d.initialize_light ? d.initialize_light(d.context,*cell) : DrawingStatus::unavailable;
        if (!techno_drawing_complete(s)) { record_techno_drawing(d,s); return false; }
    }
    const auto s = d.palette ? d.palette(d.context,object,kind,cell,house,result) : DrawingStatus::unavailable;
    if (!techno_drawing_complete(s)) record_techno_drawing(d,s);
    return s == DrawingStatus::drawn;
}
bool techno_submit_shape(TechnoDrawing& d, const TechnoClass& object, TechnoPalette kind,
        CellClass* cell, HouseClass* house, ShapeDrawingRequest& r) noexcept {
    r.target = d.types.target;
    if (!techno_drawing_palette(d,object,kind,cell,house,r.palette)) return false;
    const auto* previous=d.submitting_object;d.submitting_object=&object;
    const auto s = submit_type_shape(d.types,r);d.submitting_object=previous;record_techno_drawing(d,s);
    return techno_drawing_complete(s);
}
}
