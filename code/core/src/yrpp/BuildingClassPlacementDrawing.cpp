// YR 0x0043D030: the placement preview split above/below its foundation moved to
// presentation/builders/building.cpp.
#include "yrpp/BuildingClass.h"
#include "type_drawing.hpp"
#include <cstdlib>
#if !defined(RA2_YRPP_GAME)
// The drawing moved to the presentation layer. A call here means a caller bypassed the builders.
void BuildingClass::DrawAgain(const Point2D&, const RectangleStruct&) const {
    game::record_drawing_failure(game::DrawingStatus::unsupported, "BuildingClass::DrawAgain called outside the builders");
    std::abort();
}
#endif
