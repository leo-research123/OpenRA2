/*
    [Colors]
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ArrayClasses.h"
#include "yrpp/GeneralStructures.h"
#include "yrpp/HashTable.h"
#include "yrpp/Theater.h"

#include "yrpp/Helpers/CompileTime.h"

class LightConvertClass;

class ColorScheme {
public:
    // Allocation-free palette stages used by the original constructor. These
    // helpers do not construct a converter or change ColorScheme object layout.
    /// VA: 0x00517440.
    static ColorStruct HSVToRGB(const ColorStruct& hsv) noexcept;
    // Ramp stage of Build_Light_Converter (0x0068C3B0).
    static void BuildPalette(const ColorStruct& hsv, const BytePalette& source, BytePalette& output) noexcept;
    // Default ColorScheme LightConvert: neutral tint, indices 240..254 unlit.
    // Capacity is in WORDs. Invalid arguments preserve output; no exceptions.
    static bool BuildColorTable(const BytePalette& palette, WORD* output, std::size_t capacity, int shadeCount,
                                int colorMode, bool useMMX) noexcept;

    enum [[deprecated("Only valid for vanilla color scheme configuration and should not be used.")]] {
        // ColorScheme indices, since they are hardcoded all over the exe, why shan't we do it as well?
        Yellow = 3,
        White = 5,
        Grey = 7,
        Red = 11,
        Orange = 13,
        Pink = 15,
        Purple = 17,
        Blue = 21,
        Green = 29,
    };

    // global array
    /// Global VA: 0x00B054D0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<ColorScheme*>, Array, 0xB054D0u)
#else
    static DynamicVectorClass<ColorScheme*>& Array;
#endif

    // Player color scheme slot index to color scheme index lookup table.
    /// Global VA: 0x0083ED14.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(byte, [9u], PlayerColorToColorSchemeLUT, 0x83ED14u)
#else
    static byte (&PlayerColorToColorSchemeLUT)[9];
#endif
    /*
 * trap! most schemes are duplicated - ShadeCount 1 and ShadeCount 53
*/
    static ColorScheme* Find(const char* pID, int ShadeCount = 1) {
        int index = FindIndex(pID, ShadeCount);
        return Array.GetItemOrDefault(index);
    }

    static int FindIndex(const char* pID, int ShadeCount = 1) {
        for (int i = 0; i < Array.Count; ++i) {
            ColorScheme* pItem = Array.GetItem(i);
            if (!_strcmpi(pItem->ID, pID)) {
                if (pItem->ShadeCount == ShadeCount) {
                    return i;
                }
            }
        }
        return -1;
    }

    // Returns the registry index, as the original EAX result does. Native
    // allocation failures must be caught by the owning resource/load boundary.
    /// VA: 0x0068C9C0
#if defined(RA2_YRPP_GAME)
    static int YRPP_FASTCALL FindByName(const char* pID, const ColorStruct& BaseColor, const BytePalette& Pal1,
                                        const BytePalette& Pal2, int ShadeCount) {
        JMP_THIS(0x68C9C0);
    }
#else
    static int YRPP_FASTCALL FindByName(const char* pID, const ColorStruct& BaseColor, const BytePalette& Pal1,
                                        const BytePalette& Pal2, int ShadeCount);
#endif

    /// VA: 0x00626C60.
#if defined(RA2_YRPP_GAME)
    static int YRPP_FASTCALL GetNumberOfSchemes() { JMP_STD(0x626C60); }
#else
    static int YRPP_FASTCALL GetNumberOfSchemes() noexcept;
#endif

    /// VA: 0x006263D0.
#if defined(RA2_YRPP_GAME)
    static DynamicVectorClass<ColorScheme*>* YRPP_FASTCALL GeneratePalette(char* name) { JMP_STD(0x6263D0); }
#else
    static DynamicVectorClass<ColorScheme*>* YRPP_FASTCALL GeneratePalette(char* name) noexcept;
#endif

    // Successful registration returns the third HSV byte, as the original AL.
    // Native allocation failures stay here and mark the resource load failed.
    /// VA: 0x00626AB0
#if defined(RA2_YRPP_GAME)
    static byte YRPP_FASTCALL AddPaletteColor(const char* name, const ColorStruct& color) { JMP_STD(0x626AB0); }
