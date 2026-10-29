// UnitClass voxel drawing: see presentation/builders/unit.cpp.
#include "yrpp/UnitClass.h"
#include "type_drawing.hpp"
#include <cstdlib>

[[noreturn]] static void unit_voxel_trap(const char* what) {
    game::record_drawing_failure(game::DrawingStatus::unsupported, what);
    std::abort();
}
void UnitClass::DrawAsVXL(Point2D, RectangleStruct, int, int) { unit_voxel_trap("UnitClass::DrawAsVXL called outside the builders"); }
#if !defined(RA2_YRPP_GAME)
void UnitClass::DrawObject(Surface*, Point2D, RectangleStruct, int, int) { unit_voxel_trap("UnitClass::DrawObject called outside the builders"); }
#endif
