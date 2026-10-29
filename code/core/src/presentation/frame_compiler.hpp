#pragma once
#include "api/map_view.hpp"
#include "type_drawing_context.hpp"
#include <string>
#include "api/voxel_drawing.hpp"
#include "type_drawing_packets.hpp"
#include "lighting_batch.hpp"
#include "draw_resource_pool.hpp"
#include "raster_batch.hpp"
#include "scene_row_table.hpp"
#include "frame_storage.hpp"
#include <array>
#include <deque>
#include <map>
#include <list>
#include <memory>
#include <vector>
#include <tuple>
#include <unordered_map>

namespace game {
using render::DrawResourcePool;
using render::RasterInstance;
using render::PixelParameters;
using render::PixelRunScratch;
using render::PixelBatch;
using render::RasterDepth;
using render::ShapePlanesFlag;
using render::RasterBlends;
using render::RasterVariants;
using render::append_pixel_run;
using render::raster_packet;
using render::raster_instance;

using render::MapVoxelImage;
using render::MapSequenceTable;
using render::MapBufferData;
using render::MapSequenceSet;
using render::MapLightSource;
using render::MapPaletteSource;
using render::MapLightingImage;
using render::MapLightingSet;
using render::MapSceneRecord;
using render::MapScreenEffectMark;
using render::MapLightField;
using render::MapSceneChunk;
using render::MapSceneSlots;
using render::MapSceneOrderChunk;
using render::MapSceneOrder;
using render::MapSceneInstance;
using render::MapSceneLightChunk;
using render::MapSceneShroudChunk;
using render::MapSceneLights;
using render::MapSceneShroud;
using render::MapSceneAlpha;
using render::MapSceneData;
using render::MapSceneMark;
using render::MapCommandKind;
using render::MapCommand;
using render::MapSequenceFrameBase;
using render::MapSequenceFrameLimit;
using render::MapSceneChunkSlots;
using render::MapSceneOrderChunkEntries;
using render::MapLightChunkCells;
using render::MapLightWords;
using MapFrameData = render::FramePayload;
struct MapTileData {
    std::shared_ptr<MapBufferData> base, extra;
};
struct MapPaletteData {
    std::shared_ptr<MapBufferData> buffer;
    int shades = 53;
    // Terrain palettes keep the tint they were built with; converter copies
    // change with UpdateColors. Other palettes do not depend on lighting.
    const BytePalette* tint_source = nullptr;
    int red = 0, green = 0, blue = 0;
    bool converter = false;
    const ConvertClass* identity = nullptr;  // the converter a converter copy holds
};
// Converter copy for one (address, revision); see MapDrawingContext::converter_palette.
struct MapConverterData {
    std::uint64_t revision = 0;
    MapPaletteData palette;
};
// Bridge copy of borrowed indexed pixels, reused while the bytes stay equal.
struct MapIndexedData {
    std::shared_ptr<MapBufferData> buffer;
    std::uint64_t used = 0;
};
// Debug statistics of the foreground SHP path (RA2_SHAPE_STATS=1): per-window
// call, resource and decode counts, printed as SHAPE_STATS lines. They exist to
// size the shape sequence work and to assert that no decode happens late.
struct MapShapeStats {
    bool enabled = false, checked = false;
    std::uint64_t first_serial = 0, late_after = 300;  // late decodes start this many frames after the first
    // Current frame.
    std::uint32_t frame_calls = 0, frame_shapes = 0, frame_modes = 0;
    // Window since the last print.
    std::uint32_t frames = 0, max_calls = 0, max_shapes = 0, max_modes = 0;
    std::uint64_t calls = 0, shapes = 0, modes = 0, raster = 0, compute = 0, scene = 0, sha = 0, descriptions = 0,
                  decodes = 0, late_decodes = 0, skipped = 0, sequences = 0, sequence_frames = 0, sequence_bytes = 0,
                  prewarmed = 0, prewarm_bytes = 0;
    // Since the map was opened.
    std::uint64_t total_decodes = 0, total_late = 0, total_descriptions = 0, total_sequences = 0,
                  total_sequence_frames = 0, total_sequence_bytes = 0, total_prewarmed = 0, total_prewarm_bytes = 0;
};
class FrameCompiler {
public:
    explicit FrameCompiler(bool reference = false, std::uint32_t table_entries = MapSequenceFrameLimit)
        : reference_(reference), sequence_table_pool_(table_entries, DrawResourcePool::DefaultSlots) {}
    bool reference() const noexcept { return reference_; }
    DrawingStatus submit_shape(const ShapeDrawingRequest& request) { const auto status=shape(this, request); if (reference_ && status==DrawingStatus::drawn) retain_reference(request); return status; }
    DrawingStatus submit_tile(const TileDrawingRequest& request) { const auto status=tile(this, request); if (reference_ && status==DrawingStatus::drawn) retain_reference(request); return status; }
    DrawingStatus submit_raster(const RasterDrawingRequest& request) { const auto status=raster(this, request); if (reference_ && status==DrawingStatus::drawn) retain_reference(request); return status; }
    DrawingStatus submit_indexed(const IndexedDrawingRequest& request) { const auto status=indexed(this, request); if (reference_ && status==DrawingStatus::drawn) retain_reference(request); return status; }
    DrawingStatus submit_lighting_shape(const LightingShapeDrawingRequest& request) { return lighting_shape(this, request); }
    DrawingStatus submit_lighting_shape_batch(const LightingShapeBatchRequest& request) { return lighting_shape_batch(this, request); }
    DrawingStatus submit_mask(const MaskDrawingRequest& request) { return mask(this, request); }
    DrawingStatus submit_shape_strip(const ShapeStripDrawingRequest& request) { return shape_strip(this, request); }
    DrawingStatus submit_screen_effect(const ScreenEffectDrawingRequest& request) { return screen_effect(this, request); }
    DrawingStatus submit_background_complete(const BackgroundDrawingRequest& request) { return background_complete(this, request); }
    DrawingStatus submit_background_scene(const BackgroundSceneRequest& request) { return background_scene(this, request); }
    DrawingStatus submit_shape_sequence(SHPStruct* image, std::uint32_t use) noexcept { return shape_sequence(this, image, use); }
    const MapShapeStats& shape_stats() const { return shape_stats_; }
    std::shared_ptr<MapFrameData> acquire_frame();
    MapDrawingContext begin_frame(MapFrameData& frame);
    bool end_frame() noexcept;
    void clear();
    const char* error() const noexcept { return error_; }
    std::size_t decoded_tiles() const { return tiles_.size(); }
    std::size_t prepared_palettes() const { return palettes_.size() + converters_.size(); }
    std::size_t decoded_lighting() const { return lighting_frame_count_; }
    std::size_t shape_sequences() const { return sequences_.size(); }

private:
    bool reference_ = false;
    void retain_reference(const ShapeDrawingRequest&);
    void retain_reference(const TileDrawingRequest&);
    void retain_reference(const RasterDrawingRequest&);
    void retain_reference(const IndexedDrawingRequest&);
    void retain_reference_palette(render::ReferenceDraw&, const DrawingPaletteHandle*);
    std::vector<std::shared_ptr<MapFrameData>> frame_pool_;
    DrawResourcePool resource_pool_;
    DrawResourcePool voxel_model_pool_;
    void allocate_resource(const std::shared_ptr<MapBufferData>&, int width = 0, int height = 0);
    void retain_resource(const std::shared_ptr<MapBufferData>&, int width = 0, int height = 0);
    // Returns true when the packet became a raster instance, false for a compute packet.
    bool append_direct(const PixelParameters&, const std::shared_ptr<MapBufferData>&,
                       const std::shared_ptr<MapBufferData>&, bool constant = false);
    render::FrameCompiler render_compiler_;
    std::vector<std::uint32_t> laser_pixel_epochs_;
    std::uint32_t laser_epoch_ = 0;
    void append_lighting(LightingParameters, const std::shared_ptr<MapBufferData>&);
    void append_instance(RasterInstance, int, const std::shared_ptr<MapBufferData>&,
                         const std::shared_ptr<MapBufferData>&);
    void append_run(std::uint32_t first, int variant);
    std::uint64_t next_id_ = 1;
    std::uint64_t frame_serial_ = 0;
    MapFrameData* frame_ = nullptr;
    char error_[768]{};
    std::map<std::pair<const TMPImage*, bool>, MapTileData> tiles_;
    struct PaletteKey {
        const BytePalette* source = nullptr;
        int red = 0, green = 0, blue = 0, shades = 0, kind = 0;
        bool operator==(const PaletteKey&) const = default;
    };
    struct PaletteHash {
        std::size_t operator()(const PaletteKey& k) const noexcept {
            std::uint64_t h = reinterpret_cast<std::uintptr_t>(k.source);
            h = (h ^ unsigned(k.red)) * 0x9E3779B185EBCA87ull;
            h = (h ^ unsigned(k.green)) * 0x9E3779B185EBCA87ull;
            h = (h ^ unsigned(k.blue)) * 0x9E3779B185EBCA87ull;
            h = (h ^ unsigned(k.shades)) * 0x9E3779B185EBCA87ull;
            return std::size_t(h ^ unsigned(k.kind));
        }
    };
    std::unordered_map<PaletteKey, MapPaletteData, PaletteHash> palettes_;
    std::unordered_map<const ConvertClass*, MapConverterData> converters_;
    struct ShapeDescription {
        TypeShapeMeta meta{};
        TypeShapeProgram program{};
        bool compiled = false;
        int variant = -1;
        DrawingStatus status = DrawingStatus::unavailable;
    };
    // The auxiliary SHA image of a frame: part of the key of its mode programs.
    struct ShapeAux {
        const SHPStruct* auxiliary = nullptr;
        std::uint64_t version = 0;
        int frame = 0, x = 0, y = 0;
    };
    // Mode program of one frame. aux is 0 for the plain frame, else 1 + index into ShapeFrame::aux.
    struct ShapeMode {
        unsigned flags = 0;
        ShapeDepthMode depth{};
        ShapeBlendMode blend{};
        int gradient = 0;
        std::uint32_t aux = 0;
        ShapeDescription description;
        std::uint64_t counted = 0;  // debug statistics: last frame this mode was counted
    };
    static constexpr unsigned InlineModes = 4;
    struct ShapeFrame {
        DrawingStatus status = DrawingStatus::unavailable;      // decode status of the plain frame
        std::uint8_t modes = 0;                                 // mode programs of this frame
        std::array<std::uint32_t, InlineModes> inline_modes{};  // indices into ShapeSequence::modes
        std::unique_ptr<std::vector<std::uint32_t>> more_modes;
        std::unique_ptr<std::vector<ShapeAux>> aux;
    };
    // All frames of one SHP, decoded together on first use and resident until
    // clear() or a changed resource version.
    struct ShapeSequence {
        const SHPStruct* image = nullptr;
        std::uint32_t id = 0;
        std::uint64_t generation = 0, version = 0, used = 0, counted = 0;
        bool loaded = false;
        bool deltas = false;           // the SHP is an auxiliary SHA image: signed depth delta planes
        bool prefix = false;           // RLE prefix planes were generated (needed to draw with a SHA image)
        std::uint32_t entry_base = 0;  // frame table index of frame 0
        std::shared_ptr<MapBufferData> data;
        std::vector<ShapeFrame> frames;
        std::deque<ShapeMode> modes;  // stable addresses; frames name them by index
    };
    std::vector<std::unique_ptr<ShapeSequence>> sequences_;
    struct SequenceSlot {
        std::uintptr_t key = 0;
        std::uint32_t id = 0;
    };
    std::vector<SequenceSlot> sequence_index_;  // open addressing, power-of-two size; key = image | deltas
    ShapeSequence* sequence_cache_[2]{};        // last hit of colour and delta sequences
    DrawResourcePool sequence_table_pool_{MapSequenceFrameLimit, DrawResourcePool::DefaultSlots};
    std::shared_ptr<const MapSequenceSet> sequence_set_;
    std::vector<std::shared_ptr<const MapSequenceSet>> retired_sequences_;
    std::uint64_t sequence_revision_ = 0;
    bool sequence_dirty_ = false;
    ShapeSequence* find_sequence(const SHPStruct*, bool deltas, bool add);
    ShapeSequence& sequence(SHPStruct*, bool deltas = false);
    DrawingStatus load_sequence(ShapeSequence&, SHPStruct*, bool prefix);
    std::shared_ptr<MapBufferData> byte_buffer(const std::vector<std::uint8_t>&);
    // Appends a kind 0 packet whose source is a sequence frame (entry names its planes).
    bool append_plane(PixelParameters, std::uint32_t entry, const std::shared_ptr<MapBufferData>& palette);
    // Frame table entry of a loaded sequence frame, or an error status.
    DrawingStatus sequence_entry(ShapeSequence&, SHPStruct*, int frame, std::uint32_t& entry, bool prefix);
    void retire_sequence(ShapeSequence&);
    void publish_sequences();
    enum class ResourcePressure { pixels, sequence_table, voxel_models };
    bool reclaim_resource(ResourcePressure);
    void log_resource_pool(const char* event, std::size_t count = 0, std::size_t bytes = 0, double elapsed_us = 0) const;
    ShapeMode& sequence_mode(ShapeSequence&, ShapeFrame&, const ShapeDrawingRequest&, std::uint32_t aux, bool& created);
    void append_sequence_instance(RasterInstance, int variant, std::uint32_t entry,
                                  const std::shared_ptr<MapBufferData>&);
    bool announcing_ = false;  // a core announcement (shape_sequence) is loading a sequence
    static DrawingStatus shape_sequence(void*, SHPStruct*, std::uint32_t use) noexcept;
    MapShapeStats shape_stats_;
    void print_shape_stats(bool final) noexcept;
    std::map<WORD, std::shared_ptr<MapBufferData>> fills_;
    // Keys own canonical glyph bits, never a borrowed font address. A changed or
    // reloaded font therefore cannot alias an old glyph, even at the same address.
    std::map<std::tuple<int, int, std::string>, std::shared_ptr<MapBufferData>> masks_;
    struct StripImage {
        std::shared_ptr<MapBufferData> data;
        std::uint32_t entry = 0;
        TypeShapeMeta meta{};
        DrawingStatus status = DrawingStatus::unavailable;
    };
    std::map<std::tuple<std::uint64_t, const SHPStruct*, int, int, int, int>, StripImage> strips_;
    std::size_t mask_bytes_ = 0, strip_bytes_ = 0;
    struct VoxelEntry {
        std::weak_ptr<MapBufferData> image;
        std::shared_ptr<MapBufferData> resident;
        std::uint64_t used = 0;
        std::list<std::uint64_t>::iterator position;
    };
    std::unordered_map<std::uint64_t, VoxelEntry> voxel_images_;
    std::size_t voxel_cache_words_ = 0;
    std::list<std::uint64_t> voxel_lru_;

