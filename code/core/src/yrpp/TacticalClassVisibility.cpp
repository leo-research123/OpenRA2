// YR visibility classification/visible-cell queue, 0x6D8700 / 0x6DA7D0.
// 0x7F4194 is the original 256-entry neighbor-mask to SHP-frame lookup.
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "map_runtime.hpp"
#include <bit>
#include <cstdint>
namespace {
constexpr signed char frames[]{
-1,33,2,2,34,37,2,2,4,26,6,6,4,26,6,6,
35,45,17,17,38,41,17,17,4,26,6,6,4,26,6,6,
8,21,10,10,27,31,10,10,12,23,14,14,12,23,14,14,
8,21,10,10,27,31,10,10,12,23,14,14,12,23,14,14,
32,36,25,25,44,40,25,25,19,30,20,20,19,30,20,20,
39,43,29,29,42,46,29,29,19,30,20,20,19,30,20,20,
8,21,10,10,27,31,10,10,12,23,14,14,12,23,14,14,
8,21,10,10,27,31,10,10,12,23,14,14,12,23,14,14,
1,1,3,3,16,16,3,3,5,5,7,7,5,5,7,7,
24,24,18,18,28,28,18,18,5,5,7,7,5,5,7,7,
9,9,11,11,22,22,11,11,13,13,-2,-2,13,13,-2,-2,
9,9,11,11,22,22,11,11,13,13,-2,-2,13,13,-2,-2,
1,1,3,3,16,16,3,3,5,5,7,7,5,5,7,7,
24,24,18,18,28,28,18,18,5,5,7,7,5,5,7,7,
9,9,11,11,22,22,11,11,13,13,-2,-2,13,13,-2,-2,
9,9,11,11,22,22,11,11,13,13,-2,-2,13,13,-2,-2,
};
int wrap(unsigned value){return std::bit_cast<int>(value);}
}
char TacticalClass::GetOcclusion(const CellStruct& at,bool fog) const {
    auto& map=MapClass::Instance;
    auto* cell=map.GetCellAt(at);
    const unsigned flags=fog?static_cast<unsigned>(cell->Flags):static_cast<unsigned>(cell->AltFlags);
    const unsigned mask=fog?2u:8u;
    if(!(flags&mask))return (flags&(fog?3u:0x18u))?char(-1):char(-2);
    constexpr short dx[]{-1,0,1,-1,1,-1,0,1},dy[]{-1,-1,-1,0,0,1,1,1};
    constexpr unsigned bits[]{0x40,0x80,1,0x20,2,0x10,8,4};
    unsigned index=0;
    for(int i=0;i<8;++i) {
        const auto* neighbor_cell=map.GetCellAt(CellStruct{short(at.X+dx[i]),short(at.Y+dy[i])});
        const unsigned neighbor=fog?static_cast<unsigned>(neighbor_cell->Flags):static_cast<unsigned>(neighbor_cell->AltFlags);
        if(!(neighbor&mask))index|=bits[i];
    }
    return char(frames[index]);
}
void TacticalClass::RegisterCellAsVisible(CellClass* cell) {
    if(MapClass::Instance.Bitfield || cell->unknown_5C==static_cast<unsigned>(Unsorted::CurrentFrame)
        || (!(static_cast<unsigned>(cell->AltFlags)&8u) && !cell->VisibilityChanged))return;
    cell->unknown_5C=static_cast<unsigned>(Unsorted::CurrentFrame);
    const auto at=cell->GetCellCoords();
    const auto pixel=AdjustForZShapeMove(at.X,at.Y);
    const int x=wrap(static_cast<unsigned>(pixel.X)-static_cast<unsigned>(TacticalPos.X));
    const int y=wrap(static_cast<unsigned>(pixel.Y)-static_cast<unsigned>(AdjustForZ(0))-static_cast<unsigned>(TacticalPos.Y));
    RectangleStruct bounds;
    if(!game::map_view_bounds(bounds))return;
    if(x<bounds.X-30 || y<-30 || x>bounds.X+bounds.Width+30 || y>bounds.Y+bounds.Height+15)return;
    if(VisibleCellCount<799)VisibleCells[VisibleCellCount++]=cell;
    cell->VisibilityChanged=false;Redrawing=true;
}
