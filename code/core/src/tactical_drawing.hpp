#pragma once
#include "api/map_view.hpp"
class TacticalClass;
class CellClass;
class ObjectClass;
namespace game {
class MapWorld;
struct WorldSprite;
enum class DrawingPaletteKind;
// Synchronous borrowed host bindings. TacticalClass owns scene traversal and
// pass order; the host owns resources, cached requests and the output target.
struct TacticalDrawingFrame {
    MapWorld* world=nullptr;
    const MapDrawingContext* drawing=nullptr;
    RectangleStruct bounds{};
    MapDrawStatistics* statistics=nullptr;
    DrawingStatus status=DrawingStatus::skipped;
    bool include_terrain=true;
    bool background_prepared=false;
    unsigned background_requests=0;
    bool submit_objects=false;
    bool selection_only=false;
};
enum class TacticalObjectPass { behind, body, building_upper, extras, building_info };
// Binds per-object resources and submits that object's requests before the
// next original hook. Selection and traversal remain in TacticalClass.
DrawingStatus dispatch_tactical_object(ObjectClass&,TacticalObjectPass,Point2D*,RectangleStruct*,bool) noexcept;
TacticalDrawingFrame* tactical_drawing() noexcept;
DrawingStatus with_tactical_drawing(TacticalDrawingFrame&,void (*)(void*),void*) noexcept;
DrawingStatus draw_tactical_view(TacticalClass&,MapWorld*,const MapDrawingContext&,
    const RectangleStruct&,MapDrawStatistics&) noexcept;
SHPStruct* world_sprite_data(SHPStruct*);
WorldSprite* append_world_sprite(MapWorld&,SHPStruct*,int,ObjectClass*,CellClass*,
    Point2D,const BytePalette*,int,bool,bool=false,bool=false);
DrawingStatus resolve_world_drawing_palette(MapWorld&,DrawingPaletteKind,int,
    const DrawingPaletteHandle*&) noexcept;
DrawingStatus submit_world_sprite(WorldSprite&,const MapDrawingContext&,
    const RectangleStruct&,MapDrawStatistics&) noexcept;
void record_tactical_drawing(DrawingStatus) noexcept;
DrawingStatus draw_cell_terrain(CellClass&,const MapDrawingContext&,const Point2D&,
    const RectangleStruct&) noexcept;
// Bind resources around exactly one original visibility entry; no pass selection here.
DrawingStatus draw_tactical_object(ObjectClass&,bool,const RectangleStruct&,bool upper) noexcept;
DrawingStatus draw_lighting_shape(SHPStruct*,int,const Point2D&,const RectangleStruct&,RasterBlendMode) noexcept;
}
