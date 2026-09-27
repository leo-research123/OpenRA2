#pragma once

#include "yrpp/BasicStructures.h"
class Surface;
class BSurface;

// Layout retained from fixed YRpp Drawing.h.
class ABuffer
{
public:
    static ABuffer*& Instance;

    /// VA: 0x00410CE0
    ABuffer(RectangleStruct Rect) ;
    bool BlitTo(Surface* pSurface, int X, int Y, int Offset, int Size) ;
    void ReleaseSurface() ;
    void Blitter(unsigned short* Data, int Length, unsigned short Value) ;
    void BlitAt(int X, int Y, COLORREF Color) ;
    /// VA: 0x004112D0
    bool Fill(unsigned short Color) ;
    bool FillRect(unsigned short Color, RectangleStruct Rect) ;
    /// VA: 0x00411330
    void BlitRect(RectangleStruct Rect) ;
    /// VA: 0x004114B0
    void* GetBuffer(int X, int Y) ;

    template<typename T>
    void AdjustPointer(T*& ptr)
    {
        if (reinterpret_cast<std::uintptr_t>(ptr) >= reinterpret_cast<std::uintptr_t>(BufferTail))
            reinterpret_cast<char*&>(ptr) -= BufferSize;
    }

    RectangleStruct Bounds;
    int BufferPosition;
    BSurface* Surface;
    void* BufferHead;
    void* BufferTail;
    int BufferSize;
    int MaxValue;
    int Width;
    int Height;
};

class ZBuffer
{
public:
    static ZBuffer*& Instance;

    ZBuffer(RectangleStruct Rect) ;
    bool BlitTo(Surface* pSurface, int X, int Y, int Offset, int Size) ;
    void ReleaseSurface() ;
    void Blitter(unsigned short* Data, int Length, unsigned short Value) ;
    void BlitAt(int X, int Y, COLORREF Color) ;
    bool Fill(unsigned short Color) ;
    bool FillRect(unsigned short Color, RectangleStruct Rect) ;
    void BlitRect(RectangleStruct Rect) ;
    void* GetBuffer(int X, int Y) ;

    template<typename T>
    void AdjustPointer(T*& ptr)
    {
        if (reinterpret_cast<std::uintptr_t>(ptr) >= reinterpret_cast<std::uintptr_t>(BufferTail))
            reinterpret_cast<char*&>(ptr) -= BufferSize;
    }

    RectangleStruct Bounds;
    int BufferOffset;
    BSurface* Surface;
    void* BufferHead;
    void* BufferTail;
    int BufferSize;
    int MaxValue;
    int Width;
    int Height;
};
