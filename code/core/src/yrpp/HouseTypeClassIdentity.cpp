// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/HouseTypeClass.h"
#include <cstring>

// 0x00512640: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL HouseTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x1dd43928u, 0x11d2046bu, 0x6000a4acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x00512710
AbstractType HouseTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(HouseTypeClass::AbsID) == 14u);

// 0x00512720: native Size follows native layout; original stream adapters remain separate.
int	HouseTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(HouseTypeClass) == 432);
#endif
