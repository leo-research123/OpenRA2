#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectTypeClass.h"
#include "api/type_drawing.hpp"
#include <cstddef>
#include <cstdint>

// XCC 70358b46858973426c1ecf204485cb2a88716217, tmp_ts_file /
// cc_structures.h; calibrated to YR 547020. Disk directory entries are uint32
// offsets. Loaded directories contain native pointers, including on 64-bit.
#pragma pack(push, 1)
struct TMPImage {
    std::int32_t X, Y, ExtraOffset, ZOffset, ExtraZOffset;
    std::int32_t ExtraX, ExtraY, ExtraWidth, ExtraHeight;
    std::uint32_t Flags;
    std::int8_t Height, TerrainType, RampType;
    byte RadarLeft[3], RadarRight[3];
    std::int8_t Padding[3];
    const byte* Pixels() const { return reinterpret_cast<const byte*>(this + 1); }
    const byte* At(std::int32_t offset) const { return reinterpret_cast<const byte*>(this) + offset; }
};
#pragma pack(pop)
static_assert(sizeof(TMPImage) == 52 && offsetof(TMPImage, RadarLeft) == 43);
struct TMPStruct {
    std::int32_t Columns, Rows, Width, Height;
    // False for an empty/out-of-range slot; output is always cleared on failure.
    bool GetSubTile(int index, const TMPImage*& output) const;
};
static_assert(sizeof(TMPStruct) == 16);

// Native construction must emit this concrete class's vptrs on MSVC too.
class IsometricTileTypeClass : public ObjectTypeClass
{
public:
    static const AbstractType AbsID = AbstractType::IsotileType;

    // Array
    static DynamicVectorClass<IsometricTileTypeClass*>& Array;
    static DynamicVectorClass<AbstractClass*>& AllTypes; // B0F670, includes variants
    // Optional world notification, before releasing this type and its variants.
    // Resource-only hosts need none; a game host owns world pointer expiration.
    static void (*PointerExpirationObserver)(AbstractClass*) noexcept;
    // Original nested record (RTTI at 543E8D), vector at AA1078.
    struct TileInsertType { int OriginalIndex; int Offset; };
    static DynamicVectorClass<TileInsertType*>& TileInsertions;
    static int YRPP_FASTCALL ConvertTileIndex(int originalIndex); // 544E30
    // Catalog portion of 545150; does not initialize rendering/light buffers.
    // Caller owns the global tile registry and has stopped all tile consumers.
    // On failure the tile catalog is empty. Normal class layout is unchanged.
    static bool LoadTileSetCatalog(CCINIClass& ini, TheaterType theater) noexcept;
    static void ClearTileSetCatalog() noexcept;
    // 546DA0 used-variant counting and image residency. Host status replaces
    // the original loading-screen tick; false means a required image is absent.
    static bool LoadMapImages(bool loadAll, bool forRandomMap) noexcept;
    static int (&TileSetStarts)[256]; // AA1140, including the terminal marker
    static int& TileSetCount; // ABC558, includes the terminal marker
    static int (&ShadowTileSets)[5]; // AA102C
    static int& RampBase; // ABC1D8
    static int& RampSmooth; // AA1058
    static int& MMRampBase; // AA109C
    static int& ClearTile; // AA10B0
    static int& RoughTile; // ABC2B8
    static int& SandTile; // ABB104
    static int& GreenTile; // AA0E18
    static int& PaveTile; // ABC2B0
    static int& MiscPaveTile; // AA10A4
    static int& ClearToRoughLat; // AA1134
    static int& ClearToSandLat; // AA0E24
    static int& ClearToGreenLat; // AA0748
    static int& ClearToPaveLat; // AA10A8
    static int& HeightBase; // AA0744
    static int& BlackTile; // ABC2CC
    static int& BridgeSet; // AA0E28
    static int& WoodBridgeSet; // ABAD1C
    static int& CliffSet; // AA1020
    static int& ShorePieces; // ABAD28
    static int& WaterSet; // AA0738
    static int& SlopeSetPieces; // ABC1F8
    static int& SlopeSetPieces2; // AA1098
    static int& MonorailSlopes; // AA1024
    static int& DirtTunnels; // AA10B4
    static int& DirtTrackTunnels; // ABAD2C
    static int& Tunnels; // AA1054
    static int& TrackTunnels; // ABB108
    static int& WaterfallEast; // AA073C
    static int& WaterfallWest; // ABB110
    static int& WaterfallNorth; // AA10A0
    static int& WaterfallSouth; // AA1050
    static int& CliffRamps; // ABBEBC
    static int& PavedRoads; // ABBEC8
    static int& PavedRoadEnds; // ABBEC4
    static int& Medians; // AA0E20
    static int& RoughGround; // AA0E1C
    static int& DirtRoadJunction; // ABAD20
    static int& DirtRoadCurve; // ABC1D4
    static int& DirtRoadStraight; // AA10AC
    static int& DestroyableCliffs; // ABC2C8
    static int& WaterCliffs; // AA101C
    static int& WaterCaves; // ABAD24
    static int& PavedRoadSlopes; // AA1094
    static int& DirtRoadSlopes; // ABBEC0
    static int& Rocks; // ABB10C
    static int& WaterBridge; // AA1090
    static int& BridgeTopLeft1; // ABC2B4
    static int& BridgeTopLeft2; // AA1130
    static int& BridgeBottomRight1; // ABC1E8
    static int& BridgeBottomRight2; // AA0E38
    static int& BridgeTopRight1; // AA1548
    static int& BridgeTopRight2; // AA0740
    static int& BridgeBottomLeft1; // ABC1D0
    static int& BridgeBottomLeft2; // AA1540
    static int& BridgeMiddle1; // ABAD30
    static int& BridgeMiddle2; // AA1028

