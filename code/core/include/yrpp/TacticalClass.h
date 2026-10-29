#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/GeneralStructures.h"
#include "yrpp/AbstractClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/Timer.h"
#include "yrpp/Matrix3D.h"
#include "yrpp/Helpers/CompileTime.h"

class ObjectClass;
class DSurface;
class CellClass;

struct TacticalSelectableStruct {
    ObjectClass* Object;
    int X;
    int Y;
};

class TacticalClass : public AbstractClass {
public:
    /// VA: 0x006DBE20
#if defined(RA2_YRPP_GAME)
    void DrawRadialIndicators() { JMP_THIS(0x006DBE20); }
#else
    void DrawRadialIndicators();
#endif

    // Original entries needed by map display; reference names describe behavior.
    // Map/layer drawing entry cited by the full display implementation plan.
    /// VA: 0x006D7560.

    static TacticalClass*& Instance;  // Global VA: 0x00887324; construction lives in core.
    static RectangleStruct& ViewBounds;
    bool FocusView(const Point2D& center) noexcept;  // Original B0CE28..B0CE34.
    /// VA: 0x006DA230
#if defined(RA2_YRPP_GAME)
    int GetScrollDirection(int direction) { JMP_THIS(0x6DA230); }
#else
    int GetScrollDirection(int direction);
#endif
    /// VA: 0x006D8530
#if defined(RA2_YRPP_GAME)
    void ScrollBy(int direction, int distance) { JMP_THIS(0x6D8530); }
#else
    void ScrollBy(int direction, int distance);
#endif

    // Construction with the startup camera globals of the original constructor.
    // The inputs are the calibrated results of 0x006D1830/0x006D18A0/0x006D18C0.
    /// VA: 0x006D1C20.
    TacticalClass();
    // Core construction takes the existing camera globals explicitly. It never
    // invokes an EXE constructor, including the AbstractClass base construction.
    /// VA: unknown.
    TacticalClass(const Point2D& initialPoint, const RectangleStruct& initialBounds, float sinX, float cosX, float sinZ,
                  float cosZ, float scale);
    /// VA: unknown.
    ~TacticalClass() override;
    /// VA: unknown.
    HRESULT YRPP_STDCALL QueryInterface(REFIID, void**) override;
    /// VA: unknown.
    ULONG YRPP_STDCALL AddRef() override;
    /// VA: unknown.
    ULONG YRPP_STDCALL Release() override;
    /// VA: unknown.
    HRESULT YRPP_STDCALL GetClassID(CLSID*) override;
    /// VA: unknown.
    HRESULT YRPP_STDCALL Load(IStream*) override;
    /// VA: unknown.
    HRESULT YRPP_STDCALL Save(IStream*, BOOL) override;
    /// VA: unknown.
    void YRPP_STDCALL Create_ID() override;
    /// VA: unknown.
    void PointerExpired(AbstractClass*, bool) override;
    /// VA: unknown.
    AbstractType WhatAmI() const override;
    /// VA: unknown.
    int Size() const override;
    /// VA: unknown.
    void ComputeCRC(CRCEngine&) const override;
    /// VA: unknown.
    int GetOwningHouseIndex() const override;
    /// VA: unknown.
    CoordStruct* GetCoords(CoordStruct*) const override;
    /// VA: unknown.
    CoordStruct* GetDestination(CoordStruct*, TechnoClass* = nullptr) const override;
    /// VA: unknown.
    CoordStruct* GetCenterCoords(CoordStruct*) const override;
    /// VA: 0x6D2540
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x6D2540); }
#else
    void Update() override;
#endif
    // Draws a Z-tested line between two world coordinates; always succeeds.
    /// VA: 0x006DBB60.
    virtual bool sub_6DBB60(CoordStruct const&, CoordStruct const&, COLORREF, DWORD);

    /// VA: 0x006D6070
#if defined(RA2_YRPP_GAME)
    void SetTacticalPosition(CoordStruct* pCoord) JMP_THIS(0x6D6070);
