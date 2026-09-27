#pragma once
#include <cstdint>

namespace game {
// Internal storage contract only. Original-game startup supplies EXE-owned
// arrays; standalone uses core-owned storage. No additional VoxLib fields.
// This interface does not load voxels.vpl. Normal YR rendering uses its prebuilt
// table; an explicit VoxLib(..., true) replaces shared data with generated data.
// Implement consumers/loaders only when the selected runtime actually calls
// them. Available storage does not by itself require additional VPL support.
struct VoxelPaletteStorage {
    std::uint8_t* colors; // 768 bytes
    // 32768-byte capacity; VPL reads layers x 256 bytes. The embedded generator
    // writes 32 x 256 entries, excluding index 0 in each layer.
    std::uint8_t* lighting;
    float* levels; // 32 factors used by the embedded-palette generator only
};
VoxelPaletteStorage GetVoxelPaletteStorage() noexcept;
void BindVoxelPaletteStorage(VoxelPaletteStorage storage) noexcept;
}
