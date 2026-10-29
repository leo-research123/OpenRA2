#pragma once
#include "presentation/context.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
class UnitClass;
class Surface;
namespace game {
// The original UnitClass::DrawIt (YR 0x0073CEC0) and the virtuals it calls: DrawAsSHP (0x0073C5F0), DrawAsVXL
// (0x0073B470) and the 8-bit composite blit DrawObject (YR 0x0073C860).
void build_unit(PresentationContext&, const const UnitClass&, Point2D* location, RectangleStruct* bounds);
void build_unit_shp(PresentationContext&, const UnitClass&, Point2D point, RectangleStruct clip, int intensity, int tint);
void build_unit_vxl(PresentationContext&, const UnitClass&, Point2D point, RectangleStruct clip, int intensity, int tint);
void build_unit_composite(PresentationContext&, const UnitClass&, Surface* target, Point2D point, RectangleStruct clip,
                          int intensity, int tint);
}
