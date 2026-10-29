#pragma once
// Presentation state of one map world: every cache the drawing path keeps between and within frames
// (resident background layout, shroud/alpha bookkeeping, palettes, voxel image caches) and the per-Render frame binding. None of it is game state; the
// simulation never reads it, and the purity check does not look at it.
#include "presentation/scene/terrain_sprite_layer.hpp"
#include "building_voxel.hpp"
#include "voxel_drawing.hpp"
#include "api/map_view.hpp"
#include "type_drawing_context.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralDefinitions.h"
#include <map>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>
class BuildingTypeClass;
class ObjectClass;
class AlphaShapeClass;
class CellClass;
class ConvertClass;
class Matrix3D;
struct SHPStruct;
namespace game {
class MapWorld;
struct TechnoDrawing;
struct BackgroundInvalidationView;
// What the original keeps on the Tactical between two Render calls for its incremental redraw
// (LastTacticalPos, the view origin at +0xD6C, the first-pass flag) plus the host's redraw epoch.
// The presenter writes these instead of game memory; no simulation code reads the Tactical copies.
struct TacticalMemo {
    Point2D drawn_camera{};
    Point2D view_origin{};
    bool presented = false;
    // One count per tactical render (the host request the original makes through MarkNeedsRedraw);
    // the overlay redraw stamp is the map's own Redraws byte plus this count.
    std::uint8_t redraw_epoch = 0;
};
struct PresentationState {
    struct BuildingRect {
        const BuildingTypeClass* type = nullptr;
        std::uint64_t generation = 0;
        RectangleStruct rect{};
        bool valid = false;
    };
    std::vector<BuildingRect> building_rects;
    std::vector<std::uint8_t> layer_visibility;
    std::vector<ObjectClass*> visible_buildings;
    TacticalMemo tactical;
    // The resident background's slot layout (TerrainSpriteLayer); laid out again at every full recording.
    TerrainSpriteLayer background_layer;
    // Terrain objects the layer has seen live since that recording (comparison-only identities).
    std::unordered_set<std::uintptr_t> background_objects;
    // Set when resident background recording failed; this world then draws its
    // background immediately until the map is reloaded.
    bool background_scene_failed = false;
    // World shroud (draw_world_shroud): GetOcclusion(cell, false/true) of every
    // map cell (index y * 512 + x), recomputed for shroud invalidations only;
    // frames the scene has not received yet. Visibility/Foggedness are advanced by the render-phase state step.
    std::vector<std::int8_t> occlusion_shroud, occlusion_fog;
    bool occlusion_ready = false, occlusion_hidden = false;
    std::vector<BackgroundShroudCell> shroud_changes;
    // The alpha_shapes_changed() revision and Tactical bounds origin of the
    // AlphaShape set the scene last received.
    std::unordered_map<const AlphaShapeClass*, SHPStruct*> alpha_images;
    std::uint64_t alpha_revision = 0;
    Point2D alpha_origin{};
    bool alpha_sent = false;
    // Palettes of the host-side object drawing and the remapped copies derived from them.
    BytePalette unit_palette{}, anim_palette{}, selection_palette{};
    bool selection_palette_loaded = false;
    std::map<std::pair<const BytePalette*, std::uint32_t>, std::unique_ptr<BytePalette>> remaps;
    VoxelPalette voxel_palette;
    bool gpu_voxels = false;
    VoxelGpuAssets gpu_voxel_assets;
    bool voxel_palette_loaded = false;
    std::uint64_t voxel_asset_revision = 0;
    // The arbitrary-matrix image cache is used only by the CPU reference host.
    std::map<std::pair<const VoxelStruct*, std::array<std::uint32_t, 15>>, std::shared_ptr<VoxelSurface>>
        voxel_surfaces;
    // Original keys retain first-value semantics. GPU entries own immutable
    // generation descriptions and dependencies, never rasterized CPU pixels.
    std::map<std::pair<const VoxelStruct*, int>, std::shared_ptr<VoxelSurface>> unit_voxel_surfaces;
    // Nested original drawing borrows one mutable scratch entry per depth.
    std::vector<std::unique_ptr<VoxelCompositeScratch>> unit_composite_pool;
    std::size_t unit_composite_depth = 0;
    VoxelCompositeCache voxel_composites;
    std::map<std::pair<const void*, int>, std::shared_ptr<VoxelSurface>> shadow_voxel_surfaces;
    std::uint32_t missing_voxel_parts = 0, invalid_voxel_parts = 0;
};
// Attribution of a submitted terminal request. Borrowed only during its
// backend callback; it is not another object model or a rendering decision.
struct WorldDrawingSource {
    ObjectClass* owner = nullptr;
    CellClass* cell = nullptr;
    const BytePalette* palette = nullptr;
    const ConvertClass* converter = nullptr;
    bool cell_tint = false, color_scheme = false;
};
// Deferred background commands preserve the original pass flush. They already
// contain the actual generic packet and resolved palette. Indexed storage stays
// alive until the packet has been consumed; no geometry is regenerated here.
struct PendingWorldDrawing {
    ShapeDrawingRequest shape;
    IndexedDrawingRequest indexed;
    WorldDrawingSource source;
    std::shared_ptr<VoxelSurface> storage;
    bool is_indexed = false;
};
// One Tactical pass's lighting requests, submitted as one ordered batch (begin_lighting_batch).
struct LightingBatch {
    std::vector<LightingShapeBatchEntry> entries;
    CellStruct cell{-1, -1};
    DrawingStatus status = DrawingStatus::skipped;
    SHPStruct* shroud = nullptr;
    Point2D shroud_origin{}, shroud_camera{};
};
// Synchronous borrowed host bindings of one Render. TacticalClass owns scene traversal and
// pass order; the host owns resources, cached requests and the output target.
struct PresentationFrame {
    MapWorld* world = nullptr;
    const MapDrawingContext* drawing = nullptr;
    RectangleStruct bounds{};
    MapDrawStatistics* statistics = nullptr;
    DrawingStatus status = DrawingStatus::skipped;
    bool include_terrain = true;
    bool draw_world = false;        // Derived from the bound world at render entry.
    unsigned background_requests = 0;
    std::vector<PendingWorldDrawing> pending_background;
    const WorldDrawingSource* source = nullptr;
    bool submit_body = false;
    bool recording_background = false;
    // The lighting batch of the pass in progress, and the mark a drawing step leaves on a recorded
    // background item it skipped work for (cull_background_item).
    LightingBatch lighting;
    bool lighting_active = false;
    bool background_item_culled = false;
    const Matrix3D* voxel_camera = nullptr;
    TechnoDrawing* techno_environment = nullptr;
    bool techno_environment_ready = false;
    // The invalidations this frame consumed (set by the presenter when it takes them); null when it took none.
    const BackgroundInvalidationView* invalidations = nullptr;
    PresentationState* state = nullptr;
};
}
