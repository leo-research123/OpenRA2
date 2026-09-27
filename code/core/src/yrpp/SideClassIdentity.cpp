// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/SideClass.h"
#include <cstring>

// 0x006A4740: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL SideClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0xc53dd372u, 0x11d2151eu, 0x60007581u, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x006A4920
AbstractType SideClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(SideClass::AbsID) == 28u);

// 0x006A4910: native Size follows native layout; original stream adapters remain separate.
int SideClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(SideClass) == 180);
#endif
