#pragma once
#include "presentation/context.hpp"
#include "yrpp/GeneralStructures.h"
#include "yrpp/GeneralDefinitions.h"
class WaveClass;
namespace game { void build_wave(PresentationContext&, const WaveClass&, Point2D*, RectangleStruct*); }
