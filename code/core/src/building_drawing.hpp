#pragma once
#include "presentation/context.hpp"
#include "type_drawing_context.hpp"
class BuildingClass;
namespace game {
// Borrowed request sink for original building drawing. Object and state stay
// in BuildingClass; the host supplies the target, palette and cell lighting.
struct BuildingDrawing {
    const TypeDrawingContext& context;
    SHPStruct* depth_image;
    int intensity;
    DrawingStatus status = DrawingStatus::skipped;
};
DrawingStatus with_building_drawing(BuildingDrawing&, void (*)(void*, PresentationContext&), void*,
                                          PresentationContext = {}) noexcept;
DrawingStatus draw_building_parts(BuildingClass&, const TypeDrawingContext&, SHPStruct*, int, Point2D, RectangleStruct,
                                  bool upper, PresentationContext outer = {}) noexcept;
}
