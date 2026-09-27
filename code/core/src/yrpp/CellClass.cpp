// YRpp 9402d7da declarations, calibrated to gamemd 1.001.
#include "yrpp/CellClass.h"
#include <cstring>

HRESULT YRPP_STDCALL CellClass::GetClassID(CLSID* dest) {
    // Original 0x485200: E_POINTER leaves memory untouched.
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD id[4] = {0xc1bf99ceu, 0x11d21a8cu, 0x60007581u, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(id));
    std::memcpy(dest, id, sizeof(id));
    return 0;
}
AbstractType CellClass::WhatAmI() const { return AbsID; } // 0x487e60
int CellClass::Size() const { return sizeof(*this); } // 0x487e70
static_assert(static_cast<int>(CellClass::AbsID) == 11);
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(CellClass) == 0x148);
#endif

#include <algorithm>
#include <cassert>

int CellClass::GetFloorHeight(Point2D const& subcoords) const {
    // 0x47B3A0 initializes this table once from LevelHeight. The target's final
    // FILD/FADD at 0x47BB47 adds base height.
    // x/y wrap to the low byte. Original _ftol truncates toward zero.
    constexpr double h = Unsorted::LevelHeight;
    constexpr double ramps[20][5] = {
        { 1, 0, 0, h, 0}, { 0, 1, 0, h, 0},
        {-1, 0, h, h, 0}, { 0,-1, h, h, 0},
        { 1, 1,-h, h, 0}, {-1, 1, 0, h, 0},
        {-1,-1, h, h, 0}, { 1,-1, 0, h, 0},
        { 1, 1, 0, h, 0}, {-1, 1, h, h, 0},
        {-1,-1,2*h,h, 0}, { 1,-1, h, h, 0},
        { 1, 1, 0,2*h,0}, {-1, 1, h,2*h,0},
        {-1,-1,2*h,2*h,0},{ 1,-1, h,2*h,0},
        { 0, 0, 0,h/2,h/2}, {0,0,h,h/2,-h/2},
        { 0, 0, 0,h/2,h/2}, {0,0,h,h/2,-h/2}
    };
    const int base = static_cast<int>(h * static_cast<signed char>(Level) + 0.5);
    if (!SlopeIndex) return base;
    assert(SlopeIndex <= 20);
    const auto& ramp = ramps[SlopeIndex - 1];
    const double height = static_cast<BYTE>(subcoords.Y) * ramp[1] * (h / 256.0)
        + static_cast<BYTE>(subcoords.X) * ramp[0] * (h / 256.0)
        + ramp[2] + ramp[4];
    return static_cast<int>(base + std::clamp(height, 0.0, ramp[3]));
}
void CellClass::SetMapCoords(const CellStruct& coords) { MapCoords = coords; }
CoordStruct* CellClass::GetCellCoords(CoordStruct* dest) const {
    *dest = Cell2Coord(MapCoords, GetFloorHeight({128, 128}));
    return dest;
}
CoordStruct* CellClass::GetCoords(CoordStruct* dest) const { return GetCellCoords(dest); }
CoordStruct* CellClass::GetCenterCoords(CoordStruct* dest) const {
    GetCoords(dest); // Preserve the virtual GetCoords call in 486890.
    if (ContainsBridge()) dest->Z += BridgeHeight;
    return dest;
}
