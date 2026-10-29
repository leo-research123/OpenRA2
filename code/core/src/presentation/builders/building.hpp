#pragma once
#include "presentation/context.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
class BuildingClass;
namespace game {
// The original BuildingClass::DrawIt (YR 0x0043D290): the body, bib and under-door parts.
void build_building(PresentationContext&, const BuildingClass&, Point2D* location, RectangleStruct* bounds);
// The original virtual BuildingClass::Draw (YR 0x0043DA80), the upper pass: the object leaving a factory, the door
// and the voxel turret and barrel. Where the original writes NeedsRedraw = true (the door at its closed frame) the
// builder registers a redraw request in the presentation (redraw_requests()).
void build_building_upper(PresentationContext&, const BuildingClass&, const Point2D& point, const RectangleStruct& clip);
// The original BuildingClass::DrawAgain (YR 0x0043D030): the placement preview split above and below its foundation.
void build_building_again(PresentationContext&, const BuildingClass&, const Point2D& point, const RectangleStruct& bounds);
// The original TacticalClass::DrawBuildings (YR 0x006D3870): the buildings of the Ground layer that overlap the view,
// drawn through the world's per-object path. A building's bounds come from the memo-free query.
void build_buildings(PresentationContext&, bool forced, const RectangleStruct& area, const RectangleStruct& clip);
}
