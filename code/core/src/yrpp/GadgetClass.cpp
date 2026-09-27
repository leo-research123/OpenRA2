// Adapted from EA CnC_Remastered_Collection REDALERT/GADGET.CPP,
// f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic Arts Inc.
// GPL-3.0-or-later with EA Section 7 restrictions; see third_party/ea/LICENSE.TXT.
// YR calibration: 4E12F0..4E1A40, notably right-button capture and modifiers.
#include "yrpp/GadgetClass.h"
#include "game_ui_runtime.hpp"
#include <cstdint>

GadgetClass::GadgetClass(int x,int y,int width,int height,GadgetFlag flags,bool sticky) noexcept
    : LinkClass(),X(x),Y(y),Width(width),Height(height),NeedsRedraw(false),
      IsSticky(sticky),Disabled(false),Flags(sticky ? static_cast<GadgetFlag>(flags|GadgetFlag::LeftPress|GadgetFlag::LeftRelease) : flags) {}
GadgetClass::GadgetClass(GadgetClass& other) noexcept
    : LinkClass(),X(other.X),Y(other.Y),Width(other.Width),Height(other.Height),
      NeedsRedraw(other.NeedsRedraw),IsSticky(other.IsSticky),Disabled(other.Disabled),Flags(other.Flags) {}
GadgetClass& GadgetClass::operator=(GadgetClass& other) {
    LinkClass::operator=(other);
    X=other.X;Y=other.Y;Width=other.Width;Height=other.Height;
    NeedsRedraw=other.NeedsRedraw;IsSticky=other.IsSticky;Disabled=other.Disabled;Flags=other.Flags;
    return *this;
}
GadgetClass::~GadgetClass() {
    KillFocus();
    if (StuckOn==this) StuckOn=nullptr;
    if (LastList==this) LastList=nullptr;
    // Native lifecycle guard: the original destructor leaves this stale.
    if (Hovered==this) Hovered=nullptr;
}
GadgetClass* GadgetClass::GetNext() { return static_cast<GadgetClass*>(Next); }
GadgetClass* GadgetClass::GetPrev() { return static_cast<GadgetClass*>(Previous); }
GadgetClass* GadgetClass::Remove() { KillFocus(); return static_cast<GadgetClass*>(LinkClass::Remove()); }
void GadgetClass::Disable() { Disabled=true; KillFocus(); MarkRedraw(); }
void GadgetClass::Enable() { Disabled=false; MarkRedraw(); }
const unsigned int GadgetClass::GetID() { return 0; }
void GadgetClass::MarkRedraw() { NeedsRedraw=true; }
void GadgetClass::PeerToPeer(unsigned int,DWORD*,GadgetClass*) {}
void GadgetClass::SetFocus() {
    if (Focused) { Focused->MarkRedraw(); Focused->KillFocus(); }
    Flags|=GadgetFlag::Keyboard; Focused=this;
}
void GadgetClass::KillFocus() { if (Focused==this) { Flags&=~GadgetFlag::Keyboard; Focused=nullptr; } }
bool GadgetClass::IsFocused() { return Focused==this; }
bool GadgetClass::IsToRedraw() { return NeedsRedraw; }
bool GadgetClass::IsListToRedraw() {
    for (auto* node=this;node;node=node->GetNext()) if (node->IsToRedraw()) return true;
    return false;
}
void GadgetClass::MarkListToRedraw() { for (auto* n=this;n;n=n->GetNext()) n->MarkRedraw(); }
void GadgetClass::SetPosition(int x,int y) { X=x; Y=y; }
void GadgetClass::SetDimension(int width,int height) { Width=width; Height=height; }
bool GadgetClass::Draw(bool force) {
    if (!force && !NeedsRedraw) return false;
    NeedsRedraw=false; return true;
}
void GadgetClass::DrawAll(bool force) { for (auto* n=this;n;n=n->GetNext()) n->Draw(force); }
void GadgetClass::OnMouseEnter() { MarkRedraw(); }
void GadgetClass::OnMouseLeave() { MarkRedraw(); }
void GadgetClass::StickyProcess(GadgetFlag flags) {
    const auto bits=static_cast<unsigned>(flags);
    if (IsSticky && (bits&0x11)) StuckOn=this;
    if (StuckOn==this && (bits&0x44)) StuckOn=nullptr;
}
bool GadgetClass::Action(GadgetFlag flags,DWORD*,KeyModifier) {
    if (!static_cast<unsigned>(flags)) return false;
    MarkRedraw(); StickyProcess(flags); return true;
}
bool GadgetClass::Clicked(DWORD* key,GadgetFlag flags,int x,int y,KeyModifier modifier) {
    flags&=Flags;
    const auto bits=static_cast<unsigned>(flags);
    if (this==StuckOn || (bits&0x100) || (bits &&
        std::uint32_t(x)-std::uint32_t(X)<std::uint32_t(Width) &&
        std::uint32_t(y)-std::uint32_t(Y)<std::uint32_t(Height))) return Action(flags,key,modifier);
    return false;
}
GadgetClass* GadgetClass::ExtractGadgetAt(int x,int y) {
    GadgetClass* result=nullptr;
    std::int64_t area=1024*768;
    for (auto* n=this;n;n=n->GetNext()) {
        const auto candidate=std::int64_t(n->Width)*n->Height;
        if (!n->Disabled && x>=n->X && y>=n->Y && std::int64_t(x)<std::int64_t(n->X)+n->Width &&
            std::int64_t(y)<std::int64_t(n->Y)+n->Height && candidate<=area) { result=n; area=candidate; }
    }
    return result;
}
GadgetClass* GadgetClass::ExtractGadget(unsigned id) {
    for (auto* n=this;n;n=n->GetNext()) if (n->GetID()==id) return n;
    return nullptr;
}
void GadgetClass::DeleteList() {
    auto* n=static_cast<GadgetClass*>(HeadOfList());
    while (n) { auto* next=n->GetNext(); delete n; n=next; }
}
void GadgetClass::ResetInput() noexcept {
    try {
        if (StuckOn) { DWORD key=0; StuckOn->Action(static_cast<GadgetFlag>(0x44),&key,KeyModifier::None); }
        if (Hovered) Hovered->OnMouseLeave();
        if (Focused) Focused->KillFocus();
    } catch (...) {} // Always release device capture even if a gadget fails.
    StuckOn=LastList=Focused=Hovered=nullptr;
}
DWORD GadgetClass::Dispatch(DWORD key,GadgetFlag flags,int x,int y,KeyModifier modifier) {
    if (LastList!=this) { LastList=this; StuckOn=Focused=nullptr; }
    auto* hover=ExtractGadgetAt(x,y);
    if (hover!=Hovered) {
        if (Hovered) Hovered->OnMouseLeave();
        Hovered=hover;
        if (Hovered) Hovered->OnMouseEnter();
    }
    if (StuckOn) { StuckOn->Clicked(&key,flags,x,y,KeyModifier::None); return key; }
    if (Focused && (static_cast<unsigned>(flags)&0x100)) {
        Focused->Clicked(&key,flags,x,y,KeyModifier::None); return key;
    }
    for (auto* n=this;n;n=n->GetNext())
        if (!n->Disabled && n->Clicked(&key,flags,x,y,modifier)) break;
    return key;
}
DWORD GadgetClass::Input() {
    const auto* frame=game::game_ui_input();
    return frame ? Dispatch(frame->key,frame->flags,frame->point.X,frame->point.Y,frame->modifier) : 0;
}
