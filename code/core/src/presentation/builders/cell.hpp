#pragma once
#include "presentation/context.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
class CellClass;
class IsometricTileTypeClass;
class TacticalClass;
namespace game {
// The original CellClass::DrawIt (YR 0x00480350): the terrain tile of a cell and the smudge that follows it. The
// cell is not const: the original gives a cell that is drawn before any light pass its converter.
void build_cell_tile(PresentationContext&, CellClass&, const Point2D& unraised, const RectangleStruct& clip, bool skip);
// The original CellClass::DrawOverlay (YR 0x0047F6A0) and DrawOverlayShadow (0x00480110). A bridge overlay keeps
// its per-view redraw stamp in the cell, so the cell is not const.
void build_cell_overlay(PresentationContext&, CellClass&, const Point2D& location, const RectangleStruct& clip);
void build_cell_overlay_shadow(PresentationContext&, CellClass&, const Point2D& location, const RectangleStruct& clip);
// The original CellClass::DrawShadowCast (YR 0x004802A0) and IsometricTileTypeClass::DrawShadowCaster (0x00547230).
void build_cell_shadow_cast(PresentationContext&, CellClass&, const Point2D& point, const RectangleStruct& clip);
void build_tile_shadow_caster(PresentationContext&, IsometricTileTypeClass&, int sub_tile, Point2D point,
                              RectangleStruct clip, int depth);
// The original TacticalClass::DrawTiles (YR 0x006D2DE0) and DrawTileShadows (0x006D3040) passes.
void build_tiles(PresentationContext&, TacticalClass&, const RectangleStruct& area, const RectangleStruct& clip);
void build_tile_shadows(PresentationContext&, TacticalClass&, const RectangleStruct& area, const RectangleStruct& clip);
}
