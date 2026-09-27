#include "images/voxel_palette.hpp"

namespace {
#ifdef RA2_VOXEL_GAME
game::VoxelPaletteStorage storage{}; // bound by compat before entering VoxLib
#else
// Empty storage only; VXL/HVA loading does not initialize the original VPL map.
// Do not add a VPL loader unless the selected runtime will actually call it.
std::uint8_t colors[768]{};
std::uint8_t lighting[32768]{};
float levels[32]{};
game::VoxelPaletteStorage storage{colors, lighting, levels};
#endif
}

namespace game {
VoxelPaletteStorage GetVoxelPaletteStorage() noexcept { return storage; }
void BindVoxelPaletteStorage(VoxelPaletteStorage value) noexcept { storage = value; }
}
