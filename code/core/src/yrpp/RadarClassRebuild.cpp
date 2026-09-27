// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 radar.cpp Reset_Radar / Post_Load_Radar_Fixup.
// Electronic Arts / OpenTS; terms: third_party/opents/LICENSE.md.
// YR 0x00655990 / 0x00655B20 retain distinct ownership and movie repair.
#include "yrpp/RadarClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/FPSCounter.h"
#include "map_hash.hpp"

void RadarClass::ResetRadar() noexcept {
    game::destroy_map_hash(unknown_1258);unknown_1258=nullptr;
    InitRadar();
    try {
        Instance.SetVisibleRect(Instance.VisibleRect);
        ComputeRadarImage();
    } catch(...) {unknown_bool_14D9=true;}
    for(auto* object:TechnoClass::Array)if(object)object->IsRadarTracked=false;
}
void RadarClass::PostLoadRadarFixup() noexcept {
    // YR differs from OpenTS: serialized pointers are discarded, never freed.
    // The load coordinator must retire its live resources before Load.
    unknown_121C=unknown_1220=nullptr;unknown_123C=nullptr;
    unknown_1258=nullptr;unknown_1274=nullptr;
    InitRadar();
    try {
        Instance.SetVisibleRect(Instance.VisibleRect);
        ComputeRadarImage();
    } catch(...) {unknown_bool_14D9=true;}
    for(auto* object:TechnoClass::Array)if(object)object->IsRadarTracked=false;
    if(unknown_14B0==3){
        unknown_14AC=5;SetRadarMode(static_cast<int>(unknown_14B4),false);
        if(RulesClass::Instance)Detail::SetMinFrameRate(RulesClass::Instance->DetailMinFrameRateNormal);
    }
}
