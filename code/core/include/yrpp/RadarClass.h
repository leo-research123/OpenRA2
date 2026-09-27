#pragma once

#include "yrpp/DisplayClass.h"
#include "yrpp/Audio.h"
#include "yrpp/HashTable.h"
#include "yrpp/GadgetClass.h"
class Surface;
class ShapeButtonClass;
struct SHPStruct;
// Original 16-byte hash record: three-word key plus Techno pointer on x86.
// 655560/655740/6558B0: object identity plus content-local pixel coordinates.
struct RadarTrackingStruct {
    TechnoClass* Object{};
    int X{};
    int Y{};
    bool operator==(const RadarTrackingStruct&) const = default;
};
class NOVTABLE RadarClass : public DisplayClass
{
public:
    // Existing nested radar gadget from EA RADAR.H, YR action 6539D0.
    class RTacticalClass : public GadgetClass {
    public:
        RTacticalClass() noexcept;
        /// VA: 0x006539D0
#if defined(RA2_YRPP_GAME)
        __declspec(noinline) bool Action(GadgetFlag flags, DWORD* key, KeyModifier modifier) override { JMP_THIS(0x6539D0); }
#else
        bool Action(GadgetFlag flags, DWORD* key, KeyModifier modifier) override;
#endif
    };
    static RTacticalClass& RadarButton; // B04A10.
    static ShapeButtonClass& DiplomacyButton; // Global VA: 0x00B04978.
    static ShapeButtonClass& OptionsButton; // Global VA: 0x00B04910.
    static SHPStruct*& DiplomacyShape; // Global VA: 0x00B048E0.
    static SHPStruct*& OptionsShape; // Global VA: 0x00B048AC.
    static SHPStruct*& RadarAnim; // Global VA: 0x00B04A38; borrowed.
    static bool& OwnsDiplomacyShape; // Global VA: 0x00B048F9.
    static bool& OwnsOptionsShape; // Global VA: 0x00B048F8.
    static RectangleStruct& DiplomacyBounds; // Global VA: 0x00B04A00.
    static RectangleStruct& OptionsBounds; // Global VA: 0x00B048C8.
    // Input in sidebar coordinates, like CellToRadar/RadarToCell.
    bool Navigate(const Point2D& sidebar_point) noexcept;
private:
    // Shared post-pick block of 0x006539D0. Does not resolve the pointer again.
    bool NavigateCell(CellStruct target) noexcept;
public:

    // Static
    /// Global VA: 0x0087F7E8.
    static RadarClass& Instance;
    // Geometry from 652CF0/652E90/654320, without art or device creation.
    void InitializeLayout() noexcept;
    void InitializeButtons() noexcept;
    /// VA: 0x00652E50
    void SetRadarButtonsVisible(bool visible);
    /// VA: 0x00653010
    void Init_IO() override;
    /// VA: 0x00654320
    void InitGUI() override;
    /// VA: 0x00653810
#if defined(RA2_YRPP_GAME)
    bool MapCell(CellStruct* cell, HouseClass* house) override { JMP_THIS(0x653810); }
#else
    bool MapCell(CellStruct* cell, HouseClass* house) override;
#endif
    /// VA: 0x00653830
#if defined(RA2_YRPP_GAME)
    bool RevealFogShroud(CellStruct* cell, HouseClass* house, bool increase) override { JMP_THIS(0x653830); }
#else
    bool RevealFogShroud(CellStruct* cell, HouseClass* house, bool increase) override;
#endif
    // Modern composition coalesces this original pixel invalidation request.
    /// VA: 0x006565A0
#if defined(RA2_YRPP_GAME)
    void RadarCell(const CellStruct& cell) { JMP_THIS(0x6565A0); }
#else
    void RadarCell(const CellStruct& cell);
#endif
    /// VA: 0x00658770
    const wchar_t* GetToolTip(UINT id) override;
    // Host event delivery for the key branch of 0x00653850. Does not advance
    // movies or run the once-per-update parent effects on each device event.
    void ProcessButtonKey(DWORD key) noexcept;
    RectangleStruct GetPanelBounds() const noexcept;
    /// VA: 0x00653100
#if defined(RA2_YRPP_GAME)
    void Draw(DWORD force) override { JMP_THIS(0x653100); }
#else
    void Draw(DWORD force) override;
#endif
    /// VA: 0x00652DE0
    void Init_Clear() override;
    /// VA: 0x00656BE0
    void ActivateRadar(bool enable, bool playSound = true) noexcept;
    /// VA: 0x00656CB0
    void SetRadarMode(int mode, bool playSound = true) noexcept;
    /// VA: 0x00656DF0
    void SetRadarAvailability(bool available) noexcept;
    // Extracted animation step from Draw, 0x006531DB..0x006532A4.
    bool AdvanceRadarAnimation() noexcept;
    /// VA: 0x00652CD0
    void QueueNextMovie() noexcept;
    /// VA: 0x00656DE0
    bool IsRadarExisting() const noexcept;
    /// VA: 0x00656E50
    bool IsPlayerNames() const noexcept;
    /// VA: 0x00656E70
    bool IsPlayingMovie() const noexcept;
    /// VA: 0x00656E90
    void RedrawRadar(bool complete) noexcept;

