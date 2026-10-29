// Supplied 005FE5A0 OverlayTypeClass::Draw: the drawing is game::draw_overlay_type
// (presentation/builders/type_art.cpp).
#include "yrpp/OverlayTypeClass.h"
#include "type_drawing.hpp"
#include <cstdlib>

// The drawing moved to the presentation layer. A call here means a caller bypassed the builders.
void OverlayTypeClass::Draw(Point2D*, RectangleStruct*, int) {
    game::record_drawing_failure(game::DrawingStatus::unsupported, "OverlayTypeClass::Draw called outside the builders");
    std::abort();
}
