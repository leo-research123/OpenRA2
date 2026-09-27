// YRpp 9402d7da declarations, calibrated to gamemd 1.001.
#include "yrpp/SmudgeClass.h"
#include <cstring>

HRESULT YRPP_STDCALL SmudgeClass::GetClassID(CLSID* dest) {
    // Original 0x6b4f50: E_POINTER leaves memory untouched.
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD id[4] = {0x0e272dc5u, 0x11d19c0fu, 0xa00009b7u, 0xd1afdd24u};
    static_assert(sizeof(CLSID) == sizeof(id));
    std::memcpy(dest, id, sizeof(id));
    return 0;
}
AbstractType SmudgeClass::WhatAmI() const { return AbsID; } // 0x6b4f40
int SmudgeClass::Size() const { return sizeof(*this); } // 0x6b4f30
static_assert(static_cast<int>(SmudgeClass::AbsID) == 29);
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(SmudgeClass) == 0xb0);
#endif

ObjectTypeClass* SmudgeClass::GetType() const { return Type; }
