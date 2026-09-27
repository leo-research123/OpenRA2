// YRpp 9402d7da declarations, calibrated to gamemd 1.001.
#include "yrpp/TerrainClass.h"
#include <cstring>

HRESULT YRPP_STDCALL TerrainClass::GetClassID(CLSID* dest) {
    // Original 0x71d310: E_POINTER leaves memory untouched.
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD id[4] = {0x0e272dceu, 0x11d19c0fu, 0xa00009b7u, 0xd1afdd24u};
    static_assert(sizeof(CLSID) == sizeof(id));
    std::memcpy(dest, id, sizeof(id));
    return 0;
}
AbstractType TerrainClass::WhatAmI() const { return AbsID; } // 0x71d300
int TerrainClass::Size() const { return sizeof(*this); } // 0x71d2f0
static_assert(static_cast<int>(TerrainClass::AbsID) == 36);
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(TerrainClass) == 0xe0);
#endif

ObjectTypeClass* TerrainClass::GetType() const { return Type; }
