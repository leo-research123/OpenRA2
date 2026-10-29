#pragma once

#include <cstdint>
#include "yrpp/GeneralStructures.h"

class ConvertClass;
class IsometricTileTypeClass;
class OverlayTypeClass;
class SmudgeTypeClass;
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
    drawn,
    skipped,
    unavailable,
    unsupported,
    invalid_argument,
    backend_failure
};

enum class ShapeDepthMode : std::uint32_t { legacy, none, read, read_write };
enum class ShapeBlendMode : std::uint32_t { palette, shadow, translucent25, translucent50, translucent75 };
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
    ShapeDepthMode depth_mode = ShapeDepthMode::legacy;
    ShapeBlendMode blend_mode = ShapeBlendMode::palette;
    int absolute_depth = 0;
    // Original auxiliary SHA sampling. Bytes are signed depth deltas; borrowed
    // with the same lifetime as image. Used only by original compressed SHP.
    SHPStruct* depth_image = nullptr;
    int depth_frame = 0;
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
enum class RasterBlendMode : std::uint32_t {
    copy,
    spotlight,
    depth_glow,
    depth_alpha,
    particle,
    // Lighting-buffer producers: covered source samples are 0..255; 0x100
    // denotes absent coverage. Zero remains a valid darkening sample.
    shroud,
    fog,
    alpha_shape,
    depth_additive,
    depth_laser_pair,
    wave,
    // DSurface 0x4C0750: copy RGB565 only when the light word is nonzero/zero.
    // Read-only ABuffer test, no Z access and no color modulation.
    light_nonzero,
    light_zero
};
// Ordered in-place background samples for one original Wave draw. Hosts must
// execute these in order: a later sample can read an earlier destination.
struct WavePixel {
    Point2D destination{}, sample{};
    int strength = 0;
};
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
    int line_z = 0;  // interpolated bias - AdjustForZ(world Z); selection +14, trail -2
    // 0x4BEAC0 blends full 8-bit RGB before RGB565 quantization. Do not use
    // color (already RGB565), or treat ABuffer lighting as opacity.
    std::uint32_t line_rgb = 0;  // R | G << 8 | B << 16
    // particle (0x62CEC0): full RGB, strict Z read, ABuffer scales only below
    // 127. Its depth subtracts world height after the unsigned scanline wrap.
    int line_opacity = 255;
    // 0x4BC750: one Z/ABuffer sample gates both neighboring pixels. The
    // cubic-table indices are already resolved by the core line traversal.
    Point2D line_neighbor{};
    int line_weight = 0, line_neighbor_weight = 0;
    std::uint32_t line_channels = 7;
    // RGB multiplying holds the initial light-buffer X as the line advances;
    // the depth-major AA horizontal loop samples X once per two output pixels.
    int line_light_x = -1, line_sample_x = -1;
    // 0x4BDF00 keeps ftol(color * intensity) in signed 32-bit channels;
    // intensities above one must not wrap back into an 8-bit source color.
    int line_red = 0, line_green = 0, line_blue = 0;
    // wave: color is WaveType, line_z is -projected height-2. No ABuffer read.
    const WavePixel* wave_pixels = nullptr;
    std::uint32_t wave_pixel_count = 0;
    // Immutable pixel contents for copy/spotlight resources. Both are nonzero
    // when supplied: identity is never reused, version changes with the bytes.
    // Position and clipping do not change content. Pixels remain borrowed only
    // until return; a host must own any retained copy. Zero keeps transient use.
    std::uint64_t content_identity = 0, content_version = 0;
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
// One Tactical pass's lighting requests in submission order. The host must
// produce the ABuffer of submitting each request through lighting_shape in
// that order. cell names the map cell whose shroud/fog an entry draws, or is
// {-1,-1} for other producers, so a host may resolve the run per pixel. view
// is the pass's Tactical bounds. Entries are borrowed until the call returns.
struct LightingShapeBatchEntry {
    LightingShapeDrawingRequest request{};
    CellStruct cell{-1, -1};
};
// Version 24: with shroud set, the shroud of every map cell precedes entries.
// Cell (x, y) draws frame shroud_frame(x, y) of shroud, the frames the
// background scene's shroud operations delivered, as DrawShroud would: its
// 60x30 footprint starts at shroud_origin + (30 (x - y), 15 (x + y)), the
// image at that point moved by view.X, clipped to the footprint and view.
// Version 25: shroud_camera is the TacticalPos of shroud_origin, so a host
// may keep the field in world space (world = target + shroud_camera). A
// shroud batch carries no AlphaShape entries: the background scene's alpha
// operations deliver every AlphaShapeClass, drawn after the cells as
// AlphaShapeClass::DrawAll would, in Array order.
struct LightingShapeBatchRequest {
    DrawingTargetHandle* target = nullptr;
    RectangleStruct view{};
    const LightingShapeBatchEntry* entries = nullptr;
    std::uint32_t count = 0;
    SHPStruct* shroud = nullptr;
    Point2D shroud_origin{};
    Point2D shroud_camera{};
};
// Indexed voxel surface. Bits 0..7: palette index; 8..23: signed relative
// depth; bit 24: coverage. Index zero is transparent. Storage is borrowed only
// until the synchronous callback returns, like every other drawing request.
// legacy selects the original building VXL bitmap blit: gradient 2, strict
// scene-Z read without writing. In that mode absolute_depth is the signed
// building Z adjustment and the per-texel internal voxel depths are ignored.
class VoxelImageHandle;
struct IndexedDrawingRequest {
    DrawingTargetHandle* target = nullptr;
    const DrawingPaletteHandle* palette = nullptr;
    Point2D position{};
    RectangleStruct clip{};
    int width = 0, height = 0, intensity = 1000, absolute_depth = 0;
    const std::uint32_t* pixels = nullptr;
    std::uint32_t pixel_count = 0;
    // Optional immutable GPU generation graph, borrowed during submission.
    const VoxelImageHandle* voxel = nullptr;
    ShapeDepthMode depth_mode = ShapeDepthMode::read_write;
    bool shadow = false;
    // Original cached-voxel PlainBlit flags, including disguise translucency.
    std::uint32_t flags = 0x2800;
    std::uint32_t tint = 0;
    // Original PlainBlit gradient: flat projectiles use 0, cached voxels use 2.
    int gradient = 2;
};
// Device snapshots belong to one original screen effect. The host owns their
// storage; requests carry no Surface or graphics objects across the ABI.
enum class ScreenEffectStage : std::uint32_t { inactive, old_world, white, new_world, restore };
// Ordered marks within one GScreenClass::Render. apply writes the stage into the
// effect area at its position; the capture pair brackets the original's
// unpresented Status=0 render of the new world into the snapshot (0x53B560).
enum class ScreenEffectOperation : std::uint32_t { apply, capture_begin, capture_end, ion_blast };
struct ScreenEffectDrawingRequest {
    DrawingTargetHandle* target = nullptr;
    std::uint64_t capture_id = 0;
    ScreenEffectStage stage = ScreenEffectStage::inactive;
    ScreenEffectOperation operation = ScreenEffectOperation::apply;
    // Effect area from the target origin: the Composite width by its height
    // minus the 32-row command bar, as 0x53BBA0 processes; the white stage
    // fills the same Tactical view rectangle (0x53B560 case 2).
    int width = 0, height = 0, brightness = 0;
    int warp_step = 0, initial_duration = 1;
    int center_x = 0, center_y = 0;
    int depth_base = 0;
};
struct BackgroundDrawingRequest {
    DrawingTargetHandle* target = nullptr;
    Point2D view_origin{};
};
// Optional retained geometry transaction. begin returns unavailable when the
// host needs a full reset, otherwise drawn. commit publishes all edits atomically;
// failed transactions must be aborted. Hosts retain immutable copies for queued
// frames. Item recording uses origin and an 8192x8192 clip; draw supplies the real
// viewport. Layers: 1 tiles/smudges, 2 overlays, 3 overlay shadows, 4 static terrain,
// 5 tile shadows. Object IDs are comparison-only. No callback may throw across the
// boundary.
//
// Slots (TerrainSpriteLayer, core/src/presentation/scene/terrain_sprite_layer.hpp). The core lays the
// resident background out in instances: every instance is one layer, one drawing
// group and one pipeline state, and owns a fixed number of slots per map cell, so the
// address of a record never changes while the map is loaded. A slot holds at most one
// host record; an empty slot is a null record the host draws as nothing. The core
// decides which slots an item's records take and what to draw; the host only stores
// records by slot, uploads the slots the core names and draws the ranges the core
// gives, in the order it gives them. Slot numbers are local to an instance.
//
// Recording an item: item_begin; the host's backend callbacks produce records;
// item_end (the host keeps the records pending and reports one BackgroundRecordInfo
// per record); then place (the core names a slot for every pending record and the
// slots the item no longer holds; the host writes them and sets *changed when a slot's
// content differs afterwards). An item that recorded nothing and holds no slot needs no
// place. The host creates an instance (the instance operation: id, slots, state) before
// a place names it; instance ids are consecutive from zero after a reset. commit
// receives the slot ranges the transaction changed and the draw orders that changed.
//
// Draw order: every instance's occupied slots in ascending slot order, so empty slots
// are never drawn. The core keeps it; commit sends the tail of each order that changed
// (orders). Draw ranges index the order, not slots: draw receives, for every instance
// in drawing order, the range of its order the viewport can touch (visible) and the
// range a window BackgroundWindowMargin pixels around it can touch (window); empty
// ranges are listed.
//
// Lighting is separate from geometry. lighting delivers cells' current light
// before any item of those cells is recorded in the transaction; a host relights
// held items from it and reports in stale the cells whose items it cannot
// relight (the core then records them again). converter delivers a converter
// whose tables changed; the host refreshes every held item using it.
//
// shroud delivers the SHROUD.SHP frame of listed cells (-1: the cell draws no
// shroud); a reset forgets every frame.
//
// alpha replaces the set of AlphaShapeClass lights with the listed ones, in
// AlphaShapeClass::Array order; the core sends it when the Array changed.
inline constexpr std::uint32_t BackgroundInvalidInstance = 0xFFFFFFFFu;
// A record's source, as reported by the host: the first two share the cell-sized terrain drawing group.
enum class BackgroundRecordKind : std::uint32_t { tile, tile_extra, shape };
struct BackgroundRecordInfo {
    std::uint32_t kind = 0;    // BackgroundRecordKind
    std::uint32_t state = 0;   // pipeline state; opaque to the core, records of equal state share an instance
    RectangleStruct bounds{};  // the record's pixels, in the item's recording coordinates
};
struct BackgroundSlotAddress {
    std::uint32_t instance = BackgroundInvalidInstance;
    std::uint32_t slot = 0;
};
struct BackgroundSlotRange {
    std::uint32_t instance = 0, first = 0, count = 0;
};
// An instance's draw order from position first on: the order now has size entries and
// slots holds its entries first .. size - 1 (none when first == size).
struct BackgroundOrderUpdate {
    std::uint32_t instance = 0, first = 0, size = 0;
    const std::uint32_t* slots = nullptr;
};
// Pixels a window range extends the viewport by in every direction (a host page size must not exceed it).
inline constexpr int BackgroundWindowMargin = 256;
enum class BackgroundSceneOperation : std::uint32_t {
    begin,
    reset,
    item_begin,
    item_end,
    place,
    instance,
    commit,
    abort,
    draw,
    lighting,
    converter,
    shroud,
    alpha
};
struct BackgroundShroudCell {
    CellStruct cell{};
    std::int32_t frame = -1;
};
// One AlphaShapeClass: frame 0 of image drawn at position (its Rect origin
// moved by the Tactical bounds origin; target = position - TacticalPos). id
// identifies the shape while it lives.
struct BackgroundAlphaShape {
    const void* id = nullptr;
    SHPStruct* image = nullptr;
    Point2D position{};
};
// One tint convention of a cell: the tint and shade count of its terrain
// palette (LightConvertClass::BuildColorTable inputs) and BuildColorTable's
// per-channel multipliers of that tint.
struct BackgroundSceneTint {
    std::int32_t red = 1000, green = 1000, blue = 1000, shades = 53;
    std::int32_t factor_red = 0, factor_green = 0, factor_blue = 0;
};
// A cell's lighting inputs. tint[0]: CellClass::DrawIt's convention (an untinted
// cell keeps 1000/53); tint[1]: PrepareCellTint(Color2) as world sprites use it.
struct BackgroundSceneLight {
    CellStruct cell{};
    std::uint16_t normal = 0, terrain = 0;  // Intensity_Normal, Intensity_Terrain
    BackgroundSceneTint tint[2]{};
};
struct BackgroundSceneRequest {
    DrawingTargetHandle* target = nullptr;
    BackgroundSceneOperation operation = BackgroundSceneOperation::begin;
    std::uint64_t world = 0, object = 0;
    std::uint32_t layer = 0;
    CellStruct cell{};
    int lighting_quality = 2;
    Point2D origin{};
    RectangleStruct clip{};
    // item_end: the host writes one entry per record of the item to infos (at most
    // info_capacity) and their number to *info_count; more than the capacity fails.
    BackgroundRecordInfo* infos = nullptr;
    std::uint32_t info_capacity = 0;
    std::uint32_t* info_count = nullptr;
    // place: borrowed until return. slots[i] is the slot of pending record i; releases
    // lists the slots the item no longer holds (the host empties them).
    const BackgroundSlotAddress* slots = nullptr;
    std::uint32_t slot_count = 0;
    const BackgroundSlotAddress* releases = nullptr;
    std::uint32_t release_count = 0;
    bool* changed = nullptr;
    // instance: the new instance's id, slot count and pipeline state.
    std::uint32_t instance = 0, instance_slots = 0, instance_state = 0;
    // commit (dirty, orders) and draw (visible, window; draw_count entries in drawing order): borrowed until
    // return.
    const BackgroundSlotRange* dirty = nullptr;
    std::uint32_t dirty_count = 0;
    const BackgroundOrderUpdate* orders = nullptr;
    std::uint32_t order_count = 0;
    const BackgroundSlotRange* visible = nullptr;
    const BackgroundSlotRange* window = nullptr;
    std::uint32_t draw_count = 0;
    // lighting: borrowed until return. The host writes at most stale_capacity
    // cells to stale and their total count to *stale_count.
    const BackgroundSceneLight* lights = nullptr;
    std::uint32_t light_count = 0, stale_capacity = 0;
    CellStruct* stale = nullptr;
    std::uint32_t* stale_count = nullptr;
    // converter: borrowed until return; revision as MapDrawingContext::converter_palette.
    const ConvertClass* converter = nullptr;
    std::uint64_t revision = 0;
    // shroud: borrowed until return.
    const BackgroundShroudCell* shroud = nullptr;
    std::uint32_t shroud_count = 0;
    // alpha: borrowed until return.
    const BackgroundAlphaShape* alphas = nullptr;
    std::uint32_t alpha_count = 0;
};
// MSB-first monochrome coverage, copied with RGB565 ink. Storage is borrowed
// only for the call; padding bits and bytes have no meaning. No depth or light.
struct MaskDrawingRequest {
    DrawingTargetHandle* target = nullptr;
    Point2D position{};
    RectangleStruct clip{};
    int width = 0, height = 0, pitch = 0;
    const std::uint8_t* bits = nullptr;
    std::uint32_t byte_count = 0;
    std::uint16_t color = 0;
};
// Consecutive opaque, unlit palette copies. Transparent pixels preserve earlier
// copies, including overlap. Only flags=0, no auxiliary image/depth/blend.
struct ShapeStripDrawingRequest {
    ShapeDrawingRequest shape{};
    // Owner changes this nonzero identity whenever the immutable art is reloaded.
    // Zero requests the ordinary uncached shape path.
    std::uint64_t resource_revision = 0;
    Point2D step{};
    int count = 0;
};
// How an announced SHP image is used by the presentation recorder.
enum ShapeSequenceUse : std::uint32_t {
    shape_sequence_color = 0,        // palette image without an auxiliary SHA image
    shape_sequence_color_depth = 1,  // palette image drawn with an auxiliary SHA image (buildings)
    shape_sequence_depth = 2,        // the auxiliary SHA image itself
};
struct TypeDrawingContext;

