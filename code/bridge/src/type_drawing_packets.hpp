#pragma once
// Private host preparation, not a core ABI. It decodes indices/height metadata
// only; palette lookup, clipping, lighting and depth writes execute on the GPU.
#include "api/type_drawing.hpp"
#include <array>
#include <vector>
namespace game {
struct TypeGpuTarget;
struct TypeGpuPacket;
DrawingStatus prepare_type_particle_parameters(const RasterDrawingRequest&,const TypeGpuTarget&,TypeGpuPacket&) noexcept;
struct TypeGpuPacket {
    // Matched by res://shaders/map_draw.glsl; 20 signed 32-bit words.
    std::array<std::int32_t, 20> parameters{};
    // index:0..7, resource Z:8..15, occupied:16. Auxiliary RLE texels also
    // retain transparent-prefix overhang in bits 17..24 (including empty texels).
    std::vector<std::uint32_t> texels;
};
struct TypeGpuTarget {
    int width = 0, height = 0, shade_count = 1;
    int depth_bounds_y = 0, depth_max = 65535;
    // RLE without the original Z state repeats the first lighting scanline.
    bool shape_z_state = false;
};
DrawingStatus prepare_type_shape(const ShapeDrawingRequest&, const TypeGpuTarget&,
    TypeGpuPacket&) noexcept;
// Mode word [13]: low byte depth (0 none, 1 read/write, 2 read),
// next byte ShapeBlendMode; bits 16/17 select original SHP/auxiliary SHA.
// [14] is base depth; original SHP [15] packs phase, step, limit, signed delta,
// and [18] is the first visible row. Existing 20-word stride stays.
DrawingStatus prepare_type_shape_parameters(const ShapeDrawingRequest&, const TypeGpuTarget&,
    TypeGpuPacket&) noexcept;
DrawingStatus decode_type_shape(const ShapeDrawingRequest&, std::vector<std::uint32_t>&) noexcept;
DrawingStatus prepare_type_tile(const TileDrawingRequest&, const TypeGpuTarget&,
    TypeGpuPacket& base, TypeGpuPacket& extra) noexcept;
// Production cache: immutable resource decoding is independent of visibility,
// target size, palette and camera. Per-frame parameter preparation allocates no texels.
DrawingStatus decode_type_tile(const TileDrawingRequest&, std::vector<std::uint32_t>& base,
    std::vector<std::uint32_t>& extra) noexcept;
DrawingStatus prepare_type_tile_parameters(const TileDrawingRequest&, const TypeGpuTarget&,
    TypeGpuPacket& base, TypeGpuPacket& extra) noexcept;
DrawingStatus prepare_type_indexed_parameters(const IndexedDrawingRequest&,const TypeGpuTarget&,TypeGpuPacket&) noexcept;
bool type_gpu_packet_visible(const TypeGpuPacket&) noexcept;
}
