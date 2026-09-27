// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/txtlabel.cpp Draw; calibrated to YR 0x0072A4A0.
// Existing Gadget gating plus YR ColorScheme, scrolling and type-on animation.
#include "yrpp/TextLabelClass.h"
#include "yrpp/BitFont.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/OwnerDraw.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/Timer.h"
#include "game_ui_runtime.hpp"
#include <bit>
#include <cwchar>

bool TextLabelClass::Draw(bool forced) {
    if(SkipDraw || !GadgetClass::Draw(forced))return false;
    auto& colors=ColorScheme::Array;
    if(!colors.Count)return false;
    const int index=ColorSchemeIndex<=0 || ColorSchemeIndex>=colors.Count?0:ColorSchemeIndex;
    const auto* scheme=colors[index];if(!scheme)return false;
    auto* font=BitFont::Instance;
    auto* frame=game::game_ui_frame();Surface* surface=frame?nullptr:DSurface::Temp;
    if(!font || !Text || (!surface && !frame))return false;
    const int surface_width=surface?surface->GetWidth():DSurface::WindowBounds.Width;
    RectangleStruct rect{X,Y,std::bit_cast<int>(PixWidth)<0?surface_width:int(PixWidth),font->field_1C};
    // 0x0072A549 writes the reduced width back to the local draw rectangle.
    // The decompiler omits this observable store, but the machine-code trace
    // and downstream font clip both retain it.
    if(rect.Width+X-surface_width>0){
        rect.Width=surface_width-X;
        if(rect.Width<=0 || rect.Height<=0)return false;
    }
    const auto rgb=ColorScheme::HSVToRGB(scheme->BaseColor);
    const unsigned color=unsigned(rgb.R)|(unsigned(rgb.G)<<8)|(unsigned(rgb.B)<<16);
    const int length=int(std::wcslen(Text));
    if(Animate){
        const DWORD now=SystemTimer::GetMilliseconds(),previous=AnimPos;
        AnimPos=previous?previous+((now-AnimTiming)>>4):1;
        if(previous!=AnimPos)AnimTiming=now;
        if(std::bit_cast<int>(AnimPos)<=length && RulesClass::Instance)
            VocClass::PlayGlobal(RulesClass::Instance->MessageCharTyped,0x2000,1.0f,nullptr);
    }
    int scroll=std::bit_cast<int>(anim_dword3C);
    OwnerDraw::DrawEditText(surface,&rect,Text,length,font,color,&scroll,IsFocused(),false,true,std::bit_cast<int>(AnimPos));
    anim_dword3C=DWORD(scroll);
    return true;
}
