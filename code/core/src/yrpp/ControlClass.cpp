// EA REDALERT/CONTROL.CPP, f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// Copyright 2020 Electronic Arts Inc. GPL-3.0-or-later with EA Section 7;
// see code/third_party/ea/LICENSE.TXT. YR 48E520 / 48E5A0 / 48E600 / 48E620.
#include "yrpp/ControlClass.h"
ControlClass::ControlClass(unsigned id,int x,int y,int width,int height,GadgetFlag flags,bool sticky) noexcept
    : GadgetClass(x,y,width,height,flags,sticky),ID(static_cast<int>(id)),SendTo(nullptr) {}
const unsigned ControlClass::GetID() { return static_cast<unsigned>(ID); }
void ControlClass::MakePeer(GadgetClass* peer) { SendTo=peer; }
bool ControlClass::Draw(bool forced) { if (SendTo) SendTo->Draw(false); return GadgetClass::Draw(forced); }
bool ControlClass::Action(GadgetFlag flags,DWORD* key,KeyModifier modifier) {
    const auto bits=static_cast<unsigned>(flags);
    if (bits && key) {
        *key=ID ? static_cast<DWORD>(ID)|0x8000 : 0;
        if (ID && (bits&0x40) && (static_cast<unsigned>(Flags)&0x10)) *key|=0x4000;
    }
    if (SendTo) SendTo->PeerToPeer(bits,key,this);
    return GadgetClass::Action(flags,key,modifier);
}
