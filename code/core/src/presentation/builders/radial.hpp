#pragma once
#include "yrpp/BasicStructures.h"
class BuildingClass;
namespace game {
struct PresentationContext;
void build_radial_shape(PresentationContext&, bool spokes, bool adjust, CoordStruct, ColorStruct, float cells, bool pixels, bool truncate);
void build_building_radial(PresentationContext&, const BuildingClass&, DWORD drawLine);
void build_radial_indicators(PresentationContext&);
}
