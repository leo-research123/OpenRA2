#pragma once
#include "yrpp/BasicStructures.h"
class TacticalClass; class CellClass;
namespace game {
struct PresentationContext;
void build_placement(PresentationContext&, const TacticalClass&, bool foreground);
void build_wall_placement(PresentationContext&, const TacticalClass&, bool foreground, CellStruct);
bool build_cell_placement(PresentationContext&, const CellClass&, const Point2D&, const RectangleStruct&, bool foreground);
}
