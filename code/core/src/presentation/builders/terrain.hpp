#pragma once
#include "presentation/context.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
class TerrainClass;
namespace game {
// The original TerrainClass::DrawIt (YR 0x0071C1B0): the tree or other terrain object and its shadow.
void build_terrain(PresentationContext&, const TerrainClass&, Point2D* location, RectangleStruct* bounds);
// The original TerrainClass::DrawAgain (YR 0x0071C360).
void build_terrain_again(PresentationContext&, const TerrainClass&, const Point2D& location, const RectangleStruct& bounds);
// The original TacticalClass::DrawTerrain (YR 0x006D3AC0): the static terrain objects of the Ground layer, in
// reverse layer order, drawn through the world's per-object path.
void build_terrain_objects(PresentationContext&, bool forced, const RectangleStruct& area, const RectangleStruct& clip);
}
