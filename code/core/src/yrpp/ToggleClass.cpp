// EA REDALERT/TOGGLE.CPP, f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// Copyright 2020 Electronic Arts Inc. GPL-3.0-or-later with EA Section 7;
// see code/third_party/ea/LICENSE.TXT. YR 723E60..723FC0.
#include "yrpp/ToggleClass.h"
#include "game_ui_runtime.hpp"
ToggleClass::ToggleClass(unsigned id,int x,int y,int width,int height) noexcept
    : ControlClass(id,x,y,width,height,static_cast<GadgetFlag>(5),true),IsPressed(false),IsOn(false),ToggleType(0) {}
void ToggleClass::TurnOn() { IsOn=true; MarkRedraw(); }
void ToggleClass::TurnOff() { IsOn=false; MarkRedraw(); }
bool ToggleClass::Action(GadgetFlag flags,DWORD* key,KeyModifier modifier) {
    const auto* input=game::game_ui_input();
    const bool inside=input && input->point.X>=X && input->point.Y>=Y && input->point.X<X+Width && input->point.Y<Y+Height;
    unsigned bits=static_cast<unsigned>(flags);
    if (!bits && IsPressed!=inside) { IsPressed=inside; MarkRedraw(); }
    StickyProcess(flags);
    if (bits&0x11) {
        IsPressed=true; MarkRedraw();
        ControlClass::Action(static_cast<GadgetFlag>(bits&~0x11u),key,modifier);
        if (key) *key=0;
        return true;
    }
    if (bits&0x44) {
        if (!IsPressed) bits&=~0x44u;
        else {
            if (inside) {
                if (ToggleType==1) IsOn=!IsOn;
                else if (ToggleType==2 && !IsOn) IsOn=true;
            }
            IsPressed=false; MarkRedraw();
            // Original Toggle depends on periodic held dispatch to update the
            // pressed flag outside. Synchronous event delivery must also handle
            // release outside without an intervening motion/tick.
            if (!inside) bits&=~0x44u;
        }
    }
    return ControlClass::Action(static_cast<GadgetFlag>(bits),key,modifier);
}