#else
    static byte YRPP_FASTCALL AddPaletteColor(const char* name, const ColorStruct& color) noexcept;
#endif
    /// VA: 0x006269A0
#if defined(RA2_YRPP_GAME)
    static void ClearPaletteColors() { JMP_STD(0x6269A0); }
#else
    static void ClearPaletteColors() noexcept;
#endif
    /// VA: 0x006267A0
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL SetPaletteTheater(TheaterType theater) { JMP_STD(0x6267A0); }
#else
    static void YRPP_FASTCALL SetPaletteTheater(TheaterType theater) noexcept;
#endif
    /// VA: 0x00626DA0
#if defined(RA2_YRPP_GAME)
    static int CountLoadedPalettes() { JMP_STD(0x626DA0); }
#else
    static int CountLoadedPalettes() noexcept;
#endif
    /// VA: 0x00626FF0
#if defined(RA2_YRPP_GAME)
    static void ReleasePaletteCache() { JMP_STD(0x626FF0); }
#else
    static void ReleasePaletteCache() noexcept;
#endif

    // Game uses a hash table to store color scheme vectors for extra palettes, this table can be iterated by calling this function.
    /// VA: 0x00626690.
#if defined(RA2_YRPP_GAME)
    static DynamicVectorClass<ColorScheme*>* YRPP_FASTCALL GetPaletteSchemesFromIterator(HashIterator* it) {
        JMP_STD(0x626690);
    }
#else
    static DynamicVectorClass<ColorScheme*>* YRPP_FASTCALL GetPaletteSchemesFromIterator(HashIterator* it) noexcept;
#endif

    // Replaces the converter using this scheme's HSV and shade count.
    // Native allocation failures mark the resource scope, returning null.
    /// VA: 0x0068C860
#if defined(RA2_YRPP_GAME)
    LightConvertClass* UpdatePalette(const BytePalette& art, const BytePalette& screen, int red, int green, int blue) {
        JMP_THIS(0x68C860);
    }
#else
    LightConvertClass* UpdatePalette(const BytePalette& art, const BytePalette& screen, int red, int green,
                                     int blue) noexcept;
#endif

    // Constructor, Destructor
    /// VA: 0x0068C710.
#if defined(RA2_YRPP_GAME)
    ColorScheme(const char* pID, const ColorStruct& BaseColor, const BytePalette& Pal1, const BytePalette& Pal2,
                int ShadeCount, bool AddToArray) {
        JMP_THIS(0x68C710);
    }

    /// VA: 0x0068C8D0.
    ~ColorScheme() { JMP_THIS(0x68C8D0); }
#else
    // Native convenience entry using the same art and screen palette. It owns
    // the same original LightConvert as the two-palette constructor.
    ColorScheme(const char* id, const ColorStruct& baseColor, const BytePalette& palette, int shadeCount,
                bool addToArray);
    // Native allocation failures stay in the owning resource/load boundary.
    /// VA: 0x0068C710
    ColorScheme(const char* id, const ColorStruct& baseColor, const BytePalette& palette1, const BytePalette& palette2,
                int shadeCount, bool addToArray);
    /// VA: 0x0068C8D0
    ~ColorScheme();
#endif

    // Properties

public:
    int ArrayIndex;  // this is off by one (always one higher than the actual index). that's because consistency and reason suck.

    BytePalette Colors;

    char* ID;
    ColorStruct BaseColor;
    LightConvertClass*
        LightConvert;  //??? remap - indices #16-#31 are changed to mathefuckikally derived shades of BaseColor, think unittem.pal
    int ShadeCount;
    PROTECTED_PROPERTY(BYTE, unknown_314[0x1C]);
    int MainShadeIndex;
    PROTECTED_PROPERTY(BYTE, unknown_334[0x8]);
};

struct HashString {
    char Name[256];
    /// VA: 0x004F4CE0
    bool operator==(const HashString& other) const noexcept {
        const char* left = Name;
        const char* right = other.Name;
        while (*left && *left == *right) {
            ++left;
            ++right;
        }
        return *left == *right;
    }
};

struct SchemeNode {
    char Name[256];
    DynamicVectorClass<ColorScheme*>* Schemes;
};
