#pragma once
#include "render/frame.hpp"
#include "draw_resource_pool.hpp"
#include "raster_batch.hpp"
#include "scene_row_table.hpp"
#include "reference_frame.hpp"
#include <array>
#include <memory>
#include <vector>
namespace render {
// Presentation-owned copies survive original TMP/PAL teardown and queued GPU work.
struct MapVoxelImage;
// A shape sequence names its frames by dense indices into a frame table that
// the renderer publishes after the slot metadata: index MapSequenceFrameBase +
// allocation->first + frame. Entry words: byte offset of the 8-bit colour plane
// inside the sequence allocation, width, height, byte offset of the RLE prefix
// plane or -1. The renderer publishes absolute byte addresses. An instance
// carries that index where a single image carries its slot, so drawing a frame
// is one table read.
inline constexpr std::uint32_t MapSequenceFrameBase = DrawResourcePool::DefaultSlots, MapSequenceFrameLimit = 1u << 20;
struct MapSequenceTable {
    std::shared_ptr<const DrawResourcePool::Allocation> allocation;
    std::vector<std::array<std::int32_t, 4>> entries;
};
struct MapBufferData {
    std::uint64_t id = 0;
    std::unique_ptr<std::uint8_t[]> bytes;
    std::size_t byte_count = 0;
    std::shared_ptr<MapVoxelImage> voxel;
    std::size_t generated_words = 0;
    bool voxel_model = false;
    std::vector<std::uint32_t> voxel_reference;
    std::vector<std::shared_ptr<MapBufferData>> dependencies;
    std::shared_ptr<const DrawResourcePool::Allocation> allocation;
    std::shared_ptr<const MapSequenceTable> table;  // set for a shape sequence's texels
    std::uint64_t submit_frame = 0;
    int image_width = 0, image_height = 0;
    std::size_t word_count() const { return voxel ? generated_words : byte_count / 4; }
};
// Every live shape sequence at one moment. A frame holds the set current when it
// was acquired (sequences created later are listed in its own resources), so
// retiring a sequence never frees storage a queued frame may still draw from,
// and the renderer reads a set only when its revision is new.
struct MapSequenceSet {
    std::uint64_t revision = 0;
    std::vector<std::shared_ptr<MapBufferData>> data;
};
struct MapVoxelImage {
    VoxelKind kind{};
    int width = 0, height = 0, left = 0, top = 0;
    bool buffered = false;
    std::vector<std::array<std::int32_t, 16>> sections;
    std::array<std::uint32_t, 256> lighting{};
    std::array<std::int32_t, 10> shadow{};
    std::vector<std::shared_ptr<MapBufferData>> sources;
    std::shared_ptr<MapBufferData> palette, columns, mask;
    struct Layer {
        std::shared_ptr<MapBufferData> image;
        Point position;
        Rect clip;
    };
    std::vector<Layer> layers;
};
// Light sources of a resident background packet (map_raster.vert/.frag): intensity
// 0 = as recorded, 1..5 = the cell's Terrain (signed, unsigned), Normal
// (signed, unsigned) or the fixed 1000; palette 0 = as recorded, 1/2 = the
// base palette tinted by the cell's tint convention 0/1, 3 = a converter slot.
enum class MapLightSource : std::uint8_t { recorded, terrain_signed, terrain, normal_signed, normal, full };
enum class MapPaletteSource : std::uint8_t { recorded, cell_tint, sprite_tint, converter };
struct MapLightingImage {
    Rect bounds{};
    std::shared_ptr<MapBufferData> source;
};
struct MapLightingSet {
    std::vector<MapLightingImage> frames;
    std::shared_ptr<MapBufferData> residency;
};
// Final resident GPU record, produced once when the original background item
// changes. Resource versions are held for the immutable row's lifetime.
struct MapSceneRecord {
    std::array<std::int32_t, 12> words{};
    // A shape sequence frame is named by words[0] (frame table entry; see ShapePlanesFlag) and
    // source owns its sequence; auxiliary owns the SHA sequence of an auxiliary-depth record.
    std::shared_ptr<MapBufferData> source, palette, auxiliary;
    std::int8_t variant = 0;
    std::uint8_t kind = 0;  // BackgroundRecordKind: where the record came from
    bool operator==(const MapSceneRecord&) const = default;
    // A slot nothing was written to is a null record (all words zero): the shaders draw nothing for it.
    bool empty() const noexcept { return !source; }
};
struct MapScreenEffectMark {
    ScreenEffect request;
};
using MapLightField = LightField;
// Resident records live in the slots of TerrainSpriteLayer instances (core/src/presentation/scene/terrain_sprite_layer.hpp).
// A published scene holds each instance's slots in immutable chunks of MapSceneChunkSlots slots, found by
// (instance, chunk) in a persistent tree: a queued frame keeps the chunks it was made with, and the renderer
// compares two trees to find the chunks that changed. Only chunks that hold a record exist.
inline constexpr std::uint32_t MapSceneChunkSlots = 64;
struct MapSceneChunk {
    std::array<MapSceneRecord, MapSceneChunkSlots> slots;
};
struct MapSceneSlots : SceneRowTable<MapSceneChunk> {};
// Each instance's draw order (its occupied slots, ascending; BackgroundOrderUpdate) in immutable chunks of
// MapSceneOrderChunkEntries entries, found by (instance, chunk) like the slots. Entries at or beyond the
// instance's `drawn` count are stale.
inline constexpr std::uint32_t MapSceneOrderChunkEntries = 256;
struct MapSceneOrderChunk {
    std::array<std::uint32_t, MapSceneOrderChunkEntries> slots{};
};
struct MapSceneOrder : SceneRowTable<MapSceneOrderChunk> {};
// One instance of the layer: its slot count, raster pipeline state (depth * RasterBlends + blend) and the
// length of its draw order.
struct MapSceneInstance {
    std::uint32_t slots = 0, variant = 0, drawn = 0;
};
// Cell lights for map_raster.vert/.frag, 8 words per cell index (y * 512 + x):
// Normal | Terrain << 16; tint[0].shades | tint[1].shades << 16; the three
// tint[0] factors; the three tint[1] factors.
inline constexpr std::uint32_t MapLightChunkCells = 1024, MapLightWords = 8;
struct MapSceneLightChunk {
    std::array<std::int32_t, MapLightChunkCells * MapLightWords> words{};
};
// SHROUD.SHP frame per cell index (-1: none), for the light field.
struct MapSceneShroudChunk {
    std::array<std::int32_t, MapLightChunkCells> frames;
    MapSceneShroudChunk() { frames.fill(-1); }
};
struct MapSceneLights : SceneRowTable<MapSceneLightChunk> {};
struct MapSceneShroud : SceneRowTable<MapSceneShroudChunk> {};
// An AlphaShapeClass light: frame 0 of its image at a world position.
struct MapSceneAlpha {
    std::uint64_t id = 0;
    Point position{};
    MapLightingImage image;
};
struct MapSceneData {
    std::uint64_t epoch = 0, revision = 0;
    std::shared_ptr<const MapSceneSlots> slots = std::make_shared<MapSceneSlots>();
    std::shared_ptr<const MapSceneOrder> order = std::make_shared<MapSceneOrder>();
    std::shared_ptr<const std::vector<MapSceneInstance>> instances = std::make_shared<std::vector<MapSceneInstance>>();
    std::shared_ptr<const MapSceneLights> lights = std::make_shared<MapSceneLights>();
    std::shared_ptr<const MapSceneShroud> shroud = std::make_shared<MapSceneShroud>();
    std::uint32_t light_chunks = 0;
    std::shared_ptr<const std::vector<std::shared_ptr<MapBufferData>>> converters =
        std::make_shared<const std::vector<std::shared_ptr<MapBufferData>>>();
    std::shared_ptr<const std::vector<MapSceneAlpha>> alphas;  // AlphaShapeClass::Array order
};
// The ranges (per instance, in drawing order) the core computed for this draw: what the viewport can touch
// and what a window around it can touch (the pages of the renderer's page cache are made from the latter).
// Ranges index the instance's draw order (MapSceneOrder), not its slots.
struct MapSceneMark {
    std::shared_ptr<const MapSceneData> scene;
    Point origin{};
    Rect clip{};
    std::vector<SlotRange> visible, window;
};
// Commands are appended alongside final records, in original call order.
// A mark is an explicit command and cannot be crossed by draw batching.
using MapCommandKind = PassKind;
using MapCommand = PassRange;
struct MapFrameData {
    std::uint64_t serial = 0;
    std::shared_ptr<ReferenceFrame> reference;
    std::uint64_t generation = 0;
    int width = 0, height = 0;
    ScreenEffect screen_effect{};
    std::vector<MapScreenEffectMark> screen_effect_marks;
    Point view_origin{};
    std::vector<MapLightField> light_fields;
    std::vector<MapSceneMark> scenes;
    std::vector<RasterInstance> instances;
    std::vector<PixelParameters> compute;
    std::vector<std::int32_t> compute_resources;
    std::vector<std::int32_t> light_bins;
    std::vector<MapCommand> commands;
    std::vector<std::int32_t> field_words;
    std::vector<std::shared_ptr<MapBufferData>> resources;
    std::vector<std::shared_ptr<MapBufferData>> sequence_resources;
    std::shared_ptr<const MapSequenceSet> sequences;
    std::vector<std::shared_ptr<const MapSequenceSet>> retired_sequences;
};
struct FramePayload : MapFrameData {
    std::vector<ResourceUpdate> resource_views;
    std::vector<SceneDelta> scene_views;
    std::vector<ScreenEffect> effect_views;
};
}
