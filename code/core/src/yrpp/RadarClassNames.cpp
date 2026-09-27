// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 radar.cpp Draw_Names.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// Additional terms: third_party/opents/LICENSE.md. YR 0x00653FA0: frame 32,
// UTF-16 names, no observer exclusion, fixed 20-house kill counters.
#include "yrpp/RadarClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/StringTable.h"
#include "yrpp/Surface.h"
#include "game_ui_runtime.hpp"
#include "scenario_runtime.hpp"
#include <bit>
#include <cwchar>

void RadarClass::DrawNames() noexcept {
    using namespace game;
    if (!SidebarClass::Instance.IsSidebarActive || !game_ui_frame()) return;
    const auto* render=scenario_runtime().render;
    const auto* schemes=render ? render->color_schemes : nullptr;
    ColorScheme* grey=nullptr;
    if (schemes) for (auto* scheme : *schemes)
        if (scheme && scheme->ShadeCount==1 && scheme->ID && !_strcmpi(scheme->ID,"Grey")) {grey=scheme;break;}
    if (!grey) {record_ui_drawing(DrawingStatus::unavailable);return;}
    const RectangleStruct clip{DSurface::SidebarBounds.X,0,DSurface::SidebarBounds.Width,DSurface::WindowBounds.Height};
    draw_ui_shape(UiImage::radar,{clip.X+int(padding_11E4),int(unknown_11EC)},32);
    const int left=int(unknown_11F0),right=left+int(unknown_11F8)-2;
    int y=int(unknown_11F4)+2;
    constexpr auto style=static_cast<TextPrintType>(0x19);
    try {
        draw_ui_text(StringTable::LoadString("TXT_NAME_COLON"),clip,{left,y},grey,style);
        draw_ui_text(StringTable::LoadString("TXT_KILLS_COLON"),clip,{right,y},grey,style|TextPrintType::Right);
    } catch (...) {record_ui_drawing(DrawingStatus::backend_failure);return;}
    y+=7;
    WORD line_color;
    if (!ui_palette_color(14,line_color)) {record_ui_drawing(DrawingStatus::unavailable);return;}
    draw_ui_fill({clip.X+left,y,int(unknown_11F8),1},line_color);
    y+=4;
    for (auto* house : HouseClass::Array) {
        if (!house || !house->Type || !house->Type->Multiplay) continue;
        auto* color=grey;
        auto flags=style;
        if (!house->Defeated) {
            const int index=house->ColorSchemeIndex;
            if (!schemes || index<0 || index>=schemes->Count || !(*schemes)[index]) {
                record_ui_drawing(DrawingStatus::unavailable);return;
            }
            color=(*schemes)[index]; flags|=TextPrintType::UseGradPal;
        }
        wchar_t name[40]{};
        unsigned length=0;
        while(length<21 && house->UIName[length]) {name[length]=house->UIName[length];++length;}
        if (!length) std::wcscpy(name,L"________");
        else if (length>18) {name[18]=L'.';name[19]=0;}
        draw_ui_text(name,clip,{left,y},color,flags);
        unsigned kills=0;
        for (int i=0;i<20;++i)
            kills+=static_cast<unsigned>(house->KilledUnitsOfHouses[i])+static_cast<unsigned>(house->KilledBuildingsOfHouses[i]);
        wchar_t number[32]{};
        std::swprintf(number,32,L"%2d",std::bit_cast<int>(kills));
        draw_ui_text(number,clip,{right,y},color,flags|TextPrintType::Right);
        y+=9;
    }
}
