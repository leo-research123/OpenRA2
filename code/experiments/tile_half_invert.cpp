#include "experiments/tile_half_invert.hpp"
#include <algorithm>
#include <cstring>
#include <limits>

namespace game {
bool draw_tile_half_inverted(const TileDrawArguments& input, void*& color_table,
        int bytes_per_pixel, int shade_count, std::uint16_t rgb_mask,
        TileInvertScratch& scratch, TileDrawCall draw, void* context) {
    if (!draw) return false;
    if (!color_table || bytes_per_pixel != 2 || shade_count < 1 || shade_count > 256
            || !rgb_mask || input.clip.Width <= 0 || input.clip.Height <= 0) {
        draw(context, input);
        return false;
    }
    // gamemd 547CF0 draws a 60-pixel-wide TMP diamond. Split in its local
    // coordinates, including when the viewport clips off the tile's left edge.
    const auto split = std::int64_t(input.x) + 30;
    const auto begin = std::int64_t(input.clip.X);
    const auto end = begin + input.clip.Width;
    const auto left_end = std::clamp(split, begin, end);
    if (left_end <= begin || left_end > std::numeric_limits<int>::max()) {
        draw(context, input);
        return false;
    }
    auto part = input;
    if (left_end < end) {
        part.clip.X = static_cast<int>(left_end);
        part.clip.Width = static_cast<int>(end - left_end);
        draw(context, part);
    }
    const auto count = std::size_t(shade_count) * 256;
    if (scratch.source != color_table || scratch.count != count || scratch.mask != rgb_mask
            || std::memcmp(scratch.snapshot.data(), color_table, count * sizeof(std::uint16_t))) {
        const auto* colors = static_cast<const std::uint16_t*>(color_table);
        for (std::size_t i = 0; i < count; ++i) {
            scratch.snapshot[i] = colors[i];
            scratch.inverted[i] = colors[i] ^ rgb_mask;
        }
        scratch.source = color_table;
        scratch.count = count;
        scratch.mask = rgb_mask;
    }
    // Temporarily borrow the original Convert's table slot, without changing
    // its owned allocation, blitters, PaletteData, or construction/lifetime.
    struct Restore {
        void*& slot;
        void* original;
        ~Restore() { slot = original; }
    } restore{color_table, color_table};
    color_table = scratch.inverted.data();
    part = input;
    part.clip.Width = static_cast<int>(left_end - begin);
    draw(context, part);
    return true;
}
}
