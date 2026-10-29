#pragma once
#include <cstdint>
#include "yrpp/GeneralStructures.h"
namespace game {
class VoxelImageHandle;
// Immutable core data borrowed only during the synchronous drawing callback.
// Hosts copy views before returning; identities are never reused in a process.
struct VoxelBufferView {
    std::uint64_t identity = 0;
    const std::uint32_t* words = nullptr;
    std::uint32_t count = 0;
};
// Source: column count, sample count, pairs (first sample, count), then pairs
// (x | y<<8 | z<<16 | color<<24, normal). Columns follow original traversal.
// Parameters: start XYZ, X/Y/Z step vectors, truncated center Z, packed
// XYZ sizes, corner, reserved. Setup follows original CPU precision rules.
struct VoxelSectionView {
    VoxelBufferView source;
    std::int32_t parameters[16]{};
};
struct VoxelLayerView {
    const VoxelImageHandle* image = nullptr;
    Point2D position{};
    RectangleStruct clip{};
};
enum class VoxelImageKind : std::uint32_t { raster, shadow, composite };
struct VoxelImageView {
    std::uint64_t identity = 0;
    VoxelImageKind kind = VoxelImageKind::raster;
    int width = 0, height = 0, left = 0, top = 0;
    bool buffered = false;
    const VoxelSectionView* sections = nullptr;
    std::uint32_t section_count = 0;
    const std::uint32_t* lighting = nullptr;
    VoxelBufferView palette;
    // Shadow: sx, sy, xx, xy, yx, yy, nx, ny, mask offset x/y.
    const std::int32_t* shadow = nullptr;
    VoxelBufferView columns;
    const VoxelImageHandle* mask = nullptr;
    const VoxelLayerView* layers = nullptr;
    std::uint32_t layer_count = 0;
    // Populated only by the explicit GPU differential diagnostic.
    const std::uint32_t* reference_pixels = nullptr;
    std::uint32_t reference_count = 0;
};
// No ownership transfer and no exceptions across this boundary.
bool read_voxel_image(const VoxelImageHandle&, VoxelImageView&) noexcept;
std::uint64_t voxel_image_identity(const VoxelImageHandle&) noexcept;
}
