#pragma once
#include "presentation/context.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
class AnimClass;
namespace game {
// The original AnimClass::DrawIt (YR 0x00422CA0), drawn through the sprite scope of the context.
// force_draw draws an animation that is hidden from the ordinary pass (the voxel barrel animation of a building).
void build_anim(PresentationContext&, const AnimClass&, Point2D* location, RectangleStruct* bounds,
                bool force_draw = false);
}
