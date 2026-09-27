// Godot values are materialized only at the host boundary. Native objects remain
// authoritative, and every access is serialized with map loading/teardown.
#include "bridge/ra2_core.hpp"
#include <limits>

namespace {
bool valid_id(std::int64_t world, std::int64_t id) {
    return world > 0 && id > 0 && id <= std::numeric_limits<std::uint32_t>::max();
}
game::MapObjectId object_id(std::int64_t world, std::int64_t id) {
    return {static_cast<std::uint64_t>(world), static_cast<std::uint32_t>(id)};
}
godot::Dictionary dictionary(const game::MapObjectSnapshot& s) {
    godot::Dictionary d;
    d["world"]=std::int64_t(s.id.world); d["id"]=std::int64_t(s.id.value);
    d["kind"]=s.kind==game::MapObjectKind::building ? "building" : s.kind==game::MapObjectKind::infantry ? "infantry" : s.kind==game::MapObjectKind::unit ? "unit" : s.kind==game::MapObjectKind::aircraft ? "aircraft" : "terrain";
    d["type_id"]=godot::String::utf8(s.type_id); d["name"]=godot::String::utf8(s.name);
    d["owner_name"]=godot::String::utf8(s.owner_name);
#define FIELD(x) d[#x]=s.x
    FIELD(type_index); FIELD(owner_index); FIELD(cell_x); FIELD(cell_y);
    FIELD(world_x); FIELD(world_y); FIELD(world_z); FIELD(health); FIELD(max_health);
    FIELD(frame); FIELD(level); FIELD(alive); FIELD(selected); FIELD(hovered); FIELD(powered); FIELD(has_image);
#undef FIELD
    constexpr const char* bands[]={"dead","red","yellow","green"};
    d["health_band"]=bands[static_cast<unsigned>(s.health_band)];
    return d;
}
}
godot::Dictionary RA2Core::get_map_world_status() const {
    const std::lock_guard lock(progress_mutex_);
    godot::Dictionary d; game::MapWorldSnapshot s{};
    if (map_state_!=MapState::ready || !map_ || !game::get_map_world_snapshot(*map_,s)) return d;
#define FIELD(x) d[#x]=std::int64_t(s.x)
    FIELD(presentation_revision); FIELD(resource_revision); FIELD(simulation_tick);
    FIELD(buildings); FIELD(terrain_objects); FIELD(animations); FIELD(resource_cells);
    FIELD(infantry); FIELD(units);
    FIELD(selected); FIELD(missing_images); FIELD(unknown_records); FIELD(missing_palettes);
#undef FIELD
    d["hovered_world"]=std::int64_t(s.hovered.world); d["hovered_id"]=std::int64_t(s.hovered.value);
    d["objects_loaded"]=s.objects_loaded;
    return d;
}
godot::Array RA2Core::get_map_objects() const {
    const std::lock_guard lock(progress_mutex_);
    godot::Array result;
    if (map_state_!=MapState::ready || !map_) return result;
    std::uint32_t count=0;
    if (!game::copy_map_objects(*map_,nullptr,0,count)) return result;
    std::vector<game::MapObjectSnapshot> objects(count);
    if (!game::copy_map_objects(*map_,objects.data(),count,count)) return result;
    for (const auto& object:objects) result.append(dictionary(object));
    return result;
}
godot::Dictionary RA2Core::get_map_object(std::int64_t world,std::int64_t id) const {
    const std::lock_guard lock(progress_mutex_);
    game::MapObjectSnapshot s{};
    return map_state_==MapState::ready && map_ && valid_id(world,id) &&
        game::get_map_object(*map_,object_id(world,id),s) ? dictionary(s) : godot::Dictionary();
}
godot::Dictionary RA2Core::get_map_resource(int x,int y) const {
    const std::lock_guard lock(progress_mutex_);
    godot::Dictionary d; game::MapResourceSnapshot s{};
    if (map_state_!=MapState::ready || !map_ || !game::get_map_resource(*map_,x,y,s)) return d;
#define FIELD(x) d[#x]=s.x
    FIELD(cell_x); FIELD(cell_y); FIELD(type_index); FIELD(overlay_index); FIELD(frame);
    FIELD(units); FIELD(value_per_unit);
#undef FIELD
    d["total_value"]=s.total_value;
    return d;
}
bool RA2Core::set_map_object_health(std::int64_t world,std::int64_t id,int health) {
    const std::lock_guard lock(progress_mutex_);
    return map_state_==MapState::ready && map_ && valid_id(world,id) &&
        game::set_map_object_health(*map_,object_id(world,id),health);
}
bool RA2Core::set_map_unit_cheat_health(std::int64_t world,std::int64_t id,int health) {
    const std::lock_guard lock(progress_mutex_);
    return map_state_==MapState::ready && map_ && valid_id(world,id) &&
        game::set_map_unit_cheat_health(*map_,object_id(world,id),health);
}
bool RA2Core::set_map_object_level(std::int64_t world,std::int64_t id,int level) {
    const std::lock_guard lock(progress_mutex_);
    return map_state_==MapState::ready && map_ && valid_id(world,id) &&
        game::set_map_object_level(*map_,object_id(world,id),level);
}
bool RA2Core::grant_map_player_power() {
    const std::lock_guard lock(progress_mutex_);
    return map_state_==MapState::ready && map_ && game::grant_map_player_power(*map_);
}
bool RA2Core::set_map_building_enabled(std::int64_t world,std::int64_t id,bool enabled) {
    const std::lock_guard lock(progress_mutex_);
    return map_state_==MapState::ready && map_ && valid_id(world,id) &&
        game::set_map_building_enabled(*map_,object_id(world,id),enabled);
}
godot::Dictionary RA2Core::harvest_map_resource(int x,int y,int requested) {
    const std::lock_guard lock(progress_mutex_);
    godot::Dictionary d; int units=0; std::int64_t value=0;
    d["ok"]=map_state_==MapState::ready && map_ &&
        game::harvest_map_resource(*map_,x,y,requested,units,value);
    d["units"]=units; d["value"]=value;
    return d;
}
