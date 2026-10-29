// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/TagTypeClass.h"
#include <cstring>

// 0x006E63A0: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL TagTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x54f6e433u, 0x11d209edu, 0x6000a5acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x006E6490
AbstractType TagTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(TagTypeClass::AbsID) == 45u);

// 0x006E64A0: native Size follows native layout; original stream adapters remain separate.
int TagTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(TagTypeClass) == 164);
#endif
