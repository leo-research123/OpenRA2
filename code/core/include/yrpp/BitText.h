#pragma once

#include "yrpp/YRPPCore.h"
#include "yrpp/BitFont.h"
#include "yrpp/Helpers/CompileTime.h"

class BitFont;
class Surface;

class BitText
{
public:
    /// Global VA: 0x0089C4B8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(BitText*, Instance, 0x89C4B8)
#else
    static BitText*& Instance;
    // Native font ownership belongs to the resource session; the original
    // BitText has only its vptr and reads BitFont::Instance when drawing.
    BitText() noexcept = default;
#endif

private:
    /// VA: 0x00434AD0.
#if defined(RA2_YRPP_GAME)
    BitText() { JMP_THIS(0x434AD0); }
#endif
public:
    /// VA: unknown (legacy placeholder).
    virtual ~BitText() RX;

    // The executable takes seven stack arguments and returns the final X;
    // ECX is not a BitText instance, and BitFont is passed by pointer.
    /// VA: 0x00434B90
    static int YRPP_STDCALL Print(BitFont* font, Surface* surface, const wchar_t* text,
        int x, int y, int length, int animationPosition) noexcept
#if defined(RA2_YRPP_GAME)
        { JMP_STD(0x434B90); }
#else
        ;
#endif

    /// VA: 0x00434CD0.
    void DrawText(BitFont* pFont, Surface* pSurface, const wchar_t* pWideString, int X, int Y, int W, int H, char a8, int a9, int nColorAdjust)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x434CD0);
    }
#else
    ;
#endif
};
