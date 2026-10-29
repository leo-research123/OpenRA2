#pragma once
#include "render/frame.hpp"
#include <array>
#include <vector>
namespace render {
// Optional original-software reference input. All byte planes and tables are
// owned copies; original object layouts are reconstructed only by that consumer.
enum class ReferenceKind : std::uint32_t { shape, tile, raster, indexed };
struct ReferenceWavePixel { Point destination{}, sample{}; std::int32_t strength = 0; };
struct ReferenceDraw {
    ReferenceKind kind{};
    Point position{};
    Rect clip{};
    std::array<std::int32_t,32> words{};
    int shades = 1;
    std::vector<std::uint16_t> palette;
    std::vector<std::uint8_t> image, auxiliary, tile;
    std::vector<std::uint16_t> pixels;
    std::vector<std::uint32_t> indexed;
    std::vector<ReferenceWavePixel> wave;
};
struct ReferenceFrame { std::vector<ReferenceDraw> draws; };
}
