// Adapted from OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9,
// code/radar.cpp RTacticalClass::Action. Copyright Electronic Arts Inc.;
// GPL-3.0-or-later with EA Section 7; see code/third_party/opents/LICENSE.md.
// YR coordinate wrappers: 0x00653760 / 0x00653F70.
// RTactical::Action is implemented in RadarClassAction.cpp.
#include "yrpp/RadarClass.h"
#include "yrpp/TacticalClass.h"

RadarClass::RTacticalClass::RTacticalClass() noexcept
    : GadgetClass(0,0,0,0,static_cast<GadgetFlag>(0xdf),true) {}
#if !defined(RA2_YRPP_GAME)
CellStruct* RadarClass::vt_entry_CC(CellStruct* output,Point2D* point) {
    if (!output || !point) return nullptr;
    auto* tactical=TacticalClass::Instance;
    *output={-1,-1}; // Original 0x00B048C0 sentinel.
    if (!tactical) return output;
    CoordStruct world;
    if (!tactical->ClientToCoords(&world,*point) || world==CoordStruct{-1,-1,-1}) return output;
    tactical->PickTerrainCell(*point,{0,0,TacticalClass::ViewBounds.Width,TacticalClass::ViewBounds.Height},*output);
    return output;
}
void RadarClass::vt_entry_D0(CoordStruct* coord) {
    if (auto* tactical=TacticalClass::Instance) tactical->SetTacticalPosition(coord);
}
#endif
