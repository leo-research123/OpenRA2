#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/LinkClass.h"

enum class KeyModifier : int
{
    None = 0,
    Shift = 1,
    Ctrl = 2,
    Alt = 4
};

enum class GadgetFlag : int
{
    LeftPress = 0x1,
    LeftHeld = 0x2,
    LeftRelease = 0x4,
    LeftUp = 0x8,
    RightPress = 0x10,
    RightHeld = 0x20,
    RightRelease = 0x40,
    RightUp = 0x80,
    Keyboard = 0x100
};

MAKE_ENUM_FLAGS(GadgetFlag)

class NOVTABLE GadgetClass : public LinkClass
{
public:

    // Destructor
    /// VA: 0x004E1390.
    virtual ~GadgetClass() override ;

    // LinkClass
    /// VA: 0x004E14A0.
    virtual GadgetClass* GetNext() override ;
    /// VA: 0x004E14B0.
    virtual GadgetClass* GetPrev() override ;
    /// VA: 0x004E1480.
    virtual GadgetClass* Remove() override ;

    // GadgetClass
    /// VA: 0x004E1640.
    virtual DWORD Input() ;
    /// VA: 0x004E1570.
    virtual void DrawAll(bool bForced) ;
    /// VA: 0x004E14C0.
    virtual void DeleteList() ;
    /// VA: 0x004E1920.
    virtual GadgetClass* ExtractGadget(unsigned int nID) ;
    /// VA: 0x00488690.
    virtual void MarkListToRedraw() ;
    /// VA: 0x004E1460.
    virtual void Disable() ;
    /// VA: 0x004E1450.
    virtual void Enable() ;
    /// VA: 0x004AEBA0.
    virtual unsigned int const GetID() ;
    /// VA: 0x004E1960.
    virtual void MarkRedraw() ;
    /// VA: 0x0048E650.
    virtual void PeerToPeer(unsigned int Flags, DWORD* pKey, GadgetClass* pSendTo) ;
    /// VA: 0x004E19A0.
    virtual void SetFocus() ;
    /// VA: 0x004E19D0.
    virtual void KillFocus() ;
    /// VA: 0x004E19F0.
    virtual bool IsFocused() ;
    /// VA: 0x004E1A00.
    virtual bool IsListToRedraw() ;
    /// VA: 0x004886A0.
    virtual bool IsToRedraw() ;
    /// VA: 0x004E1A20.
    virtual void SetPosition(int X, int Y) ;
    /// VA: 0x004E1A40.
    virtual void SetDimension(int Width, int Height) ;
    /// VA: 0x004E1550.
    virtual bool Draw(bool bForced) ;
    /// VA: 0x004E1510.
    virtual void OnMouseEnter() ;
    /// VA: 0x004E1520.
    virtual void OnMouseLeave() ;
    /// VA: 0x004E1970.
    virtual void StickyProcess(GadgetFlag Flags) ;
    /// VA: 0x004E1530.
    virtual bool Action(GadgetFlag Flags, DWORD* pKey, KeyModifier Modifier) ;
    /// VA: 0x004E13F0.
    virtual bool Clicked(DWORD* pKey, GadgetFlag Flags, int X, int Y, KeyModifier Modifier) ; // Clicked On

    // Non virtual
    /// VA: 0x004B5780.
    GadgetClass& operator=(GadgetClass& another) ;
    /// VA: 0x004E15A0.
    GadgetClass* ExtractGadgetAt(int X, int Y) ;

    // Original static pointers 8B3E88..8B3E94. No object layout changes.
    static GadgetClass*& StuckOn;
    static GadgetClass*& LastList;
    static GadgetClass*& Focused;
    static GadgetClass*& Hovered;
    // Device-neutral synchronous form of 4E1640's list dispatch.
    DWORD Dispatch(DWORD key, GadgetFlag flags, int x, int y, KeyModifier modifier);
    static void ResetInput() noexcept;

    // Statics
    /// VA: 0x004E12D0.
    static int YRPP_FASTCALL GetColorScheme() { JMP_STD(0x4E12D0); }

    // Constructors
    /// VA: 0x004E12F0.
    GadgetClass(int nX,int nY,int nWidth,int nHeight,GadgetFlag eFlag, bool bSticky) noexcept
        ;

    /// VA: 0x004E1340.
    GadgetClass(GadgetClass& another) noexcept
        ;

protected:
    explicit __forceinline GadgetClass(noinit_t)  noexcept
        : LinkClass(noinit_t())
    {
    }

    // Properties
public:

    int X;
    int Y;
    int Width;
    int Height;
    bool NeedsRedraw;
    bool IsSticky;
    bool Disabled;
    GadgetFlag Flags;
};
