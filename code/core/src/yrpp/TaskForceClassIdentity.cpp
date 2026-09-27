// Interface baseline: YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc.
// Calibrated only to supplied functions/*.c, *.asm and primary-vtable JSON.
// This file implements identity, not the whole type or its persistence.
#include "yrpp/TaskForceClass.h"
#include <cstring>

// 0x006E8710: E_POINTER does not write to the destination.
HRESULT YRPP_STDCALL TaskForceClass::GetClassID(CLSID* pClassID) {
    if (!pClassID) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x61de341eu, 0x11d20774u, 0x6000a5acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(words));
    std::memcpy(pClassID, words, sizeof(words));
    return 0;
}

// 0x006E87D0
AbstractType TaskForceClass::WhatAmI() const { return AbsID; }
static_assert(static_cast<unsigned>(TaskForceClass::AbsID) == 33u);

// 0x006E87E0: native Size follows native layout; original stream adapters remain separate.
int TaskForceClass::Size() const { return sizeof(*this); }
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(TaskForceClass) == 212);
#endif
