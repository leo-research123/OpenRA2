// EA REDALERT/SCROLL.CPP f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// Copyright 2020 Electronic Arts Inc.; GPL-3.0-or-later with EA Section 7,
// see code/third_party/ea/LICENSE.TXT. YR 692B60 / 693440 calibration.
#include "yrpp/MouseClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Surface.h"
#include <algorithm>
#include <cmath>

void ScrollClass::ResetScrollInput() noexcept {
    unknown_int_5548=0;
    unknown_byte_554C=unknown_byte_5548=unknown_byte_554A=0;
    unknown_int_5550=unknown_int_5554=0;
}
bool ScrollClass::DragScroll(const Point2D& point,Point2D& warp,bool& should_warp) noexcept {
    should_warp=false;
    auto* tactical=TacticalClass::Instance;
    if (!tactical || !unknown_byte_554A || !InputManagerClass::Instance ||
        !InputManagerClass::Instance->IsKeyPressed(2)) return false;
    int dx=point.X-static_cast<int>(unknown_int_5550),dy=point.Y-static_cast<int>(unknown_int_5554);
    // Default Win32 drag metrics (4 px), doubled at 693489/6934B4. Device
    // scaling is applied once to the entire logical canvas by the adapter.
    if (!unknown_byte_5548 && (std::abs(dx)>8 || std::abs(dy)>8)) unknown_byte_5548=1;
    if (!unknown_byte_5548) return false;
    unknown_byte_554C=1;
    auto& options=GameOptionsClass::Instance;
    const int rate=std::clamp(options.ScrollRate,0,7);
    // confirms BOTH axes are displacement * multiplier, with truncation.
    const double multiplier=(options.ScrollMethod ? 12.0 : 1.0)/(rate+1);
    if (options.ScrollMethod==2) { dx=-dx; dy=-dy; }
    dx=static_cast<int>(dx*multiplier); dy=static_cast<int>(dy*multiplier);
    if (!dx && !dy) return false;
    if (options.ScrollMethod) {
        warp={static_cast<int>(unknown_int_5550),static_cast<int>(unknown_int_5554)};
        should_warp=true;
    }
    return tactical->FocusView({tactical->TacticalCoord1.X+dx,tactical->TacticalCoord1.Y+dy});
}
bool ScrollClass::ScrollAtEdge(const Point2D& point) noexcept {
    if (!TacticalClass::Instance || unknown_byte_554A || !unknown_byte_5549 || !GameOptionsClass::Instance.AutoScroll) return false;
    const auto& window=DSurface::WindowBounds;
    // The original tests the outer WINDOW edge, not the map/sidebar divider.
    const bool edge=point.X<=0 || point.Y<=0 || point.X>=window.Width-1 || point.Y>=window.Height-1;
    if (!edge) { if (unknown_int_5548) --unknown_int_5548; return false; }
    const int x=point.X<window.Width*0.16 ? -1 : point.X>(1.0-0.16)*window.Width ? 1 : 0;
    const int y=point.Y<window.Height*0.21 ? -1 : point.Y>(1.0-0.21)*window.Height ? 1 : 0;
    const int minimum=std::clamp(GameOptionsClass::Instance.ScrollRate,0,7)+1;
    const int rate=std::clamp(8-static_cast<int>(unknown_int_5548),minimum,8);
    static constexpr int speeds[]{448,384,320,256,192,128,64,32,16}; // 83E748.
    const double multiplier=RulesClass::Instance ? RulesClass::Instance->ScrollMultiplier : 1.0;
    if (!std::isfinite(multiplier) || multiplier<0 || multiplier>16) return false;
    const int distance=static_cast<int>(speeds[rate]*multiplier);
    unknown_int_5548=static_cast<DWORD>(std::min(8-minimum,static_cast<int>(unknown_int_5548)+1));
    auto& tactical=*TacticalClass::Instance;
    return tactical.FocusView({tactical.TacticalCoord1.X+x*distance,tactical.TacticalCoord1.Y+y*distance});
}
