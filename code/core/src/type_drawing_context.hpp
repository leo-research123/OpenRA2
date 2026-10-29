#pragma once
#include "api/type_drawing.hpp"
class MapClass;
namespace game {
// Private original-request adapters and legacy reference-test capture. Hosts
// submit owned render::Frame views and never construct or receive this table.
struct TypeDrawingBackend {
    std::uint32_t struct_size = sizeof(TypeDrawingBackend);
    std::uint32_t version = 31;
    DrawingStatus (*shape)(void*, const ShapeDrawingRequest&) = nullptr;
    DrawingStatus (*tile)(void*, const TileDrawingRequest&) = nullptr;
    DrawingStatus (*raster)(void*, const RasterDrawingRequest&) = nullptr;
    DrawingStatus (*indexed)(void*, const IndexedDrawingRequest&) = nullptr;
    // Optional resource-aware path; absent callbacks use the existing raster
    // callback synchronously, preserving software/simple-host behavior.
    DrawingStatus (*lighting_shape)(void*, const LightingShapeDrawingRequest&) = nullptr;
    DrawingStatus (*screen_effect)(void*, const ScreenEffectDrawingRequest&) = nullptr;
    // Optional; hosts without it receive no phase boundary.
    DrawingStatus (*background_complete)(void*, const BackgroundDrawingRequest&) = nullptr;
    // Optional; hosts without it receive each entry through lighting_shape.
    DrawingStatus (*lighting_shape_batch)(void*, const LightingShapeBatchRequest&) = nullptr;
    DrawingStatus (*background_scene)(void*, const BackgroundSceneRequest&) = nullptr;
    // Version 23: optional GPU image graphs; absent hosts keep CPU pixels.
    DrawingStatus (*voxel_indexed)(void*, const IndexedDrawingRequest&) = nullptr;
    // Version 26: optional immutable UI resources. Missing callbacks expand to
    // the original raster/shape requests. Unsupported must submit nothing.
    DrawingStatus (*mask)(void*, const MaskDrawingRequest&) = nullptr;
    DrawingStatus (*shape_strip)(void*, const ShapeStripDrawingRequest&) = nullptr;
    // Version 28: optional. Tells the host an image will be drawn so it can keep all
    // of its frames resident ahead of the first draw (a ShapeSequenceUse value says
    // how it is drawn). The core has already called image->Load(). A failure, or
    // `unavailable` when the host's budget for such announcements is spent, is not an
    // error: drawing falls back to preparing the image on first use. Hosts without it
    // are unaffected. Must not let exceptions escape.
    DrawingStatus (*shape_sequence)(void*, SHPStruct* image, std::uint32_t use) noexcept = nullptr;
};

struct TypeDrawingContext {
    TypeDrawingBackend backend{};
    void* backend_context = nullptr;
    DrawingTargetHandle* target = nullptr;
    const DrawingPaletteHandle* palette = nullptr;
    Point2D overlay_offset{};  // original 886FA0/886FA4; not TMP buffer_offset_y
    int buffer_offset_y = 0;
    bool translucent = true;

    // World adapters supply actual cell state; they must not invent a neutral
    // cell when lookup fails. They are separate from the graphics backend.
    void* world_context = nullptr;
    DrawingStatus (*smudge_cell)(void*, const CellStruct&, const DrawingPaletteHandle*&, int& intensity) = nullptr;
    // 6D20E0 needs the target's B0CD48 constant, absent from the function export.
    // Hosts must supply their calibrated projection; height==0 needs no hook.
    DrawingStatus (*height_to_pixels)(void*, int height, int& pixels) = nullptr;

    // Only legacy entry points use these adapters. Modern hosts supply the
    // opaque target/palette directly and do not construct fake Surface/Convert.
    DrawingStatus (*legacy_target)(void*, Surface*, DrawingTargetHandle*&) = nullptr;
    DrawingStatus (*legacy_palette)(void*, ConvertClass*, const DrawingPaletteHandle*&) = nullptr;
};

struct MapDrawingContext {
    TypeDrawingContext types{};
    int lighting_quality = 2;
    // Original unlit ConvertClass palette (no LightConvert tint arithmetic).
    DrawingStatus (*plain_palette)(void*, const BytePalette&, const DrawingPaletteHandle*&) noexcept = nullptr;
    // Resolve a backend-owned palette from original normalized/quantized tint.
    // The PAL and returned identity are borrowed synchronously, like type draws.
    DrawingStatus (*terrain_palette)(void*, const BytePalette&, int red, int green, int blue, int shade_count,
                                     const DrawingPaletteHandle*&) noexcept = nullptr;
    // Original ConvertClass shade table for global animation/resource palettes.
    // Neutral LightConvert arithmetic is different even without a color tint.
    DrawingStatus (*shape_palette)(void*, const BytePalette&, int shade_count,
                                   const DrawingPaletteHandle*&) noexcept = nullptr;
    // Original house ColorScheme converter, including its unlit color mask.
    // Borrowed synchronously; failures return status and never throw.
    DrawingStatus (*color_scheme_palette)(void*, const BytePalette&, int shade_count,
                                          const DrawingPaletteHandle*&) noexcept = nullptr;
    // Borrow the exact original converter selected by the object. Hosts copy
    // PaletteData and FullColorData synchronously. revision changes whenever
    // the tables are written and is never reused, also not by a converter at a
    // reused address, so hosts may keep a copy per (address, revision) without
    // comparing table contents. No exceptions escape.
    DrawingStatus (*converter_palette)(void*, const ConvertClass&, std::uint64_t revision,
                                       const DrawingPaletteHandle*&) noexcept = nullptr;
};
// Link ra2_software_render, not just ra2_core. Borrowed objects must outlive the
// synchronous call. No ownership transfer, allocation, or original EXE lookup.
// Height projection is deliberately unbound (the supplied export omits the
// target constant); set height_to_pixels for nonzero Smudge heights.
TypeDrawingContext make_software_type_drawing(Surface* target, ConvertClass* palette, MapClass* map = nullptr) noexcept;
}
