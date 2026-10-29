#pragma once
#include "api/map_view.hpp"
#include "type_drawing_context.hpp"
#include "presentation/context.hpp"
#include "presentation/state.hpp"
class TacticalClass;
class CellClass;
class ObjectClass;
class BuildingClass;
class Matrix3D;
class TechnoClass;
class TechnoTypeClass;
class HouseClass;
class ConvertClass;
namespace game {
class MapWorld;
struct TechnoDrawing;
struct PresentationContext;
struct BackgroundInvalidationView;
DrawingStatus draw_background_scene(PresentationContext&, const BackgroundInvalidationView&) noexcept;
// True when this frame's background goes through the resident scene and the
// host resolves shroud from world cells (drawing version 24).
bool world_shroud_available(const PresentationFrame&) noexcept;
// Replaces DrawShroud's per-cell pass: updates cached occlusion for shroud
// invalidations, gives cells in view the Visibility/Foggedness DrawShroudAndFog
// would write, and submits the shroud batch; AlphaShapes are scene alphas.
DrawingStatus draw_world_shroud(PresentationContext&, TacticalClass&, const BackgroundInvalidationView&,
                                const RectangleStruct& bounds) noexcept;
enum class TacticalObjectPass { behind, body, building_upper, extras, building_info };
// Binds per-object resources and submits that object's requests before the
// next original hook. Selection and traversal remain in TacticalClass.
DrawingStatus dispatch_tactical_object(PresentationContext&, ObjectClass&, TacticalObjectPass, Point2D*, RectangleStruct*, bool) noexcept;
DrawingStatus draw_tactical_building_preview(PresentationContext&, BuildingClass&, Point2D, const RectangleStruct&) noexcept;
// Binds the per-render environment of `frame` (the exact voxel camera and the techno drawing environment)
// around `call` and hands it a context whose frame is `frame`. `outer` supplies the scopes it inherits.
// The callback is synchronous; an exception becomes backend_failure.
DrawingStatus with_tactical_frame(PresentationFrame&, void (*)(void*, PresentationContext&), void*,
                                  PresentationContext outer = {}) noexcept;
// One original TacticalClass::Render pass (mode 0x3) inside the verification scope.
void render_tactical(TacticalClass&, PresentationFrame&);
DrawingStatus draw_tactical_view(TacticalClass&, MapWorld*, const MapDrawingContext&, const RectangleStruct&,
                                 MapDrawStatistics&) noexcept;
// Bind resources and own request/cache lifetime around an original pass sequence.
// The callback is synchronous; exceptions and backend failures stay on this side.
DrawingStatus with_tactical_render_resources(PresentationFrame&, bool foreground, void (*)(void*, PresentationContext&),
                                             void*, PresentationContext outer = {}) noexcept;
void submit_tactical_background(PresentationContext&) noexcept;
SHPStruct* world_sprite_data(SHPStruct*);
// The single palette decision for retained and direct world drawing. Each
// consumer maps its own request kind onto a source and applies the choice to its
// output: a converter wins over the raw palette, which is owned by the world.
enum class WorldPaletteSource { selection, animation, cell, color_scheme, techno_house, techno_unit };
struct WorldPaletteRequest {
    const TechnoTypeClass* appearance_type = nullptr;
    WorldPaletteSource source = WorldPaletteSource::selection;
    int scheme_index = 0;                 // color_scheme only
    const TechnoClass* object = nullptr;  // techno_house and techno_unit
    HouseClass* house = nullptr;          // optional owner override for techno sources
};
struct WorldPaletteChoice {
    const ConvertClass* converter = nullptr;
    const BytePalette* palette = nullptr;
};
DrawingStatus choose_world_palette(MapWorld&, const WorldPaletteRequest&, WorldPaletteChoice&) noexcept;
// Submit the actual generic packet, or retain that packet until the original
// background flush when a background body/overlay is being traversed.
DrawingStatus submit_world_shape(PresentationFrame&, const ShapeDrawingRequest&, ObjectClass*, CellClass*,
                                 const ConvertClass*, bool cell_tint, bool color_scheme) noexcept;
DrawingStatus submit_world_indexed(PresentationFrame&, const IndexedDrawingRequest&, ObjectClass*, CellClass*,
                                   const ConvertClass*, bool cell_tint, bool color_scheme, bool use_clip = false,
                                   std::shared_ptr<VoxelSurface> storage = {}) noexcept;
DrawingStatus flush_world_background(PresentationFrame&) noexcept;
// Folds a result into the status of the frame the context (or the frame itself) is drawing.
void record_tactical_drawing(PresentationFrame&, DrawingStatus) noexcept;
inline void record_tactical_drawing(const PresentationContext& ctx, DrawingStatus status) noexcept {
    if (ctx.frame) record_tactical_drawing(*ctx.frame, status);
}
DrawingStatus draw_cell_terrain(CellClass&, const MapDrawingContext&, const Point2D&, const RectangleStruct&) noexcept;
// Bind resources around exactly one original visibility entry; no pass selection here.
DrawingStatus draw_tactical_object(PresentationContext&, ObjectClass&, bool, const RectangleStruct&, bool upper) noexcept;
DrawingStatus draw_lighting_shape(PresentationFrame&, SHPStruct*, int, const Point2D&, const RectangleStruct&,
                                  RasterBlendMode) noexcept;
// Optional batching of one pass's lighting requests (see LightingShapeBatchRequest).
// begin returns false when the backend has no batch callback; requests are then
// submitted one by one. lighting_batch_cell names the cell of following requests.
bool begin_lighting_batch(PresentationFrame&) noexcept;
void lighting_batch_cell(PresentationFrame&, const CellStruct&) noexcept;
// The batch starts with the world shroud of every cell (LightingShapeBatchRequest::shroud).
void lighting_batch_world_shroud(PresentationFrame&, SHPStruct* image, const Point2D& footprint_origin,
                                 const Point2D& camera) noexcept;
// AlphaShapeClass::Array changed; the resident scene sends the new set with
// its next transaction (BackgroundSceneOperation::alpha).
DrawingStatus end_lighting_batch(PresentationFrame&, const RectangleStruct& view) noexcept;
void cancel_lighting_batch(PresentationFrame&) noexcept;
// A drawing step inside a recorded background item skipped work because of the
// clip, so the item ends with discard instead of end. take_ returns and clears
// the mark.
void cull_background_item(PresentationFrame&) noexcept;
bool take_background_item_culled(PresentationFrame&) noexcept;
// The same on the frame of a context; without a frame they do nothing and report unavailable.
inline DrawingStatus draw_lighting_shape(const PresentationContext& ctx, SHPStruct* image, int index, const Point2D& at,
                                         const RectangleStruct& clip, RasterBlendMode mode) noexcept {
    return ctx.frame ? draw_lighting_shape(*ctx.frame, image, index, at, clip, mode) : DrawingStatus::unavailable;
}
inline bool begin_lighting_batch(const PresentationContext& ctx) noexcept {
    return ctx.frame && begin_lighting_batch(*ctx.frame);
}
inline void lighting_batch_cell(const PresentationContext& ctx, const CellStruct& cell) noexcept {
    if (ctx.frame) lighting_batch_cell(*ctx.frame, cell);
}
inline void lighting_batch_world_shroud(const PresentationContext& ctx, SHPStruct* image, const Point2D& origin,
                                        const Point2D& camera) noexcept {
    if (ctx.frame) lighting_batch_world_shroud(*ctx.frame, image, origin, camera);
}
inline DrawingStatus end_lighting_batch(const PresentationContext& ctx, const RectangleStruct& view) noexcept {
    return ctx.frame ? end_lighting_batch(*ctx.frame, view) : DrawingStatus::unavailable;
}
inline void cancel_lighting_batch(const PresentationContext& ctx) noexcept {
    if (ctx.frame) cancel_lighting_batch(*ctx.frame);
}
inline void cull_background_item(const PresentationContext& ctx) noexcept {
    if (ctx.frame) cull_background_item(*ctx.frame);
}
inline bool take_background_item_culled(const PresentationContext& ctx) noexcept {
    return ctx.frame && take_background_item_culled(*ctx.frame);
}
}
