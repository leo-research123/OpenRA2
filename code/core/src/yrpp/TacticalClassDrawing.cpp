// Native invalidation adapter for TacticalClass's existing dirty-area entry.
#include "background_invalidation.hpp"
#include "yrpp/TacticalClass.h"
#include "map_world.hpp"

void TacticalClass::RegisterDirtyArea(RectangleStruct area, bool refreshShroud) {
    // Capture the camera at notification time, before any viewport clipping.
    const auto x = std::int64_t(area.X) + TacticalPos.X;
    const auto y = std::int64_t(area.Y) + TacticalPos.Y;
    if (x < INT32_MIN || x > INT32_MAX || y < INT32_MIN || y > INT32_MAX)
        game::invalidate_background_all();
    else {
        area.X = int(x);
        area.Y = int(y);
        game::invalidate_background_area(area, refreshShroud);
    }
    // OpenTS 44fac744 Tactical::Register_Dirty_Area / YR 0x6D2790 queues
    // rectangles for the cached background, optionally including shroud/alpha.
    // Native Render rebuilds the whole frame, including its depth/light
    // buffers; publish invalidation for that existing renderer. No dirty queue
    // is consumed here. RA2_YRPP_GAME retains the original rectangle scheduler.
    game::map_object_changed();
}
