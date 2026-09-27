#pragma once

#include "yrpp/platform/ABI.h"

struct Color16Struct;

// used for most colors
struct ColorStruct
{
    ColorStruct() = default;

    ColorStruct(BYTE const r, BYTE const g, BYTE const b);

    ColorStruct(const ColorStruct& c);

    explicit ColorStruct(Color16Struct const color);

    explicit ColorStruct(DWORD const color);

    explicit ColorStruct(WORD const color);

    bool operator == (ColorStruct const rhs) const;

    bool operator != (ColorStruct const rhs) const;

    ColorStruct operator + (ColorStruct const rhs) const;

    void operator += (ColorStruct const rhs);

    explicit operator DWORD() const;

    explicit operator WORD() const;

    BYTE R, G, B;
};

struct BytePalette {
    ColorStruct Entries[256];

    ColorStruct& operator [](int const idx);

    ColorStruct const& operator [](int const idx) const;
};

// used for light colors
struct TintStruct
{
    TintStruct() = default;

    TintStruct(int r, int g, int b);

    int Red, Green, Blue;

    bool operator == (TintStruct const rhs) const;

    bool operator != (TintStruct const rhs) const;

    bool operator < (TintStruct const rhs) const;
};

//16bit colors
#pragma pack(push, 1)
struct Color16Struct
{
    Color16Struct() = default;

    explicit Color16Struct(ColorStruct const color);

    explicit Color16Struct(WORD const color);

    explicit Color16Struct(DWORD const color);

    bool operator == (Color16Struct const rhs) const;

    bool operator != (Color16Struct const rhs) const;

    explicit operator WORD() const;

    explicit operator DWORD() const;

    unsigned short B : 5;
    unsigned short G : 6;
    unsigned short R : 5;
};
#pragma pack(pop)

// Random number range
struct RandomStruct
{
    int Min, Max;
};

// obvious
struct RectangleStruct
{
    int X, Y, Width, Height;
};

struct LTRBStruct
{
    int Left;
    int Top;
    int Right;
    int Bottom;
};