#else
    void SetTacticalPosition(CoordStruct* coord);
#endif

    /// VA: 0x006D6590.
    CellStruct* CoordsToCell(CellStruct* pDest, CoordStruct* pSource) JMP_THIS(0x6D6590);

    /// VA: 0x006D6410
#if defined(RA2_YRPP_GAME)
    static CellStruct* YRPP_STDCALL AdjustCellForHeight(CellStruct* out, const CoordStruct* at) { JMP_STD(0x6D6410); }
#else
    static CellStruct* YRPP_STDCALL AdjustCellForHeight(CellStruct* out, const CoordStruct* at);
#endif

    // 6D6590's ground/bridge scan with a correctly typed screen input. The
    // legacy declaration above has the wrong source type. Host-domain failures
    // leave output unchanged. No exceptions propagate to the host.
    /// VA: 0x006D6590
    bool PickTerrainCell(const Point2D& point, const RectangleStruct& viewport, CellStruct& output) noexcept;

    /// VA: 0x006D2140.
    // Uses the bound original/host viewport. Missing viewport or null pointers
    // return false without changing output; otherwise false means not visible.
    bool CoordsToClient(CoordStruct const* coords, Point2D* pOutClient) const;

    // Same 6D2140 projection/culling with explicit borrowed camera and bounds.
    // Always writes the projected point; the result is its visibility. Bounds
    // X/Y do not participate in this original entry's expanded culling test.
    static bool CoordsToClient(const CoordStruct& coords, const Point2D& camera, const RectangleStruct& bounds,
                               Point2D& output) noexcept;

    // Header-only convenience wrapper; keep the existing STL return type out of new binary interfaces.
    // Returns the projected point and whether it is currently visible.
    /// VA: unknown.
    std::pair<Point2D, bool> CoordsToClient(const CoordStruct& coords) const {
        Point2D point{};
        const bool visible = CoordsToClient(&coords, &point);
        return std::make_pair(point, visible);
    }

    /// VA: 0x006D1F10.
    Point2D* CoordsToScreen(Point2D* pDest, const CoordStruct* pSource);

    /// VA: unknown.
    static Point2D CoordsToScreen(const CoordStruct& coord) noexcept;
    /// VA: 0x006D8DB0
    void DrawObjects(bool forced);
    /// VA: 0x006D7560
    void DrawTiles(const RectangleStruct& area, const RectangleStruct& clip);
    /// VA: 0x006D6D10
    void DrawOverlays(const RectangleStruct& area);
    /// VA: 0x006D71E0
    void DrawShroud(const RectangleStruct& area);
    /// VA: 0x006D7C00
    void DrawTileShadows(const RectangleStruct& area, const RectangleStruct& clip);
    /// VA: 0x006D97D0
    static void YRPP_STDCALL DrawTerrain(bool forced, const RectangleStruct& area, const RectangleStruct& clip);
    /// VA: 0x006D9920
    static void YRPP_STDCALL DrawBuildings(bool forced, const RectangleStruct& area, const RectangleStruct& clip);
    /// VA: 0x6D5030
#if defined(RA2_YRPP_GAME)
    void DrawPlacement(bool foreground) { JMP_THIS(0x6D5030); }
#else
    void DrawPlacement(bool foreground);
#endif
    /// VA: 0x006D5C50
    void DrawWallPlacement(bool foreground, CellStruct center)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6D5C50);
    }
#else
        ;
#endif
    // 6D8640 camera-center limits, with validated original map/viewport inputs
    // (map coordinate range <=512, viewport axes 1..8192).
    static void CameraCenterBounds(int map_width, const RectangleStruct& visible, const RectangleStruct& viewport,
                                   Point2D& minimum, Point2D& maximum) noexcept;
    static bool ClampCameraCenter(Point2D& center, const Point2D& minimum, const Point2D& maximum) noexcept;
    // Center publication part of 6D6000/6D8B30. Caller performs boundary policy.
    void SetViewCenter(const Point2D& center, const RectangleStruct& viewport) noexcept;
    // 6D62E0: fixed map-preview projection, independent of camera and height.
    // Explicit coordinates preserve WriteINI's two virtual GetCoords calls.
    /// VA: unknown.
    static Point2D CoordsToMapPixel(int x, int y);

    /// VA: 0x006D2280
