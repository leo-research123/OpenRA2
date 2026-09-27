#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/BasicStructures.h"
#include "yrpp/YRMathVector.h"
#include "yrpp/YRAllocator.h"

#include "yrpp/Helpers/CompileTime.h"

using Point2D = Vector2D<int>;
namespace game { struct TypeDrawingContext; enum class DrawingStatus : std::uint32_t; }
enum class BlitterFlags : unsigned int;
enum class ZGradient : int;
enum class TextPrintType : int;
struct IDirectDrawSurface;
struct _DDSURFACEDESC2;
using DDSURFACEDESC2 = _DDSURFACEDESC2;

// Unported original text helpers retain their target-only transition. The portable declaration does not include Windows/Syringe or inline ASM.
#if defined(RA2_IMAGE_GAME) || defined(RA2_FILES_GAME) || defined(RA2_YRPP_GAME)
#include "yrpp/ASMMacros.h"
#define RA2_SURFACE_LEGACY_LINKAGE static
#define RA2_SURFACE_LEGACY_INLINE inline
#define RA2_SURFACE_LEGACY_BODY(address) { JMP_STD(address); }
#else
#define RA2_SURFACE_LEGACY_LINKAGE
#define RA2_SURFACE_LEGACY_INLINE
#define RA2_SURFACE_LEGACY_BODY(address) ;
#endif

class ConvertClass;
struct SHPStruct;

class Surface
{
public:
    /// Global VA: 0x0084310C.
    DEFINE_ARRAY_REFERENCE(bool, [16], Pattern, 0x84310C)
    /// Global VA: 0x00843128.
    DEFINE_ARRAY_REFERENCE(bool, [16], PatternLong, 0x843128)

    Surface() = default;

    virtual ~Surface();

    // Surface
    virtual bool CopyFromWhole(Surface* pSrc, bool bUnk1, bool bUnk2);

    virtual bool CopyFromPart(
        RectangleStruct* pClipRect, //ignored and retrieved again...
        Surface* pSrc,
        RectangleStruct* pSrcRect,	//desired source rect of pSrc ?
        bool bUnk1,
        bool bUnk2);

    virtual bool CopyFrom(
        RectangleStruct* pClipRect,
        RectangleStruct* pClipRect2,	//again? hmm
        Surface* pSrc,
        RectangleStruct* pDestRect,	//desired dest rect of pSrc ? (stretched? clipped?)
        RectangleStruct* pSrcRect,	//desired source rect of pSrc ?
        bool bUnk1,
        bool bUnk2);

    virtual bool FillRectEx(RectangleStruct* pClipRect, RectangleStruct* pFillRect, COLORREF nColor);

    virtual bool FillRect(RectangleStruct* pFillRect, COLORREF nColor);

    virtual bool Fill(COLORREF nColor);

    virtual bool FillRectTrans(RectangleStruct* pClipRect, ColorStruct* pColor, int nOpacity);

    virtual bool DrawEllipse(
        int XOff, int YOff, int CenterX, int CenterY, RectangleStruct Rect, COLORREF nColor);

    virtual bool SetPixel(Point2D* pPoint, COLORREF nColor);

    virtual COLORREF GetPixel(Point2D* pPoint);

    virtual bool DrawLineEx(RectangleStruct* pClipRect, Point2D* pStart, Point2D* pEnd, COLORREF nColor);

    virtual bool DrawLine(Point2D* pStart, Point2D* pEnd, COLORREF nColor);

    // The next following 4 functions are Alpha&ZData related
    virtual bool DrawLineColor(
        RectangleStruct* pRect, Point2D* pStart, Point2D* pEnd, COLORREF nColor,
        int startZ, int endZ, bool bUnk);

    virtual bool DrawMultiplyingLine(
        RectangleStruct* pRect, Point2D* pStart, Point2D* pEnd, DWORD dwMultiplier,
        DWORD dwUnk1, DWORD dwUnk2, bool bUnk);

    virtual bool DrawSubtractiveLine(
        RectangleStruct* pRect, Point2D* pStart, Point2D* pEnd, ColorStruct* pColor,
        DWORD dwUnk1, DWORD dwUnk2, bool bUnk1, bool bUnk2,
        bool bUkn3, bool bUkn4, float fUkn);

