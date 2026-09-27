// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/BulletTypeClass.h"
#include <cstring>

// 0x0046C750: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL BulletTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x5af2ce77u, 0x11d20634u, 0x6000a4acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x0046C850
AbstractType BulletTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(BulletTypeClass::AbsID) == 9u);

// 0x0046C860: native Size follows native layout; original stream adapters remain separate.
int BulletTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(BulletTypeClass) == 760);
#endif
