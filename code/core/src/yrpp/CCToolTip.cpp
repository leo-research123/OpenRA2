// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/cctooltip.cpp; YR 0x478BA0..0x479050 calibration.
// Copyright 2026 OpenTS contributors; EA terms: third_party/opents/LICENSE.md.
#include "yrpp/CCToolTip.h"
#include "yrpp/BitFont.h"
#include "yrpp/BitText.h"
#include "yrpp/MouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/TacticalClass.h"
#include "tooltip_platform.hpp"
#include "game_ui_runtime.hpp"
#include <algorithm>

namespace { bool hide_name=false;CCToolTip* instance=nullptr; }
bool& CCToolTip::HideName=hide_name;
CCToolTip*& CCToolTip::Instance=instance;
CCToolTip::CCToolTip(HWND window) noexcept:ToolTipManager(window),FullRedraw(false),Delay(7){}

bool CCToolTip::Update(ToolTipManagerData& data) {
    Point2D pointer;auto* font=BitFont::Instance;
    if(!game::tooltip_pointer(pointer) || !font || !font->InternalPTR)return false;
    auto& r=data.Dimension;
    const auto& tactical=DSurface::ViewBounds;
    const auto& sidebar=DSurface::SidebarBounds;
    const bool side=GameOptionsClass::Instance.SidebarMode ? r.X>tactical.X+tactical.Width
                                                        : r.X<=sidebar.X+sidebar.Width;
    const auto& region=side?sidebar:tactical;
    if(region.Width<=0 || region.Height<=0)return false;
    if(side)SidebarClass::Instance.SidebarNeedsRedraw=true;
    int width=0,height=0;font->GetTextDimension(data.HelpText,&width,&height,region.Width);
    width+=4;height+=3;r.Width=std::max(r.Width,width);r.Height=std::max(r.Height,height);
    if(r.Width>=region.Width){
        font->GetTextDimension(data.HelpText,&width,&height,region.Width-4);
        width+=4;height+=3;r.Width=std::max(r.Width,width);r.Height=std::max(r.Height,height);
    }
    if(CurrentToolTip && CurrentToolTip->field_18){
        const auto& anchor=CurrentToolTip->Bounds;
        r.X=anchor.X>=region.X+region.Width/2 ? anchor.X-width+5 : anchor.X+anchor.Width-5;
        r.Y=anchor.Y>=region.Y+region.Height/2 ? anchor.Y-height+5 : anchor.Y+anchor.Height-5;
        r.X=std::max(r.X,region.X);r.Y=std::max(r.Y,region.Y);
    }else{
        const int overflow=r.X+r.Width-region.X-region.Width;if(overflow>0)r.X-=overflow;
        r.Y+=16;if(r.Y+r.Height>region.Y+region.Height)r.Y-=r.Height+16;
        // YR does not retain OpenTS's final top clamp in this branch.
    }
    return true;
}
void CCToolTip::MarkToRedraw(ToolTipManagerData& data) {
    const auto& tactical=DSurface::ViewBounds;const auto& sidebar=DSurface::SidebarBounds;
    const bool side=GameOptionsClass::Instance.SidebarMode ? data.Dimension.X>=tactical.X+tactical.Width
                                                        : data.Dimension.X<=sidebar.X+sidebar.Width;
    if(side)SidebarClass::Instance.SidebarNeedsRedraw=SidebarClass::Instance.SidebarBackgroundNeedsRedraw=true;
    // Native whole-target drawing has no saved pixels to restore.
    if(TacticalClass::Instance)TacticalClass::Instance->Redrawing=true; // 0x004F42F0(0)
    ToolTipManager::MarkToRedraw(data);
}
void CCToolTip::Draw(bool onSidebar){FullRedraw=onSidebar;ToolTipManager::Draw(!onSidebar);}
wchar_t* CCToolTip::GetToolTipText(unsigned int id) {
    Point2D point;if(!game::tooltip_pointer(point))return nullptr;
    return const_cast<wchar_t*>(MouseClass::Instance.GetToolTip(id));
}
void CCToolTip::DrawText(ToolTipManagerData& data) {
    auto* frame=game::game_ui_frame();auto* font=BitFont::Instance;
    if(!frame || !font || !font->InternalPTR || !BitText::Instance)return;
    const auto& tactical=DSurface::ViewBounds;const auto& sidebar=DSurface::SidebarBounds;
    const auto& r=data.Dimension;
    const int edge=GameOptionsClass::Instance.SidebarMode?tactical.X+tactical.Width:sidebar.X+sidebar.Width;
    const bool map=GameOptionsClass::Instance.SidebarMode?r.X+r.Width<=edge:r.X>=edge;
    const bool side=GameOptionsClass::Instance.SidebarMode?r.X>=edge:r.X+r.Width<edge;
    if(!map && !(FullRedraw && side))return;
    // 0x478EF5 uses Surface::GetWidth (vtable 0x7C); the draw box adds
    // 8x4 and reclamps X, distinct from Update's 4x3 measurement.
    const auto& region=map?tactical:sidebar;
    int width=0,height=0;font->GetTextDimension(data.HelpText,&width,&height,region.Width);
    width+=8;height+=4;
    const int x=std::min(r.X,region.X+region.Width-width);
    const RectangleStruct box{x,r.Y,width,height};
    const auto& rgb=ToolTipTextColor;
    const WORD color=WORD((rgb.Red>>3)<<11|(rgb.Green>>2)<<5|(rgb.Blue>>3));
    game::draw_ui_fill(box,0);
    game::draw_ui_fill({box.X,box.Y,box.Width,1},color);
    game::draw_ui_fill({box.X,box.Y+box.Height-1,box.Width,1},color);
    game::draw_ui_fill({box.X,box.Y,1,box.Height},color);
    game::draw_ui_fill({box.X+box.Width-1,box.Y,1,box.Height},color);
    LTRBStruct clip{box.X,box.Y,box.X+box.Width,box.Y+box.Height};
    font->SetClipMode(true);font->SetRectangle(&clip);font->SetColor(color);
    BitText::Instance->DrawText(font,nullptr,data.HelpText,box.X+4,box.Y+2,width,height,0,0,0);
}