    virtual bool DrawRGBMultiplyingLine(
        RectangleStruct* pRect, Point2D* pStart, Point2D* pEnd, ColorStruct* pColor,
        float Intensity, int zSource, int zTarget);

    virtual bool PlotLine(
        RectangleStruct* pRect, Point2D* pStart, Point2D* pEnd, bool(YRPP_FASTCALL* fpDrawCallback)(int*));

    virtual bool DrawDashedLine(
        Point2D* pStart, Point2D* pEnd, int nColor, bool* Pattern, int nOffset);

    virtual bool DrawDashedLine_(
        Point2D* pStart, Point2D* pEnd, int nColor, bool* Pattern, int nOffset, bool bUkn);

    virtual bool DrawLine_(Point2D* pStart, Point2D* pEnd, int nColor, bool bUnk);

    virtual bool DrawRectEx(RectangleStruct* pClipRect, RectangleStruct* pDrawRect, int nColor);

    virtual bool DrawRect(RectangleStruct* pDrawRect, DWORD dwColor);

    virtual void* Lock(int X, int Y);

    virtual bool Unlock();

    virtual bool CanLock(DWORD dwUkn1 = 0, DWORD dwUkn2 = 0);

    virtual bool vt_entry_68(DWORD dwUnk1, DWORD dwUnk2);

    virtual bool IsLocked();

    virtual int GetBytesPerPixel();

    virtual int GetPitch();	//Bytes per scanline

    virtual RectangleStruct* GetRect(RectangleStruct* pRect);

    virtual int GetWidth();

    virtual int GetHeight();

    virtual bool IsDSurface(); // guessed - secsome

    // Helper
    RectangleStruct GetRect();

    // Properties

    int Width;
    int Height;
};

class XSurface : public Surface
{
public:
    // Shared C++ construction; compat supplies unported game drawing methods.
    XSurface(int nWidth = 640, int nHeight = 400);
    XSurface(noinit_t) : Surface {} {}
#ifdef RA2_IMAGE_GAME
    // Preserve existing EXE algorithms while using this real core object.
    // These are typed methods, not virtual-slot lookups or constructor handoffs.
    bool CopyFromWhole(Surface* src, bool a, bool b) override;
    bool CopyFromPart(RectangleStruct* clip, Surface* src, RectangleStruct* rect, bool a, bool b) override;
    bool CopyFrom(RectangleStruct* clip, RectangleStruct* clip2, Surface* src, RectangleStruct* dest, RectangleStruct* rect, bool a, bool b) override;
    bool DrawEllipse(int x, int y, int cx, int cy, RectangleStruct rect, COLORREF color) override;
    bool SetPixel(Point2D* point, COLORREF color) override;
    COLORREF GetPixel(Point2D* point) override;
    bool DrawLineEx(RectangleStruct* clip, Point2D* start, Point2D* end, COLORREF color) override;
    bool DrawLine(Point2D* start, Point2D* end, COLORREF color) override;
    bool PlotLine(RectangleStruct* rect, Point2D* start, Point2D* end, bool(YRPP_FASTCALL* callback)(int*)) override;
    bool DrawDashedLine(Point2D* start, Point2D* end, int color, bool* pattern, int offset) override;
    bool DrawRectEx(RectangleStruct* clip, RectangleStruct* rect, int color) override;
    bool DrawRect(RectangleStruct* rect, DWORD color) override;
#endif

    bool FillRectEx(RectangleStruct* clip, RectangleStruct* fill, COLORREF color) override;
    bool FillRect(RectangleStruct* fill, COLORREF color) override;
    bool Fill(COLORREF color) override;

    void* Lock(int X, int Y) override;
    bool Unlock() override;
    bool IsLocked() override;

#ifdef RA2_IMAGE_GAME
    virtual bool PutPixelClip(Point2D* pPoint, short color, RectangleStruct* pRect);
    virtual short GetPixelClip(Point2D* pPoint, RectangleStruct* pRect);
#else
    virtual bool PutPixelClip(Point2D*, short, RectangleStruct*);
    virtual short GetPixelClip(Point2D*, RectangleStruct*);
#endif

    int LockLevel;
    int BytesPerPixel;
};

class BSurface : public XSurface
{
public:
    /// Global VA: 0x00B2D928.
    DEFINE_REFERENCE(BSurface, VoxelSurface, 0xB2D928)

