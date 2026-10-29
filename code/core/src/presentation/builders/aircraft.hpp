#pragma once
#include "presentation/context.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
class AircraftClass;
namespace game {
// The original AircraftClass::DrawIt (YR 0x004144B0).
void build_aircraft(PresentationContext&, const AircraftClass&, Point2D* location, RectangleStruct* bounds);
}
