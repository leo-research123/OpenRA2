#pragma once
// Private renderer scheduling: which packets map_raster.vert/.frag draws with
// fixed-function depth and blending instead of map_draw.comp.
#include "pixel_batch.hpp"
#include "render/frame.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace render {
// Pipeline state of a rasterized packet: depth (none; strict test with or
// without write; accepting equal depth with or without write) by blend
// (palette, shadow, 25/50/75% transparent). Index = depth * RasterBlends + blend.
enum class RasterDepth : int { none, less_write, less, less_equal_write, less_equal };
inline constexpr int RasterBlends = 5;
inline constexpr int RasterVariants = 5 * RasterBlends;
// True when map_raster.vert/.frag draws this packet: kind 0 or 3 with any blend, or
// a kind 1 copy / kind 2 fill; row origin 0 and no plain warp. The D32F
// attachment holds Z / 65536 and clamps Z the original would wrap. Compute
// paths never write Z, so every Z writer the core submits must pass. variant
// receives the pipeline state index.
// Compact, ordered foreground image instance. Source/palette offsets are
// embedded so raster draws need no per-frame resource schedule. Shader mode 2.
using RasterInstance = Instance;
// Packed draw mode bit (word 13 of a packet, low 24 bits of instance word 9): the
// kind 0 source is a shape sequence frame, 8-bit planes named by a frame table
// entry (MapSequenceTable) instead of 32-bit texels named by a resource slot.
// The packet's source word is then the entry index; an auxiliary-depth packet
// carries the SHA frame's entry in word 15 and its frame offset (x | y << 16) in
// word 18. Matches SHAPE_PLANES_FLAG in map_raster.vert/.frag and map_draw.comp.
inline constexpr int ShapePlanesFlag = 0x80000;
RasterInstance raster_instance(const PixelParameters&, std::int32_t source, std::int32_t palette) noexcept;
bool raster_packet(const PixelParameters&, int width, int height, int& variant);
}
