#pragma once
#include "presentation/context.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
class InfantryClass;
namespace game {
// The original InfantryClass::DrawIt (YR 0x00518F90).
void build_infantry(PresentationContext&, const InfantryClass&, Point2D* location, RectangleStruct* bounds);
}
