#pragma once
// Private host preparation, not a core ABI. It decodes indices/height metadata
// only; palette lookup, clipping, lighting and depth writes execute on the GPU.
#include "api/type_drawing.hpp"
#include "render/frame.hpp"
#include <array>
#include <vector>
namespace game {
struct TypeGpuTarget;
struct TypeGpuPacket;
DrawingStatus prepare_type_laser_parameters(const RasterDrawingRequest&, const TypeGpuTarget&, TypeGpuPacket&) noexcept;
DrawingStatus prepare_type_particle_parameters(const RasterDrawingRequest&, const TypeGpuTarget&,
                                               TypeGpuPacket&) noexcept;
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
DrawingStatus prepare_type_light_test_parameters(const RasterDrawingRequest&, const TypeGpuTarget&,
                                                 TypeGpuPacket&) noexcept;
DrawingStatus prepare_type_shape(const ShapeDrawingRequest&, const TypeGpuTarget&, TypeGpuPacket&) noexcept;
// Mode word [13]: low byte depth (0 none, 1 read/write, 2 read),
// next byte ShapeBlendMode; bits 16/17 select original SHP/auxiliary SHA.
// Bit 18 executes plain warp destination reads in original row/pixel order.
// [14] is base depth; original SHP [15] packs phase, step, limit, signed delta,
// and [18] is the first visible row. Existing 20-word stride stays.
DrawingStatus prepare_type_shape_parameters(const ShapeDrawingRequest&, const TypeGpuTarget&, TypeGpuPacket&) noexcept;
// The image facts parameter preparation reads, extracted while the request's
// resources are borrowed. Preparing from a description dereferences nothing,
// so a host may re-prepare a held request later (e.g. at another camera).
struct TypeShapeMeta {
    RectangleStruct body{};  // frame bounds, narrowed by the auxiliary image
    Point2D auxiliary_offset{};
    int width = 0, height = 0;  // SHP size for centered (0x200) requests
    bool rle = false;
};
DrawingStatus describe_type_shape(const ShapeDrawingRequest&, TypeShapeMeta&) noexcept;
DrawingStatus prepare_type_shape_parameters(const ShapeDrawingRequest&, const TypeShapeMeta&, const TypeGpuTarget&,
                                            TypeGpuPacket&) noexcept;
struct TypeTileMeta {
    unsigned flags = 0;  // TMPImage flags: 1 extra image, 2 depth
    std::int64_t extra_x = 0, extra_y = 0;
    int extra_width = 0, extra_height = 0;
};
DrawingStatus describe_type_tile(const TileDrawingRequest&, TypeTileMeta&) noexcept;
DrawingStatus prepare_type_tile_parameters(const TileDrawingRequest&, const TypeTileMeta&, const TypeGpuTarget&,
                                           TypeGpuPacket& base, TypeGpuPacket& extra) noexcept;
// Immutable resource/mode program, compiled once; per-frame emission only
// applies placement, clip, light and depth values to its instance words.
struct TypeShapeProgram {
    TypeShapeMeta meta;
    std::array<std::int32_t, 20> words{};
    // Original depth with the SHA image of a compressed frame (AUXILIARY_DEPTH_FLAG and a depth mode).
    bool auxiliary_depth() const { return (words[13] & 0x20000) != 0 && (words[13] & 0xFF) != 0; }
};
DrawingStatus compile_type_shape(const ShapeDrawingRequest&, const TypeShapeMeta&, TypeShapeProgram&) noexcept;
DrawingStatus emit_type_shape(const ShapeDrawingRequest&, const TypeShapeProgram&, const TypeGpuTarget&,
                              TypeGpuPacket&) noexcept;
// Final instance emission; resource placement is filled by the registry after
// visibility succeeds. This writes no intermediate 20-word packet.
// A request whose program reads auxiliary depth (program.auxiliary_depth()) names the
// frame table index of its SHA frame; the instance then carries it in word 14 and the
// frame offset (x | y << 16) in word 15, which the gradient and first row use otherwise.
DrawingStatus emit_type_shape_instance(const ShapeDrawingRequest&, const TypeShapeProgram&, const TypeGpuTarget&,
                                       render::Instance&, std::uint32_t auxiliary_entry = 0) noexcept;
DrawingStatus decode_type_shape(const ShapeDrawingRequest&, std::vector<std::uint32_t>&) noexcept;
// Every frame of one SHP decoded once as 8-bit planes (the resident storage of a
// shape sequence). A colour plane holds the palette index of each pixel of the
// frame's original bounds (0 is transparent), row after row. A compressed
// (RLE) frame also has a prefix plane of the same size: at a texel inside a
// transparent run it is the run length left from that texel (0 elsewhere), the
// distance the original RLE clipping advances the destination but not the
// auxiliary pointer, so the GPU can reproduce that shift for any clip. Planes
// of a frame are placed one after another in `bytes`; a frame that cannot be
// decoded keeps its status and occupies no bytes, and drawing it reports that
// status, as a lone decode_type_shape would.
inline constexpr std::uint32_t TypeShapeNoPlane = 0xFFFFFFFFu;
struct TypeShapeSequenceFrame {
    RectangleStruct bounds{};  // original frame bounds (a plane is bounds.Width * bounds.Height bytes)
    DrawingStatus status = DrawingStatus::unavailable;
    std::uint32_t color = 0;                  // byte offset of the colour plane in TypeShapeSequence::bytes
    std::uint32_t prefix = TypeShapeNoPlane;  // byte offset of the prefix plane, or TypeShapeNoPlane
    bool rle = false;
};
struct TypeShapeSequence {
    std::vector<TypeShapeSequenceFrame> frames;
    std::vector<std::uint8_t> bytes;
};
// prefix=false omits the prefix planes (images that are never drawn with a SHA image).
DrawingStatus decode_type_shape_sequence(SHPStruct*, TypeShapeSequence&, bool prefix = true) noexcept;
// The signed depth deltas of an auxiliary SHA image: one plane per frame, raw
// bytes of the frame's bounds. Compressed frames are unsupported, as for
// describe_type_shape.
DrawingStatus decode_type_shape_deltas(SHPStruct*, TypeShapeSequence&) noexcept;
DrawingStatus prepare_type_tile(const TileDrawingRequest&, const TypeGpuTarget&, TypeGpuPacket& base,
                                TypeGpuPacket& extra) noexcept;
// Production cache: immutable resource decoding is independent of visibility,
// target size, palette and camera. Per-frame parameter preparation allocates no texels.
DrawingStatus decode_type_tile(const TileDrawingRequest&, std::vector<std::uint32_t>& base,
                               std::vector<std::uint32_t>& extra) noexcept;
DrawingStatus prepare_type_tile_parameters(const TileDrawingRequest&, const TypeGpuTarget&, TypeGpuPacket& base,
                                           TypeGpuPacket& extra) noexcept;
DrawingStatus prepare_type_indexed_parameters(const IndexedDrawingRequest&, const TypeGpuTarget&,
                                              TypeGpuPacket&) noexcept;
bool type_gpu_packet_visible(const TypeGpuPacket&) noexcept;
DrawingStatus prepare_type_mask(const MaskDrawingRequest&, const TypeGpuTarget&, TypeGpuPacket&) noexcept;
DrawingStatus describe_type_shape_strip(const ShapeStripDrawingRequest&, TypeShapeMeta&) noexcept;
DrawingStatus decode_type_shape_strip(const ShapeStripDrawingRequest&, std::vector<std::uint32_t>&) noexcept;
}