#if defined(RA2_YRPP_GAME)
    CoordStruct* ClientToCoords(CoordStruct* pOutBuffer, Point2D const& client) const JMP_THIS(0x6D2280);
#else
    CoordStruct* ClientToCoords(CoordStruct* output, Point2D const& client) const;
#endif

    /// VA: unknown.
    CoordStruct ClientToCoords(Point2D const& client) const;

    /// VA: 0x006D8700.
#if defined(RA2_YRPP_GAME)
    char GetOcclusion(const CellStruct& cell, bool fog) const JMP_THIS(0x6D8700);
#else
    char GetOcclusion(const CellStruct& cell, bool fog) const;
#endif

    /*
    [[deprecated]] // inlined in game
    Point2D * AdjustForZShapeMove(Point2D* pDest, Point2D* pClient)
        JMP_THIS(0x6D1FE0);
    */

    // wrong name
    /// VA: unknown.
    static Point2D AdjustForZShapeMove(int x, int y) noexcept;

    // in-game height to on-screen height
    // Also inlined in the game.
    /// VA: 0x006D20E0
    static int YRPP_FASTCALL AdjustForZ(int Height) noexcept;
    /// VA: 0x006D2120
    static int YRPP_FASTCALL PixelToZ(int pixels) noexcept;

    /// VA: 0x006D2420.
#if defined(RA2_YRPP_GAME)
    void FocusOn(CoordStruct* pDest, int Velocity) JMP_THIS(0x6D2420);
#else
    void FocusOn(CoordStruct* destination, int velocity);
#endif

    // called when area needs to be marked for redrawing due to external factors
    // - alpha lights, terrain changes like cliff destruction, etc
    /// VA: 0x006D2790.
#if defined(RA2_YRPP_GAME)
    void RegisterDirtyArea(RectangleStruct Area, bool refreshShroud) JMP_THIS(0x6D2790);
#else
    void RegisterDirtyArea(RectangleStruct Area, bool refreshShroud);
#endif

    /// VA: 0x006DA7D0.
#if defined(RA2_YRPP_GAME)
    void RegisterCellAsVisible(CellClass* pCell) JMP_THIS(0x6DA7D0)
#else
    void RegisterCellAsVisible(CellClass* pCell);
#endif

    // The original callers ignore the incidental EAX result.
    /// VA: 0x006D4B50
#if defined(RA2_YRPP_GAME)
    static int YRPP_FASTCALL DrawTimer(int index, ColorScheme* scheme, int seconds, wchar_t* text,
                                     LARGE_INTEGER* blinkTimer, bool* blinkState) {
#if defined(__clang__) && defined(_M_IX86)
        // Keep the epilogue and jump indivisible: clang-cl /Od can insert
        // return-value spills between the separate JMP_STD assembly blocks.
        __asm {
            mov esp, ebp
            pop ebp
            mov eax, 0x6D4B50
            jmp eax
        }
#else
        JMP_STD(0x6D4B50);
#endif
    }
#else
    static int YRPP_FASTCALL DrawTimer(int index, ColorScheme* scheme, int seconds, wchar_t* text,
                                     LARGE_INTEGER* blinkTimer, bool* blinkState);
#endif

    /// VA: 0x006D9EF0
#if defined(RA2_YRPP_GAME)
    bool AddSelectable(ObjectClass* object, int x, int y) JMP_THIS(0x6D9EF0);
#else
    bool AddSelectable(ObjectClass* object, int x, int y);
#endif
    /// VA: 0x006D9CE0
    void AddBuildingsToSelectables(RectangleStruct bounds);
    // Screen point relative to the tactical viewport; no pixel-mask query.
    /// VA: 0x006DA380
    ObjectClass* GetSelectableObject(const Point2D& point);
    // Original draw candidate storage is fixed at 0x00B0CEC8. Standalone builds
    // share the control capacity with the player-order queues.
