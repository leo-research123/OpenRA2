#include "background_invalidation.hpp"
#include <algorithm>
#include <array>
#include <cstddef>

namespace game {
namespace {
constexpr int Span = 512;
constexpr std::size_t CellCapacity = Span * Span;
constexpr std::size_t ObjectCapacity = 4096;
struct DirtySet {
    std::array<std::uint32_t, CellCapacity> indices{};
    std::array<BackgroundDirtyCell, CellCapacity> cells{};
    std::array<std::uintptr_t, ObjectCapacity> objects{};
    std::array<std::uintptr_t, ObjectCapacity * 2> object_slots{};
    std::uint32_t cell_count = 0, object_count = 0;
    bool all = false, lighting = false;
    void clear() noexcept {
        for (std::uint32_t i = 0; i < cell_count; ++i) {
            const auto& c = cells[i].cell;
            indices[std::size_t(c.Y) * Span + c.X] = 0;
        }
        if (object_count) object_slots.fill(0);
        cell_count = object_count = 0;
        all = lighting = false;
    }
};
// Bounded storage also covers a host that never consumes invalidations.
std::array<DirtySet, 2> sets;
unsigned active = 0;
int extent_x = -1, extent_y = -1;
std::int64_t floor_div(std::int64_t n, std::int64_t d) noexcept { return n / d - (n % d < 0 ? 1 : 0); }
}
void invalidate_background_shroud(const CellStruct& cell) noexcept {
    // Occlusion reads the eight neighboring cells, including offscreen ones.
    for (int y = std::max(0, int(cell.Y) - 1); y <= std::min(511, int(cell.Y) + 1); ++y)
        for (int x = std::max(0, int(cell.X) - 1); x <= std::min(511, int(cell.X) + 1); ++x)
            invalidate_background_cell({short(x), short(y)}, BackgroundShroud);
}
void invalidate_background_all() noexcept { sets[active].all = true; }
void set_background_invalidation_extent(int horizontal, int vertical) noexcept {
    extent_x = horizontal;
    extent_y = vertical;
    invalidate_background_all();
}
void invalidate_background_cell(const CellStruct& cell, std::uint32_t layers) noexcept {
    auto& set = sets[active];
    layers &= 0x7Fu;
    if (!layers || set.all) return;
    if (cell.X < 0 || cell.Y < 0 || cell.X >= Span || cell.Y >= Span) {
        invalidate_background_all();
        return;
    }
    auto& index = set.indices[std::size_t(cell.Y) * Span + cell.X];
    if (!index) {
        index = ++set.cell_count;
        set.cells[index - 1] = {cell, 0};
    }
    set.cells[index - 1].layers |= layers;
}
void invalidate_background_area(const RectangleStruct& rectangle, bool shroud) noexcept {
    if (rectangle.Width <= 0 || rectangle.Height <= 0 || sets[active].all) return;
    if (extent_x < 0 || extent_y < 0) {
        invalidate_background_all();
        return;
    }
    const auto left = std::int64_t(rectangle.X) - extent_x;
    // Raised cells appear above their unraised position, so only the bottom
    // takes the vertical extent; the top keeps one cell.
    const auto top = std::int64_t(rectangle.Y) - std::min(extent_y, 30);
    const auto right = std::int64_t(rectangle.X) + rectangle.Width + extent_x;
    const auto bottom = std::int64_t(rectangle.Y) + rectangle.Height + extent_y;
    // Inverse of x = 30*(cx-cy), y = 15*(cx+cy). Round outwards,
    // including cell footprints touching the rectangle's boundary.
    const auto x0 = std::clamp<std::int64_t>(floor_div(left + 2 * top, 60) - 1, 0, Span);
    const auto x1 = std::clamp<std::int64_t>(floor_div(right + 2 * bottom, 60) + 1, -1, Span - 1);
    const auto y0 = std::clamp<std::int64_t>(floor_div(2 * top - right, 60) - 1, 0, Span);
    const auto y1 = std::clamp<std::int64_t>(floor_div(2 * bottom - left, 60) + 1, -1, Span - 1);
    const auto layers = std::uint32_t(BackgroundLayerMask::all) | (shroud ? BackgroundShroud : 0u);
    // The cell range bounds a diagonal band; keep the cells whose footprint
    // (60x30 around the projected point, with half a cell of slack) touches
    // the rectangle.
    for (auto y = y0; y <= y1; ++y)
        for (auto x = x0; x <= x1; ++x) {
            const auto sx = 30 * (x - y), sy = 15 * (x + y);
            if (sx + 60 < left || sx - 60 > right || sy + 45 < top || sy - 15 > bottom) continue;
            invalidate_background_cell({static_cast<short>(x), static_cast<short>(y)}, layers);
        }
}
void invalidate_background_object(const ObjectClass& object) noexcept {
    auto& set = sets[active];
    if (set.all) return;
    const auto id = reinterpret_cast<std::uintptr_t>(&object);
    auto slot = (id >> 3) % set.object_slots.size();
    while (set.object_slots[slot]) {
        if (set.object_slots[slot] == id) return;
        slot = (slot + 1) % set.object_slots.size();
    }
    if (set.object_count == ObjectCapacity) {
        invalidate_background_all();
        return;
    }
    set.object_slots[slot] = id;
    set.objects[set.object_count++] = id;
}
void invalidate_background_lighting(const CellStruct* cells, std::uint32_t count) noexcept {
    if (!count) {
        sets[active].lighting = true;
        return;
    }
    if (!cells) {
        invalidate_background_all();
        return;
    }
    for (std::uint32_t i = 0; i < count; ++i)
        invalidate_background_cell(cells[i], BackgroundLighting);
}
bool take_background_invalidations(BackgroundInvalidationView& output) noexcept {
    auto& set = sets[active];
    output = {set.cells.data(), set.cell_count, set.objects.data(), set.object_count, set.all, set.lighting};
    active ^= 1;
    sets[active].clear();
    return output.all || output.global_lighting || output.cell_count || output.object_count;
}
}
