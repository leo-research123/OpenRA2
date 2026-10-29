// UnitClass drawing: see presentation/builders/unit.cpp.
#include "yrpp/UnitClass.h"
#include "type_drawing.hpp"
#include <cstdlib>

// The drawing moved to the presentation layer (presentation/builders/unit.cpp). A call here means a caller
// bypassed the builders.
[[noreturn]] static void unit_draw_trap(const char* what) {
    game::record_drawing_failure(game::DrawingStatus::unsupported, what);
    std::abort();
}
void UnitClass::DrawIt(Point2D*, RectangleStruct*) const { unit_draw_trap("UnitClass::DrawIt called outside the builders"); }
void UnitClass::DrawAsSHP(Point2D, RectangleStruct, int, int) { unit_draw_trap("UnitClass::DrawAsSHP called outside the builders"); }
