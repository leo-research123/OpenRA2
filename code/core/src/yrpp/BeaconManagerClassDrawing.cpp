// YR-only original radar beacon dispatch, 0x00431700. Animation advances with
// the existing simulation frame, never with an additional repaint request.
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/Unsorted.h"
#include "game_ui_runtime.hpp"
#include "type_drawing.hpp"
#if !defined(RA2_YRPP_GAME)
void BeaconManagerClass::DrawRadar(Surface* surface,RectangleStruct bounds) noexcept {
    if(!AllocatedCount)return;
    if(RadarBeaconAnimPeriod<=0) {
        game::record_ui_drawing(game::DrawingStatus::unavailable);
        game::record_type_drawing_result(game::DrawingStatus::unavailable);return;
    }
    if(Unsorted::CurrentFrame%RadarBeaconAnimPeriod>=Instance.RadarBeaconFrameCount+1)return;
    for(auto& house:Beacons)for(auto* beacon:house)
        if(beacon && beacon->VisibleToPlayer())beacon->DrawRadar(surface,bounds,false);
}
#endif
