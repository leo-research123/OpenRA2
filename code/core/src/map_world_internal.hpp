#pragma once
#include "map_world.hpp"
#include "presentation/state.hpp"
#include "api/map_objects.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/EventClass.h"
#include "yrpp/MissionClass.h"
#include <map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <string>
class HouseClass;
class AnimClass;
class HouseClass;
struct SHPStruct;
namespace game {
struct MapWorld::Impl {
    MapViewHandle& view;
    std::uint64_t world_id = 0;
    // Everything the drawing path caches for this world (see presentation/state.hpp).
    PresentationState presentation;
    int building_types = 0, terrain_types = 0;
    int buildings = 0, terrains = 0, animations = 0, infantry = 0, units = 0, aircraft = 0;
    int bullets = 0, line_trails = 0, electric_bolts = 0, lasers = 0, particle_systems = 0;
    // Projectile visuals existed after the previous frame; their disappearance also redraws.
    bool projectile_visuals = false;
    bool loaded = false;
    MapObjectId hover{};
    int player_power_bonus = 0;
    std::vector<std::pair<std::string, std::string>> deferred_sound_fields;
    std::uint32_t missing_images = 0, unknown_records = 0, missing_palettes = 0;
    std::uint64_t selectable_revision = ~std::uint64_t{}, selectable_tick = ~std::uint64_t{},
                  selectable_spatial_generation = 0;
    Point2D selectable_camera{}, selectable_viewport{};
    RectangleStruct selectable_bounds{}, selectable_view_bounds{};
    const HouseClass* selectable_player = nullptr;
    std::uint64_t selectable_resources = 0, selectable_table_revision = 0;
    int selectable_count = 0;
    unsigned selectable_environment = 0;
    bool selectables_valid = false;
    std::vector<CellClass*> resource_cells;
    std::vector<unsigned char> resource_membership;
    explicit Impl(MapViewHandle&);
};
ObjectClass* resolve_map_object(MapWorld&, MapObjectId) noexcept;
MapObjectId object_id(const MapWorld&, const ObjectClass*) noexcept;
void snapshot_object(MapWorld&, ObjectClass&, MapObjectSnapshot&) noexcept;
struct TechnoVoxelRequest;
DrawingStatus cache_world_voxel(MapWorld&, const TechnoVoxelRequest&, std::shared_ptr<VoxelSurface>&,
                                std::shared_ptr<const VoxelImageHandle> mask = {}) noexcept;
DrawingStatus cache_world_voxel(MapWorld&, const BuildingVoxelPart&, std::shared_ptr<VoxelSurface>&,
                                std::shared_ptr<const VoxelImageHandle> mask = {}) noexcept;
void rebuild_world_selectables(MapWorld&);
void register_world_selectables(MapWorld&);
ObjectClass* pick_world_object(MapWorld&, Point2D);
const BytePalette* world_palette(MapWorld&, const BytePalette&, HouseClass*);
ColorStruct world_house_hsv(const HouseClass&);
void load_world_palettes(MapWorld&);
void changed_world_cell(MapWorld&, CellClass&);
}
