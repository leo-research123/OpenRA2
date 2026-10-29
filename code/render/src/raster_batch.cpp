#include "raster_batch.hpp"

namespace render {
namespace {
constexpr int PlainWarpFlag = 0x40000, OriginalDepthFlag = 0x10000, AuxiliaryDepthFlag = 0x20000;
constexpr std::int32_t DestinationLimit = std::int32_t(1) << 30;
}
RasterInstance raster_instance(const PixelParameters& p, std::int32_t source, std::int32_t palette) noexcept {
    return {source,
            palette,
            std::int32_t(std::uint32_t(p[2]) | (std::uint32_t(p[3]) << 16)),
            p[4],
            p[5],
            p[6],
            p[7],
            p[8],
            p[9],
            std::int32_t(std::uint32_t(p[13]) | (std::uint32_t(p[19]) << 24)),
            std::int32_t(std::uint32_t(p[10]) | (std::uint32_t(p[17]) << 1)),
            p[11],
            p[12],
            p[14],
            p[15],
            p[18]};
}
bool raster_packet(const PixelParameters& p, int width, int height, int& variant) {
    constexpr int Shape = 0, Copy = 1, Fill = 2, Indexed = 3;
    const int kind = p[19];
    if ((kind != Shape && kind != Copy && kind != Fill && kind != Indexed) || p[0] != width || p[1] != height ||
        p[16] != 0 || (p[13] & PlainWarpFlag) || p[2] < 0 || p[3] < 0 || p[2] > 0xFFFF || p[3] > 0x7FFF ||
        p[4] <= -DestinationLimit || p[4] >= DestinationLimit || p[5] <= -DestinationLimit || p[5] >= DestinationLimit)
        return false;
    // Copies and fills ignore depth and replace the color.
    if (kind == Copy || kind == Fill) {
        variant = 0;
        return p[13] == 0;
    }
    const int depth = p[13] & 0xFF, blend = (p[13] >> 8) & 0xFF;
    if (blend >= RasterBlends || depth > 2) return false;
    RasterDepth state = RasterDepth::none;
    if (depth != 0) {
        // The original SHP gradient divides by its row limit.
        if ((p[13] & OriginalDepthFlag) && !(p[13] & AuxiliaryDepthFlag) && ((p[15] >> 16) & 0xFF) == 0) return false;
        const bool strict = (p[13] & OriginalDepthFlag) != 0, write = depth == 1;
        state = strict ? (write ? RasterDepth::less_write : RasterDepth::less)
                       : (write ? RasterDepth::less_equal_write : RasterDepth::less_equal);
    }
    variant = static_cast<int>(state) * RasterBlends + blend;
    return true;
}
}
