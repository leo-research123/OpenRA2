#pragma once
#include "api/type_drawing.hpp"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include "yrpp/FileSystem.h"
#include <array>
#include <memory>
#include <vector>
#include <cstdint>
class AircraftClass;class ObjectTypeClass;class BuildingClass;class BuildingTypeClass;class UnitClass;class UnitTypeClass;
namespace game {
// Internal host-owned presentation data; no additions to original class layout.
struct VoxelPalette { std::vector<std::uint8_t> lighting; unsigned sections=0; };
struct VoxelSurface {
 int width=0,height=0;Point2D offset{};
 std::vector<std::uint32_t> pixels;
};
struct BuildingVoxelPart { const VoxelStruct* resource=nullptr;Matrix3D local;unsigned frame=0;bool barrel=false;bool use_buffer=false;bool shadow=false;int cache_key=-1;int shadow_layer=0;bool half_shadow=false;bool camera_applied=false; };
struct BuildingVoxelPlan { std::array<BuildingVoxelPart,2> parts;unsigned count=0;bool mixed=false,barrel_after_shp=false; };
bool load_building_voxels(BuildingTypeClass&) noexcept;
bool load_object_voxels(ObjectTypeClass&) noexcept;
// Uses original UnitType voxel resources and the actual locomotor transform.
bool unit_voxel_parts(UnitClass&,BuildingVoxelPart* parts,unsigned capacity,unsigned& count) noexcept;
bool load_voxel_palette(VoxelPalette&) noexcept;
bool building_voxel_plan(const BuildingClass&,BuildingVoxelPlan&) noexcept;
Matrix3D building_voxel_camera() noexcept;
DrawingStatus render_building_voxel(const BuildingVoxelPart&,const VoxelPalette&,VoxelSurface&) noexcept;
std::uint64_t voxel_resource_revision() noexcept;
std::uint64_t building_voxel_signature(const BuildingClass&) noexcept;
}
