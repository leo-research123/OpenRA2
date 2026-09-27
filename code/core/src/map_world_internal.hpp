#pragma once
#include "map_world.hpp"
#include "building_voxel.hpp"
#include "api/map_objects.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/EventClass.h"
#include "yrpp/MissionClass.h"
#include <map>
#include <vector>
#include <memory>
#include <string>
class AnimClass;class HouseClass;struct SHPStruct;
namespace game {
struct WorldSprite {
 std::shared_ptr<VoxelSurface> voxel;
 SHPStruct*image=nullptr; int frame=0; ObjectClass*owner=nullptr; CellClass*cell=nullptr;
 Point2D position{};const BytePalette*palette=nullptr;
 int sort=0,depth=0,intensity=1000;std::uint32_t flags=0xE00; // sort is original world X+Y, never screen pixels
 Layer layer=Layer::Ground;
 std::size_t draw_group=0; // Frame-local ordering ticket, never simulation state.
 bool parachute=false;
 bool flat=false,shadow=false,pickable=false;
 bool original_depth=false;int depth_adjustment=0,gradient=0;
 int tint=0;
 SHPStruct*depth_image=nullptr;int depth_frame=0;Point2D depth_offset{};
 bool cell_tint=true;
 bool color_scheme=false;
 bool draw_object=false;
};
struct MapWorld::Impl {
 MapViewHandle&view;
 std::uint64_t world_id=0;
 int side_types=0,building_types=0,terrain_types=0,overlay_types=0,smudge_types=0,anim_types=0,house_types=0,tiberium_types=0;
 int buildings=0,terrains=0,houses=0,animations=0,spotlights=0;
 int infantry_types=0,weapon_types=0,infantry=0;
 int unit_types=0,units=0,aircraft_types=0,aircraft=0;
 int teams=0,scripts=0,tags=0,triggers=0;
 int ai_trigger_types=0,script_types=0,task_forces=0,team_types=0,trigger_types=0,tag_types=0;
 int movie_names=0;
 int color_schemes=0;
 // Borrowed static configuration tables must not leak map overlays into the
 // next world or overwrite a caller's original table after this world closes.
 std::array<MissionControlClass,32> previous_missions;
 std::array<int,19> previous_powerup_weights,previous_powerup_anims;
 std::array<double,19> previous_powerup_arguments;
 std::array<bool,19> previous_powerup_naval;
 int bullet_types=0,warhead_types=0,bullets=0,line_trails=0,alpha_shapes=0;
 int particle_system_types=0,particle_types=0,particle_systems=0,particles=0,voxel_anim_types=0;
 bool loaded=false,pressed=false,dragging=false;Point2D press_point{};MapObjectId hover{},press_target{};
 int player_power_bonus=0;
 // A successful owning reader is distinct from retaining a source document.
 // These name known schemas, not arbitrary extra keys in the same section.
 bool scenario_fields_parsed=false;
 bool script_fields_parsed=false,task_force_fields_parsed=false;
 std::vector<std::string> parsed_rule_sections;
 std::vector<std::pair<std::string,std::string>> deferred_sound_fields;
 // A display world's local outgoing commands. Bound to the original queue
 // while this world is active; never survive destruction/reload.
 QueueClass<EventClass,EventClass::MAX_EVENTS> outgoing;
 std::uint32_t missing_images=0,unknown_records=0,missing_palettes=0;
 BytePalette unit_palette{},anim_palette{},selection_palette{};
 bool selection_palette_loaded=false;
 SHPStruct*building_zshape=nullptr; // owned original BUILDNGZ.SHA, biased by 0x41
 SHPStruct*overlay_slope_depth[5]{}; // owned SLOP01Z..04Z; slot zero is null
 SHPStruct*repair_wrench=nullptr;
 SHPStruct*health_pips=nullptr; // original name-cache-owned PIPS.SHP
 SHPStruct*pip_border=nullptr; // YR PIPBRD.SHP (0x00AC1478), not TS SELECT.SHP
 SHPStruct*mobile_pips=nullptr; // PIPS2.SHP, original mobile cargo indicators
 SHPStruct*ore_gatherer=nullptr; // name-cache-owned OREGATH.SHP
 std::map<std::pair<const BytePalette*,std::uint32_t>,std::unique_ptr<BytePalette>> remaps;
 std::map<std::string,std::unique_ptr<BytePalette>> custom_palettes;
 std::vector<WorldSprite>sprites;
 VoxelPalette voxel_palette;
 bool voxel_palette_loaded=false;
 std::uint64_t voxel_asset_revision=0;
 std::map<std::pair<const VoxelStruct*,std::array<std::uint32_t,14>>,std::shared_ptr<VoxelSurface>> voxel_surfaces;
 std::map<std::pair<const VoxelStruct*,int>,std::shared_ptr<VoxelSurface>> unit_voxel_surfaces;
 std::uint32_t missing_voxel_parts=0,invalid_voxel_parts=0;
 std::uint64_t sprite_revision=~std::uint64_t{};
 std::uint64_t selectable_revision=~std::uint64_t{};
 Point2D selectable_camera{},selectable_viewport{};
 std::vector<CellClass*>decorated_cells,resource_cells;
 std::vector<unsigned char>decorated_membership,resource_membership;
 explicit Impl(MapViewHandle&);
};
ObjectClass*resolve_map_object(MapWorld&,MapObjectId) noexcept;
MapObjectId object_id(const MapWorld&,const ObjectClass*) noexcept;
void snapshot_object(MapWorld&,ObjectClass&,MapObjectSnapshot&) noexcept;
struct TechnoVoxelRequest;
DrawingStatus cache_world_voxel(MapWorld&,const TechnoVoxelRequest&,std::shared_ptr<VoxelSurface>&) noexcept;
void rebuild_world_sprites(MapWorld&);
void rebuild_world_selectables(MapWorld&);
ObjectClass*pick_world_object(MapWorld&,Point2D);
const BytePalette*world_palette(MapWorld&,const BytePalette&,HouseClass*);
ColorStruct world_house_hsv(MapWorld&,const HouseClass&);
void changed_world_cell(MapWorld&,CellClass&);
}
