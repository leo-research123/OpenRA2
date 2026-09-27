// SPDX-License-Identifier: GPL-3.0-or-later
// Electronic Arts / OpenTS; terms: third_party/opents/LICENSE.md.
// OpenTS radar.cpp Map_Cell / Radar_Cell; YR 0x00653810 / 0x00653830 /
// 0x006565A0. Modern whole composition coalesces pixel invalidation.
#include "yrpp/RadarClass.h"
#include "yrpp/Drawing.h"
bool RadarClass::MapCell(CellStruct* cell,HouseClass* house) {
    return DisplayClass::MapCell(cell,house);
}
bool RadarClass::RevealFogShroud(CellStruct* cell,HouseClass* house,bool increase) {
    return DisplayClass::RevealFogShroud(cell,house,increase);
}
void RadarClass::RadarCell(const CellStruct& cell) {
    if(!unknown_123C)return;
    RectangleStruct raw;CellRadarRect(&raw,cell);
    const RectangleStruct bounds{0,0,int(unknown_1240),int(unknown_1244)};
    if(Drawing::Intersect(raw,bounds).Width>0)RedrawRadar(false);
}
