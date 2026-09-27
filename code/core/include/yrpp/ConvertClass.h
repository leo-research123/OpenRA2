/*
    Converts are palettes... AFAIK
*/

#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/BasicStructures.h"
#include <cstddef>

template<typename T> class DynamicVectorClass;
enum class BlitterFlags : unsigned int;
struct noinit_t;
class Surface;

class Blitter;
class RLEBlitter;
class RGBClass;
struct ColorStruct;
class DSurface;

// struct Blitter;

class ConvertClass
{
public:
    // global array
    static DynamicVectorClass<ConvertClass*>& Array; // Global VA: 0x0089ECF8; x86 binding in compat/src/images/resource_globals.cpp

    // Original mutable blitter selection masks (81DC24 / 81DC28).
    static unsigned int& PlainZFlags;
    static unsigned int& RLEZFlags;
    // Shared 16-bit palette algorithm, independent of Surface/Blitter lifetime.
    // Capacity counts WORDs; failure preserves output. Mode 0..3 selects the
    // original RGB packing, other values use bound Drawing shifts.
    static bool BuildColorTable(const BytePalette& palette, WORD* output,
        std::size_t capacity, int shadeCount, int colorMode) noexcept;

    static ConvertClass* FindOrAllocate(const char* pFilename);

    // Migrated body in ConvertClass.cpp; original entry 0x72ADE0.
    /// VA: 0x0072ADE0.
    static void YRPP_FASTCALL CreateFromFile(const char* pFilename, BytePalette* &pPalette, ConvertClass* &pDestination);

    // if you're drawing a SHP, call SHPStruct::HasCompression and choose one of these two based on that
    Blitter* SelectPlainBlitter(BlitterFlags flags) const; // 0x490B90; ConvertClassLifecycle.cpp

    RLEBlitter* SelectRLEBlitter(BlitterFlags flags) const; // 0x490E50; ConvertClassLifecycle.cpp

    virtual ~ConvertClass(); // Shared C++ lifecycle; compat adapters preserve calling convention.

    ConvertClass(
        BytePalette const& palette,
        BytePalette const& eightbitpalette, //???
        DSurface* pSurface,
        size_t shadeCount,
        bool skipBlitters); // 0x48E740; compat entry executes this constructor.

    // Native construction without an original DSurface. Color conversion and
    // registry storage are provided by the optional core software renderer.
    ConvertClass(BytePalette const& palette, BytePalette const& eightbitpalette,
        int bytesPerPixel, size_t shadeCount, bool skipBlitters);

protected:
    explicit ConvertClass(noinit_t);

    // Properties

public:
    int BytesPerPixel;
    Blitter* Blitters[50];
    RLEBlitter* RLEBlitters[39];
    int ShadeCount;
    void* FullColorData; // new(ShadeCount* 8* BytesPerPixel) - gets filled with palette values on CTOR
    void* PaletteData; // points to the middle of FullColorData above
    void* ByteColorData; // if(BytesPerPixel == 1) { ByteColorData = new byte[0x100]; }
    BYTE* CurrentZRemap; // remap-table pointer; still four bytes on x86
    DWORD HalfTranslucencyMask; // Used by 50 alpha blending
    DWORD QuatTranslucencyMask; // Used by 25 and 75 alpha blending
};

class LightConvertClass : public ConvertClass
{
public:
    // global array
    static DynamicVectorClass<LightConvertClass*>& Array; // Global VA: 0x0087F698; x86 binding in compat/src/images/resource_globals.cpp

    // Original light-conversion state; static declarations add no object fields.
    static BYTE (&DefaultIndexes)[256]; // 829C20
    static int& LightMode;              // 829D20 (-1: initialize from Drawing)
    static BYTE& UseMMX;                // 84E862; controls arithmetic, not host CPUID
    static int& Quality;               // A8EB78
    // Same arithmetic as UpdateColors, with already-resolved tint values.
    // No allocation or global mutation. Null indexes means all colors lit;
    // otherwise indexCount must be >=256. Zero shades normalizes to one;
    // 2..13 is invalid, matching the LightConvert constructor contract.
    static bool BuildColorTable(const BytePalette& palette, WORD* output,
        std::size_t capacity, int shadeCount, int red, int green, int blue,
        const BYTE* indexes, std::size_t indexCount, int colorMode, bool useMMX) noexcept;
    // Original 5558E0: split maximum-channel brightness from the palette tint,
    // retaining the 16.16 multiplier and signed 32-bit terrain product.
    static int NormalizeCellLight(DWORD& intensity, int& terrain,
        int& red, int& green, int& blue) noexcept;
    // 555AC0 / 544E70 quantization; returns the resulting 27/53 shade count.
    // The neutral preset is selected by the caller before quantization.
    static int PrepareCellTint(int& red, int& green, int& blue, int quality) noexcept;

    // Destructor
    virtual ~LightConvertClass(); // Shared C++ lifecycle; compat adapters preserve calling convention.

    // Keep the original virtual dispatch usable by the base core without
    // imposing the optional software renderer as a link dependency.
    virtual void UpdateColors(int red, int green, int blue, bool tinted); // 0x556090; LightConvertClass.cpp

    static LightConvertClass* YRPP_FASTCALL InitLightConvert(int red, int green, int blue); // Global VA: 0x00544E70; LightConvertClass.cpp

    // All constructors reject shadeCount 2..13 before base allocation; zero
    // retains Convert's normalization to one. Exceptions stay within the caller
    // module; compat entry points handle failures before returning to the EXE.
    LightConvertClass(
        BytePalette* palette1,
        BytePalette* palette2,
        Surface* pSurface,
        int color_R,
        int color_G,
        int color_B,
        bool skipBlitters,
        BYTE* pBuffer, // allowed to be null
        size_t shadeCount); // 0x555DA0; compat entry executes this constructor.

    LightConvertClass(BytePalette* palette1, BytePalette* palette2,
        int bytesPerPixel, int color_R, int color_G, int color_B,
        bool skipBlitters, BYTE* pBuffer, size_t shadeCount);

protected:
    explicit LightConvertClass(noinit_t);

public:

    // Properties
    RGBClass* UsedPalette1;
    RGBClass* UsedPalette2;
    BYTE* IndexesToIgnore;
    int RefCount;
    TintStruct Color1;
    TintStruct Color2;
    bool Tinted;
protected:
    BYTE align_1B1[3];
};