    // Destructor
    /// VA: 0x00652C00
    virtual ~RadarClass();

    // MapClass
    /// VA: 0x00653F50.
    virtual void CreateEmptyMap(const RectangleStruct& pMapRect, bool reuse, char nLevel, bool bUnk2) override
        { JMP_THIS(0x653F50); }

    /// VA: 0x00654490
#if defined(RA2_YRPP_GAME)
    virtual void SetVisibleRect(const RectangleStruct& mapRect) override
        { JMP_THIS(0x654490); }
#else
    void SetVisibleRect(const RectangleStruct& mapRect) override;
#endif

    // DisplayClass
    // Original in-place load reconstructs the transient vectors/controller.
    // The caller must retire their previous resources before entering Load.
    /// VA: 0x006568A0
#if defined(RA2_YRPP_GAME)
    HRESULT Load(IStream* stream) override { JMP_THIS(0x6568A0); }
#else
    HRESULT Load(IStream* stream) override;
#endif
    /// VA: 0x00656AC0
#if defined(RA2_YRPP_GAME)
    HRESULT Save(IStream* stream) override { JMP_THIS(0x656AC0); }
#else
    HRESULT Save(IStream* stream) override;
#endif

    // RadarClass
    /// VA: 0x00652D90
    virtual void DisposeOfArt();
    /// VA: 0x00653760
#if defined(RA2_YRPP_GAME)
    virtual CellStruct* vt_entry_CC(CellStruct* output, Point2D* point) { JMP_THIS(0x653760); }
#else
    virtual CellStruct* vt_entry_CC(CellStruct* output, Point2D* point);
#endif
    /// VA: 0x00653F70
#if defined(RA2_YRPP_GAME)
    virtual void vt_entry_D0(CoordStruct* coord) { JMP_THIS(0x653F70); }
#else
    virtual void vt_entry_D0(CoordStruct* coord);
#endif
    /// VA: 0x00652E90
    virtual void Init_For_House();

    // Non-virtual
    /// VA: 0x006557F0.
    Point2D* GetCrdOnRadar(Point2D *pOutBuffer, CoordStruct *pCrd, bool bRestrictToBound);

    Point2D GetCrdOnRadar(CoordStruct crd, bool bRestrictToBound = true)
    {
        Point2D output{};
        GetCrdOnRadar(&output, &crd, bRestrictToBound);
        return output;
    }

