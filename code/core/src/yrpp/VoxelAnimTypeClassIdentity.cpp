// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/VoxelAnimTypeClass.h"
#include <cstring>

// 0x0074B7D0: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL VoxelAnimTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x2ebb6d66u, 0x11d20d4du, 0x60007281u, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x0074B9F0
AbstractType VoxelAnimTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(VoxelAnimTypeClass::AbsID) == 42u);

// 0x0074BA00: native Size follows native layout; original stream adapters remain separate.
int	VoxelAnimTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(VoxelAnimTypeClass) == 776);
#endif
