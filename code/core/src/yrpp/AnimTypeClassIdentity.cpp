// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/AnimTypeClass.h"
#include <cstring>

// 0x00428990: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL AnimTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0xae8b33dau, 0x11d2061cu, 0x6000a4acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x00428E50
AbstractType AnimTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(AnimTypeClass::AbsID) == 5u);

// 0x00428E70: native Size follows native layout; original stream adapters remain separate.
int	AnimTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(AnimTypeClass) == 888);
#endif
