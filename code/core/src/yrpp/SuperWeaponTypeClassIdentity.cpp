// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/SuperWeaponTypeClass.h"
#include <cstring>

// 0x006CE7C0: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL SuperWeaponTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x0cf2bce7u, 0x11d236e4u, 0x6000d8b8u, 0xed09c808u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x006CE8F0
AbstractType SuperWeaponTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(SuperWeaponTypeClass::AbsID) == 32u);

// 0x006CE900: native Size follows native layout; original stream adapters remain separate.
int SuperWeaponTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(SuperWeaponTypeClass) == 256);
#endif
