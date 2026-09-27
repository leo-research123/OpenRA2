#pragma once

#include "yrpp/GeneralStructures.h"
#include "yrpp/Surface.h"

class BeaconClass
{
public:
    /// Global VA: 0x0089C3B0.
    static BeaconClass* (&Array)[8][3];
    /// Global VA: 0x0089C410.
    static int& Count;
    /// VA: 0x00430210
#if defined(RA2_YRPP_GAME)
    BeaconClass() JMP_THIS(0x430210)
#else
    BeaconClass() noexcept;
#endif

    /// VA: 0x00430250.
    void Draw(Surface* pSurface, RectangleStruct bounds) JMP_THIS(0x430250)
    /// VA: 0x00430590
#if defined(RA2_YRPP_GAME)
    void SetCoordAndHouse(CoordStruct coord, int houseId) JMP_THIS(0x430590)
#else
    void SetCoordAndHouse(CoordStruct coord, int houseId) noexcept;
#endif
    // TODO bitfield functions
    /// VA: 0x00430620
#if defined(RA2_YRPP_GAME)
    void SetText(const wchar_t* pText) JMP_THIS(0x430620)
#else
    void SetText(const wchar_t* text) noexcept;
#endif
    /// VA: 0x00430650
#if defined(RA2_YRPP_GAME)
    void DrawRadar(Surface* pSurface, RectangleStruct bounds, bool toClear = false) JMP_THIS(0x430650)
#else
    // Whole composition supplies the background, so toClear submits no shape.
    void DrawRadar(Surface* surface, RectangleStruct bounds, bool toClear = false) noexcept;
#endif
    /// VA: 0x004308B0
#if defined(RA2_YRPP_GAME)
    bool VisibleToPlayer() const JMP_THIS(0x4308B0)
#else
    bool VisibleToPlayer() const noexcept;
#endif

    CoordStruct Coord;
    byte Bitfield;
    byte gapD[1];
    wchar_t Text[128];
    byte field_10E;
    byte field_10F;
    int HouseID;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(BeaconClass) == 0x114, "BeaconClass size is incorrect");
#endif
