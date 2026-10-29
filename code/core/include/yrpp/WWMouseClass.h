#pragma once

#include "yrpp/MouseClass.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/GeneralStructures.h"
#include "yrpp/Drawing.h"
#include "yrpp/Helpers/CompileTime.h"

struct SHPStruct;

class WWMouseClass {
public:
    /// Global VA: 0x00887640.
    static WWMouseClass*& Instance;

    static void PrepareScreen() {
        WWMouseClass::Instance->HideCursor();

        DSurface::Hidden->Fill(0);
        GScreenClass::DoBlit(true, DSurface::Hidden);
        DSurface::Temp = DSurface::Hidden;

        WWMouseClass::Instance->ShowCursor();

        MouseClass::Instance.SetCursor(MouseCursorType::NoMove, false);
        MouseClass::Instance.RestoreCursor();

        TabClass::Instance.Activate(1);
        MouseClass::Instance.RedrawSidebar(0);
    }

    /// VA: unknown (legacy zero-address placeholder).
#if defined(RA2_YRPP_GAME)
    virtual ~WWMouseClass() { JMP_THIS(0x0); }
#else
    virtual ~WWMouseClass() = default;
#endif

    /// VA: 0x007B8A00.
#if defined(RA2_YRPP_GAME)
    virtual void Draw(Point2D const& coords, SHPStruct const* pImage, int idxFrame) { JMP_THIS(0x7B8A00); }
#else
    virtual void Draw(Point2D const& coords, SHPStruct const* pImage, int idxFrame);
#endif

    /// VA: 0x007BA320.
#if defined(RA2_YRPP_GAME)
    virtual bool IsRefCountNegative() { JMP_THIS(0x7BA320); }
#else
    virtual bool IsRefCountNegative();
#endif

    /// VA: 0x007B9930.
#if defined(RA2_YRPP_GAME)
    virtual void HideCursor() { JMP_THIS(0x7B9930); }
#else
    virtual void HideCursor();
#endif

    /// VA: 0x007B9750.
#if defined(RA2_YRPP_GAME)
    virtual void ShowCursor() { JMP_THIS(0x7B9750); }
#else
    virtual void ShowCursor();
#endif

    /// VA: 0x007B9C30.
#if defined(RA2_YRPP_GAME)
    virtual void ReleaseMouse() { JMP_THIS(0x7B9C30); }
#else
    virtual void ReleaseMouse();
#endif

    /// VA: 0x007B9A60.
#if defined(RA2_YRPP_GAME)
    virtual void CaptureMouse() { JMP_THIS(0x7B9A60); }
#else
    virtual void CaptureMouse();
#endif

    // Non-zero while the game owns (draws) the software mouse cursor. The engine's
    // in-game dialogs release the mouse on open and re-capture it on close only
    // when this is set (e.g. Wait_For_Players @0x648C7C).
    /// VA: 0x007BA330.
#if defined(RA2_YRPP_GAME)
    virtual byte IsCaptured() { JMP_THIS(0x7BA330); }
#else
    virtual byte IsCaptured();
#endif

    /// VA: 0x007B9D70.
#if defined(RA2_YRPP_GAME)
    virtual void func_20(RectangleStruct Useless) { JMP_THIS(0x7B9D70); }
#else
    virtual void func_20(RectangleStruct Useless);
#endif

    /// VA: 0x007B9D80.
#if defined(RA2_YRPP_GAME)
    virtual void CallFunc10() { JMP_THIS(0x7B9D80); }
#else
    virtual void CallFunc10();
#endif

    /// VA: 0x007B89F0.
#if defined(RA2_YRPP_GAME)
    virtual DWORD GetRefCount() { JMP_THIS(0x7B89F0); }
#else
    virtual DWORD GetRefCount();
#endif

    /// VA: 0x007BA340.
#if defined(RA2_YRPP_GAME)
    virtual int GetX() { JMP_THIS(0x7BA340); }
#else
    virtual int GetX();
#endif

    /// VA: 0x007BA350.
#if defined(RA2_YRPP_GAME)
    virtual int GetY() { JMP_THIS(0x7BA350); }
#else
    virtual int GetY();
#endif

    /// VA: 0x007BA360.
#if defined(RA2_YRPP_GAME)
    virtual Point2D* GetCoords(Point2D* buffer) { JMP_THIS(0x7BA360); }
#else
    virtual Point2D* GetCoords(Point2D* buffer);
#endif

    /// VA: 0x007BA380.
#if defined(RA2_YRPP_GAME)
    virtual void SetCoords(Point2D buffer) { JMP_THIS(0x7BA380); }
#else
    virtual void SetCoords(Point2D buffer);
#endif

    /// VA: 0x007B90C0.
#if defined(RA2_YRPP_GAME)
    virtual void func_3C(DSurface* pSurface, bool bUnk) { JMP_THIS(0x7B90C0); }
#else
    virtual void func_3C(DSurface* pSurface, bool bUnk);
#endif

    /// VA: 0x007B92D0.
#if defined(RA2_YRPP_GAME)
    virtual void func_40(DSurface* pSurface, bool bUnk) { JMP_THIS(0x7B92D0); }
#else
    virtual void func_40(DSurface* pSurface, bool bUnk);
#endif

    /// VA: 0x007B9D90.
#if defined(RA2_YRPP_GAME)
    virtual void func_44(int* arg1, int* arg2) { JMP_THIS(0x7B9D90); }
#else
    virtual void func_44(int* arg1, int* arg2);
#endif

public:
    SHPStruct* Image;
    int ImageFrameIndex;
    int RefCount;
    bool field_10;
    byte field_11;
    byte field_12;
    byte field_13;
    Point2D field_14;
    Point2D XY1;
    DSurface* Surface;
    HWND hWnd;
    RectangleStruct Rect0;
    Point2D XY2;
    BSurface* field_44;
    RectangleStruct Rect1;
    BSurface* field_58;
    RectangleStruct Rect2;
    BSurface* field_6C;
    RectangleStruct Rect3;
    RectangleStruct Rect4;
    DWORD field_90;
    DWORD field_94;
};
