#pragma once
#include "presentation/context.hpp"
#include "yrpp/GeneralStructures.h"
class CellClass; class TacticalClass;
namespace game {
void build_cell_shroud(PresentationContext&, const CellClass&, const Point2D&, const RectangleStruct&);
void build_shroud(PresentationContext&, TacticalClass&, const RectangleStruct&);
}
