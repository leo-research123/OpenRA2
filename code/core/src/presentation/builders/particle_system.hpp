#pragma once
#include "presentation/context.hpp"
#include "yrpp/GeneralStructures.h"
#include "yrpp/GeneralDefinitions.h"
class ParticleSystemClass;
namespace game { void build_particle_system(PresentationContext&, const ParticleSystemClass&, Point2D*, RectangleStruct*); }
