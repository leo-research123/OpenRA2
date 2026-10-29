#pragma once
#include "presentation/context.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
#include "yrpp/GeneralDefinitions.h"
class ObjectClass;
namespace game {
// The original ObjectClass::DrawIfVisible (YR 0x005F4B10) for the kinds whose drawing has moved into builders;
// any other kind goes through its virtual DrawIfVisible. Returns what the original returns.
bool draw_object_if_visible(PresentationContext&, const ObjectClass&, RectangleStruct* bounds, bool forced, DWORD pass);
// The original virtual DrawIt: the builder of the kind, or the virtual while the kind has not moved.
// force_draw only means something for an animation (see build_anim).
void draw_it(PresentationContext&, const ObjectClass&, Point2D* location, RectangleStruct* bounds,
             bool force_draw = false);
// The original virtual DrawAgain (the placement preview): the building builder, or the virtual of any other kind.
void draw_again(PresentationContext&, const ObjectClass&, const Point2D& location, const RectangleStruct& bounds);
}
