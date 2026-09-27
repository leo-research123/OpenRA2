// POWERP.SHP from 63F7C0; segment order/offsets from 63FB20..63FDB1.
// Counts stay in PowerClass's original fields. House production/consumption
// updates are a separate world pass; absent House data displays empty segments.
#include "yrpp/PowerClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Surface.h"
#include "game_ui_runtime.hpp"
#include <bit>

void PowerClass::Draw(DWORD force) {
    if (!game::game_ui_frame()) return;
    if (!(force || PowerNeedRedraw) || !SidebarClass::Instance.IsSidebarActive) return;
    const int count=(SidebarClass::CameoHeight+3)/3;
    const int green=std::bit_cast<int>(unknown_152C);
    const int yellow=std::bit_cast<int>(unknown_1530);
    const int red=std::bit_cast<int>(unknown_1534);
    const bool other=ScenarioClass::Instance && ScenarioClass::Instance->PlayerSideIndex;
    const int x=DSurface::SidebarBounds.X+(other ? 0 : 5);
    int y=DSurface::SidebarBounds.Y+69;
    const auto segments=[&](int number,int frame) {
        for (int i=0;i<number;++i,y+=3) game::draw_ui_shape(game::UiImage::power,{x,y},frame);
    };
    segments(count-green-yellow-red,0);
    const bool flash=std::bit_cast<int>(unknown_151C)>0 && !(unknown_151C&1);
    if (flash) segments(1,4);
    // The flash replaces the first populated band, including all-red low
    // power. 0x0063FC72 resets the index only when that band is nonempty.
    int skip=int(flash);
    if(green>0){segments(green-skip,1);skip=0;}
    if(yellow>0){segments(yellow-skip,2);skip=0;}
    if(red>0)segments(red-skip,3);
    PowerNeedRedraw=false;
}