    std::unordered_map<std::uint64_t, std::weak_ptr<MapBufferData>> voxel_sources_;
    std::shared_ptr<MapBufferData> voxel_image(const VoxelImageHandle&, unsigned depth = 0);
    std::shared_ptr<MapBufferData> voxel_source(const VoxelBufferView&);
    struct LightingFrame {
        MapLightingImage image;
        DrawingStatus status = DrawingStatus::unavailable;
        bool decoded = false;
    };
    struct LightingAsset {
        std::uint64_t generation = 0, version = 0;
        std::vector<LightingFrame> frames;
        std::shared_ptr<const MapLightingSet> set;
    };
    std::unordered_map<const SHPStruct*, LightingAsset> lighting_assets_;
    std::size_t lighting_frame_count_ = 0;
    LightingAsset& lighting_asset(SHPStruct*);
    std::map<std::tuple<const std::uint32_t*, int, int>, MapIndexedData> indexed_;
    struct RasterContent {
        std::uint64_t version = 0, used = 0;
        int width = 0, height = 0, pitch = 0;
        RasterBlendMode mode{};
        std::shared_ptr<MapBufferData> source;
    };
    std::unordered_map<std::uint64_t, RasterContent> raster_contents_;
    // Working copy of the scene's slots. A chunk shared with a published scene is copied before its next
    // write; commit publishes the chunks the core names (BackgroundSceneRequest::dirty).
    struct SceneInstance {
        std::uint32_t slots = 0, variant = 0, drawn = 0;
        std::vector<std::shared_ptr<MapSceneChunk>> chunks;
        std::vector<std::shared_ptr<const MapSceneOrderChunk>> order;  // published draw order chunks
        std::vector<std::uint8_t> shared;                              // chunk is part of a published scene
    };
    std::vector<SceneInstance> scene_slot_instances_;
    bool scene_instances_dirty_ = false;
    bool scene_placing_ = false;   // item_end kept pending records for place
    std::uint8_t scene_kind_ = 0;  // BackgroundRecordKind of the record append_scene adds
    // The slot a core address names (std::out_of_range for an address no instance has).
    const MapSceneRecord& scene_peek(const BackgroundSlotAddress&) const;
    MapSceneRecord& scene_write(const BackgroundSlotAddress&);
    void clear_scene_slots();
    std::shared_ptr<const MapSceneData> scene_;
    std::uint64_t scene_world_ = 0, scene_epoch_ = 0, scene_version_ = 0;
    bool scene_recording_ = false, scene_reset_ = false;
    BackgroundSceneRequest scene_request_{};
    std::vector<MapSceneRecord> scene_pending_;
    int scene_width_ = 0, scene_height_ = 0, scene_quality_ = 2;
    // Scene lighting: the latest light per cell, the light-source combinations
    // held items of each cell use (see scene_light_use), working light chunks
    // (copied on the first write after a commit) and converter slots.
    std::unordered_map<std::uint32_t, BackgroundSceneLight> scene_light_;
    std::unordered_map<std::uint32_t, std::vector<std::uint16_t>> scene_light_use_;
    std::vector<std::shared_ptr<MapSceneLightChunk>> scene_light_chunks_;
    std::vector<char> scene_light_dirty_;
    std::vector<std::shared_ptr<MapSceneShroudChunk>> scene_shroud_chunks_;
    std::vector<char> scene_shroud_dirty_;
    std::shared_ptr<const std::vector<MapSceneAlpha>> scene_alphas_;
    bool scene_alphas_dirty_ = false;
    std::unordered_map<const ConvertClass*, std::int32_t> scene_converter_slots_;
    std::vector<std::shared_ptr<MapBufferData>> scene_converters_;
    bool scene_converters_dirty_ = false;
    std::map<const BytePalette*, std::shared_ptr<MapBufferData>> base_palettes_;
    void clear_scene_lighting();
    void set_scene_light(const BackgroundSceneLight&);
    bool scene_light_current(std::uint32_t cell, const BackgroundSceneLight&) const;
    // entry != 0: source is a shape sequence frame (entry names it, words[15]/[18] of an
    // auxiliary-depth packet name the SHA frame held by `auxiliary`); else a standalone image.
    void append_scene(const PixelParameters&, const std::shared_ptr<MapBufferData>&, const MapPaletteData&,
                      int intensity, int gradient = -1, int adjustment = 0, bool rle = false, std::uint32_t entry = 0,
                      const std::shared_ptr<MapBufferData>& auxiliary = {});
    std::shared_ptr<MapBufferData> base_palette(const BytePalette&);
    static DrawingStatus background_scene(void*, const BackgroundSceneRequest&) noexcept;
    std::uint64_t item_serial_ = 1;
    std::shared_ptr<MapBufferData> buffer(const std::vector<std::uint32_t>&);
    std::shared_ptr<MapBufferData> buffer(const std::uint32_t*, std::size_t);
    static DrawingStatus palette(void*, const BytePalette&, int, int, int, int, const DrawingPaletteHandle*&) noexcept;
    static DrawingStatus shape_palette(void*, const BytePalette&, int, const DrawingPaletteHandle*&) noexcept;
    static DrawingStatus color_scheme_palette(void*, const BytePalette&, int, const DrawingPaletteHandle*&) noexcept;
    static DrawingStatus converter_palette(void*, const ConvertClass&, std::uint64_t,
                                           const DrawingPaletteHandle*&) noexcept;
    enum class PaletteKind { terrain, shape, color_scheme };
    static DrawingStatus prepare_palette(void*, const BytePalette&, int, int, int, int, PaletteKind,
                                         const DrawingPaletteHandle*&) noexcept;
    static DrawingStatus tile(void*, const TileDrawingRequest&);
    static DrawingStatus shape(void*, const ShapeDrawingRequest&);
    static DrawingStatus indexed(void*, const IndexedDrawingRequest&);
    static DrawingStatus raster(void*, const RasterDrawingRequest&);
    static DrawingStatus mask(void*, const MaskDrawingRequest&) noexcept;
    static DrawingStatus shape_strip(void*, const ShapeStripDrawingRequest&) noexcept;
    static DrawingStatus lighting_shape(void*, const LightingShapeDrawingRequest&);
    static DrawingStatus lighting_shape_batch(void*, const LightingShapeBatchRequest&);
    DrawingStatus lighting_image(SHPStruct*, int, const MapLightingImage*&);
    static DrawingStatus screen_effect(void*, const ScreenEffectDrawingRequest&);
    static DrawingStatus background_complete(void*, const BackgroundDrawingRequest&);
};
}
