#pragma once
#include "api/type_drawing.hpp"
#include "yrpp/Matrix3D.h"
class TechnoClass;
class FootClass;
class CellClass;
class HouseClass;
class AnimClass;
class ObjectClass;
struct VoxelStruct;
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
};
struct TechnoDrawing {
    TypeDrawingContext types;
    const TechnoClass* submitting_object = nullptr;
    const Matrix3D* local_voxel_matrix = nullptr;
    void* context = nullptr;
    CellClass* (*cell_at)(void*, const CellStruct&) noexcept = nullptr;
    CellClass* (*cell_at_world)(void*, const CoordStruct&) noexcept = nullptr;
    DrawingStatus (*initialize_light)(void*, CellClass&) noexcept = nullptr;
    DrawingStatus (*palette)(void*, const TechnoClass&, TechnoPalette, CellClass*,
        HouseClass*, const DrawingPaletteHandle*&) noexcept = nullptr;
    DrawingStatus (*voxel)(void*, const TechnoVoxelRequest&) noexcept = nullptr;
    DrawingStatus (*animation)(void*, const TechnoClass&, AnimClass&,
        const Point2D&, const RectangleStruct&) noexcept = nullptr;
    SHPStruct* (*shape_data)(SHPStruct*) = nullptr;
    RectangleStruct (*frame_bounds)(SHPStruct*,int) = nullptr;
    SHPStruct* (*load_shape)(void*, const char*) = nullptr;
    int (*height)(int) noexcept = nullptr;
    int (*ground_height)(void*, const CoordStruct&) noexcept = nullptr;
    Point2D (*project)(void*, const CoordStruct&) noexcept = nullptr;
    bool (*shrouded)(CellClass&) noexcept = nullptr;
    bool (*fogged)(void*, const CoordStruct&) noexcept = nullptr;
    void (*selectable)(void*, ObjectClass&, const Point2D&) noexcept = nullptr;
    Matrix3D camera;
    HouseClass* player = nullptr;
    SHPStruct* building_depth = nullptr;
    RectangleStruct tactical_rect{}, composite_rect{};
    int frame = 0, bridge_height = 416, level_height = 104;
    int level_light = 0, extra_infantry_light = 0, extra_unit_light = 0, extra_aircraft_light = 0;
    unsigned laser_tint = 0, shield_tint = 0, berserk_tint = 0, iron_tint = 0;
    bool draw_shadows = true, fog_of_war = false, debug_map = false, window_active = false;
    bool use_voxel_buffer = false;
    DrawingStatus status = DrawingStatus::skipped;
    TechnoDrawing() { camera.MakeIdentity(); }
};
class VoxelMatrixScope {
    TechnoDrawing& drawing;
    const Matrix3D* previous;
public:
    VoxelMatrixScope(TechnoDrawing& d,const Matrix3D& m) noexcept : drawing(d),previous(d.local_voxel_matrix) { d.local_voxel_matrix=&m; }
    ~VoxelMatrixScope() { drawing.local_voxel_matrix=previous; }
};
TechnoDrawing* techno_drawing() noexcept;
DrawingStatus with_techno_drawing(TechnoDrawing&, void(*)(void*), void*) noexcept;
DrawingStatus draw_techno_object(const TechnoClass&, TechnoDrawing&, Point2D, RectangleStruct) noexcept;
bool techno_drawing_complete(DrawingStatus) noexcept;
void record_techno_drawing(TechnoDrawing&, DrawingStatus) noexcept;
CellClass* techno_drawing_cell(TechnoDrawing&, const TechnoClass&);
int techno_drawing_height(const TechnoDrawing&, int) noexcept;
bool techno_drawing_palette(TechnoDrawing&, const TechnoClass&, TechnoPalette,
    CellClass*, HouseClass*, const DrawingPaletteHandle*&) noexcept;
bool techno_submit_shape(TechnoDrawing&, const TechnoClass&, TechnoPalette, CellClass*,
    HouseClass*, ShapeDrawingRequest&) noexcept;
unsigned techno_color_mask(const unsigned char*, int format) noexcept;
}
