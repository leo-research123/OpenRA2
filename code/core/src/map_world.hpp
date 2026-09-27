#pragma once
#include "api/map_view.hpp"
#include "api/type_resources.hpp"
#include "yrpp/CCINIClass.h"
#include <cstdint>
#include <memory>
class ObjectClass; class BuildingClass; class TerrainClass; class CellClass; class HouseClass;
namespace game {
class MapWorld {
public:
 struct Impl;
 std::unique_ptr<Impl> impl;
 // Retain the map separately: merged type rules are not a lossless substitute
 // for map lists, script records or fields whose reader is not implemented.
 CCINIClass map_ini,base_rules_ini,rules_ini,art_ini;
 std::uint64_t presentation_revision=0,resource_revision=0,simulation_tick=0;
 TypeResourceServices type_services{};
 explicit MapWorld(MapViewHandle&); ~MapWorld();
 MapWorld(const MapWorld&)=delete;MapWorld& operator=(const MapWorld&)=delete;
};
bool load_map_world_types(MapWorld&,CCINIClass&,char* error,std::size_t error_size);
bool load_map_world_objects(MapWorld&,CCINIClass&);
void update_map_world(MapWorld&);
void input_map_world(MapWorld&,const GameInputEvent&,GameInputResult&);
void refresh_map_world_hover(MapWorld&);
DrawingStatus draw_map_world(MapWorld&,const MapDrawingContext&,const RectangleStruct&,MapDrawStatistics&) noexcept;
MapWorld* current_map_world() noexcept;
bool with_map_world(MapWorld*,void(*)(void*),void*) noexcept;
void map_object_changed() noexcept;
int active_map_player_power_bonus(const HouseClass*) noexcept;
void map_resource_changed(CellClass&) noexcept;
void attach_map_object(ObjectClass&) noexcept;
// Derived objects release class-specific occupancy before base destruction.
// The base fallback only unlinks generic content, without virtual dispatch.
void detach_map_object(ObjectClass&, bool baseDestruction = false) noexcept;
bool foundation_contains(const ObjectClass&,const CellStruct&) noexcept;
CellStruct* native_building_foundation(int) noexcept;
bool native_foundation(void*,int,CellStruct*&) noexcept;
bool native_arctic_name(void*,const char*,char*,std::size_t) noexcept;
}
