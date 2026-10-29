#pragma once
// Private renderer scheduling; original pixel arithmetic and order are unchanged.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace render {
using PixelParameters = std::array<std::int32_t, 20>;
// Must match PIXEL_BIN_SIZE in map_draw.comp; a multiple of its 8 by 8 workgroup.
inline constexpr int PixelBinSize = 32;
struct PixelBatch {
    std::size_t first = 0, end = 0;
    std::int32_t header = 0;
    // Bin rectangle covered by the dispatch; zero when no member is visible.
    int columns = 0, rows = 0;
};
struct PixelBatchOptions {
    // Packets before split form the published background; no run crosses it.
    std::size_t split = 0;
};
// Appends kind-13 headers for consecutive pixel-local packets. Each member may
// read and write only its own color pixel and circular Z/ABuffer entry, so one
// invocation per pixel can apply the bin's members in submission order.
// eligible[i] is the caller's verdict for state it alone knows (lighting and
// laser batches); packet words are validated here. The schedule receives the
// per-packet resource words (zero, filled by the caller), then the spatial bins.
bool pixel_local_packet(const PixelParameters&, int width, int height);
struct PixelRunScratch {
    std::vector<std::array<int, 4>> bounds;
    std::vector<std::int32_t> cursors;
};
// The producer has already formed this ordered, pixel-local run. Resource
// entries (including one reserved header pair per run) precede appended bins.
PixelBatch append_pixel_run(std::vector<PixelParameters>&, std::size_t first, std::size_t end,
                            std::vector<std::int32_t>& schedule, PixelRunScratch&);
void append_pixel_batches(std::vector<PixelParameters>& words, const std::vector<std::uint8_t>& eligible, int width,
                          int height, const PixelBatchOptions& options, std::vector<PixelBatch>& batches,
                          std::vector<std::int32_t>& schedule);
}
