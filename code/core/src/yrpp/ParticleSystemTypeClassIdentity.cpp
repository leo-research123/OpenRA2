// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/ParticleSystemTypeClass.h"
#include <cstring>

// 0x006447A0: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL ParticleSystemTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x703e044au, 0x11d20fb1u, 0x60007281u, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x00644930
AbstractType ParticleSystemTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(ParticleSystemTypeClass::AbsID) == 25u);

// 0x00644920: native Size follows native layout; original stream adapters remain separate.
int ParticleSystemTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(ParticleSystemTypeClass) == 784);
#endif
