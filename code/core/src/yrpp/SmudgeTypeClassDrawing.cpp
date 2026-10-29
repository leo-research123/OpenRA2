// YR 0x006B55F0 SmudgeTypeClass::DrawIt: the drawing is game::draw_smudge_type
// (presentation/builders/type_art.cpp), called by the cell builder.
#include "yrpp/SmudgeTypeClass.h"
#include "type_drawing.hpp"
#include <cstdlib>

// The drawing moved to the presentation layer. A call here means a caller bypassed the builders.
void SmudgeTypeClass::DrawIt(const Point2D&, const RectangleStruct&, int, int, const CellStruct&) {
    game::record_drawing_failure(game::DrawingStatus::unsupported, "SmudgeTypeClass::DrawIt called outside the builders");
    std::abort();
}
