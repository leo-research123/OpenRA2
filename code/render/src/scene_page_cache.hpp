#pragma once
#include <array>
#include <cstdint>

namespace render {
// A page may replace only an ordered prefix of the scene. Original SHP depth
// restarts at the visible edge and quantizes screen rows; keep those records
// in the resident raster stream. Destination-dependent blends stay there too.
inline bool scene_page_cacheable(const std::array<std::int32_t, 12>& w, int viewport_height) noexcept {
    const auto mode = std::uint32_t(w[6]) & 0xFFFFFFu;
    if ((mode & 0x10000u) || ((mode >> 8) & 0xFFu) != 0) return false;
    if ((mode & 0xFFu) == 0 || w[9] == 0) return true;
    // Wrapped TMP Z is camera-independent only when no visible source sample
    // can cross either 16-bit boundary. For source row r and screen row t,
    // pre-wrap Z = world_base + world_y + r - t + texel_z (0..255).
    const auto base = std::int64_t(w[5]) + w[4];
    const int height = int(std::uint32_t(w[2]) >> 16);
    return height > 0 && viewport_height > 0 && base >= viewport_height - 1 && base + height - 1 + 255 <= 0xFFFF;
}
}
