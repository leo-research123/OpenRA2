#pragma once
#include "presentation/context.hpp"
#include "yrpp/GeneralStructures.h"
#include "yrpp/GeneralDefinitions.h"
class LaserDrawClass;
namespace game {
void build_laser(PresentationContext&, const LaserDrawClass&);
void build_house_laser(PresentationContext&, const LaserDrawClass&);
void build_lasers(PresentationContext&);
}
