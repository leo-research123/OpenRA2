#pragma once
#include "type_drawing_context.hpp"

namespace game {
// Concrete compiler contexts use version 32. Version 31 is confined to legacy diagnostics/software.
bool valid_drawing_context(const TypeDrawingContext&) noexcept;
enum class DrawingKind : unsigned { shape, tile, raster, indexed, voxel_indexed,
    lighting_shape, lighting_shape_batch, mask, shape_strip, screen_effect,
    background_complete, background_scene, shape_sequence };
bool supports_drawing(const TypeDrawingContext&, DrawingKind) noexcept;

// The single sink of drawing submissions. The submit_type_*, submit_background_* and submit_screen_effect
// entry points validate a request and hand it to one of the functions below; they invoke the concrete presentation compiler synchronously and return its
// status unchanged apart from rejecting values outside DrawingStatus (shape_sequence: unchanged), and
// notes the request, in submission order, for the stream hash.
//
// The stream is not deferred to the end of the frame. Callers act on the status at once (a failed stage
// ends the pass), background_scene, lighting_shape_batch and shape_sequence answer through output fields
// and statuses the core reads before its next step, and every request borrows pointers (stack arrays,
// transient pixel storage) that are valid only until the callback returns. An implementation that throws leaves
// the exception to the caller's boundary handler; the request is not noted.
DrawingStatus record_shape(const TypeDrawingContext&, const ShapeDrawingRequest&);
DrawingStatus record_tile(const TypeDrawingContext&, const TileDrawingRequest&);
DrawingStatus record_raster(const TypeDrawingContext&, const RasterDrawingRequest&);
DrawingStatus record_indexed(const TypeDrawingContext&, const IndexedDrawingRequest&);
DrawingStatus record_voxel_indexed(const TypeDrawingContext&, const IndexedDrawingRequest&);
DrawingStatus record_lighting_shape(const TypeDrawingContext&, const LightingShapeDrawingRequest&);
DrawingStatus record_lighting_shape_batch(const TypeDrawingContext&, const LightingShapeBatchRequest&);
DrawingStatus record_mask(const TypeDrawingContext&, const MaskDrawingRequest&);
DrawingStatus record_shape_strip(const TypeDrawingContext&, const ShapeStripDrawingRequest&);
DrawingStatus record_screen_effect(const TypeDrawingContext&, const ScreenEffectDrawingRequest&);
DrawingStatus record_background_complete(const TypeDrawingContext&, const BackgroundDrawingRequest&);
// Output fields (item infos, changed flag, stale cells) are written by the callback and are part of the entry.
DrawingStatus record_background_scene(const TypeDrawingContext&, const BackgroundSceneRequest&);
DrawingStatus record_shape_sequence(const TypeDrawingContext&, SHPStruct* image,
                                    std::uint32_t use) noexcept;

// RA2_VERIFY_DRAW_STREAM: the stream of each drawn frame is hashed and reported. recorder_frame_end closes
// the stream of one drawn frame: it prints "DRAW_STREAM frame=... requests=... hash=..." (per-kind counts
// and hashes follow) and starts the next. Both are one flag test when the mode is off.
bool recorder_enabled() noexcept;
void recorder_frame_end() noexcept;
}
