// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from pinned Renegade WWLib convert.cpp, rgb.cpp, hsv.cpp,
// palette.cpp, with YRpp ConvertClass fields and YR calibration.
#include "yrpp/Memory.h"
#include "yrpp/ArrayClasses.h"
#include "ConvertClassHelpers.hpp"
#include "BlitterVariants.hpp"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include <algorithm>
#include <bit>
#include <new>
#include <stdexcept>
#include <utility>

namespace game {
ConvertClass* construct_convert(void* storage, const BytePalette& art, const BytePalette& screen, DSurface* surface,
                                int shades, bool skip) {
    try {
        return new (storage) ConvertClass(art, screen, surface, std::size_t(std::max(shades, 1)), skip);
    } catch (...) {
        YRMemory::Deallocate(storage);
        throw;
    }
}
namespace {
template <class T, class... Args>
T* create_blitter(Args&&... args) {
    auto* memory = YRMemory::Allocate(sizeof(T));
    if (!memory) return nullptr;
    try {
        return new (memory) T(std::forward<Args>(args)...);
    } catch (...) {
        YRMemory::Deallocate(memory);
        throw;
    }
}

}

void initialize_blitters(ConvertClass* self) {
    // Rebuilds (including retries after a missing first blitter) release previous
    // instances before populating the same slots again.
    clear_blitters(self);
#define RA2_BLITTER(array, index, type, ...) self->array[index] = create_blitter<type>(__VA_ARGS__)
#include "ConvertClassBlitterRecipes.inc"
#undef RA2_BLITTER
}

}

ConvertClass::ConvertClass(const BytePalette& art, const BytePalette& screen, DSurface* surface, std::size_t shades,
                           bool skip)
    : ConvertClass(art, screen, surface->GetBytesPerPixel(), shades, skip) {}

ConvertClass::ConvertClass(const BytePalette& art, const BytePalette& screen, int bytes_per_pixel, std::size_t shades,
                           bool skip) {
    if (bytes_per_pixel != 1 && bytes_per_pixel != 2)
        throw std::invalid_argument("ConvertClass requires 8- or 16-bit pixels");
    if (shades > std::size_t(INT32_MAX) / 512) throw std::length_error("ConvertClass shade table is too large");
    try {
        game::initialize_convert_storage(this, art, screen, bytes_per_pixel, int(shades), skip);
        if (!skip) game::initialize_blitters(this);
        if (!Array.AddItem(this)) throw std::bad_alloc();
    } catch (...) {
        // Initialization sets every owned pointer before the first allocation.
        game::destroy_convert(this);
        throw;
    }
}
