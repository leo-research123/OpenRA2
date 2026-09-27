// Original Radar dirty vectors/bitset and foundation masks: 655250, 6562D0,
// 6563B0. Full redraw backends consume these same original invalidations.
#include "yrpp/RadarClass.h"
#include <algorithm>
#include <bit>
#include <cmath>

bool RadarClass::BuildFoundationPixels() noexcept {
    if (!std::isfinite(RadarSizeFactor) || RadarSizeFactor<=0 || RadarSizeFactor>140) return false;
    constexpr int width[22]={1,2,1,2,2,3,3,3,4,3,1,3,4,1,1,2,2,5,4,3,6,0};
    constexpr int height[22]={1,1,2,2,3,2,3,5,2,3,3,1,3,4,5,6,5,3,4,4,4,0};
    try {
        for (int i=0;i<22;++i) {
            auto& output=FoundationTypePixels[i];output.Count=0;
            const int x=int(std::max(double(width[i])*RadarSizeFactor+0.5,width[i]==1 ? 1.0 : 2.0));
            const int y=int(std::max(double(height[i])*RadarSizeFactor+0.5,height[i]==1 ? 1.0 : 2.0));
            for (int row=0;row<x+y-1;++row) {
                const int begin=row<y ? -row : row-2*y+2;
                const int end=row<x ? row : 2*x-row-2;
                for (int col=begin;col<=end;++col) if (!output.AddItem({col,row})) return false;
            }
        }
        return true;
    } catch (...) { return false; }
}
void RadarClass::RefreshCrd(Point2D* point) {
    if (!point || !unknown_1274 || point->X<0 || point->Y<0 ||
        point->X>=unknown_rect_149C.Width || point->Y>=unknown_rect_149C.Height) return;
    const int index=point->X+point->Y*unknown_rect_149C.Width;
    auto& flags=unknown_1274[index/8]; const byte mask=byte(1u<<(index&7));
    if (flags&mask) return;
    // 0x00656327 sets membership before attempting the vector allocation.
    flags|=mask;
    try { (void)unknown_points_125C.AddItem(*point); } catch (...) { }
    unknown_bool_14D9=true;
}
bool RadarClass::MarkTerrainCellDirty(const CellStruct& cell) noexcept {
    if (!unknown_123C || !MapClass::Instance.IsWithinUsableArea(cell,true)) return false;
    RadarBackground(cell);
    return unknown_cells_1124.FindItemIndex(cell)>=0;
}
void RadarClass::RadarBackground(const CellStruct& cell) noexcept {
    // 0x006551C0 does no map/resource validation. Only a new request marks
    // redraw; allocation failure still performs that original flag write.
    for (int i=unknown_cells_1124.Count-1;i>=0;--i)
        if (unknown_cells_1124[i]==cell) return;
    try { (void)unknown_cells_1124.AddItem(cell); } catch (...) { }
    unknown_bool_14D9=true;
}
bool RadarClass::UpdateTerrainRadar(bool& changed) noexcept {
    changed=false;
    if (!unknown_123C) return false;
    for (int i=unknown_cells_1124.Count-1;i>=0;--i) {
        auto* cell=MapClass::Instance.TryGetCellAt(unknown_cells_1124[i]);
        if (!cell || !MapClass::Instance.IsWithinUsableArea(cell->MapCoords,true)) continue;
        const int x=cell->MapCoords.X+std::bit_cast<int>(unknown_1490)-cell->MapCoords.Y;
        const int y=cell->MapCoords.X+cell->MapCoords.Y-std::bit_cast<int>(unknown_1498);
        if (y<0 || y>=int(unknown_1244) || x<-1 || x>=int(unknown_1240)) continue;
        const auto color=cell->GetTerrainRadarColor();
        for (int col=std::max(0,x);col<std::min(x+2,int(unknown_1240));++col) {
            auto& pixel=unknown_123C[y*unknown_1240+col];
            changed|=pixel.R!=color.R || pixel.G!=color.G || pixel.B!=color.B;
            pixel=color;
        }
    }
    unknown_cells_1124.Count=0;
    if (changed) unknown_bool_14D9=true;
    // A failed rebuild leaves a null cache, so the next presentation retries
    // even after the canonical RGB buffer has already been updated.
    return (!changed && unknown_1220) || RebuildTerrainRadarCache();
}
bool RadarClass::ApplyTerrainVisibility(WORD* pixels,unsigned count) const noexcept {
    const auto& rect=unknown_rect_149C;
    if (!pixels || rect.Width<1 || rect.Height<1 || rect.Width>140 || rect.Height>108 || count<unsigned(rect.Width*rect.Height)) return false;
    for (int y=0;y<rect.Height;++y) for (int x=0;x<rect.Width;++x) {
        CellStruct cell;
        if (!RadarToTerrainCell({x+rect.X,y+rect.Y},cell)) return false;
        auto* terrain=MapClass::Instance.GetCellAt(cell);
        CoordStruct world=CellClass::Cell2Coord(cell,terrain->GetFloorHeight({128,128}));
        auto& pixel=pixels[y*rect.Width+x];
        if (MapClass::Instance.IsLocationGapped(world)) pixel=WORD((pixel>>1)&0x7BEF);
        else if (MapClass::Instance.IsLocationShrouded(world)) pixel=0;
    }
    return true;
}
