#pragma once
#include "api/map_view.hpp"
#include "type_drawing_context.hpp"
#include "api/type_resources.hpp"
#include "yrpp/CCINIClass.h"
#include <cstdint>
#include <memory>
class ObjectClass;
class BuildingClass;
class TerrainClass;
class CellClass;
class HouseClass;
namespace game {
class MapWorld {
public:
    struct Impl;
    std::unique_ptr<Impl> impl;
    // The rules file and the map stay separate INIs, as the original reads them
    // (RULESMD.INI first, then the scenario through the same readers).
    CCINIClass map_ini, base_rules_ini, art_ini;
    std::uint64_t presentation_revision = 0, resource_revision = 0, simulation_tick = 0;
    TypeResourceServices type_services{};
    explicit MapWorld(MapViewHandle&);
    ~MapWorld();
    MapWorld(const MapWorld&) = delete;
    MapWorld& operator=(const MapWorld&) = delete;
};
// Loads world.map_ini through ScenarioClass::InitializeWorldINI (0x00686B20)
// inside the caller's map-view operation, then finishes the scenario start.
bool load_map_world(MapWorld&, char* error, std::size_t error_size, const SkirmishOptions* skirmish = nullptr);
// Host bookkeeping after the last original phase of an iteration.
void end_map_world_frame(MapWorld&);
void refresh_map_world_hover(MapWorld&);
DrawingStatus draw_map_world(MapWorld&, const MapDrawingContext&, const RectangleStruct&, MapDrawStatistics&) noexcept;
MapWorld* current_map_world() noexcept;
bool current_map_pointer_cell(CellStruct& cell) noexcept;
bool with_map_world(MapWorld*, void (*)(void*), void*) noexcept;
void map_object_changed() noexcept;
int active_map_player_power_bonus(const HouseClass*) noexcept;
void map_resource_changed(CellClass&) noexcept;
CellStruct* native_building_foundation(int) noexcept;
bool native_foundation(void*, int, CellStruct*&) noexcept;
bool native_arctic_name(void*, const char*, char*, std::size_t) noexcept;
}