#if defined(RA2_YRPP_GAME)
    enum { SELECTABLE_CAPACITY = 500 };
#else
    enum { SELECTABLE_CAPACITY = RA2_CONTROL_CAPACITY };
#endif
    static TacticalSelectableStruct (&SelectableObjects)[SELECTABLE_CAPACITY];
    // Callback runs synchronously and must not propagate exceptions.
    using SelectionCallback = void(YRPP_FASTCALL*)(ObjectClass*) noexcept;
    /// VA: 0x006D9F80
    void StartRubberBand(const Point2D& point);
    /// VA: 0x006D9FC0
    void ModifyRubberBand(const Point2D& point);
    /// VA: 0x006D9FF0
    void SelectRubberBand(SelectionCallback callback);
    /// VA: 0x006DA080
    bool HasBandObjects() const;
    /// VA: 0x006DA160
    void EndRubberBand();
    /// VA: 0x006DA180
    void DrawRubberBand();
    /// VA: 0x006DA9D0
#if defined(RA2_YRPP_GAME)
    void DrawRallyPoints(bool foreground) { JMP_THIS(0x6DA9D0); }
#else
    void DrawRallyPoints(bool foreground) noexcept;
#endif
    /// VA: 0x006DA5C0
    void SelectThese(const RectangleStruct& rect, SelectionCallback callback);

    /// VA: 0x0070D150.
#if defined(RA2_YRPP_GAME)
    static void StartDrawActionLineTimer() JMP_STD(0x70D150);
#else
    static void StartDrawActionLineTimer();
#endif

    // Original caller 0x004F4480: 0x0 prepares/pans, 0x1 draws background,
    // 0x2 draws foreground; 0x3 is the combined path. The native host supports
    // full-target background (0x1) and combined (0x3) rendering. Cached modes
    // 0x0 and 0x2 are not implemented.
    // Native failures are recorded in that binding; no exception crosses out.
    /// VA: 0x006D3D10
#if defined(RA2_YRPP_GAME)
    void Render(DSurface* pSurface, bool flag, int eMode) JMP_THIS(0x6D3D10);
#else
    void Render(DSurface* pSurface, bool flag, int eMode);
#endif

    /// VA: 0x006D2070.
    [[deprecated]]
    Point2D* ApplyMatrix_Pixel(Point2D* coords, Point2D* offset) JMP_THIS(0x6D2070);

    /// VA: unknown.
    Point2D ApplyMatrix_Pixel(const Point2D& offset);
    /// VA: 0x006D2360
    CoordStruct* PixelToCoordsAbsolute(CoordStruct* output, const Point2D& pixel);

public:
    wchar_t ScreenText[64];
    int EndGameGraphicsFrame;
    int LastAIFrame;
    bool field_AC;
    bool field_AD;
    PROTECTED_PROPERTY(char, gap_AE[2]);
    Point2D TacticalPos;
    Point2D LastTacticalPos;
    double ZoomInFactor;
    Point2D Point_C8;
    Point2D Point_D0;
    float field_D8;
    float field_DC;
    int VisibleCellCount;
    CellClass* VisibleCells[800];
    Point2D TacticalCoord1;
    DWORD field_D6C;
    DWORD field_D70;
    Point2D TacticalCoord2;
    bool field_D7C;
    bool Redrawing;  // set while redrawing - cheap mutex // TacticalPosUpdated
    PROTECTED_PROPERTY(char, gap_D7E[2]);
    RectangleStruct ContainingMapCoords;
    LTRBStruct Band;
    DWORD MouseFrameIndex;
    SysTimerClass StartTime;  // 6D1D35 starts the 16-ms system clock, not frame time.
    int SelectableCount;
    Matrix3D Unused_Matrix3D;
    Matrix3D IsoTransformMatrix;
    DWORD field_E14;
};
