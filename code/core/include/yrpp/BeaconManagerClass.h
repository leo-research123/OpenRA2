#pragma once

#include "yrpp/GeneralStructures.h"
#include "yrpp/BeaconClass.h"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/Surface.h"

class __declspec(align(4)) BeaconManagerClass
{
public:
    /// Global VA: 0x0089C3B0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(BeaconManagerClass, Instance, 0x89C3B0)
#else
    static BeaconManagerClass& Instance;
#endif
    /// Global VA: 0x0089C474.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(SHPStruct*, BeaconArt, 0x89C474)
#else
    static SHPStruct*& BeaconArt;
#endif
    /// Global VA: 0x0089C478.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(SHPStruct*, RadarBeaconArt, 0x89C478)
#else
    static SHPStruct*& RadarBeaconArt;
#endif

    /// VA: 0x00430910
#if defined(RA2_YRPP_GAME)
    BeaconManagerClass() JMP_THIS(0x430910)
#else
    BeaconManagerClass() noexcept;
#endif

    /// VA: 0x00430930
#if defined(RA2_YRPP_GAME)
    ~BeaconManagerClass() JMP_THIS(0x430930) // just an inlined Reset
#else
    ~BeaconManagerClass() noexcept;
#endif

    /// VA: 0x00430980
#if defined(RA2_YRPP_GAME)
    void Reset() JMP_THIS(0x430980)
#else
    void Reset() noexcept;
#endif
    /// VA: 0x004309D0
#if defined(RA2_YRPP_GAME)
    void LoadArt() JMP_THIS(0x4309D0)
#else
    void LoadArt() noexcept;
    // Standalone resource retirement, after all borrowed drawing consumers
    // have stopped. This helper has no original entry; Reset retains art.
    static void ReleaseArt() noexcept;
#endif
    /// VA: 0x00430AC0.
    void Draw(Surface* pSurface, RectangleStruct bounds) JMP_THIS(0x430AC0)
    /// VA: 0x00430BA0.
    __declspec(noinline) void PlaceBeacon(int houseId, CoordStruct coord, int houseBeaconId = -1) JMP_THIS(0x430BA0)
    /// VA: 0x00430F30
#if defined(RA2_YRPP_GAME)
    bool CanPlaceBeacon(int houseId) JMP_THIS(0x430F30)
#else
    bool CanPlaceBeacon(int houseId) noexcept;
#endif
    /// VA: 0x004311C0.
    void DeleteBeacon(int houseId, int houseBeaconId) JMP_THIS(0x4311C0)
    /// VA: 0x00431410.
    void DeleteAllBeacons(int houseId) JMP_THIS(0x431410)
    /// VA: 0x00430F70
#if defined(RA2_YRPP_GAME)
    bool SelectBeacon(int X, int Y, int Z) JMP_THIS(0x430F70)
#else
    bool SelectBeacon(int x, int y, int z) noexcept;
#endif
    /// VA: 0x00431450.
    void EditBeaconMessage(wchar_t* message, int houseId, int houseBeaconId, bool unknownBool)  JMP_THIS(0x431450)

    // TODO rest of the functions

    /// VA: 0x00431700
#if defined(RA2_YRPP_GAME)
    void DrawRadar(Surface* surface, RectangleStruct bounds) JMP_THIS(0x431700)
#else
    // Null Surface submits to the borrowed modern UI frame; bounds then names
    // the absolute radar content rectangle. A Surface uses legacy coordinates.
    void DrawRadar(Surface* surface, RectangleStruct bounds) noexcept;
#endif

    // TODO rest of the functions

    BeaconClass* Beacons[8][3];
    int AllocatedCount;
    Point2D BeaconSize;
    int BeaconFrameCount;
    Point2D RadarBeaconSize;
    int RadarBeaconFrameCount;
    int RadarBeaconAnimPeriod;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(BeaconManagerClass) == 0x80, "BeaconManagerClass is the wrong size.");
#endif
