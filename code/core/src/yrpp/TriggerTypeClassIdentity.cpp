// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/TriggerTypeClass.h"
#include <cstring>

// 0x00727BB0: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL TriggerTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0xc02d1591u, 0x11d20a2au, 0x6000a7acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x00727CA0
AbstractType TriggerTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(TriggerTypeClass::AbsID) == 39u);

// 0x00727CB0: native Size follows native layout; original stream adapters remain separate.
int TriggerTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(TriggerTypeClass) == 180);
#endif
