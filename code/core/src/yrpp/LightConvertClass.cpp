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
#if !defined(RA2_IMAGE_GAME)
#include "convert_revision.hpp"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SuperClass.h"
#include "tactical_drawing.hpp"
#endif
#include <array>
#include <algorithm>
#include <bit>
#include <new>
#include <stdexcept>

#ifndef RA2_IMAGE_GAME
namespace {
DynamicVectorClass<LightConvertClass*> light_converts;
// Original default mask at 0x00829C20 contains 256 enabled entries. Transparent
// index zero is handled by the color-table algorithm, not by this mask.
BYTE default_indexes[256];
struct InitIndexes {
    InitIndexes() { std::fill_n(default_indexes, 256, BYTE(1)); }
} init_indexes;
// 0xA8EB78 is GameOptionsClass::DetailLevel in the original; native storage
// starts at that option's default, the quality the drawing context uses.
int light_mode = -1, light_quality = 2;
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
    if (bytes_per_pixel != 2) throw std::invalid_argument("LightConvertClass requires 16-bit pixels");
    return bytes_per_pixel;
}
std::size_t light_shade_count(std::size_t shades) {
    // Shared constructor precondition, checked before base allocation.
    // Keep zero's existing normalization to one in the Convert base constructor.
    if (shades >= 2 && shades <= 13) throw std::invalid_argument("LightConvertClass cannot use 2..13 shades");
    return shades;
}

}
namespace game {
unsigned build_light_palette(const BytePalette* palette, WORD* destination, int red, int green, int blue, int unlit,
                             const BYTE* indexes, int mode) {
    return build_light_palette_row(palette, destination, red, green, blue, unlit, indexes, mode,
                                   LightConvertClass::UseMMX != 0);
}

namespace {
LightConvertClass* initialize_light_fields(LightConvertClass* self, const BytePalette* art, const BytePalette* screen,
                                           int r, int g, int b, bool skip, BYTE* indexes) {
    self->UsedPalette1 = reinterpret_cast<RGBClass*>(const_cast<BytePalette*>(art));
    self->UsedPalette2 = reinterpret_cast<RGBClass*>(const_cast<BytePalette*>(screen));
    self->IndexesToIgnore = indexes;
    self->RefCount = 0;
    self->Color1 = self->Color2 = TintStruct{r, g, b};
    self->Tinted = false;
    if (self->ShadeCount < 1) return self;
    if (!indexes) self->IndexesToIgnore = LightConvertClass::DefaultIndexes;
    if (LightConvertClass::LightMode == -1) LightConvertClass::LightMode = static_cast<int>(Drawing::ColorMode);
#ifdef RA2_IMAGE_GAME
    TintStruct tint;
    if (game::OriginalScenarioTint(tint)) {
#else
    // 0x555E50..0x556064: storm, then nuke/chrono, then dominator.
    const LightingStruct* lighting = nullptr;
    if (LightningStorm::Active)
        lighting = &ScenarioClass::Instance->IonLighting;
    else if (NukeFlash::IsFadingIn() || ChronoScreenEffect::Active())
        lighting = &ScenarioClass::Instance->NukeLighting;
    else if (PsyDom::Active())
        lighting = &ScenarioClass::Instance->DominatorLighting;
    if (lighting) {
        const auto scale = [](int value) { return std::bit_cast<std::int32_t>(std::uint32_t(value) * 1000u) / 100; };
        const TintStruct tint{scale(lighting->Tint.Red), scale(lighting->Tint.Green), scale(lighting->Tint.Blue)};
#endif
        self->Color1 = r == -1 ? TintStruct{1000, 1000, 1000} : TintStruct{r, g, b};
        self->UpdateColors(tint.Red, tint.Green, tint.Blue, true);
    } else
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
    if (red == -1) {
        red = color.Red;
        green = color.Green;
        blue = color.Blue;
    } else {
        red = std::clamp(red, 0, 2000);
        green = std::clamp(green, 0, 2000);
        blue = std::clamp(blue, 0, 2000);
        color = TintStruct{red, green, blue};
    }
    Tinted = tinted;
    BuildColorTable(*reinterpret_cast<const BytePalette*>(UsedPalette1), static_cast<WORD*>(FullColorData),
                    std::size_t(ShadeCount) * 256, ShadeCount, red, green, blue, IndexesToIgnore, 256, LightMode,
                    UseMMX != 0);
#if !defined(RA2_IMAGE_GAME)
    game::note_converter_tables(*this);
#endif
}

LightConvertClass* YRPP_FASTCALL LightConvertClass::InitLightConvert(int red, int green, int blue) {
#ifdef RA2_IMAGE_GAME
    auto* surface = DSurface::Primary;
    if (!surface) return nullptr;
#endif
    if (red == 1000 && green == 1000 && blue == 1000 && Array.Count) return Array.Items[0];
    const int shades = PrepareCellTint(red, green, blue, Quality);
    for (int i = 1; i < Array.Count; ++i) {
        auto* item = Array.Items[i];
        if (item->Color1.Red == red && item->Color1.Green == green && item->Color1.Blue == blue) return item;
    }
    // The original registers a failed allocation as null; every later reader
    // dereferences Array items, so a failure registers nothing.
    auto* item = static_cast<LightConvertClass*>(YRMemory::Allocate(sizeof(LightConvertClass)));
    if (!item) return nullptr;
    try {
#ifdef RA2_IMAGE_GAME
        new (item) LightConvertClass(const_cast<BytePalette*>(&FileSystem::ISOx_PAL),
                                     const_cast<BytePalette*>(&FileSystem::TEMPERAT_PAL), surface, red, green, blue,
                                     Array.Count != 0, nullptr, shades);
#else
        // Native hosts have no Primary surface; converters use 16-bit pixels.
        new (item) LightConvertClass(const_cast<BytePalette*>(&FileSystem::ISOx_PAL),
                                     const_cast<BytePalette*>(&FileSystem::TEMPERAT_PAL), 2, red, green, blue,
                                     Array.Count != 0, nullptr, shades);
#endif
    } catch (...) {
        YRMemory::Deallocate(item);
#ifdef RA2_IMAGE_GAME
        throw;
#else
        // Callers treat a missing converter as unbound; nothing propagates.
        return nullptr;
#endif
    }
    if (!Array.AddItem(item)) {
        item->~LightConvertClass();
        YRMemory::Deallocate(item);
        return nullptr;
    }
    return item;
}

LightConvertClass::LightConvertClass(BytePalette* art, BytePalette* screen, Surface* surface, int red, int green,
                                     int blue, bool skip, BYTE* indexes, std::size_t shades)
    : LightConvertClass(art, screen, surface->GetBytesPerPixel(), red, green, blue, skip, indexes, shades) {}

LightConvertClass::LightConvertClass(BytePalette* art, BytePalette* screen, int bytes_per_pixel, int red, int green,
                                     int blue, bool skip, BYTE* indexes, std::size_t shades)
    : ConvertClass(*art, *screen, light_pixel_depth(bytes_per_pixel), light_shade_count(shades), true) {
    game::initialize_light_fields(this, art, screen, red, green, blue, skip, indexes);
}

// The C++ base destructor owns the Convert buffers, blitters and registry entry.
LightConvertClass::~LightConvertClass() = default;
