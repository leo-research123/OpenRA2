// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/WeaponTypeClass.h"
#include <cstring>

// 0x00772C90: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL WeaponTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x9fd219cau, 0x11d20f7bu, 0x60007281u, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x007730E0
AbstractType WeaponTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(WeaponTypeClass::AbsID) == 49u);

// 0x007730D0: native Size follows native layout; original stream adapters remain separate.
int WeaponTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(WeaponTypeClass) == 352);
#endif
