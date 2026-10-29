#pragma once
#include "type_drawing_context.hpp"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include "yrpp/FileSystem.h"
#include <array>
#include <memory>
#include <vector>
#include <cstdint>
class Surface;
class BSurface;
class AircraftClass;
class ObjectTypeClass;
class BuildingClass;
class BuildingTypeClass;
class UnitClass;
class UnitTypeClass;
namespace game {
class VoxelImageHandle;
struct VoxelGpuAssets;
// Internal host-owned presentation data; no additions to original class layout.
struct VoxelPalette {
    std::vector<std::uint8_t> lighting;
    unsigned sections = 0;
};
struct VoxelSurface {
    int width = 0, height = 0;
    Point2D offset{};
    std::vector<std::uint32_t> pixels;
    std::shared_ptr<VoxelImageHandle> gpu;
};
struct BuildingVoxelPart {
    const VoxelStruct* resource = nullptr;
    Matrix3D local;
    unsigned frame = 0;
    bool barrel = false;
    bool use_buffer = false;
    bool shadow = false;
    int cache_key = -1;
    int shadow_layer = 0;
    bool half_shadow = false;
    bool camera_applied = false;
    bool apply_hva = true;
    int voxel_index = -1;
    Surface* shadow_exclusion = nullptr;
    Point2D shadow_offset{};
    const void* shadow_cache = nullptr;
    bool shadow_force = true;
};
struct BuildingVoxelPlan {
    std::array<BuildingVoxelPart, 2> parts;
    unsigned count = 0;
    bool mixed = false, barrel_after_shp = false;
};
// Called when a type replaces its VXL/HVA objects, so cached surfaces are rebuilt.
void voxel_assets_changed() noexcept;
// Uses original UnitType voxel resources and the actual locomotor transform.
bool unit_voxel_parts(UnitClass&, BuildingVoxelPart* parts, unsigned capacity, unsigned& count) noexcept;
bool load_voxel_palette(VoxelPalette&) noexcept;
bool building_voxel_plan(const BuildingClass&, BuildingVoxelPlan&) noexcept;
Matrix3D building_voxel_camera() noexcept;
DrawingStatus composite_unit_voxel(BSurface&, Point2D, const RectangleStruct&, const VoxelSurface&) noexcept;
// 0x756860: occupied XY columns, including the original linear right neighbour
// write. The caller supplies 0x10001 bytes and already validated span storage.
DrawingStatus raster_voxel_shadow_columns(const std::int32_t* spans, unsigned nx, unsigned ny,
                                          const Vector3D<float>* points, Point2D& offset, float cx, float cy,
                                          Surface* exclusion, std::uint8_t* pixels, std::uint32_t pixel_count) noexcept;
DrawingStatus render_building_voxel(const BuildingVoxelPart&, const VoxelPalette&, VoxelSurface&) noexcept;
DrawingStatus prepare_building_voxel(const BuildingVoxelPart&, const VoxelPalette&, VoxelGpuAssets&,
                                     VoxelSurface&) noexcept;
std::uint64_t voxel_resource_revision() noexcept;
std::uint64_t building_voxel_signature(const BuildingClass&) noexcept;
}
