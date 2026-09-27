// Adapted from OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9,
// code/radar.cpp RTacticalClass::Action. Copyright Electronic Arts Inc.;
// GPL-3.0-or-later with EA Section 7; see code/third_party/opents/LICENSE.md.
// YR 0x00653D92..0x00653EC1, shared by click and retained continuous dragging.
#include "yrpp/RadarClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/DrawingBuffers.h"
#include <bit>

bool RadarClass::Navigate(const Point2D& point) noexcept {
    const auto& rect=unknown_rect_149C;
    if (!unknown_123C || point.X<rect.X || point.Y<rect.Y ||
        point.X>=rect.X+rect.Width || point.Y>=rect.Y+rect.Height) return false;
    CellStruct target;
    TechnoClass* object=nullptr;
    if (!RadarToCell(point,target,object) || target==CellStruct{-1,-1} ||
        target==CellStruct{0,0}) return false;
    return NavigateCell(target);
}
bool RadarClass::NavigateCell(CellStruct target) noexcept {
    if (target==CellStruct{0,0}) return false;
    auto* tactical=TacticalClass::Instance;
    if (!tactical) return false;
    // 0x00653D92..0x00653E66. The camera's later pixel clamp does not replace
    // these ordered diagonal corrections. In particular, the lower horizontal
    // limit is asymmetric and each correction is stored as a signed 16-bit cell.
    const int side=MapRect.Width-(TacticalClass::ViewBounds.Width/60+2)/2-1;
    const int top=TacticalClass::ViewBounds.Height/60+MapRect.Width+1;
    const int bottom=2*MapRect.Height-TacticalClass::ViewBounds.Height/60+MapRect.Width-1;
    const auto word=[](int value) noexcept {
        return std::bit_cast<short>(static_cast<unsigned short>(value));
    };
    if (target.Y-target.X>side) {
        const short adjust=word(target.Y-target.X-side);
        target.Y=word(target.Y-adjust);target.X=word(target.X+adjust);
    }
    if (target.X-target.Y>side-1) {
        const short adjust=word(target.X-target.Y-side+1);
        target.Y=word(target.Y+adjust);target.X=word(target.X-adjust);
    }
    if (target.X+target.Y<top) {
        const short adjust=word(top-target.Y-target.X);
        target.X=word(target.X+adjust);target.Y=word(target.Y+adjust);
    }
    if (target.X+target.Y>bottom) {
        const short adjust=word(target.X+target.Y-bottom);
        target.X=word(target.X-adjust);target.Y=word(target.Y-adjust);
    }
    auto* cell=GetCellAt(target);
    CoordStruct world; cell->GetCellCoords(&world);
    vt_entry_D0(&world);
    GScreenClass::Instance.MarkNeedsRedraw(1);
    if (ZBuffer::Instance) ZBuffer::Instance->MaxValue=0x8000;
    return true;
}
