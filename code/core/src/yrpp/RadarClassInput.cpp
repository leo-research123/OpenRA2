// Adapted from OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9,
// code/radar.cpp RTacticalClass::Action. Copyright Electronic Arts Inc.;
// GPL-3.0-or-later with EA Section 7; see code/third_party/opents/LICENSE.md.
// YR coordinate wrappers: 0x00653760 / 0x00653F70.
// RTactical::Action is implemented in RadarClassAction.cpp.
#include "yrpp/RadarClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/Surface.h"
#include "yrpp/OwnerDraw.h"
#include "yrpp/VoxClass.h"
#include "yrpp/SuperClass.h"

RadarClass::RTacticalClass::RTacticalClass() noexcept : GadgetClass(0, 0, 0, 0, static_cast<GadgetFlag>(0xdf), true) {}
#if !defined(RA2_YRPP_GAME)
// Complete 0x00653850; movie frames advance in Radar Draw.
void RadarClass::Update(const int& key, const Point2D& point) {
    DisplayClass::Update(key, point);
    const int shift = GameOptionsClass::Instance.SidebarMode ? DSurface::ViewBounds.Width : 0;
    const auto over = [&](const RectangleStruct& bounds) {
        return point.X >= shift + bounds.X && point.X < shift + bounds.X + bounds.Width && point.Y >= bounds.Y &&
               point.Y < bounds.Y + bounds.Height;
    };
    if (over(DiplomacyBounds)) MouseClass::Instance.MouseClass::UpdateCursor(MouseCursorType::Default, false);
    if (over(OptionsBounds)) MouseClass::Instance.MouseClass::UpdateCursor(MouseCursorType::Default, false);
    ProcessButtonKey(static_cast<DWORD>(key));
    if (WWMovieHandle::Array.Count > 0 && unknown_14B0 != 3 && unknown_14B0 != 4) {
        unknown_14B4 = unknown_14B0;
        SetRadarMode(4, false);
        unknown_14B8 = 0;
    }
    if (unknown_14B0 == 4)
        PrepareMovie();
    else if (!ChronoScreenEffect::Active())
        VoxClass::Unsuppress();
}
CellStruct* RadarClass::vt_entry_CC(CellStruct* output, Point2D* point) {
    if (!output || !point) return nullptr;
    auto* tactical = TacticalClass::Instance;
    *output = {-1, -1};  // Original 0x00B048C0 sentinel.
    if (!tactical) return output;
    CoordStruct world;
    if (!tactical->ClientToCoords(&world, *point) || world == CoordStruct{-1, -1, -1}) return output;
    tactical->PickTerrainCell(*point, {0, 0, TacticalClass::ViewBounds.Width, TacticalClass::ViewBounds.Height},
                              *output);
    return output;
}
void RadarClass::vt_entry_D0(CoordStruct* coord) {
    if (auto* tactical = TacticalClass::Instance) tactical->SetTacticalPosition(coord);
}
#endif
