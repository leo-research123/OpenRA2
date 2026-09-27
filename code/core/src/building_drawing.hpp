#pragma once
#include "api/type_drawing.hpp"
class BuildingClass;
namespace game {
// Borrowed request sink for original building drawing. Object and state stay
// in BuildingClass; the host supplies the target, palette and cell lighting.
struct BuildingDrawing {
 const TypeDrawingContext& context;
 SHPStruct* depth_image;
 int intensity;
 DrawingStatus status=DrawingStatus::skipped;
};
BuildingDrawing* building_drawing() noexcept;
DrawingStatus with_building_drawing(BuildingDrawing&,void(*)(void*),void*) noexcept;
DrawingStatus draw_building_parts(BuildingClass&,const TypeDrawingContext&,SHPStruct*,
    int,Point2D,RectangleStruct,bool upper) noexcept;
}
