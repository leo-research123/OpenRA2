#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/BasicStructures.h"
#include <cstdint>

class Surface;
namespace game { struct TypeDrawingContext; enum class DrawingStatus : std::uint32_t; }

class BitFont
{
public:
    static BitFont*& Instance; // Global VA: 0x0089C4D0; binding in compat.

    // Public construction also permits standalone use of the original class.
    explicit BitFont(const char* pFileName);
    virtual ~BitFont();

    /// VA: 0x00433CF0
    bool GetTextDimension(const wchar_t* pText, int* pWidth, int* pHeight, int nMaxWidth);
    /// VA: 0x00433ED0
    int GetTextWidth(const wchar_t* text,int maxWidth = 0) noexcept;
    /// VA: 0x00433F50
    int GetTextFit(const wchar_t* text,int width,int maxCharacters = 0,bool breakAtWord = false) noexcept;
    // Original ABI: no exceptions cross this allocation-free entry.
    /// VA: 0x00434500
    int DrawString(const wchar_t* text, int x, int y, int length, int animationPosition) noexcept;
    int Blit(wchar_t wch, int X, int Y, int nColor);
    // Same original glyph bits/advance, submitted to a generic canvas backend.
    game::DrawingStatus SubmitGlyph(const game::TypeDrawingContext&,wchar_t,int x,int y,
        const RectangleStruct& clip,WORD color,int& next_x) noexcept;

    // Calibrated against 4348F0/434990 and their callers: state operations
    // without a return-value protocol. YRpp's bool declaration is not retained;
    // arithmetic/helper residues in EAX must not become public return values.
    void Lock(Surface* pSurface);
    void UnLock(Surface* pSurface);
    unsigned char* GetCharacterBitmap(wchar_t wch);

    void SetBounds(LTRBStruct* pBound);

    /// VA: 0x00433CA0
    void SetRectangle(LTRBStruct* pRect);

    /// VA: 0x00433C70
    void SetColor(WORD nColor);

    /// VA: 0x00433C90
    void SetClipMode(bool drawClipped);

    void SetField20(int x);

    void SetField41(char flag);

    /// Properties
    struct InternalData
    {
        int FontWidth;
        int Stride;
        int FontHeight;
        int Lines;
        int Count;
        int SymbolDataSize;
        short* SymbolTable;
        char* Bitmaps;
        int ValidSymbolCount;
    };

    // Owns SymbolTable/Bitmaps and the returned record, all in YRMemory's domain.
    // A BitFont owns its InternalPTR; callers of this standalone entry must free
    // all three allocations themselves. File words are always little-endian.
    static InternalData* YRPP_FASTCALL LoadInternalData(const char* pFileName);

    InternalData* InternalPTR;
    void* Pointer_8;
    short* pGraphBuffer;
    int PitchDiv2;
    int Unknown_14;
    int field_18; // Glyph row stride; original x86 +0x18 is an integer, not a pointer.
    int field_1C;
    int field_20;
    WORD Color;
    short DefaultColor2;
    int Unknown_28;
    int State_2C;
    LTRBStruct Bounds;
    bool Bool_40;
    bool field_41;
    bool field_42;
    bool field_43;
};
