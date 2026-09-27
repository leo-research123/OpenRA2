#pragma once

#include "yrpp/GadgetClass.h"

// Original construction installs its EXE vtable; native construction must let
// the compiler install the actual TextLabel vtable (including IsFocused/Draw).
#if defined(RA2_YRPP_GAME)
class NOVTABLE TextLabelClass : public GadgetClass
#else
class TextLabelClass : public GadgetClass
#endif
{
public:
    /// VA: 0x0072A4A0
    bool Draw(bool forced) override
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x72A4A0); }
#else
        ;
#endif
    // Non virtual

    // Statics

    // Constructors
    /// VA: 0x0072A440.
#if defined(RA2_YRPP_GAME)
    TextLabelClass(wchar_t* pText, int X, int Y, int ColorSchemeIndex, TextPrintType style) noexcept
        : GadgetClass(noinit_t()) { JMP_THIS(0x72A440); }
#else
    TextLabelClass(wchar_t* text, int x, int y, int colorSchemeIndex, TextPrintType style) noexcept;
#endif
protected:
    explicit __forceinline TextLabelClass(noinit_t)  noexcept
        : GadgetClass(noinit_t())
    {
    }

    // Properties
public:

    void* UserData1;
    void* UserData2;
    DWORD Style;
    wchar_t* Text;
    int ColorSchemeIndex;
    DWORD PixWidth;
    DWORD anim_dword3C;
    bool SkipDraw;
    bool Animate;
    DWORD AnimPos;
    DWORD AnimTiming;
};
