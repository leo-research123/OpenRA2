// Native invalidation adapter for TacticalClass's existing dirty-area entry.
#include "yrpp/TacticalClass.h"
#include "map_world.hpp"

void TacticalClass::RegisterDirtyArea(RectangleStruct, bool) {
    // OpenTS 44fac744 Tactical::Register_Dirty_Area / YR 0x6D2790 queues
    // rectangles for the cached background, optionally including shroud/alpha.
    // Native Render rebuilds the whole frame, including its depth/light
    // buffers; publish invalidation for that existing renderer. No dirty queue
    // is consumed here. RA2_YRPP_GAME retains the original rectangle scheduler.
    game::map_object_changed();
}
