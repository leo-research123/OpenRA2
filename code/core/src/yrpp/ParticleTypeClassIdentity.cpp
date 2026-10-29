// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/ParticleTypeClass.h"
#include <cstring>

// 0x00645620: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL ParticleTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x703e044bu, 0x11d20fb1u, 0x60007281u, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x00645920
AbstractType ParticleTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(ParticleTypeClass::AbsID) == 23u);

// 0x00645910: native Size follows native layout; original stream adapters remain separate.
int ParticleTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(ParticleTypeClass) == 792);
#endif
