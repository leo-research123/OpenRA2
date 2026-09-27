// YRpp 9402d7da declarations, calibrated to gamemd 1.001.
#include "yrpp/OverlayClass.h"
#include <cstring>

HRESULT YRPP_STDCALL OverlayClass::GetClassID(CLSID* dest) {
    // Original 0x5fdf10: E_POINTER leaves memory untouched.
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD id[4] = {0x0e272dc7u, 0x11d19c0fu, 0xa00009b7u, 0xd1afdd24u};
    static_assert(sizeof(CLSID) == sizeof(id));
    std::memcpy(dest, id, sizeof(id));
    return 0;
}
AbstractType OverlayClass::WhatAmI() const { return AbsID; } // 0x5fdf50
int OverlayClass::Size() const { return sizeof(*this); } // 0x5fdf00
static_assert(static_cast<int>(OverlayClass::AbsID) == 20);
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(OverlayClass) == 0xb0);
#endif

ObjectTypeClass* OverlayClass::GetType() const { return Type; }