    // IPersist
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    /// VA: 0x00549C80; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x00549C80); }
    /// VA: 0x00549D70; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x00549D70); }

    // AbstractClass
    /// VA: 0x00549DD0; local body in core/src/yrpp.
    virtual void PointerExpired(AbstractClass* object, bool removed) override;
virtual AbstractType WhatAmI() const override;
    virtual int Size() const override;
    /// VA: 0x00549B70; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x00549B70); }

    virtual int GetArrayIndex() const override;

    // ObjectTypeClass
    /// VA: 0x00549B50; reference retained, not a local implementation.
    virtual CoordStruct* vt_entry_6C(CoordStruct* pDest, CoordStruct* pSrc) const override { JMP_THIS(0x00549B50); }

    /// VA: 0x00549AA0; reference retained, not a local implementation.
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords, HouseClass* pOwner) override { JMP_THIS(0x00549AA0); }

    /// VA: 0x00549AE0; reference retained, not a local implementation.
    virtual ObjectClass* CreateObject(HouseClass* pOwner) override { JMP_THIS(0x00549AE0); }
    /// VA: unknown (legacy placeholder).
    virtual void vt_entry_90(DWORD dwUnk) RX;

    virtual SHPStruct* GetImage() const override; // 544CB0; actually a loaded TMPStruct
    int LoadTMP(); // 547020; FileName[14], bytes read, 0 on failure
    // Disk bytes only. Validates all offsets before relocation; failure preserves
    // the previous resource. Image and radar tables stay owned by the original class.
    // Borrows size readable bytes for this call only; never modifies or retains
    // the input. Null data or an incomplete header returns false.
    bool ReadTMP(const byte* data, size_t size);
    void UnloadTMP() noexcept;
    // Original TMP queries 544BE0 / 5471B0 / 547150. Negative indices and
    // malformed/missing resources use the existing zero/failure result.
    LandType GetLandType(int subTile) const noexcept;
    int GetSlopeIndex(int subTile) const noexcept;
    bool GetTileDimensions(int subTile, int& width, int& height) const noexcept;
    /// VA: 0x00544C20
    bool IsTileIndexValid(int subTile, bool reload);
    /// VA: 0x00547370
    const CellStruct* ShadowCasterList() const;
    /// VA: 0x00547230
    void DrawShadowCaster(int subTile,Surface*,Point2D point,RectangleStruct clip,int depth);
    bool UpdateRadarColors(); // 549E90, 13 pairs per nonempty sub-tile
    // 547CF0 legacy signature. A bound TypeDrawingContext selects the backend;
    // without one this retains the original JUMP (unavailable on native hosts).
    void DrawTMP(ConvertClass*, int subTile, Surface*, int x, int y,
        RectangleStruct clip, int level, int intensity, bool useZ, int alternate,
        bool flat, bool flag16, bool flag17, int color);
    game::DrawingStatus DrawTMP(const game::TypeDrawingContext&, int subTile,
        int x, int y, RectangleStruct clip, int level, int intensity, bool useZ,
        int alternate, bool flat, bool flag16, bool flag17, int color) noexcept;
    /// VA: 0x00547CF0; explicit unresolved/original fallback.
    void DrawTMPOriginal(ConvertClass*, int subTile, Surface*, int x, int y,
        RectangleStruct clip, int level, int intensity, bool useZ, int alternate,
        bool flat, bool flag16, bool flag17, int color) { JMP_THIS(0x00547CF0); }

    // Destructor
    virtual ~IsometricTileTypeClass();

    // Constructor
    IsometricTileTypeClass(int ArrayIndex, int Minus65, int Zero1,
        const char* pName, int Zero2) noexcept;

protected:
    explicit __forceinline IsometricTileTypeClass(noinit_t) noexcept
        : ObjectTypeClass(noinit_t())
    { }

    // Properties

public:
    int ArrayIndex;
    int MarbleMadnessTile;
    int NonMarbleMadnessTile;
    DWORD unk_2A0;
    DynamicVectorClass<Color16Struct*> unk_2A4;
    IsometricTileTypeClass* NextVariant = nullptr; // 2BC, owning variant chain
    int ToSnowTheater;
    int ToTemperateTheater;
    int TileAnimIndex; //Tile%02dAnim, actually an AnimTypeClass array index...
    int TileXOffset; //Tile%02dXOffset
    int TileYOffset; //Tile%02dYOffset
    int TileAttachesTo; //Tile%02dAttachesTo, iso tile index?
    int TileZAdjust; //Tile%02dZAdjust
    DWORD unk_2DC; //0xBF
    bool Morphable;
    bool ShadowCaster;
    bool AllowToPlace; //default true
    bool RequiredByRMG;
    DWORD unk_2E4;
    DWORD unk_2E8;
    DWORD unk_2EC;
    int unk_2F0; //default 1, no idea
    bool unk_2F4; //like always true
    char FileName[0xE]; // WARNING! Westwood strncpy's 0xE bytes into this buffer without NULL terminating it.
    BYTE unused_303[2]; // target AllowBurrowing is at 305, not 303
    bool AllowBurrowing;
    bool AllowTiberium;
    DWORD unk_308;
};
