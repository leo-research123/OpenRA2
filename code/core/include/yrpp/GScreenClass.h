#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/YRPPCore.h"
#include "yrpp/Interfaces.h"
#include "yrpp/Helpers/CompileTime.h"

class DSurface;
struct RectangleStruct;
class GadgetClass;
class Surface;
class NOVTABLE GScreenClass : public IGameMap
{
public:
    // Static
    /// Global VA: 0x0087F7E8.
    static GScreenClass& Instance;
    static GadgetClass*& Buttons; // A8EF54.

    /// VA: 0x004F4780.
    static void YRPP_FASTCALL DoBlit(bool mouseCaptured, DSurface* surface, RectangleStruct* rect = nullptr)
        { JMP_STD(0x4F4780); }

    // IUnknown
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject);
    /// VA: unknown (legacy placeholder).
    virtual ULONG YRPP_STDCALL AddRef();
    /// VA: unknown (legacy placeholder).
    virtual ULONG YRPP_STDCALL Release();

    // IGameMap

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~GScreenClass();

    // GScreenClass
    /// VA: unknown (legacy placeholder).
    virtual void One_Time() RX;
    /// VA: unknown (legacy placeholder).
    virtual void Init() RX;
    /// VA: unknown (legacy placeholder).
    virtual void Init_Clear() RX;
    /// VA: unknown (legacy placeholder).
    virtual void Init_IO() RX;
    /// VA: unknown (legacy placeholder).
    virtual void GetInputAndUpdate(DWORD& outKeyCode, int& outMouseX, int& outMouseY) RX;
    /// VA: unknown (legacy placeholder).
    virtual void Update(const int& keyCode, const Point2D& mouseCoords) RX;
    /// VA: 0x004F43F0
#if defined(RA2_YRPP_GAME)
    virtual bool SetButtons(GadgetClass* pGadget) { JMP_THIS(0x4F43F0); }
#else
    virtual bool SetButtons(GadgetClass* pGadget);
#endif
    /// VA: 0x004F4410
#if defined(RA2_YRPP_GAME)
    virtual bool AddButton(GadgetClass* pGadget) { JMP_THIS(0x4F4410); }
#else
    virtual bool AddButton(GadgetClass* pGadget);
#endif
    /// VA: 0x004F4450
#if defined(RA2_YRPP_GAME)
    virtual bool RemoveButton(GadgetClass* pGadget) { JMP_THIS(0x4F4450); }
#else
    virtual bool RemoveButton(GadgetClass* pGadget);
#endif
    /// VA: 0x004F42F0
    virtual void MarkNeedsRedraw(int mode);
    /// VA: unknown (legacy placeholder).
    virtual void DrawOnTop() RX;
    /// VA: unknown (legacy placeholder).
    virtual void Draw(DWORD dwUnk) RX;
    /// VA: unknown (legacy placeholder).
    virtual void vt_entry_44() RX;
    /// VA: implementation-defined (pure virtual).
    virtual bool SetCursor(MouseCursorType idxCursor, bool miniMap) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual bool UpdateCursor(MouseCursorType idxCursor, bool miniMap) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual bool RestoreCursor() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual void UpdateCursorMinimapState(bool miniMap) = 0;

    /// VA: 0x004F4480.
    void Render();

    /// VA: 0x004F4780.
    static void YRPP_FASTCALL UpdatePrimarySurface(bool mouse_captured, Surface* surface, RectangleStruct* rect) { JMP_STD(0x4F4780); }

protected:
    // Constuctor
    GScreenClass() noexcept;

    // Properties

public:
    int ScreenShakeX;
    int ScreenShakeY;
    int Bitfield;	//default is 2
};