    BSurface();
    BSurface(int width, int height);
    // Existing EA BSurface constructor, calibrated to YR's fields. Software core
    // supports 1/2-byte pixels, nonnegative sizes fitting MemoryBuffer's int Size.
    // A supplied buffer is borrowed and must cover width * height * bytesPerPixel.
    BSurface(int width, int height, int bytesPerPixel, void* buffer = nullptr);

    void* Lock(int X, int Y) override;
    int GetBytesPerPixel() override;
    int GetPitch() override;

    // Storage/lifetime operations for original BSurface construction call sites.
    // Create owns object storage; Initialize requires aligned caller storage.
    // Destroy releases an object created by Create. Borrowed pixels stay borrowed.
    static BSurface* Create(int width, int height, int bytesPerPixel, void* buffer = nullptr);
    static BSurface* Initialize(void* storage, int width, int height, int bytesPerPixel, void* buffer);
    static void Destroy(BSurface* surface) noexcept;

    MemoryBuffer Buffer;
};

#pragma warning(push)
#pragma warning( disable : 4505) // 'function' : unreferenced local function has been removed

// The coordinate points will be modified
/// VA: 0x007BC2B0.
RA2_SURFACE_LEGACY_LINKAGE bool YRPP_FASTCALL Line_In_Bounds(Point2D* pStart, Point2D* pEnd, RectangleStruct* pBounds)
RA2_SURFACE_LEGACY_BODY(0x7BC2B0)

// Comments from thomassneddon
// Optional original SOFTWARE rasterizer, not a mandatory graphics-platform API.
void YRPP_FASTCALL CC_Draw_Shape(Surface* Surface, ConvertClass* Palette, SHPStruct* SHP, int FrameIndex,
    const Point2D* const Position, const RectangleStruct* const Bounds, BlitterFlags Flags,
    BYTE* Remap,
    int ZAdjust, // + 1 = sqrt(3.0) pixels away from screen
    ZGradient ZGradientDescIndex,
    int Brightness, // 0~2000. Final color = saturate(OriginalColor * Brightness / 1000.0f)
    int TintColor, SHPStruct* ZShape, int ZShapeFrame, int XOffset, int YOffset)
;

/// VA: 0x004A60E0.
RA2_SURFACE_LEGACY_LINKAGE Point2D* Fancy_Text_Print_Wide(const Point2D& retBuffer, const wchar_t* Text, Surface* Surface, const RectangleStruct& Bounds,
    const Point2D& Location, COLORREF ForeColor, COLORREF BackColor, TextPrintType Flag, ...)
RA2_SURFACE_LEGACY_BODY(0x4A60E0)

class ColorScheme;
/// VA: 0x004A61C0.
RA2_SURFACE_LEGACY_LINKAGE Point2D* Fancy_Text_Print_Wide(const Point2D& retBuffer, const wchar_t* Text, Surface* Surface, const RectangleStruct& Bounds,
    const Point2D& Location, ColorScheme* ForeScheme, ColorScheme* BackScheme, TextPrintType Flag, ...)
RA2_SURFACE_LEGACY_BODY(0x4A61C0)

#pragma warning(pop)

// takes a plain string; Fancy_Text_Print_Wide (0x4A60E0) is the formatting wrapper
// that calls this. A second overload at 0x4A6010 takes a ColorScheme* in place of
// the two colour dwords.
/// VA: 0x004A5EB0.
RA2_SURFACE_LEGACY_INLINE Point2D* YRPP_FASTCALL Simple_Text_Print_Wide(
    Point2D* pRetVal, const wchar_t* pText, Surface* pSurface,
    RectangleStruct* pBounds, Point2D* pLocation,
    unsigned int nForeColor, unsigned int nBackColor,
    TextPrintType nFlags, int nUnused)
        RA2_SURFACE_LEGACY_BODY(0x4A5EB0)

class DSurface : public XSurface
{
public:
    /// VA: 0x004BEAC0
    bool DrawBlendedLine(RectangleStruct* clip,Point2D* first,Point2D* last,ColorStruct* color,int opacity,int firstZ,int lastZ);
    // Native borrowed-target adapter; same primitive, no fake DSurface for
    // a GPU target. Requires an active drawing scope; no exceptions escape.
    static bool SubmitBlendedLine(const RectangleStruct& clip,Point2D first,Point2D last,ColorStruct color,int opacity,int firstZ,int lastZ) noexcept;
    DSurface(noinit_t) : XSurface { noinit_t{} } {}

