#pragma once
#include <cstdint>
namespace game {
class MapViewHandle;
// Session-qualified identifiers, never native pointers or mutable array indexes.
struct MapObjectId { std::uint64_t world=0; std::uint32_t value=0; };
enum class MapObjectKind : std::uint32_t { none,building,terrain,infantry,unit,aircraft };
enum class MapHealthBand : std::uint32_t { dead,red,yellow,green };
struct MapObjectSnapshot {
 MapObjectId id{}; MapObjectKind kind=MapObjectKind::none;
 int type_index=-1,owner_index=-1,cell_x=0,cell_y=0,world_x=0,world_y=0,world_z=0;
 int health=0,max_health=0,frame=0,level=-1; MapHealthBand health_band=MapHealthBand::dead;
 bool alive=false,selected=false,hovered=false,powered=false,has_image=false;
 char type_id[32]{},name[192]{},owner_name[64]{}; // NUL-terminated UTF-8.
};
struct MapWorldSnapshot {
 std::uint64_t presentation_revision=0,resource_revision=0,simulation_tick=0;
 std::uint32_t buildings=0,terrain_objects=0,animations=0,resource_cells=0;
 std::uint32_t selected=0,missing_images=0,unknown_records=0,missing_palettes=0;
 MapObjectId hovered{}; bool objects_loaded=false;
 std::uint32_t infantry=0; // Actual InfantryClass instances; commanded infantry run original object frames.
 std::uint32_t units=0;
};
struct MapResourceSnapshot {
 int cell_x=0,cell_y=0,type_index=-1,overlay_index=-1,frame=0,units=0,value_per_unit=0;
 std::int64_t total_value=0;
};
// Serialized, synchronous map operations. Exceptions never cross the ABI.
// Null output/capacity zero queries required count. Insufficient capacity writes
// no partial list. Object commands reject stale IDs and cannot revive dead objects.
bool get_map_world_snapshot(MapViewHandle&,MapWorldSnapshot&) noexcept;
bool copy_map_objects(MapViewHandle&,MapObjectSnapshot*,std::uint32_t capacity,std::uint32_t& required) noexcept;
bool get_map_object(MapViewHandle&,MapObjectId,MapObjectSnapshot&) noexcept;
bool get_map_resource(MapViewHandle&,int cell_x,int cell_y,MapResourceSnapshot&) noexcept;
bool set_map_object_health(MapViewHandle&,MapObjectId,int health) noexcept;
// Console-only health override; unlike the regular setter, permits values above Strength.
bool set_map_unit_cheat_health(MapViewHandle&,MapObjectId,int health) noexcept;
// Rank values: 0 rookie, 1 veteran, 2 elite. Rejects non-techno objects.
bool set_map_object_level(MapViewHandle&,MapObjectId,int level) noexcept;
// Gives the current player persistent power for this map session.
bool grant_map_player_power(MapViewHandle&) noexcept;
bool set_map_building_enabled(MapViewHandle&,MapObjectId,bool enabled) noexcept;
// Outputs actual credited yield. Clearing the last visible resource frame
// yields zero, even though get_map_resource reports its nominal units/value.
bool harvest_map_resource(MapViewHandle&,int cell_x,int cell_y,int requested,int& units,std::int64_t& value) noexcept;
}
