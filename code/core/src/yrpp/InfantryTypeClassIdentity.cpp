// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/InfantryTypeClass.h"
#include <cstring>

// 0x00524C70: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL InfantryTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0xae8b33d8u, 0x11d2061cu, 0x6000a4acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x00524D40
AbstractType InfantryTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(InfantryTypeClass::AbsID) == 16u);

// 0x00524D50: native Size follows native layout; original stream adapters remain separate.
int	InfantryTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(InfantryTypeClass) == 3792);
#endif