// All pointers, callbacks, resources and request fields are borrowed until the
// synchronous call returns. A queuing backend must copy the request and retain
// its own resources before returning; a borrowed pointer cannot be queued.
// Callbacks must not throw across a module boundary. Defensive catches convert
// a same-runtime violation into backend_failure; they are not a cross-CRT ABI.
DrawingStatus submit_screen_effect(const TypeDrawingContext&, const ScreenEffectDrawingRequest&) noexcept;
DrawingStatus submit_background_complete(const TypeDrawingContext&, const BackgroundDrawingRequest&) noexcept;
DrawingStatus submit_background_scene(const TypeDrawingContext&, const BackgroundSceneRequest&) noexcept;
DrawingStatus submit_type_shape(const TypeDrawingContext&, const ShapeDrawingRequest&) noexcept;
DrawingStatus submit_type_tile(const TypeDrawingContext&, const TileDrawingRequest&) noexcept;
DrawingStatus submit_type_indexed(const TypeDrawingContext&, const IndexedDrawingRequest&) noexcept;
DrawingStatus submit_type_raster(const TypeDrawingContext&, const RasterDrawingRequest&) noexcept;
DrawingStatus submit_type_mask(const TypeDrawingContext&, const MaskDrawingRequest&) noexcept;
DrawingStatus submit_type_shape_strip(const TypeDrawingContext&, const ShapeStripDrawingRequest&) noexcept;
DrawingStatus submit_type_lighting_shape(const TypeDrawingContext&, const LightingShapeDrawingRequest&) noexcept;
// Every entry must be a request submit_type_lighting_shape would pass to the
// backend (nonempty clip, image present); any other entry fails the batch.
DrawingStatus submit_type_lighting_shape_batch(const TypeDrawingContext&, const LightingShapeBatchRequest&) noexcept;

// Original type-class drawing on an explicit context: the resource and flag
// selection of IsometricTileTypeClass::DrawTMP (YR 0x00547CF0),
// OverlayTypeClass::Draw (0x005FE5A0) and SmudgeTypeClass::DrawIt (0x006B55F0).
DrawingStatus draw_isometric_tile(IsometricTileTypeClass&, const TypeDrawingContext&, int sub_tile, int x, int y,
                                  RectangleStruct clip, int level, int intensity, bool use_z, int alternate, bool flat,
                                  bool flag16, bool flag17, int color) noexcept;
DrawingStatus draw_overlay_type(OverlayTypeClass&, const TypeDrawingContext&, const Point2D&,
                                const RectangleStruct& clip, int frame) noexcept;
DrawingStatus draw_smudge_type(SmudgeTypeClass&, const TypeDrawingContext&, const Point2D&, const RectangleStruct& clip,
                               int data, int height, const CellStruct&) noexcept;

const char* drawing_status_name(DrawingStatus) noexcept;
}
