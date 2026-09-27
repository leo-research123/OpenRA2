// YRpp 9402d7da declarations, calibrated to gamemd 1.001.
#include "yrpp/IsometricTileClass.h"
#include <cstring>
#include "yrpp/IsometricTileTypeClass.h"

HRESULT YRPP_STDCALL IsometricTileClass::GetClassID(CLSID* dest) {
    // Original 0x543ab0: E_POINTER leaves memory untouched.
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD id[4] = {0x0e272dc0u, 0x11d19c0fu, 0xa00009b7u, 0xd1afdd24u};
    static_assert(sizeof(CLSID) == sizeof(id));
    std::memcpy(dest, id, sizeof(id));
    return 0;
}
AbstractType IsometricTileClass::WhatAmI() const { return AbsID; } // 0x543aa0
int IsometricTileClass::Size() const { return sizeof(*this); } // 0x543a90
static_assert(static_cast<int>(IsometricTileClass::AbsID) == 17);
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(IsometricTileClass) == 0xb0);
#endif

ObjectTypeClass* IsometricTileClass::GetType() const { return Type; }
