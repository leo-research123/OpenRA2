#pragma once

#include <cstdint>
#include "yrpp/GeneralStructures.h"

class ConvertClass;
class Surface;
struct SHPStruct;
struct TMPStruct;
struct TMPImage;

namespace game {
// Backend-owned identities. The core never dereferences these, and never owns
// a texture, a Surface, a device, or a color-conversion cache through them.
struct DrawingTargetHandle;
struct DrawingPaletteHandle;

enum class DrawingStatus : std::uint32_t {
    drawn, skipped, unavailable, unsupported, invalid_argument, backend_failure
};

enum class ShapeDepthMode : std::uint32_t { legacy,none,read,read_write };
enum class ShapeBlendMode : std::uint32_t { palette,shadow,translucent25,translucent50,translucent75 };
struct ShapeDrawingRequest {
    DrawingTargetHandle* target = nullptr;
    const DrawingPaletteHandle* palette = nullptr;
    SHPStruct* image = nullptr;
    int frame = 0;
    Point2D position{};
    RectangleStruct clip{};
    // Original drawing semantics, NOT an instruction to use CPU Blitters.
    std::uint32_t flags = 0;
    int depth_adjustment = 0;
    int gradient = 0;
    int intensity = 1000;
    // Original Draw_Shape packed tint; only the selected tinted blitters use it.
    int tint = 0;
    // Same 16-bit depth space as TMP; lower values are closer.
    ShapeDepthMode depth_mode=ShapeDepthMode::legacy;
    ShapeBlendMode blend_mode=ShapeBlendMode::palette;
    int absolute_depth=0;
    // Original auxiliary SHA sampling. Bytes are signed depth deltas; borrowed
    // with the same lifetime as image. Used only by original compressed SHP.
    SHPStruct* depth_image=nullptr;
    int depth_frame=0;
    Point2D depth_offset{};
};

struct TileDrawingRequest {
    DrawingTargetHandle* target = nullptr;
    const DrawingPaletteHandle* palette = nullptr;
    const TMPStruct* resource = nullptr;
    const TMPImage* image = nullptr;
    int sub_tile = 0;
    Point2D position{};
    RectangleStruct clip{};
    int level = 0;
    int intensity = 1000;
    bool use_depth = false;
    bool flat = false;
    bool flag16 = false;
    bool flag17 = false;
    int color = 0;
    int buffer_offset_y = 0;
    bool translucent = true;
};

// RGB565 copy/fill or original destination-light modulation. Coordinates/clip/order are decided
// by the core. Null pixels means a fill; non-null storage is borrowed only
// during submission and must cover the last addressed row.
enum class RasterBlendMode : std::uint32_t { copy, spotlight, depth_glow, depth_alpha, particle,
    // Lighting-buffer producers: covered source samples are 0..255; 0x100
    // denotes absent coverage. Zero remains a valid darkening sample.
    shroud, fog, alpha_shape };
struct RasterDrawingRequest {
    DrawingTargetHandle* target = nullptr;
    Point2D position{};
    RectangleStruct clip{};
    int width = 0, height = 0, pitch = 0;
    const std::uint16_t* pixels = nullptr;
    std::uint32_t pixel_count = 0;
    std::uint16_t color = 0;
    // Native DSurface::DrawLineColor pixel: strict Z read (no write), with
    // ABuffer RGB scaling. Only a 1x1 solid request is valid in this mode.
    bool original_line = false;
    RasterBlendMode blend_mode = RasterBlendMode::copy;
    // spotlight pixels contain an 8-bit strength; color is the solid strength.
    std::uint32_t spotlight_flags = 0;
    // DSurface glow strength is signed; negative values dim the beam.
    int light_strength = 0;
    int line_z = 0; // interpolated bias - AdjustForZ(world Z); selection +14, trail -2
    // 0x4BEAC0 blends full 8-bit RGB before RGB565 quantization. Do not use
    // color (already RGB565), or treat ABuffer lighting as opacity.
    std::uint32_t line_rgb = 0; // R | G << 8 | B << 16
    // particle (0x62CEC0): full RGB, strict Z read, ABuffer scales only below
    // 127. Its depth subtracts world height after the unsigned scanline wrap.
    int line_opacity = 255;
};
// Original lighting SHP identity. Position is the uncentered image origin;
// the frame's own offset still applies. Immutable frame bytes can be cached
// independently of camera/clip. Resources are borrowed during submission.
struct LightingShapeDrawingRequest {
    DrawingTargetHandle* target = nullptr;
    SHPStruct* image = nullptr;
    int frame = 0;
    Point2D position{};
    RectangleStruct clip{};
    RasterBlendMode operation = RasterBlendMode::shroud;
};
// Indexed voxel surface. Bits 0..7: palette index; 8..23: signed relative
// depth; bit 24: coverage. Index zero is transparent. Storage is borrowed only
// until the synchronous callback returns, like every other drawing request.
// legacy selects the original building VXL bitmap blit: gradient 2, strict
// scene-Z read without writing. In that mode absolute_depth is the signed
// building Z adjustment and the per-texel internal voxel depths are ignored.
struct IndexedDrawingRequest {
    DrawingTargetHandle* target = nullptr;
    const DrawingPaletteHandle* palette = nullptr;
    Point2D position{};
    RectangleStruct clip{};
    int width=0, height=0, intensity=1000, absolute_depth=0;
    const std::uint32_t* pixels=nullptr;
    std::uint32_t pixel_count=0;
    ShapeDepthMode depth_mode=ShapeDepthMode::read_write;
    bool shadow=false;
    // Original cached-voxel PlainBlit flags, including disguise translucency.
    std::uint32_t flags=0x2800;
    std::uint32_t tint=0;
};
struct TypeDrawingBackend {
    std::uint32_t struct_size = sizeof(TypeDrawingBackend);
    std::uint32_t version = 12;
    DrawingStatus (*shape)(void*, const ShapeDrawingRequest&) = nullptr;
    DrawingStatus (*tile)(void*, const TileDrawingRequest&) = nullptr;
    DrawingStatus (*raster)(void*, const RasterDrawingRequest&) = nullptr;
    DrawingStatus (*indexed)(void*, const IndexedDrawingRequest&) = nullptr;
    // Optional resource-aware path; absent callbacks use the existing raster
    // callback synchronously, preserving software/simple-host behavior.
    DrawingStatus (*lighting_shape)(void*, const LightingShapeDrawingRequest&) = nullptr;
};

struct TypeDrawingContext {
    TypeDrawingBackend backend{};
    void* backend_context = nullptr;
    DrawingTargetHandle* target = nullptr;
    const DrawingPaletteHandle* palette = nullptr;
    Point2D overlay_offset{}; // original 886FA0/886FA4; not TMP buffer_offset_y
    int buffer_offset_y = 0;
    bool translucent = true;