    // Terrain portions of 654490/654650/654EA0. Caller owns a loaded map,
    // mounted theater and tile catalog. RGB storage uses the original 123C
    // slot; rebuilding failure releases it. No exceptions leave these entries.
    bool BuildTerrainRadar() noexcept;
    void ReleaseTerrainRadar() noexcept;
    /// VA: 0x006558D0
    void InitRadar() noexcept;
    /// VA: 0x00655990
    void ResetRadar() noexcept;
    // The caller retires live transient resources before loading. This entry
    // only discards serialized resource addresses; it must never delete them.
    /// VA: 0x00655B20
    void PostLoadRadarFixup() noexcept;
    /// VA: 0x00654650
    void ComputeRadarImage() noexcept;
    /// VA: 0x00655A90
    void ClearRadar() noexcept;
    // Full-image replacement for the background update in 0x006547C0.
    // Owns an original Surface in the 0x1220 slot; no exceptions escape.
    bool RebuildTerrainRadarCache() noexcept;
    bool CopyTerrainRadar(WORD* pixels, unsigned count) const noexcept;
    // Modern full composition of 0x00656EC0's tactical branch.
    bool RenderRadar() noexcept;
    /// VA: 0x00653FA0
    void DrawNames() noexcept;
    bool BuildFoundationPixels() noexcept; // 6563B0: original 22 foundation masks.
    bool MarkTerrainCellDirty(const CellStruct& cell) noexcept;
    /// VA: 0x006551C0
    void RadarBackground(const CellStruct& cell) noexcept;
    bool UpdateTerrainRadar(bool& changed) noexcept; // 655250 RGB refresh.
    bool ApplyTerrainVisibility(WORD* pixels,unsigned count) const noexcept; // 655C50 terrain branch.
    // Original 655560/655740 hash membership. Objects remain owned by the world;
    // callers must untrack before destruction. Failure never escapes this boundary.
    bool TrackObject(TechnoClass* object, int x, int y) noexcept;
    bool UntrackObject(TechnoClass* object, int x, int y) noexcept;
    bool ApplyTrackedObjects(WORD* pixels, unsigned count) const noexcept; // 655C50 object branch.
    bool RadarToCell(const Point2D& point, CellStruct& cell, TechnoClass*& object) const noexcept; // 656750.
    // Pure numerical portions of 6547C0; full-image output, RGB565. Buffers are
    // borrowed for the call, must not overlap, and are measured in pixels.
    // Invalid sizes/capacities preserve output. Native radar target is 140x108.
    static bool FitTerrainRadar(int width, int height, Point2D& size, float& factor) noexcept;
    static bool ResampleTerrainRadar(const ColorStruct* source, unsigned source_count,
        int width, int height, WORD* output, unsigned output_count) noexcept;
    // 6550C0 and the terrain-only (no tracked object) branch of 656750.
    Point2D CellToRadar(const CellStruct& cell) const noexcept;
    /// VA: 0x00655050
    RectangleStruct* CellRadarRect(RectangleStruct* output, const CellStruct& cell) const noexcept;
    /// VA: 0x006550C0
    RectangleStruct* CellToRadarPixel(RectangleStruct* output, const CellStruct& cell) const noexcept;
    /// VA: 0x00653F90
    bool CellOnRadar(const CellStruct& cell) const;
    bool RadarToTerrainCell(const Point2D& point, CellStruct& output) const noexcept;
    // 656F5E..65712E, after Tactical has picked the terrain cell at viewport
    // center. Keeps the original clamp branch order, including oversized views.
    bool UpdateViewportFrame(const CellStruct& center, const Point2D& viewport) noexcept;

    /// VA: 0x006562D0.
    void RefreshCrd(Point2D* pCrd);

protected:
    // Constructor
    /// VA: 0x00652960
    RadarClass();

    // Properties

public:
    DWORD unknown_11E8;
    DWORD unknown_11EC;
    DWORD unknown_11F0;
    DWORD unknown_11F4;
    DWORD unknown_11F8;
    DWORD unknown_11FC;
    DWORD unknown_1200;
    DWORD unknown_1204;
    DWORD unknown_1208;
    RectangleStruct unknown_rect_120C;
    Surface* unknown_121C; // Owned foreground, 0x121C on x86.
    Surface* unknown_1220; // Owned terrain background, 0x1220 on x86.
    DynamicVectorClass<CellStruct> unknown_cells_1124;
    ColorStruct* unknown_123C; // Raw terrain RGB buffer, owned; x86 pointer at 123C.
    DWORD unknown_1240;
    DWORD unknown_1244;
    DWORD unknown_1248;
    DWORD unknown_124C;
    DWORD unknown_1250;
    DWORD unknown_1254;
    HashTable<RadarTrackingStruct, TechnoClass*>* unknown_1258;
    DynamicVectorClass<Point2D> unknown_points_125C;
    byte* unknown_1274; // Original owned dirty-pixel bitset, one bit per radar pixel.
    DynamicVectorClass<Point2D> FoundationTypePixels[22];
    float RadarSizeFactor;
    int unknown_int_148C;
    DWORD unknown_1490;
    DWORD unknown_1494;
    DWORD unknown_1498;
    RectangleStruct unknown_rect_149C; // Content bounds in sidebar coordinates.
    DWORD unknown_14AC;
    DWORD unknown_14B0;
    DWORD unknown_14B4;
    DWORD unknown_14B8;
    bool unknown_bool_14BC;
    bool unknown_bool_14BD;
    AudioController RadarAudio; // 14C0..14D3; 405BE0/405C00, including Unused.
    int unknown_int_14D4;
    bool IsAvailableNow;
    bool unknown_bool_14D9;
    bool unknown_bool_14DA;
    RectangleStruct unknown_rect_14DC;
    DWORD unknown_14EC;
    DWORD unknown_14F0;
    DWORD unknown_14F4;
    DWORD unknown_14F8;
    DWORD unknown_14FC;
    SysTimerClass unknown_timer_1500;
};
