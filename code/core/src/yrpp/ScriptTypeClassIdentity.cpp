// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/ScriptTypeClass.h"
#include <cstring>

// 0x00691D50: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL ScriptTypeClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x42f3a647u, 0x11d20789u, 0x6000a5acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x00691F70
AbstractType ScriptTypeClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(ScriptTypeClass::AbsID) == 27u);

// 0x00691F80: native Size follows native layout; original stream adapters remain separate.
int ScriptTypeClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(ScriptTypeClass) == 564);
#endif
