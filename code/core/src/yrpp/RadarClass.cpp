// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 radar.cpp lifecycle.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// Additional terms: third_party/opents/LICENSE.md. YR calibration below.
#include "yrpp/RadarClass.h"
#include "yrpp/Surface.h"
#include "yrpp/TechnoClass.h"
#include "map_hash.hpp"
namespace {
DWORD YRPP_FASTCALL radar_hash(const RadarTrackingStruct& key) {
    return static_cast<DWORD>(key.Y) * 251u + static_cast<DWORD>(key.X);
}
}
RadarClass::RadarClass()
    : DisplayClass(), unknown_11E8{}, unknown_11EC{}, unknown_11F0{}, unknown_11F4{},
      unknown_11F8{}, unknown_11FC{}, unknown_1200{}, unknown_1204{},
      unknown_1208{}, unknown_rect_120C{}, unknown_121C{}, unknown_1220{},
      unknown_cells_1124{}, unknown_123C{}, unknown_1240{}, unknown_1244{},
      unknown_1248{}, unknown_124C{}, unknown_1250{}, unknown_1254{},
      unknown_1258{}, unknown_points_125C{}, unknown_1274{}, FoundationTypePixels{},
      RadarSizeFactor{}, unknown_int_148C{}, unknown_1490{}, unknown_1494{},
      unknown_1498{}, unknown_rect_149C{}, unknown_14AC{}, unknown_14B0{},
      unknown_14B4{}, unknown_14B8{}, unknown_bool_14BC{}, unknown_bool_14BD{},
      RadarAudio{}, unknown_int_14D4{}, IsAvailableNow{}, unknown_bool_14D9{},
      unknown_bool_14DA{}, unknown_rect_14DC{}, unknown_14EC{}, unknown_14F0{},
      unknown_14F4{}, unknown_14F8{}, unknown_14FC{}, unknown_timer_1500{} {
    unknown_int_148C = 1;
    unknown_int_14D4 = -1;
    unknown_points_125C.CapacityIncrement = 500;
    unknown_timer_1500.StartTime = static_cast<int>(SystemTimer::GetTime());
    InitRadar();
}
RadarClass::~RadarClass() {
    ClearRadar();
}
void RadarClass::InitRadar() noexcept {
    unknown_1258 = game::create_map_hash<RadarTrackingStruct, TechnoClass*>(radar_hash, 10);
}
void RadarClass::ClearRadar() noexcept {
    // OpenTS Clear_Radar, calibrated to 0x00655A90: resource pointers only.
    // Do not reset geometry, queues, mode or animation time here.
    GameDelete(unknown_121C); unknown_121C=nullptr;
    GameDelete(unknown_1220); unknown_1220=nullptr;
    YRMemory::Deallocate(unknown_123C); unknown_123C=nullptr;
    game::destroy_map_hash(unknown_1258);
    unknown_1258 = nullptr;
    YRMemory::Deallocate(unknown_1274); unknown_1274=nullptr;
}
void RadarClass::ReleaseTerrainRadar() noexcept {
    // Host map teardown also retires coordinate-dependent work. Rebuilding
    // must permit canonical Technos to register at their new radar position.
    ClearRadar();
    for (auto* object : TechnoClass::Array) if (object) object->IsRadarTracked=false;
    unknown_points_125C.Count=0; unknown_cells_1124.Count=0;
    for (auto& pixels : FoundationTypePixels) pixels.Count=0;
    unknown_1240=unknown_1244=0;
    RadarSizeFactor=0;
    unknown_rect_149C={};
    unknown_rect_14DC={};
}
