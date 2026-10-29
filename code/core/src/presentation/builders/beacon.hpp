#pragma once
#include "yrpp/BasicStructures.h"
class BeaconClass; class BeaconManagerClass; class Surface;
namespace game {
struct PresentationContext; struct GameUiFrame;
void build_beacon(PresentationContext&, const BeaconClass&, Surface*, RectangleStruct) noexcept;
void build_beacons(PresentationContext&, const BeaconManagerClass&, Surface*, RectangleStruct) noexcept;
void build_radar_beacon(GameUiFrame&, const BeaconClass&, Surface*, RectangleStruct, bool clear = false) noexcept;
void build_radar_beacons(GameUiFrame&, const BeaconManagerClass&, Surface*, RectangleStruct) noexcept;
}
