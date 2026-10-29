// Original full radar foreground refresh, 0x00657CE0, on native surfaces.
#include "yrpp/MapClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/Surface.h"

void MapClass::sub_657CE0() noexcept {
    auto& radar = static_cast<RadarClass&>(*this);
    const DWORD x = radar.unknown_1248, y = radar.unknown_124C;
    const DWORD width = radar.unknown_1250, height = radar.unknown_1254;
    radar.unknown_1248 = radar.unknown_124C = 0;
    if (auto* foreground = radar.unknown_121C) {
        radar.unknown_1250 = static_cast<DWORD>(foreground->GetWidth());
        radar.unknown_1254 = static_cast<DWORD>(foreground->GetHeight());
    }
    radar.UpdateRadarImage();
    radar.unknown_1248 = x;
    radar.unknown_124C = y;
    radar.unknown_1250 = width;
    radar.unknown_1254 = height;
}
