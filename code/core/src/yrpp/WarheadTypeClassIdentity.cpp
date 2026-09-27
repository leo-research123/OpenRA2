// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/WarheadTypeClass.h"
#include <cstring>

// 0x0075E080: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL WarheadTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0xa8c54da4u, 0x11d20f7bu, 0x60007281u, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x0075E500
AbstractType WarheadTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(WarheadTypeClass::AbsID) == 50u);

// 0x0075E4F0: native Size follows native layout; original stream adapters remain separate.
int WarheadTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(WarheadTypeClass) == 464);
#endif
