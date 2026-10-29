// AircraftClass drawing: see presentation/builders/aircraft.cpp.
#include "yrpp/AircraftClass.h"
#include "type_drawing.hpp"
#include <cstdlib>

// The drawing moved to the presentation layer. A call here means a caller bypassed the builders.
void AircraftClass::DrawIt(Point2D*, RectangleStruct*) const {
    game::record_drawing_failure(game::DrawingStatus::unsupported, "AircraftClass::DrawIt called outside the builders");
    std::abort();
}
