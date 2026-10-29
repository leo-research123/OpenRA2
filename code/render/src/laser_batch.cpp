#include "laser_batch.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace render {
void append_laser_batches(std::vector<LaserParameters>& words, int width, int height, std::vector<LaserBatch>& batches,
                          std::vector<std::uint32_t>& pixel_epochs, std::uint32_t& epoch) {
    if (width <= 0 || height <= 0) throw std::invalid_argument("laser batch viewport");
    const auto count = words.size();
    const auto next_epoch = [&] {
        if (++epoch == 0) {
            std::fill(pixel_epochs.begin(), pixel_epochs.end(), 0);
            epoch = 1;
        }
    };
    const auto finish = [&](std::size_t first, std::size_t end) {
        if (first == end) return;
        // Shader word addressing uses signed 32-bit indices.
        if (words.size() >= std::size_t(std::numeric_limits<std::int32_t>::max() / 20))
            throw std::length_error("laser batch parameters");
        LaserParameters header{width, height, int(end - first), 1, int(first)};
        header[19] = 11;
        batches.push_back({first, end, std::int32_t(words.size())});
        words.push_back(header);
    };
    std::size_t first = count;
    for (std::size_t i = 0; i < count; ++i) {
        const auto& p = words[i];
        const bool pair = p[19] == 10;
        // Other packets can read or change color, Z, or lighting: never cross
        // them. The one-pixel size is required by the laser packet contract.
        if ((p[19] != 9 && !pair) || p[0] != width || p[1] != height || p[2] != 1 || p[3] != 1) {
            if (first != count) {
                finish(first, i);
                first = count;
            }
            continue;
        }
        const int x = p[4], y = p[5];
        const std::int64_t nx = std::int64_t(x) + (pair ? p[17] : 0), ny = std::int64_t(y) + (pair ? p[18] : 0);
        if (x < 0 || y < 0 || x >= width || y >= height || nx < 0 || ny < 0 || nx >= width || ny >= height) {
            if (first != count) {
                finish(first, i);
                first = count;
            }
            continue;
        }
        const auto a = std::size_t(y) * width + x, b = std::size_t(ny) * width + std::size_t(nx);
        if (first == count) {
            pixel_epochs.resize(std::size_t(width) * height, 0);
            next_epoch();
            first = i;
        } else if (i - first >= 65536 || pixel_epochs[a] == epoch || pixel_epochs[b] == epoch) {
            // Bound the dispatch to 1024 workgroups even on a large viewport.
            finish(first, i);
            next_epoch();
            first = i;
        }
        pixel_epochs[a] = pixel_epochs[b] = epoch;
    }
    if (first != count) finish(first, count);
}
}
