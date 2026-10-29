#pragma once
// Private renderer scheduling; original line geometry and pixel values are unchanged.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace render {
using LaserParameters = std::array<std::int32_t, 20>;
struct LaserBatch {
    std::size_t first = 0, end = 0;
    std::int32_t header = 0;
};
// Appends kind-11 headers for consecutive laser packets with disjoint color
// read/write footprints. Z and ABuffer are read-only throughout these batches.
// Reuse the pixel epochs between frames to avoid clearing a viewport-sized map.
void append_laser_batches(std::vector<LaserParameters>& words, int width, int height, std::vector<LaserBatch>& batches,
                          std::vector<std::uint32_t>& pixel_epochs, std::uint32_t& epoch);
}
