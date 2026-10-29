#pragma once
#include "presentation/context.hpp"
#include "type_drawing_context.hpp"
#include "yrpp/Matrix3D.h"
class TechnoClass;
class TechnoTypeClass;
class FootClass;
class CellClass;
class HouseClass;
class AnimClass;
class ObjectClass;
struct VoxelStruct;
class Surface;
namespace game {
// Private synchronous drawing environment. Original objects own all choices;
// hosts supply borrowed resources and consume the resulting drawing requests.
enum class TechnoPalette { house, cell, animation, normal, eight_bit };
struct TechnoVoxelRequest {
    const TechnoClass* object = nullptr;
    const VoxelStruct* resource = nullptr;
    Matrix3D matrix;
    // Borrowed pre-camera transform for hosts that already own projection.
    // The original entry still receives the complete matrix above.
    const Matrix3D* local_matrix = nullptr;
    Point2D position{};
    RectangleStruct clip{};
    const DrawingPaletteHandle* palette = nullptr;
    int frame = 0, cache_key = -1, shadow_layer = 0;
    int intensity = 1000, depth = 0;
    unsigned flags = 0x2800, tint = 0;
    bool shadow = false, half_shadow = false, use_buffer = false;
    // Borrowed only until the synchronous voxel callback returns. Cached
    // shadows own the sampled pixels, never this temporary composite surface.
    Surface* shadow_exclusion = nullptr;
    Point2D shadow_offset{};
    const void* shadow_cache = nullptr;
    bool shadow_force = true;
    RectangleStruct* raster_bounds = nullptr;
};
struct UnitVoxelCompositeRequest {
    const TechnoClass* object = nullptr;
    Surface* source = nullptr;
    Surface* target = nullptr;
    RectangleStruct source_rect{}, clip{};
    Point2D position{};
    const DrawingPaletteHandle* palette = nullptr;
    int intensity = 1000, depth = 0, gradient = 2, ripple = 0;
    unsigned flags = 0x2800, tint = 0;
};
struct TechnoDrawing {
    const TechnoTypeClass* appearance_type = nullptr;
    TypeDrawingContext types;
    const TechnoClass* submitting_object = nullptr;
    const Matrix3D* local_voxel_matrix = nullptr;
    void* context = nullptr;
    DrawingStatus (*palette)(void*, const TechnoClass&, TechnoPalette, CellClass*, HouseClass*,
                             const DrawingPaletteHandle*&) noexcept = nullptr;
    DrawingStatus (*voxel)(void*, const TechnoVoxelRequest&) noexcept = nullptr;
    // The last argument draws an animation that the ordinary pass hides (the voxel barrel animation).
    DrawingStatus (*animation)(void*, const TechnoClass&, AnimClass&, const Point2D&, const RectangleStruct&,
                               bool) noexcept = nullptr;
    SHPStruct* (*shape_data)(SHPStruct*) = nullptr;
    RectangleStruct (*frame_bounds)(SHPStruct*, int) = nullptr;
    Point2D (*project)(void*, const CoordStruct&) noexcept = nullptr;
    Matrix3D camera;
    SHPStruct* building_depth = nullptr;
    RectangleStruct tactical_rect{}, composite_rect{};
    int frame = 0, bridge_height = 416, level_height = 104;
    int level_light = 0, extra_infantry_light = 0, extra_unit_light = 0, extra_aircraft_light = 0;
    unsigned laser_tint = 0, shield_tint = 0, berserk_tint = 0, iron_tint = 0;
    bool draw_shadows = true, fog_of_war = false, debug_map = false, window_active = false;
    bool use_voxel_buffer = false;
    Surface* voxel_surface = nullptr;
    Surface* unit_voxel_surface = nullptr;
    // GPU composites retain the surface's bounds and identity, but neither
    // consume its pixels nor need the original software cleanup writes.
    bool unit_voxel_software = true;
    DrawingStatus (*begin_unit_voxel)(void*, Surface*&) noexcept = nullptr;
    DrawingStatus (*submit_unit_voxel)(void*, const UnitVoxelCompositeRequest&) noexcept = nullptr;
    bool inspect_voxel_parts = false, voxel_cache_enabled = true;
    DrawingStatus status = DrawingStatus::skipped;
    TechnoDrawing() { camera.MakeIdentity(); }
};
class VoxelMatrixScope {
    TechnoDrawing& drawing;
    const Matrix3D* previous;

public:
    VoxelMatrixScope(TechnoDrawing& d, const Matrix3D& m) noexcept : drawing(d), previous(d.local_voxel_matrix) {
        d.local_voxel_matrix = &m;
    }
    ~VoxelMatrixScope() { drawing.local_voxel_matrix = previous; }
};
DrawingStatus with_techno_drawing(TechnoDrawing&, void (*)(void*, PresentationContext&), void*,
                                          PresentationContext = {}) noexcept;
DrawingStatus draw_techno_object(const TechnoClass&, TechnoDrawing&, Point2D, RectangleStruct,
                                 PresentationContext outer = {}) noexcept;
bool techno_drawing_complete(DrawingStatus) noexcept;
void record_techno_drawing(TechnoDrawing&, DrawingStatus) noexcept;
// Original cell/location queries; the host supplies no map or fog callbacks.
CellClass* techno_drawing_cell(const TechnoClass&);
bool techno_cell_shrouded(CellClass&) noexcept;
bool techno_location_fogged(const CoordStruct&) noexcept;
bool techno_drawing_palette(TechnoDrawing&, const TechnoClass&, TechnoPalette, CellClass*, HouseClass*,
                            const DrawingPaletteHandle*&) noexcept;
bool techno_submit_shape(TechnoDrawing&, const TechnoClass&, TechnoPalette, CellClass*, HouseClass*,
                         ShapeDrawingRequest&) noexcept;
// Original 0x706919 / 0x707597 union, including right/bottom extension.
void accumulate_unit_voxel_bounds(RectangleStruct&, const RectangleStruct&) noexcept;
unsigned techno_color_mask(const unsigned char*, int format) noexcept;
}
