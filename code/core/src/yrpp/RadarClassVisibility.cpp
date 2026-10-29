// SPDX-License-Identifier: GPL-3.0-or-later
// Electronic Arts / OpenTS; terms: third_party/opents/LICENSE.md.
// OpenTS radar.cpp Map_Cell / Radar_Cell; YR 0x00653810 / 0x00653830 /
// 0x006565A0.
#include "yrpp/RadarClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/Surface.h"
#include <algorithm>
bool RadarClass::MapCell(CellStruct* cell, HouseClass* house) { return DisplayClass::MapCell(cell, house); }
bool RadarClass::RevealFogShroud(CellStruct* cell, HouseClass* house, bool increase) {
    return DisplayClass::RevealFogShroud(cell, house, increase);
}
void RadarClass::RadarCell(const CellStruct& cell) {
    // The cell's raw terrain span is scaled to foreground pixels and widened
    // by one on each side; each covered pixel is queued like any other point.
    auto* foreground = unknown_121C;
    if (!foreground) return;
    RectangleStruct raw;
    CellRadarRect(&raw, cell);
    const auto rect = Drawing::Intersect(raw, {0, 0, int(unknown_1240), int(unknown_1244)});
    if (rect.Width <= 0 || rect.Height <= 0) return;
    const int left = std::max(int(double(rect.X) * RadarSizeFactor) - 1, 0);
    const int right = std::min(int(double(rect.X + rect.Width) * RadarSizeFactor) + 1, foreground->GetWidth());
    const int top = std::max(int(double(rect.Y) * RadarSizeFactor) - 1, 0);
    const int bottom = std::min(int(double(rect.Y + rect.Height) * RadarSizeFactor) + 1, foreground->GetHeight());
    for (int y = top; y < bottom; ++y)
        for (int x = left; x < right; ++x) {
            Point2D point{x, y};
            RefreshCrd(&point);
        }
}
