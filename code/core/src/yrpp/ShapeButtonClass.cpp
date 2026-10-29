// EA REDALERT/SHAPEBTN.CPP, f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// Copyright 2020 Electronic Arts Inc. GPL-3.0-or-later with EA Section 7;
// see code/third_party/ea/LICENSE.TXT. YR 69DCF0 / 69DE00 / 69DEB0.
#include "yrpp/ShapeButtonClass.h"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/Memory.h"
#include "game_ui_runtime.hpp"
#include <algorithm>
#include <cstdlib>
#include <bit>
#if !defined(RA2_YRPP_GAME)
bool ShapeButtonClass::StartFlashing(int delay, int initialDelay, bool start) {
    if (FlashDelay) return false;
    FlashDelay = delay;
    FlashCounter = std::bit_cast<int>(unsigned(initialDelay) + unsigned(delay));
    IsToFlash = start;
    return true;
}
bool ShapeButtonClass::StopFlashing() {
    if (!FlashDelay) return false;
    IsToFlash = false;
    FlashCounter = FlashDelay = 0;
    return true;
}
bool ShapeButtonClass::AI() {
    if (Disabled) return StopFlashing();
    if (FlashCounter) {
        FlashCounter = std::bit_cast<int>(unsigned(FlashCounter) - 1u);
        if (!FlashCounter) {
            IsToFlash = !IsToFlash;
            FlashCounter = FlashDelay;
            return true;
        }
    }
    return false;
}
bool ShapeButtonClass::IsFlashing() { return FlashDelay != 0; }
#endif
ShapeButtonClass::ShapeButtonClass() noexcept
    : ToggleClass(0, 0, 0, 0, 0), IsToFlash(false), FlashDelay(0), FlashCounter(0), UseFlash(false), DrawPosition{},
      UseSidebarSurface(false), Drawer(nullptr), IsDrawn(false), IsAlpha(false), ShapeData(nullptr),
      IsShapeLoaded(false) {}
ShapeButtonClass::ShapeButtonClass(unsigned id, SHPStruct* shape, int x, int y, int width, int height,
                                   bool alpha) noexcept
    : ShapeButtonClass() {
    ID = static_cast<int>(id);
    X = x;
    Y = y;
    IsAlpha = alpha;
    SetShape(shape, width, height);
}
void ShapeButtonClass::SetShape(SHPStruct* shape, int width, int height) {
    if (IsShapeLoaded && ShapeData) {
        YRMemory::Deallocate(ShapeData);
        IsShapeLoaded = false;
    }
    ShapeData = shape;
    Width = width ? width : shape ? shape->Width : 0;
    Height = height ? height : shape ? shape->Height : 0;
    MarkRedraw();
}
bool ShapeButtonClass::Draw(bool) { std::abort(); }
