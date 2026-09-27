// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/AITriggerTypeClass.h"
#include <cstring>

// 0x0041E500: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL AITriggerTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0xba093524u, 0x11d24cf4u, 0x100026bcu, 0x4db08f4bu};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x0041FFD0
AbstractType AITriggerTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(AITriggerTypeClass::AbsID) == 59u);

// 0x0041FFE0: native Size follows native layout; original stream adapters remain separate.
int AITriggerTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(AITriggerTypeClass) == 272);
#endif
