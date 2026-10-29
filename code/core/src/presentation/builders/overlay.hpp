#pragma once
#include "presentation/context.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
class TacticalClass;
namespace game {
// The original TacticalClass::DrawOverlays (YR 0x006D6D10): the overlays of the cells covering `area`, bodies then
// shadows, reverse rows, ascending columns; each cell is drawn by build_cell_overlay / build_cell_overlay_shadow.
void build_overlays(PresentationContext&, TacticalClass&, const RectangleStruct& area);
}
