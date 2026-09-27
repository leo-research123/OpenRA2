// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from CnC_Renegade 3e00c3a1, WWLib surface.h/xsurface.h/bsurface.h.
// YR fields, virtual slots, counter behavior and query order calibrated against
// gamemd 5FE020, 4114F0..411675. All construction and destruction use the core class hierarchy.
#include "yrpp/Surface.h"
#include "yrpp/Memory.h"
#include <new>
#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <algorithm>

namespace {
int buffer_bytes(int width, int height, int bytes_per_pixel) {
    // Shared construction contract for native and original-game callers.
    if (width < 0 || height < 0 || (bytes_per_pixel != 1 && bytes_per_pixel != 2))
        throw std::invalid_argument("software BSurface requires nonnegative dimensions and 1/2-byte pixels");
    const auto pitch = std::int64_t(width) * bytes_per_pixel;
    const auto bytes = pitch * height;
    if (pitch > std::numeric_limits<int>::max() || bytes > std::numeric_limits<int>::max())
        throw std::length_error("software BSurface exceeds MemoryBuffer capacity");
    return static_cast<int>(bytes);
}
}

Surface::~Surface() = default;
bool Surface::CanLock(DWORD, DWORD) { return true; }
bool Surface::vt_entry_68(DWORD, DWORD) { return true; }
RectangleStruct* Surface::GetRect(RectangleStruct* rectangle) {
    *rectangle = {0, 0, Width, Height};
    return rectangle;
}
int Surface::GetWidth() { return Width; }
int Surface::GetHeight() { return Height; }

XSurface::XSurface(int width, int height) {
    Width = width;
    Height = height;
    LockLevel = 0;
    // The original XSurface constructor leaves the derived pixel depth alone.
}
void* XSurface::Lock(int, int) {
    LockLevel = std::bit_cast<int>(std::uint32_t(LockLevel) + 1u);
    return nullptr;
}
bool XSurface::Unlock() {
    // Preserve original decrement even at zero, and its nonzero IsLocked rule.
    LockLevel = std::bit_cast<int>(std::uint32_t(LockLevel) - 1u);
    return true;
}
bool XSurface::IsLocked() { return LockLevel != 0; }

// EA WWLib XSurface::Fill / Fill_Rect, calibrated to 7BB020/7BB050/7BBAB0.
// Fill coordinates are relative to the supplied clipping rectangle.
bool XSurface::FillRectEx(RectangleStruct* clip, RectangleStruct* fill, COLORREF color) {
    if (!clip || !fill || fill->Width <= 0 || fill->Height <= 0) return false;
    RectangleStruct bounds; GetRect(&bounds);
    const auto x = std::max({std::int64_t(bounds.X), std::int64_t(clip->X), std::int64_t(clip->X) + fill->X});
    const auto y = std::max({std::int64_t(bounds.Y), std::int64_t(clip->Y), std::int64_t(clip->Y) + fill->Y});
    const auto right = std::min({std::int64_t(bounds.X) + bounds.Width, std::int64_t(clip->X) + clip->Width,
        std::int64_t(clip->X) + fill->X + fill->Width});
    const auto bottom = std::min({std::int64_t(bounds.Y) + bounds.Height, std::int64_t(clip->Y) + clip->Height,
        std::int64_t(clip->Y) + fill->Y + fill->Height});
    if (x >= right || y >= bottom) return false;
    const int pitch = GetPitch();
    auto* pixels = static_cast<byte*>(Lock(int(x), int(y)));
    if (!pixels) return false;
    const int bpp = GetBytesPerPixel();
    for (auto row = y; row < bottom; ++row, pixels += pitch) {
        if (bpp == 1) std::memset(pixels, byte(color), std::size_t(right - x));
        else for (auto col = x; col < right; ++col) {
            const WORD value = WORD(color);
            std::memcpy(pixels + (col - x) * 2, &value, 2);
        }
    }
    Unlock(); return true;
}
bool XSurface::FillRect(RectangleStruct* fill, COLORREF color) {
    RectangleStruct bounds; GetRect(&bounds);
    return XSurface::FillRectEx(fill, &bounds, color);
}
bool XSurface::Fill(COLORREF color) {
    RectangleStruct clip, fill; GetRect(&fill); GetRect(&clip);
    return FillRectEx(&clip, &fill, color);
}

BSurface::BSurface() : BSurface(640, 400, 2) {}
BSurface::BSurface(int width, int height) : BSurface(width, height, 2) {}
BSurface::BSurface(int width, int height, int bytes_per_pixel, void* buffer)
    : XSurface(width, height), Buffer(buffer, buffer_bytes(width, height, bytes_per_pixel)) {
    BytesPerPixel = bytes_per_pixel;
    // MemoryBuffer retains its original null-buffer allocation-failure state.
}
void* BSurface::Lock(int x, int y) {
    XSurface::Lock(x, y);
    // Lock does no clipping in the original. The caller supplies valid positions;
    // capture the buffer before virtual pixel-depth then pitch queries, since
    // an override can change the object's buffer during either query.
    const auto buffer = reinterpret_cast<std::uintptr_t>(Buffer.Buffer);
    const auto horizontal = std::ptrdiff_t(x) * GetBytesPerPixel();
    const auto offset = std::ptrdiff_t(y) * GetPitch() + horizontal;
    return reinterpret_cast<void*>(buffer + static_cast<std::uintptr_t>(offset));
}
int BSurface::GetBytesPerPixel() { return BytesPerPixel; }
int BSurface::GetPitch() { return GetWidth() * BytesPerPixel; }

