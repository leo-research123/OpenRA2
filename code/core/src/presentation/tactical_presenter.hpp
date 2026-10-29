#pragma once
#include "presentation/context.hpp"
class TacticalClass;
namespace game {
struct PresentationFrame;
// The stage order of the original TacticalClass::Render (0x6D3D10, forced modes 0x1 and 0x3) for the
// native targets, driven by the frame the caller bound. Stages stay original calls on TacticalClass;
// the presenter owns only the order, the early exits on a failed stage and the Tactical bookkeeping
// that surrounds them.
void present_tactical(TacticalClass&, PresentationFrame&, int mode);
// The five-layer traversal of the original TacticalClass::DrawObjects (0x6D8DB0).
void present_objects(PresentationContext&, TacticalClass&, bool forced);
}
