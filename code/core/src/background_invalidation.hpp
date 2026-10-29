#pragma once

#include "yrpp/GeneralStructures.h"
#include "yrpp/BasicStructures.h"
#include <cstdint>

class ObjectClass;
class CellClass;

namespace game {
enum class BackgroundLayerMask : std::uint32_t {
    tiles = 0x1,
    overlays = 0x2,
    overlay_shadows = 0x4,
    terrain = 0x8,
    tile_shadows = 0x10,
    all = 0x1F
};
inline constexpr std::uint32_t BackgroundShroud = 0x20;
inline constexpr std::uint32_t BackgroundLighting = 0x40;
struct BackgroundDirtyCell {
    CellStruct cell;
    std::uint32_t layers;
};
struct BackgroundInvalidationView {
    const BackgroundDirtyCell* cells = nullptr;
    std::uint32_t cell_count = 0;
    const std::uintptr_t* objects = nullptr;
    std::uint32_t object_count = 0;
    bool all = false;
    bool global_lighting = false;
};
// Single world-thread producer/consumer. A view stays valid until the next take.
// Coordinates use the unraised world projection, independent of the camera.
void invalidate_background_area(const RectangleStruct& world_pixels, bool shroud) noexcept;
void invalidate_background_cell(const CellStruct& cell, std::uint32_t layers) noexcept;
void invalidate_background_object(const ObjectClass& object) noexcept;
void invalidate_background_all() noexcept;
void invalidate_background_shroud(const CellStruct& cell) noexcept;
void reset_background_invalidation_extent() noexcept;
void prepare_background_invalidation_extent() noexcept;
void invalidate_background_lighting(const CellStruct* cells, std::uint32_t count) noexcept;
bool take_background_invalidations(BackgroundInvalidationView& output) noexcept;
// Debug-only full-world comparison; no traversal when disabled.
void audit_background_invalidations(const BackgroundInvalidationView* consumed = nullptr) noexcept;
// Shared with the existing item path; neither helper consumes cached records.
void background_cell_records(const CellClass& cell, std::uint64_t& geometry, std::uint64_t& lighting) noexcept;
// Call after resource loading. Until an envelope is known, area invalidations
// conservatively request a full rebuild. Negative extents discard the envelope.
void set_background_invalidation_extent(int horizontal, int vertical) noexcept;
}
