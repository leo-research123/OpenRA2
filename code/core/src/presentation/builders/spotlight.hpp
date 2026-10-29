#pragma once
#include "presentation/context.hpp"
#include "yrpp/GeneralStructures.h"
#include "yrpp/GeneralDefinitions.h"
namespace game {
void build_spotlight(PresentationContext&, const CoordStruct&, int size, int movement_radius, SpotlightFlags);
void build_spotlights(PresentationContext&);
}
