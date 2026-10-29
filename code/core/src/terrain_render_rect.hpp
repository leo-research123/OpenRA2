#pragma once
// The rectangle TerrainClass::GetRenderDimensions (YR 0x0071D160) returns. The original method is not const, but it only
// reads; the drawing asks through this function so that it needs no mutable object.
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
class TerrainClass;
namespace game {
RectangleStruct terrain_render_rect(const TerrainClass&);
}
