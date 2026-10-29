// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/TiberiumClass.h"
#include <cstring>

// 0x00721E40: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL TiberiumClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0xc53dd373u, 0x11d2151eu, 0x60007581u, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x007236F0
AbstractType TiberiumClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(TiberiumClass::AbsID) == 46u);

// 0x007236E0: native Size follows native layout; original stream adapters remain separate.
int TiberiumClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(TiberiumClass) == 296);
#endif
