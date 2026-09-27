// YRpp 9402d7da ConvertClass.h object model. Palette scaling extends the
// fixed EA Renegade 3e00c3a1 WWLib convert.cpp/DSurface remap-table algorithm.
// Copyright 2025 Electronic Arts Inc.; SPDX-License-Identifier: GPL-3.0-or-later
// YR-specific tint state, ignored indexes, CPU paths and ownership calibrated
// against 555DA0/556090/556510/544E70 and the four 7DE200..7DEBBA routines.
#include "yrpp/Memory.h"
#include "LightConvertClassHelpers.hpp"
#include "ConvertClassHelpers.hpp"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include "images/original_abi.hpp"
#include "yrpp/ArrayClasses.h"
#include "yrpp/FileSystem.h"
#include <array>
#include <algorithm>
#include <bit>
#include <new>
#include <stdexcept>

#ifndef RA2_IMAGE_GAME
namespace {
DynamicVectorClass<LightConvertClass*> light_converts;
// Standalone scalar policy: all nonzero indexes participate in lighting.
// Original targets bind the exact EXE table instead. Callers may pass their own
// index array to the existing constructor; this is not a copied test provider.
BYTE default_indexes[256];
struct InitIndexes { InitIndexes() { std::fill_n(default_indexes, 256, BYTE(1)); default_indexes[0] = 0; } } init_indexes;
int light_mode = -1, light_quality = 3;
BYTE use_mmx = 0;
}
BYTE (&LightConvertClass::DefaultIndexes)[256] = default_indexes;
int& LightConvertClass::LightMode = light_mode;
BYTE& LightConvertClass::UseMMX = use_mmx;
int& LightConvertClass::Quality = light_quality;
DynamicVectorClass<LightConvertClass*>& LightConvertClass::Array = light_converts;
#endif

namespace {
int light_pixel_depth(int bytes_per_pixel) {
    if (bytes_per_pixel != 2)
        throw std::invalid_argument("LightConvertClass requires 16-bit pixels");
    return bytes_per_pixel;
}
std::size_t light_shade_count(std::size_t shades) {
    // Shared constructor precondition, checked before base allocation.
    // Keep zero's existing normalization to one in the Convert base constructor.
    if (shades >= 2 && shades <= 13)
        throw std::invalid_argument("LightConvertClass cannot use 2..13 shades");
    return shades;
}

}
namespace game {
unsigned build_light_palette(const BytePalette* palette, WORD* destination, int red, int green,
    int blue, int unlit, const BYTE* indexes, int mode) {
    return build_light_palette_row(palette,destination,red,green,blue,unlit,indexes,mode,LightConvertClass::UseMMX!=0);
}

namespace {
LightConvertClass* initialize_light_fields(LightConvertClass* self, const BytePalette* art,
    const BytePalette* screen, int r, int g, int b, bool skip, BYTE* indexes) {
    self->UsedPalette1 = reinterpret_cast<RGBClass*>(const_cast<BytePalette*>(art));
    self->UsedPalette2 = reinterpret_cast<RGBClass*>(const_cast<BytePalette*>(screen));
    self->IndexesToIgnore = indexes;
    self->RefCount = 0;
    self->Color1 = self->Color2 = TintStruct{r, g, b};
    self->Tinted = false;
    if (self->ShadeCount < 1) return self;
    if (!indexes) self->IndexesToIgnore = LightConvertClass::DefaultIndexes;
    if (LightConvertClass::LightMode == -1)
        LightConvertClass::LightMode = static_cast<int>(Drawing::ColorMode);
#ifdef RA2_IMAGE_GAME
    TintStruct tint;
    if (game::OriginalScenarioTint(tint)) {
        self->Color1 = r == -1 ? TintStruct{1000, 1000, 1000} : TintStruct{r, g, b};
        self->UpdateColors(tint.Red, tint.Green, tint.Blue, true);
    } else
#endif
    self->UpdateColors(r, g, b, false);
    if (!skip) initialize_blitters(self);
    return self;
}
}
}

void LightConvertClass::UpdateColors(int red, int green, int blue, bool tinted) {
    if (ShadeCount < 1) return;
    if (!IndexesToIgnore) IndexesToIgnore = LightConvertClass::DefaultIndexes;
    auto& color = tinted ? Color2 : Color1;
    if (red == -1) { red = color.Red; green = color.Green; blue = color.Blue; }
    else {
        red = std::clamp(red, 0, 2000); green = std::clamp(green, 0, 2000); blue = std::clamp(blue, 0, 2000);
        color = TintStruct{red, green, blue};
    }
    Tinted = tinted;
    BuildColorTable(*reinterpret_cast<const BytePalette*>(UsedPalette1),static_cast<WORD*>(FullColorData),
        std::size_t(ShadeCount)*256,ShadeCount,red,green,blue,IndexesToIgnore,256,LightMode,UseMMX!=0);
}

LightConvertClass* YRPP_FASTCALL LightConvertClass::InitLightConvert(int red, int green, int blue) {
    auto* surface = DSurface::Primary;
    if (!surface) return nullptr;
    if (red == 1000 && green == 1000 && blue == 1000 && Array.Count) return Array.Items[0];
    const int shades = PrepareCellTint(red,green,blue,Quality);
    for (int i = 1; i < Array.Count; ++i) {
        auto* item = Array.Items[i];
        if (item->Color1.Red == red && item->Color1.Green == green && item->Color1.Blue == blue) return item;
    }
    auto* item = static_cast<LightConvertClass*>(YRMemory::Allocate(sizeof(LightConvertClass)));
    if (item) {
        try {
            new (item) LightConvertClass(const_cast<BytePalette*>(&FileSystem::ISOx_PAL),
                const_cast<BytePalette*>(&FileSystem::TEMPERAT_PAL), surface, red, green, blue,
                Array.Count != 0, nullptr, shades);
        } catch (...) {
            YRMemory::Deallocate(item);
            throw;
        }
    }
    if (!Array.AddItem(item)) {
        if (item) { item->~LightConvertClass(); YRMemory::Deallocate(item); }
        return nullptr;
    }
    return item;
}

LightConvertClass::LightConvertClass(BytePalette* art, BytePalette* screen, Surface* surface,
    int red, int green, int blue, bool skip, BYTE* indexes, std::size_t shades)
    : LightConvertClass(art, screen, surface->GetBytesPerPixel(), red, green, blue,
        skip, indexes, shades) {}

LightConvertClass::LightConvertClass(BytePalette* art, BytePalette* screen, int bytes_per_pixel,
    int red, int green, int blue, bool skip, BYTE* indexes, std::size_t shades)
    : ConvertClass(*art, *screen, light_pixel_depth(bytes_per_pixel), light_shade_count(shades), true) {
    game::initialize_light_fields(this, art, screen, red, green, blue, skip, indexes);
}

// The C++ base destructor owns the Convert buffers, blitters and registry entry.
LightConvertClass::~LightConvertClass() = default;
