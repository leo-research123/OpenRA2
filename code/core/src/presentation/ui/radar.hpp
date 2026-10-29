#pragma once
#include "yrpp/platform/ABI.h"
class RadarClass;
namespace game {
struct GameUiFrame;
void build_radar(GameUiFrame&, const RadarClass&, DWORD force);
bool build_radar_map(GameUiFrame&, const RadarClass&) noexcept;
void build_radar_names(GameUiFrame&, const RadarClass&) noexcept;
}