    /// Global VA: 0x008872FC.
    DEFINE_REFERENCE(DSurface*, Tile, 0x8872FCu)
    /// Global VA: 0x00887300.
    DEFINE_REFERENCE(DSurface*, Sidebar, 0x887300u)
    static DSurface*& Primary; // Global VA: 0x00887308.
    /// Global VA: 0x0088730C.
    DEFINE_REFERENCE(DSurface*, Hidden, 0x88730Cu)
    static DSurface*& Alternate; // Global VA: 0x00887310.
    /// Global VA: 0x00887314.
#if defined(RA2_IMAGE_GAME) || defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DSurface*, Temp, 0x887314u)
#else
    static DSurface*& Temp;
#endif
    /// Global VA: 0x0088731C.
    DEFINE_REFERENCE(DSurface*, Composite, 0x88731Cu)

    /// Global VA: 0x00886F90.
    static RectangleStruct& SidebarBounds;
    /// Global VA: 0x00886FA0.
    static RectangleStruct& ViewBounds;
    /// Global VA: 0x00886FB0.
    static RectangleStruct& WindowBounds;

    virtual bool DrawGradientLine(RectangleStruct* pRect, Point2D* pStart, Point2D* pEnd,
        ColorStruct* pStartColor, ColorStruct* pEndColor, float* step, float* phase) { return 0; }
    // 4BF750's clipped RGB565 gradient through a generic canvas. Endpoints
    // are relative to clip origin; phase/step are in-out, as in the original.
    static game::DrawingStatus SubmitGradientLine(const game::TypeDrawingContext&,
        const RectangleStruct& clip,Point2D start,Point2D end,
        ColorStruct first,ColorStruct last,float& step,float& phase) noexcept;

    virtual bool CanBlit() { return 0; }

    // The coordinate points will be modified
    void DrawDashed(Point2D* pStart, Point2D* pEnd, int color,
        int offset, bool* pPattern = Surface::PatternLong)
    {
        if (Line_In_Bounds(pStart, pEnd, &DSurface::ViewBounds))
            this->DrawDashedLine_(pStart, pEnd, color, pPattern, offset, false);
    }

    // Comments from thomassneddon
    void DrawSHP(ConvertClass* Palette, SHPStruct* SHP, int FrameIndex,
        const Point2D* const Position, const RectangleStruct* const Bounds, BlitterFlags Flags, BYTE* Remap,
        int ZAdjust, // + 1 = sqrt(3.0) pixels away from screen
        ZGradient ZGradientDescIndex,
        int Brightness, // 0~2000. Final color = saturate(OriginalColor * Brightness / 1000.0f)
        int TintColor, SHPStruct* ZShape, int ZShapeFrame, int XOffset, int YOffset);

    void DrawText(const wchar_t* pText, RectangleStruct* pBounds, Point2D* pLocation,
        COLORREF ForeColor, COLORREF BackColor, TextPrintType Flag)
    {
        Point2D tmp = { 0, 0 };

        Fancy_Text_Print_Wide(tmp, pText, this, *pBounds, *pLocation, ForeColor, BackColor, Flag);
    }

    void DrawText(const wchar_t* pText, Point2D* pLoction, COLORREF Color)
    {
        RectangleStruct rect = { 0, 0, 0, 0 };
        this->GetRect(&rect);

        Point2D tmp { 0,0 };
        Fancy_Text_Print_Wide(tmp, pText, this, rect, *pLoction, Color, 0, static_cast<TextPrintType>(0x10));
    }

    void DrawText(const wchar_t* pText, int X, int Y, COLORREF Color)
    {
        Point2D P = { X ,Y };
        DrawText(pText, &P, Color);
    }

    void* Buffer;
    bool IsAllocated;
    bool IsInVideoRam;
protected:
    char field_1A[2];
public:
    IDirectDrawSurface* VideoSurfacePtr;
    DDSURFACEDESC2* VideoSurfaceDescription;
};

#undef RA2_SURFACE_LEGACY_LINKAGE
#undef RA2_SURFACE_LEGACY_INLINE
#undef RA2_SURFACE_LEGACY_BODY
