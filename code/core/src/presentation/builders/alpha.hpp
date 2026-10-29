#pragma once
#include "presentation/context.hpp"
#include "yrpp/GeneralStructures.h"
class AlphaShapeClass;
struct SHPStruct;
namespace game {
struct PresentationState;
SHPStruct* alpha_presentation_image(PresentationState*, const AlphaShapeClass&);
void build_alpha_shapes(PresentationContext&, const RectangleStruct&);
}
