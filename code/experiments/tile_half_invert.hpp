#pragma once
#include "yrpp/BasicStructures.h"
#include <array>
#include <cstdint>

namespace game {
// Borrowed arguments of the original TMP draw call; no owned game objects.
struct TileDrawArguments {
    void* tile_type;
    void* convert;
    int sub_tile;
    void* surface;
    int x, y;
    RectangleStruct clip;
    int level, intensity;
    bool use_z;
    int alternate;
    bool flat, flag16, flag17;
    int color;
};
using TileDrawCall = void (*)(void*, const TileDrawArguments&);
struct TileInvertScratch {
    // The target's shade remap produces 16-bit indexes into FullColorData.
    std::array<std::uint16_t, 65536> snapshot{}, inverted{};
    const void* source = nullptr;
    std::size_t count = 0;
    std::uint16_t mask = 0;
};
// Returns whether a visible left half was submitted. The callback retains the
// original TMP drawing, Z/Alpha, clipping, transparency and extra-image rules.
// The host serializes original rendering. No callback may retain this palette.
bool draw_tile_half_inverted(const TileDrawArguments& arguments, void*& color_table,
    int bytes_per_pixel, int shade_count, std::uint16_t rgb_mask,
    TileInvertScratch& scratch, TileDrawCall draw, void* context);
}
