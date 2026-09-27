// Fixed YRpp 9402d7da Blitters/Blitter.h declarations.
#pragma once

#include "yrpp/platform/ABI.h"
using byte = unsigned char;
class AlphaLightingRemapClass;

class Blitter {
public:
    // Incomplete nested algorithm helper; defined only in the implementation.
    // No state, base-class relationship, virtual slot or callable public API.
    struct Kernel;
    virtual ~Blitter();
    /// VA: implementation-defined (pure virtual).
    virtual void Blit_Copy(void*, byte*, int, int, WORD*, WORD*, int, int) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual void Blit_Copy_Tinted(void*, byte*, int, int, WORD*, WORD*, int, int, WORD) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual void Blit_Move(void*, byte*, int, int, WORD*, WORD*, int) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual void Blit_Move_Tinted(void*, byte*, int, int, WORD*, WORD*, int, WORD) = 0;
protected:
    static WORD BlendAlphaRGB(WORD source, WORD destination, WORD alpha);
    static WORD* Lookup_Alpha_Remapper(int alvl, AlphaLightingRemapClass* remapper);
};

class RLEBlitter {
    friend struct Blitter::Kernel;
public:
    virtual ~RLEBlitter();
    /// VA: implementation-defined (pure virtual).
    virtual void Blit_Copy(void*, byte*, int, int, int, WORD*, WORD*, int, int, byte*) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual void Blit_Copy_Tinted(void*, byte*, int, int, int, WORD*, WORD*, int, int, byte*, WORD) = 0;
protected:
    static WORD* Lookup_Alpha_Remapper(int alvl, AlphaLightingRemapClass* remapper);
    template<bool UseZBuffer, bool UseABuffer, typename T>
    static void Process_Pre_Lines(T*& dest, byte*& src, int& len, const int& line, WORD*& zbuf, WORD*& abuf);
    template<bool UseZBuffer, bool UseABuffer, int ZMode = 0, typename T, typename Fn>
    static void Process_Pixel_Datas(T* dest, byte* src, int len, int zbase, WORD* zbuf, WORD* abuf, byte* zadjust, Fn f);
};

#define DEFINE_BLITTER(x) template<typename T> class x final : public Blitter
#define DEFINE_RLE_BLITTER(x) template<typename T> class x final : public RLEBlitter