#ifndef RA2_IMAGE_GAME
namespace { DSurface* primary_surface = nullptr; DSurface* alternate_surface = nullptr; }
DSurface*& DSurface::Primary = primary_surface;
DSurface*& DSurface::Alternate = alternate_surface;
#endif

BSurface* BSurface::Initialize(void* storage, int width, int height, int bpp, void* buffer) {
    return new (storage) BSurface(width, height, bpp, buffer);
}
BSurface* BSurface::Create(int width, int height, int bpp, void* buffer) {
    void* storage = YRMemory::Allocate(sizeof(BSurface));
    if (!storage) return nullptr;
    try { return Initialize(storage, width, height, bpp, buffer); }
    catch (...) { YRMemory::Deallocate(storage); throw; }
}
void BSurface::Destroy(BSurface* surface) noexcept {
    if (!surface) return;
    surface->~BSurface();
    YRMemory::Deallocate(surface);
}

// Non-template interface helpers; bodies retained from the corresponding header.

bool Surface::CopyFromWhole(Surface* pSrc, bool bUnk1, bool bUnk2)
{ return 0; }

bool Surface::CopyFromPart(
        RectangleStruct* pClipRect, //ignored and retrieved again...
        Surface* pSrc,
        RectangleStruct* pSrcRect,	//desired source rect of pSrc ?
        bool bUnk1,
        bool bUnk2)
{ return 0; }

bool Surface::CopyFrom(
        RectangleStruct* pClipRect,
        RectangleStruct* pClipRect2,	//again? hmm
        Surface* pSrc,
        RectangleStruct* pDestRect,	//desired dest rect of pSrc ? (stretched? clipped?)
        RectangleStruct* pSrcRect,	//desired source rect of pSrc ?
        bool bUnk1,
        bool bUnk2)
{ return 0; }

bool Surface::FillRectEx(RectangleStruct* pClipRect, RectangleStruct* pFillRect, COLORREF nColor)
{ return 0; }

bool Surface::FillRect(RectangleStruct* pFillRect, COLORREF nColor)
{ return 0; }

bool Surface::Fill(COLORREF nColor)
{ return 0; }

bool Surface::FillRectTrans(RectangleStruct* pClipRect, ColorStruct* pColor, int nOpacity)
{ return 0; }

bool Surface::DrawEllipse(
        int XOff, int YOff, int CenterX, int CenterY, RectangleStruct Rect, COLORREF nColor)
{ return 0; }

bool Surface::SetPixel(Point2D* pPoint, COLORREF nColor)
{ return 0; }

COLORREF Surface::GetPixel(Point2D* pPoint)
{ return 0; }

bool Surface::DrawLineEx(RectangleStruct* pClipRect, Point2D* pStart, Point2D* pEnd, COLORREF nColor)
{ return 0; }

bool Surface::DrawLine(Point2D* pStart, Point2D* pEnd, COLORREF nColor)
{ return 0; }

bool Surface::DrawLineColor(
        RectangleStruct* pRect, Point2D* pStart, Point2D* pEnd, COLORREF nColor,
        int startZ, int endZ, bool bUnk)
{ return 0; }

bool Surface::DrawMultiplyingLine(
        RectangleStruct* pRect, Point2D* pStart, Point2D* pEnd, DWORD dwMultiplier,
        DWORD dwUnk1, DWORD dwUnk2, bool bUnk)
{ return 0; }

bool Surface::DrawSubtractiveLine(
        RectangleStruct* pRect, Point2D* pStart, Point2D* pEnd, ColorStruct* pColor,
        DWORD dwUnk1, DWORD dwUnk2, bool bUnk1, bool bUnk2,
        bool bUkn3, bool bUkn4, float fUkn)
{ return 0; }

bool Surface::DrawRGBMultiplyingLine(
        RectangleStruct* pRect, Point2D* pStart, Point2D* pEnd, ColorStruct* pColor,
        float Intensity, int zSource, int zTarget)
{ return 0; }

bool Surface::PlotLine(
        RectangleStruct* pRect, Point2D* pStart, Point2D* pEnd, bool(YRPP_FASTCALL* fpDrawCallback)(int*))
{ return 0; }

bool Surface::DrawDashedLine(
        Point2D* pStart, Point2D* pEnd, int nColor, bool* Pattern, int nOffset)
{ return 0; }

bool Surface::DrawDashedLine_(
        Point2D* pStart, Point2D* pEnd, int nColor, bool* Pattern, int nOffset, bool bUkn)
{ return 0; }

bool Surface::DrawLine_(Point2D* pStart, Point2D* pEnd, int nColor, bool bUnk)
{ return 0; }

bool Surface::DrawRectEx(RectangleStruct* pClipRect, RectangleStruct* pDrawRect, int nColor)
{ return 0; }

bool Surface::DrawRect(RectangleStruct* pDrawRect, DWORD dwColor)
{ return 0; }

void* Surface::Lock(int X, int Y)
{ return 0; }

bool Surface::Unlock()
{ return 0; }

bool Surface::IsLocked()
{ return 0; }

int Surface::GetBytesPerPixel()
{ return 0; }

int Surface::GetPitch()
{ return 0; }

bool Surface::IsDSurface()
{ return 0; }

RectangleStruct Surface::GetRect()
{
    RectangleStruct ret;
    this->GetRect(&ret);
    return ret;
}

#ifndef RA2_IMAGE_GAME
// Original-image targets bind these two unported slots in surface_methods.cpp.
bool XSurface::PutPixelClip(Point2D*, short, RectangleStruct*)
{ return false; }

short XSurface::GetPixelClip(Point2D*, RectangleStruct*)
{ return 0; }
#endif
