// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/AircraftTypeClass.h"
#include <cstring>

// 0x0041CEB0: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL AircraftTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0xae8b33d9u, 0x11d2061cu, 0x6000a4acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x0041CFB0
AbstractType AircraftTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(AircraftTypeClass::AbsID) == 3u);

// 0x0041CFC0: native Size follows native layout; original stream adapters remain separate.
int	AircraftTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(AircraftTypeClass) == 3600);
#endif