    // World adapters supply actual cell state; they must not invent a neutral
    // cell when lookup fails. They are separate from the graphics backend.
    void* world_context = nullptr;
    DrawingStatus (*smudge_cell)(void*, const CellStruct&,
        const DrawingPaletteHandle*&, int& intensity) = nullptr;
    // 6D20E0 needs the target's B0CD48 constant, absent from the function export.
    // Hosts must supply their calibrated projection; height==0 needs no hook.
    DrawingStatus (*height_to_pixels)(void*, int height, int& pixels) = nullptr;

    // Only legacy entry points use these adapters. Modern hosts supply the
    // opaque target/palette directly and do not construct fake Surface/Convert.
    DrawingStatus (*legacy_target)(void*, Surface*, DrawingTargetHandle*&) = nullptr;
    DrawingStatus (*legacy_palette)(void*, ConvertClass*, const DrawingPaletteHandle*&) = nullptr;
};

// All pointers, callbacks, resources and request fields are borrowed until the
// synchronous call returns. A queuing backend must copy the request and retain
// its own resources before returning; a borrowed pointer cannot be queued.
// Callbacks must not throw across a module boundary. Defensive catches convert
// a same-runtime violation into backend_failure; they are not a cross-CRT ABI.
DrawingStatus submit_type_shape(const TypeDrawingContext&, const ShapeDrawingRequest&) noexcept;
DrawingStatus submit_type_tile(const TypeDrawingContext&, const TileDrawingRequest&) noexcept;
DrawingStatus submit_type_indexed(const TypeDrawingContext&, const IndexedDrawingRequest&) noexcept;
DrawingStatus submit_type_raster(const TypeDrawingContext&, const RasterDrawingRequest&) noexcept;
DrawingStatus submit_type_lighting_shape(const TypeDrawingContext&, const LightingShapeDrawingRequest&) noexcept;

// Bind the legacy void drawing entries for one synchronous operation. Nested
// scopes restore the previous context. The first failed entry is reported;
// unsupported modes must never be retried through the original EXE implicitly.
// No scope means a legacy entry keeps its explicit original JUMP.
DrawingStatus with_type_drawing(const TypeDrawingContext&, void (*operation)(void*), void*) noexcept;
const char* drawing_status_name(DrawingStatus) noexcept;
}
