#pragma once
#include "presentation/context.hpp"
#include "yrpp/GeneralStructures.h"
#include "yrpp/GeneralDefinitions.h"
class ParticleClass;
namespace game { void build_particle(PresentationContext&, const ParticleClass&, Point2D*, RectangleStruct*); }
