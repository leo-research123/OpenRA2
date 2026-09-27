// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 map.cpp Clip_To_Map; YR 0x00586E50.
// Electronic Arts / OpenTS; EA Section 7: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include <bit>
#include <cstdint>

CellStruct* MapClass::ClipToMap(CellStruct* output,const CellStruct& cell) const {
    int x=cell.X,y=cell.Y;
    if(x-y>=2*(VisibleRect.X+VisibleRect.Width)-MapRect.Width) {
        const int adjust=(x+MapRect.Width+2*(1-VisibleRect.X-VisibleRect.Width)-y)/2;
        x-=adjust;y+=adjust;
    } else if(y-x>=MapRect.Width-2*VisibleRect.X) {
        const int adjust=(y+2*VisibleRect.X+2-MapRect.Width-x)/2;
        y-=adjust;x+=adjust;
    }
    const auto word=[](int v){return std::bit_cast<short>(static_cast<unsigned short>(v));};
    CellStruct adjusted{word(x),word(y)};
    const auto* terrain=MapClass::Instance.GetCellAt(adjusted);
    int height=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(terrain->Level));
    if(terrain->SlopeIndex && x+y<height+MapRect.Width+2*VisibleRect.Y+4)++height;
    if(x+y<=height+MapRect.Width+2*VisibleRect.Y) {
        do {adjusted.X=word(adjusted.X+1);adjusted.Y=word(adjusted.Y+1);}
        while(!IsWithinUsableArea(adjusted,true));
    } else if(x+y>height+MapRect.Width+2*(VisibleRect.Y+VisibleRect.Height)+2) {
        do {adjusted.X=word(adjusted.X-1);adjusted.Y=word(adjusted.Y-1);}
        while(!IsWithinUsableArea(adjusted,true));
    }
    *output=adjusted;return output;
}
