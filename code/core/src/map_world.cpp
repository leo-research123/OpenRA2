#include "yrpp/FactoryClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/MouseClass.h"
#include "map_world_internal.hpp"
#include "scenario_loading.hpp"
#include "yrpp/AITriggerTypeClass.h"
#include "map_configuration.hpp"
#include "map_view.hpp"
#include "map_runtime.hpp"
#include "type_resources.hpp"
#include "yrpp/RulesClassReaders.hpp"
#include "api/rules_runtime.hpp"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/AlphaShapeClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/Powerups.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/LineTrail.h"
#include "yrpp/TagClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/VoxelAnimTypeClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/SideClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/Surface.h"
#include "yrpp/GadgetClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
namespace game {
namespace {
thread_local MapWorld*active_world=nullptr;
std::atomic<std::uint64_t> next_world{1};
void expire_world_tag(TagClass* tag,bool)noexcept{
 MapClass::Instance.PointerGotInvalid(tag,true);
 LogicClass::Instance.PointerGotInvalid(tag,true);
}
struct WorldScope{
 MapWorld*previous;
 MapWorld*bound;
 bool previous_game_active;
 bool previous_scenario_started;
 decltype(TagClass::NativeWorldExpiration) previous_tag_hook;
 bool exchange_commands;
 QueueClass<EventClass,EventClass::MAX_EVENTS> previous_commands;
 explicit WorldScope(MapWorld*w):previous(active_world),bound(w),previous_game_active(Game::IsActive),previous_scenario_started(Unsorted::ScenarioStarted),previous_tag_hook(TagClass::NativeWorldExpiration),exchange_commands(w&&w!=previous){
  if(exchange_commands){previous_commands=EventClass::OutList;EventClass::OutList=w->impl->outgoing;}
  active_world=w;if(w)TagClass::NativeWorldExpiration=expire_world_tag;
  Game::IsActive=w&&w->impl->loaded;
  Unsorted::ScenarioStarted=Game::IsActive;
 }
 ~WorldScope(){
  if(exchange_commands){
   // close/reload may destroy the bound world inside this scope.
   if(active_world==bound)bound->impl->outgoing=EventClass::OutList;
   EventClass::OutList=previous_commands;
  }
  Game::IsActive=previous_game_active;Unsorted::ScenarioStarted=previous_scenario_started;TagClass::NativeWorldExpiration=previous_tag_hook;active_world=previous;
 }
};
void set_world_hover(MapWorld& world,ObjectClass* object)noexcept{
 auto& w=*world.impl;
 auto* previous=resolve_map_object(world,w.hover);
 if(previous&&(previous->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None)
  static_cast<TechnoClass*>(previous)->IsMouseHovering=false;
 // Native input bridge for the hover assignment in Display::ConvertAction
 // 0x4AAEEC. Cursor/planning branches of that entry are not implemented here.
 // Original also gates on Unsorted::ArmageddonMode (0xA8ED6B); the
 // native map host does not expose that special mode.
 if(object&&(object->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None){
  auto* techno=static_cast<TechnoClass*>(object);
  techno->IsMouseHovering=object->WhatAmI()!=AbstractType::Building
   || !static_cast<BuildingClass*>(object)->Type->InvisibleInGame;
 }
 const auto id=object_id(world,object);
 if(id.world!=w.hover.world||id.value!=w.hover.value){w.hover=id;++world.presentation_revision;}
}
void merge(CCINIClass&dst,CCINIClass&src){for(auto*s:src.Sections)for(auto*e:s->Entries)if(!dst.WriteString(s->Name,e->Key,e->Value))throw std::runtime_error("INI merge failed");}
void read_optional(CCINIClass&ini,const char*name,const char*fallback=nullptr){CCFileClass f(name);if(!f.Exists()&&fallback)f.SetFileName(fallback);if(f.Exists()&&ini.ReadCCFile(&f,false,false)<=0)throw std::runtime_error("Cannot read type INI");}
std::string text(CCINIClass&ini,const char*s,const char*k,const char*def=""){char v[512]{};ini.ReadString(s,k,def,v,sizeof(v));return v;}
template<class Allocate>void allocate_list(CCINIClass&ini,const char*section,Allocate allocate){for(int i=0;i<ini.GetKeyCount(section);++i){auto v=text(ini,section,ini.GetKeyName(section,i));if(!v.empty()&&!INIClass::IsBlankValue(v.c_str())&&!allocate(v.c_str()))throw std::runtime_error("Type allocation failed");}}
void register_building_types(CCINIClass& base, CCINIClass& map) {
 // Use the migrated 0x00672660 reader, including its 32-byte name buffer.
 // The map host already owns native BuildingType construction defaults;
 // the general rules resolver intentionally does not allocate buildings.
 RulesRuntimeServices services{};
 services.resolve=[](void*,AbstractType kind,const char* id,AbstractTypeClass*& out){
  if(kind!=AbstractType::BuildingType)return false;
  out=BuildingTypeClass::FindOrAllocate(id);
  return true;
 };
 struct Inputs{CCINIClass& base;CCINIClass& map;} inputs{base,map};
 with_rules_runtime(services,[](void* p){
  auto& input=*static_cast<Inputs*>(p);
  RulesClass::Read_BuildingTypes(&input.base);
  RulesClass::Read_BuildingTypes(&input.map);
 },&inputs);
}
template<class T>void read_types(CCINIClass&ini,int first){for(int i=first;i<T::Array.Count;++i){auto*t=T::Array[i];if(ini.GetSection(t->ID)&&!t->LoadFromINI(&ini))throw std::runtime_error(std::string("Cannot load type ")+t->ID);}}
bool palette_file(const char*name,BytePalette&out){CCFileClass f(name);if(!f.Exists()||f.GetFileSize()<768||f.ReadBytes(&out,768)!=768)return false;for(auto&c:out.Entries){c.R=BYTE(c.R<<2);c.G=BYTE(c.G<<2);c.B=BYTE(c.B<<2);}return true;}
template<class T>void delete_tail(int n){while(T::Array.Count>n)GameDelete(T::Array[T::Array.Count-1]);}
void initialize_palette(MapWorld&world){auto&w=*world.impl;w.voxel_palette_loaded=load_voxel_palette(w.voxel_palette);char name[64];auto&t=Theater::GetTheater(w.view.scenario.Theater);std::snprintf(name,sizeof(name),"UNIT%s.PAL",t.Extension);if(!palette_file(name,w.unit_palette)){w.unit_palette=FileSystem::TEMPERAT_PAL;++w.missing_palettes;}w.selection_palette_loaded=palette_file("PALETTE.PAL",w.selection_palette);if(!w.selection_palette_loaded){w.selection_palette=w.unit_palette;++w.missing_palettes;}if(!palette_file("ANIM.PAL",w.anim_palette)){w.anim_palette=w.unit_palette;++w.missing_palettes;}}

}
ColorStruct world_house_hsv(MapWorld&world,const HouseClass&owner){
 auto&ini=world.rules_ini;
 const auto country=owner.Type?owner.Type->ID:owner.PlainName;
 const auto color=text(ini,owner.PlainName,"Color",text(ini,country,"Color").c_str());
 return ini.ReadColor("Colors",color.c_str(),ColorStruct{0,0,128});
}
MapWorld::Impl::Impl(MapViewHandle&v):view(v),world_id(next_world.fetch_add(1)){
 aircraft_types=AircraftTypeClass::Array.Count;
 aircraft=AircraftClass::Array.Count;teams=TeamClass::Array.Count;scripts=ScriptClass::Array.Count;tags=TagClass::Array.Count;triggers=TriggerClass::Array.Count;
 ai_trigger_types=AITriggerTypeClass::Array.Count;
 script_types=ScriptTypeClass::Array.Count;task_forces=TaskForceClass::Array.Count;
 team_types=TeamTypeClass::Array.Count;trigger_types=TriggerTypeClass::Array.Count;tag_types=TagTypeClass::Array.Count;
 movie_names=MovieInfo::Array.Count;
 color_schemes=ColorScheme::Array.Count;
 std::copy_n(MissionControlClass::Array,32,previous_missions.begin());
 std::copy_n(Powerups::Weights,19,previous_powerup_weights.begin());
 std::copy_n(Powerups::Anims,19,previous_powerup_anims.begin());
 std::copy_n(Powerups::Arguments,19,previous_powerup_arguments.begin());
 std::copy_n(Powerups::Naval,19,previous_powerup_naval.begin());
 side_types=SideClass::Array.Count;building_types=BuildingTypeClass::Array.Count;terrain_types=TerrainTypeClass::Array.Count;overlay_types=OverlayTypeClass::Array.Count;smudge_types=SmudgeTypeClass::Array.Count;anim_types=AnimTypeClass::Array.Count;house_types=HouseTypeClass::Array.Count;tiberium_types=TiberiumClass::Array.Count;
 buildings=BuildingClass::Array.Count;terrains=TerrainClass::Array.Count;houses=HouseClass::Array.Count;animations=AnimClass::Array.Count;spotlights=SpotlightClass::Array.Count;
 unit_types=UnitTypeClass::Array.Count;units=UnitClass::Array.Count;
 infantry_types=InfantryTypeClass::Array.Count;weapon_types=WeaponTypeClass::Array.Count;infantry=InfantryClass::Array.Count;
 bullet_types=BulletTypeClass::Array.Count;warhead_types=WarheadTypeClass::Array.Count;bullets=BulletClass::Array.Count;
 line_trails=LineTrail::Array.Count;alpha_shapes=AlphaShapeClass::Array.Count;
 particle_system_types=ParticleSystemTypeClass::Array.Count;particle_types=ParticleTypeClass::Array.Count;particle_systems=ParticleSystemClass::Array.Count;particles=ParticleClass::Array.Count;voxel_anim_types=VoxelAnimTypeClass::Array.Count;
}
MapWorld::MapWorld(MapViewHandle&v):impl(std::make_unique<Impl>(v)){TechnoClass::ActionLineTimer.Start(0);type_services.context=this;type_services.art=&art_ini;type_services.theater=&v.scenario.Theater;type_services.audio_unavailable=true;type_services.combat_unavailable=false;type_services.terrain_foundation=native_foundation;type_services.arctic_image_name=native_arctic_name;}
MapWorld::~MapWorld(){auto&w=*impl;
 while(ColorScheme::Array.Count>w.color_schemes)delete ColorScheme::Array[ColorScheme::Array.Count-1];
 std::copy(w.previous_missions.begin(),w.previous_missions.end(),MissionControlClass::Array);
 std::copy(w.previous_powerup_weights.begin(),w.previous_powerup_weights.end(),Powerups::Weights);
 std::copy(w.previous_powerup_anims.begin(),w.previous_powerup_anims.end(),Powerups::Anims);
 std::copy(w.previous_powerup_arguments.begin(),w.previous_powerup_arguments.end(),Powerups::Arguments);
 std::copy(w.previous_powerup_naval.begin(),w.previous_powerup_naval.end(),Powerups::Naval);
 w.view.tactical.EndRubberBand();w.view.tactical.SelectableCount=0;
 YRMemory::Deallocate(w.building_zshape);
 for(auto* shape:w.overlay_slope_depth)YRMemory::Deallocate(shape);
 AbstractClass::RemoveAllInactive();
 while(ParticleSystemClass::Array.Count>w.particle_systems)delete ParticleSystemClass::Array[ParticleSystemClass::Array.Count-1];
 while(ParticleClass::Array.Count>w.particles)delete ParticleClass::Array[ParticleClass::Array.Count-1];
 while(BulletClass::Array.Count>w.bullets)delete BulletClass::Array[BulletClass::Array.Count-1];
 for(int i=FactoryClass::Array.Count-1;i>=0;--i){
  auto* f=FactoryClass::Array[i];if(!f->Owner||f->Owner->ArrayIndex<w.houses)continue;
  auto* type=f->Object?f->Object->GetTechnoType():nullptr;
  if(type){f->Owner->SetPrimaryFactory(nullptr,type->WhatAmI(),type->Naval,BuildCat(0));SidebarClass::UnlinkFactory(type->WhatAmI(),type->GetArrayIndex(),f);}
  GameDelete(f);
 }
 delete_tail<TeamClass>(w.teams);
 delete_tail<AircraftClass>(w.aircraft);
 while(UnitClass::Array.Count>w.units)delete UnitClass::Array[UnitClass::Array.Count-1];
 while(InfantryClass::Array.Count>w.infantry)delete InfantryClass::Array[InfantryClass::Array.Count-1];
 while(BuildingClass::Array.Count>w.buildings)delete BuildingClass::Array[BuildingClass::Array.Count-1];
 while(TerrainClass::Array.Count>w.terrains)delete TerrainClass::Array[TerrainClass::Array.Count-1];
 while(AnimClass::Array.Count>w.animations)delete AnimClass::Array[AnimClass::Array.Count-1];
 while(SpotlightClass::Array.Count>w.spotlights)delete SpotlightClass::Array[SpotlightClass::Array.Count-1];
 // Release this world's alpha records before their borrowed SHP/type resources.
 while(AlphaShapeClass::Array.Count>w.alpha_shapes)GameDelete(AlphaShapeClass::Array[AlphaShapeClass::Array.Count-1]);
 while(LineTrail::Array.Count>w.line_trails){auto* trail=LineTrail::Array[LineTrail::Array.Count-1];LineTrail::Array.Remove(trail);GameDelete(trail);}
 while(HouseClass::Array.Count>w.houses)delete HouseClass::Array[HouseClass::Array.Count-1];
 AbstractClass::RemoveAllInactive();
 delete_tail<TagClass>(w.tags);AbstractClass::RemoveAllInactive();
 delete_tail<TriggerClass>(w.triggers);delete_tail<ScriptClass>(w.scripts);
 delete_tail<AITriggerTypeClass>(w.ai_trigger_types);
 delete_tail<TagTypeClass>(w.tag_types);delete_tail<TriggerTypeClass>(w.trigger_types);delete_tail<TeamTypeClass>(w.team_types);
 delete_tail<ScriptTypeClass>(w.script_types);delete_tail<TaskForceClass>(w.task_forces);
 while(MovieInfo::Array.Count>w.movie_names){
  const int i=MovieInfo::Array.Count-1;YRMemory::Deallocate(MovieInfo::Array[i]);MovieInfo::Array.RemoveItem(i);
 }
 delete_tail<AircraftTypeClass>(w.aircraft_types);delete_tail<UnitTypeClass>(w.unit_types);delete_tail<InfantryTypeClass>(w.infantry_types);delete_tail<WeaponTypeClass>(w.weapon_types);
 delete_tail<BulletTypeClass>(w.bullet_types);delete_tail<WarheadTypeClass>(w.warhead_types);
 delete_tail<ParticleSystemTypeClass>(w.particle_system_types);delete_tail<ParticleTypeClass>(w.particle_types);delete_tail<VoxelAnimTypeClass>(w.voxel_anim_types);
 delete_tail<BuildingTypeClass>(w.building_types);delete_tail<TerrainTypeClass>(w.terrain_types);delete_tail<TiberiumClass>(w.tiberium_types);delete_tail<OverlayTypeClass>(w.overlay_types);delete_tail<SmudgeTypeClass>(w.smudge_types);delete_tail<AnimTypeClass>(w.anim_types);delete_tail<HouseTypeClass>(w.house_types);delete_tail<SideClass>(w.side_types);
 if(active_world==this)active_world=nullptr;
}
bool native_arctic_name(void*,const char*base,char*out,std::size_t size)noexcept{if(!base||!out||!size)return false;int n=std::snprintf(out,size,"%sA",base);return n>=0&&std::size_t(n)<size;}
bool with_map_world(MapWorld*world,void(*op)(void*),void*arg)noexcept{
 if(!op)return false;WorldScope scope(world);
 try{
  if(!world){op(arg);return true;}
  auto runtime=map_runtime();
  // CoordsToClient (0x006D2140) reads the active Tactical viewport. Bind it
  // for every native world operation, including LineTrail::Draw; the default
  // host runtime deliberately has no viewport and leaves projection output
  // untouched when it is unavailable.
  runtime.view_bounds=&TacticalClass::ViewBounds;
  runtime.drawing_bounds=DSurface::ViewBounds.Width>0?&DSurface::ViewBounds:&TacticalClass::ViewBounds;
  runtime.has_window=[]() noexcept { return true; };
  runtime.action_line_palette=&world->impl->selection_palette;
  struct Call{MapWorld& world;void(*operation)(void*);void* argument;bool complete=false;}call{*world,op,arg};
  const bool invoked=with_map_runtime(runtime,[](void* p){
   auto& call=*static_cast<Call*>(p);
   call.complete=with_type_resources(call.world.type_services,call.operation,call.argument)==TypeResourceStatus::complete;
  },&call);
  return invoked&&call.complete;
 }catch(...){return false;}
}
void map_object_changed()noexcept{if(active_world)++active_world->presentation_revision;}
int active_map_player_power_bonus(const HouseClass* house) noexcept {
 return active_world && house==HouseClass::CurrentPlayer ? active_world->impl->player_power_bonus : 0;
}
void changed_world_cell(MapWorld& world, CellClass& cell) {
 auto& w = *world.impl;
 const int i = MapClass::GetCellIndex(cell.MapCoords);
 if (i < 0 || std::size_t(i) >= w.decorated_membership.size()) return;
 if (!w.decorated_membership[i] && (cell.OverlayTypeIndex >= 0 || cell.SmudgeTypeIndex >= 0)) {
  w.decorated_cells.push_back(&cell); w.decorated_membership[i] = 1;
 }
 const int resource = cell.GetContainedTiberiumIndex();
 if (resource >= 0 && !w.resource_membership[i]) {
  w.resource_cells.push_back(&cell); w.resource_membership[i] = 1;
 }

}
void map_resource_changed(CellClass&cell)noexcept{if(!active_world)return;try{changed_world_cell(*active_world,cell);}catch(...){++active_world->impl->unknown_records;}++active_world->resource_revision;++active_world->presentation_revision;RadarClass::Instance.MarkTerrainCellDirty(cell.MapCoords);}
bool foundation_contains(const ObjectClass&o,const CellStruct&cell)noexcept{CellStruct*entries=nullptr;if(o.WhatAmI()==AbstractType::Building){auto*t=static_cast<const BuildingClass&>(o).Type;if(t)entries=t->FoundationData;}if(o.WhatAmI()==AbstractType::Terrain){auto*t=static_cast<const TerrainClass&>(o).Type;if(t)entries=t->FoundationData;}CellStruct anchor{short(o.Location.X/256),short(o.Location.Y/256)};if(!entries)return cell==anchor;for(int i=0;i<30;++i){auto e=entries[i];if(e.X==32767&&e.Y==32767)return false;if(anchor.X+e.X==cell.X&&anchor.Y+e.Y==cell.Y)return true;}return false;}
void attach_map_object(ObjectClass& object)noexcept {
 // Compatibility entry for internal callers. All game-state transitions belong
 // to the original placement virtuals, shared with runtime creation.
 if(object.IsOnMap||!MapClass::Instance.TryGetCellAt(object.Location))return;
 try {
  if(object.WhatAmI()==AbstractType::Unit){auto& unit=static_cast<UnitClass&>(object);if(!unit.Locomotor&&!unit.InitializeLocomotor())return;}
  if(object.WhatAmI()==AbstractType::Infantry){auto& infantry=static_cast<InfantryClass&>(object);if(!infantry.Locomotor&&!infantry.InitializeLocomotor())return;}
  const bool active=Game::IsActive;const int depth=Unsorted::ScenarioInit;
  struct Restore{bool active;int depth;~Restore(){Game::IsActive=active;Unsorted::ScenarioInit=depth;}}restore{active,depth};
  Game::IsActive=true;Unsorted::ScenarioInit=depth+1;
  auto at=object.Location;at.Z=MapClass::Instance.GetCellFloorHeight(at)+(object.OnBridge?CellClass::BridgeHeight:0);
  DirType facing=DirType::North;
  if((object.AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None)
   facing=DirType(static_cast<TechnoClass&>(object).PrimaryFacing.Current().Raw>>8);
  object.Unlimbo(at,facing);
 }catch(...){return;}
}
void detach_map_object(ObjectClass&o,bool baseDestruction)noexcept{
 // Native teardown can delete an object directly, without the ordinary
 // Object::Limbo path (which calls Display::Remove at 0x5F4D30).
 // Pair Unlimbo's layer registration before the borrowed pointer expires.
 DisplayClass::Remove(&o);
 if(active_world){auto& w=*active_world->impl;w.view.tactical.SelectableCount=0;w.selectable_revision=~std::uint64_t{};}
 if(!baseDestruction&&(o.AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None)
  static_cast<TechnoClass&>(o).IsMouseHovering=false;
 if(active_world){
  auto& w=*active_world->impl;
  if(w.hover.world==w.world_id&&w.hover.value==std::uint32_t(o.UniqueID)){
   w.hover={};++active_world->presentation_revision;
  }
 }
 // Original pointer-expiry responsibility for native spotlight tracking.
 // Clear borrowed targets before a unit/building can leave the world.
 for(auto*light:BuildingLightClass::Array)light->PointerExpired(&o,true);
 // Direct native destruction can bypass Disappear/NotifyObjectExpired. Keep
 // alpha's deferred-purge flag consistent with that original notification.
 for(auto*shape:AlphaShapeClass::Array)shape->PointerExpired(&o,true);
 LogicClass::Instance.RemoveObject(&o);
 if(!o.IsOnMap){
  // Native cache coherence, not an original redraw rule: Limbo has already
  // removed the original layer/cell membership, but our cached packets may
  // still borrow the object. Invalidate only a clean cache that retains it.
  if(active_world){auto& w=*active_world->impl;
   if(w.sprite_revision==active_world->presentation_revision&&
      std::any_of(w.sprites.begin(),w.sprites.end(),[&](const auto& sprite){return sprite.owner==&o;}))
    map_object_changed();
  }
  return;
 }
 if(!baseDestruction){
  const bool active=Game::IsActive;Game::IsActive=true;
  o.Limbo();Game::IsActive=active;map_object_changed();return;
 }
 if(auto*c=MapClass::Instance.TryGetCellAt(o.Location)){
  auto**link=o.OnBridge?&c->AltObject:&c->FirstObject;
  while(*link&&*link!=&o)link=&(*link)->NextObject;
  if(*link)*link=o.NextObject;
 }
 o.NextObject=nullptr;o.IsOnMap=false;o.InLimbo=true;
 // ObjectClass destruction also enters here, after the derived vtable is gone.
 // Only live registered buildings can receive the damage-fire update.
 for(auto*b:BuildingClass::Array)if(static_cast<ObjectClass*>(b)==&o){b->UpdateDamageFires();if(b->Owner)b->Owner->RecheckPower=true;delete b->Spotlight;b->Spotlight=nullptr;delete b->LightSource;b->LightSource=nullptr;break;}
 map_object_changed();}
bool load_map_world_types(MapWorld&world,CCINIClass&map,char*error,std::size_t error_size){
 if(!error||!error_size)return false;
 error[0]=0;
 WorldScope scope(&world);
 struct Load{MapWorld&w;CCINIClass&m;char*error;std::size_t error_size;bool ok=false;}load{world,map,error,error_size};
 auto result=with_type_resources(world.type_services,[](void*p){
 auto&a=*static_cast<Load*>(p);auto&w=*a.w.impl;auto&rules=a.w.rules_ini;
 try{
 auto& base_rules=a.w.base_rules_ini;
 read_optional(base_rules,"RULESMD.INI","RULES.INI");read_optional(a.w.art_ini,"ARTMD.INI","ART.INI");
 merge(rules,base_rules);merge(rules,a.m);
 for(auto* source:{&base_rules,&a.m}) {
  allocate_list(*source,"OverlayTypes",OverlayTypeClass::FindOrAllocate);
  allocate_list(*source,"SmudgeTypes",SmudgeTypeClass::FindOrAllocate);
  allocate_list(*source,"TerrainTypes",TerrainTypeClass::FindOrAllocate);
 }
 // 0x00686B20 loads base rules before the map; 0x004653C0 preserves names.
 // Keep registry sources separate so map-local numeric keys cannot erase
 // base registrations. Type content uses the effective merged configuration.
 register_building_types(base_rules,a.m);
 RulesClass::Read_VehicleTypes(&base_rules);RulesClass::Read_VehicleTypes(&a.m);
 RulesClass::Read_InfantryTypes(&base_rules);RulesClass::Read_InfantryTypes(&a.m);
 // Retain original identities for references such as TaskForce entries;
 // aircraft content/instances remain deferred until their native reader exists.
 RulesClass::Read_AircraftTypes(&base_rules);RulesClass::Read_AircraftTypes(&a.m);
 for(auto* source:{&base_rules,&a.m}) {
  allocate_list(*source,"Animations",AnimTypeClass::FindOrAllocate);
  allocate_list(*source,"Tiberiums",[](const char*id){auto*t=TiberiumClass::Find(id);return t?t:GameCreate<TiberiumClass>(id);});
  allocate_list(*source,"Countries",[](const char*id){auto*t=HouseTypeClass::Find(id);return t?t:GameCreate<HouseTypeClass>(id);});
 }
 RulesClass::Read_Sides(&rules);RulesClass::Read_Particles(&base_rules);RulesClass::Read_Particles(&a.m);
 RulesClass::Read_ParticleSystems(&base_rules);RulesClass::Read_ParticleSystems(&a.m);
 if(!parse_map_rule_fields(a.w,rules,a.error,a.error_size))return;
 read_types<OverlayTypeClass>(rules,w.overlay_types);
 OverlayTypeClass::LoadFromIniList(static_cast<int>(a.w.impl->view.scenario.Theater));
 read_types<SmudgeTypeClass>(rules,w.smudge_types);read_types<TerrainTypeClass>(rules,w.terrain_types);read_types<BuildingTypeClass>(rules,w.building_types);read_types<TiberiumClass>(rules,w.tiberium_types);
 read_types<InfantryTypeClass>(rules,w.infantry_types);read_types<UnitTypeClass>(rules,w.unit_types);read_types<AircraftTypeClass>(rules,w.aircraft_types);
 // Projectile readers may allocate another weapon (airburst/shrapnel), whose
 // reader may in turn allocate another projectile. Resolve each type once.
 int weaponCursor=w.weapon_types,bulletCursor=w.bullet_types,warheadCursor=w.warhead_types,particleCursor=w.particle_types,systemCursor=w.particle_system_types;
 while(weaponCursor<WeaponTypeClass::Array.Count||bulletCursor<BulletTypeClass::Array.Count||warheadCursor<WarheadTypeClass::Array.Count||particleCursor<ParticleTypeClass::Array.Count||systemCursor<ParticleSystemTypeClass::Array.Count){
  const auto readNext=[&](auto& registry,int& cursor){while(cursor<registry.Count){
   auto* type=registry[cursor++];if(rules.GetSection(type->ID)&&!type->LoadFromINI(&rules))throw std::runtime_error(std::string("Cannot load type ")+type->ID);
  }};
  readNext(WeaponTypeClass::Array,weaponCursor);readNext(BulletTypeClass::Array,bulletCursor);readNext(WarheadTypeClass::Array,warheadCursor);readNext(ParticleSystemTypeClass::Array,systemCursor);readNext(ParticleTypeClass::Array,particleCursor);
 }
 for(int i=w.weapon_types;i<WeaponTypeClass::Array.Count;++i)WeaponTypeClass::Array[i]->CalculateSpeed();
 for(int i=w.anim_types;i<AnimTypeClass::Array.Count;++i)if(!AnimTypeClass::Array[i]->LoadFromINI(&a.w.art_ini))throw std::runtime_error("Animation art load failed");initialize_palette(a.w);
 // Original Colors list order, one shade followed by 53 shades. These are
 // real ColorScheme objects shared by radar text, tracking and house readers.
 for(int i=0;i<rules.GetKeyCount("Colors");++i){
  const auto* name=rules.GetKeyName("Colors",i);
  const auto hsv=rules.ReadColor("Colors",name,{0,0,0});
  for(int shades:{1,53}){
   auto color=std::make_unique<ColorScheme>(name,hsv,w.unit_palette,shades,true);
   if(!ColorScheme::Array.Count || ColorScheme::Array[ColorScheme::Array.Count-1]!=color.get())
    throw std::runtime_error("Color scheme registration failed");
   color.release();
  }
 }
 // Debris art can introduce warheads after the weapon dependency pass.
 while(warheadCursor<WarheadTypeClass::Array.Count){auto* type=WarheadTypeClass::Array[warheadCursor++];
  if(rules.GetSection(type->ID)&&!type->LoadFromINI(&rules))throw std::runtime_error("Debris warhead load failed");
 }
 w.repair_wrench=static_cast<SHPStruct*>(FileSystem::LoadFile("WRENCH.SHP",true));
 w.health_pips=static_cast<SHPStruct*>(FileSystem::LoadFile("PIPS.SHP",true));if(!w.health_pips)++w.missing_images;
 w.pip_border=static_cast<SHPStruct*>(FileSystem::LoadFile("PIPBRD.SHP",true));if(!w.pip_border)++w.missing_images;
 w.mobile_pips=static_cast<SHPStruct*>(FileSystem::LoadFile("PIPS2.SHP",true));if(!w.mobile_pips)++w.missing_images;
 w.ore_gatherer=static_cast<SHPStruct*>(FileSystem::LoadFile("OREGATH.SHP",true));
 // Original 0x0045E8F0 loads frame 0 and subtracts 0x41 from nonzero
 // BUILDNGZ.SHA bytes. Retain ownership with this map's resource lifetime.
 CCFileClass depth_file("BUILDNGZ.SHA");const int depth_bytes=depth_file.Exists(false)?depth_file.GetFileSize():0;
 if(depth_bytes&&depth_bytes<int(sizeof(SHPStruct)+sizeof(SHPFrame)))throw std::runtime_error("Truncated BUILDNGZ.SHA");
 if(!load_owned_type_shape("BUILDNGZ.SHA",w.building_zshape))throw std::runtime_error("Cannot load BUILDNGZ.SHA");
 if(auto*z=w.building_zshape){
  if(z->Frames!=1||z->HasCompression(0))throw std::runtime_error("Invalid BUILDNGZ.SHA layout");
  auto b=z->GetFrameBounds(0);const int offset=z->GetData()->GetFrameHeader(0).Offset;
  if(b.X||b.Y||b.Width<=0||b.Height<=0||b.Width!=z->Width||b.Height!=z->Height||
      offset<int(sizeof(SHPStruct)+sizeof(SHPFrame))||std::int64_t(offset)+std::int64_t(b.Width)*b.Height>depth_bytes)
      throw std::runtime_error("Invalid BUILDNGZ.SHA bounds");
  auto*p=z->GetPixels(0);if(!p)throw std::runtime_error("Missing BUILDNGZ.SHA pixels");
  for(int i=0;i<b.Width*b.Height;++i)if(p[i])p[i]=BYTE(p[i]-0x41);
 }
 // 0x0054533E..0x00545458: theater-specific, unmodified slope depth SHPs.
 // Missing resources remain null and are reported if a sloped overlay needs
 // one; never silently submit flat depth for that branch.
 for(int slope=1;slope<=4;++slope){
  char name[64];std::snprintf(name,sizeof(name),"SLOP0%dZ.%s",slope,
   Theater::GetTheater(w.view.scenario.Theater).Extension);
  if(!load_owned_type_shape(name,w.overlay_slope_depth[slope]))
   throw std::runtime_error("Cannot load overlay slope depth shape");
 }
 a.ok=true;
 }catch(const std::exception&e){std::snprintf(a.error,a.error_size,"Could not load map object types and art: %s",e.what());}
 catch(...){std::snprintf(a.error,a.error_size,"Could not load map object types and art: unexpected native failure");}
 },&load);
 if(result==TypeResourceStatus::complete&&load.ok)return true;
 if(!error[0])std::snprintf(error,error_size,"Could not load map object types and art (resource status %d)",static_cast<int>(result));
 return false;
}
bool load_map_world_objects(MapWorld& world,CCINIClass& ini){
 WorldScope scope(&world);auto& w=*world.impl;
 try {
  w.decorated_membership.assign(MapClass::MaxCells,0);w.resource_membership.assign(MapClass::MaxCells,0);
  unsigned int rejected=0;
  const bool loaded=load_scenario_objects(ini,world.rules_ini,world.art_ini,rejected);
  w.unknown_records+=rejected;
  if(!loaded)throw std::runtime_error("Could not load scenario objects");
  load_scenario_tag_instances();
  w.script_fields_parsed=w.task_force_fields_parsed=true;
  for(int i=0;i<MapClass::Instance.Cells.Capacity;++i)if(auto* cell=MapClass::Instance.Cells[i])changed_world_cell(world,*cell);
  for(int i=w.building_types;i<BuildingTypeClass::Array.Count;++i)if(!BuildingTypeClass::Array[i]->Image)++w.missing_images;
  for(int i=w.terrain_types;i<TerrainTypeClass::Array.Count;++i)if(!TerrainTypeClass::Array[i]->Image)++w.missing_images;
  w.loaded=true;++world.presentation_revision;return true;
 }catch(const std::exception& e){std::snprintf(w.view.error,sizeof(w.view.error),"%s",e.what());return false;}
 catch(...){return false;}
}
void update_map_world(MapWorld& world){
 auto& w=*world.impl;if(!w.loaded)return;WorldScope scope(&world);
 // Local session scheduling, before object logic. This is not a network
 // lockstep implementation. Execute the original command, not SetDestination
 // from an input callback. Player orders and object-generated movement or
 // retaliation orders admit actors to the same original Logic queue.
 while(EventClass::OutList.Count) {
  EventClass event(EventClass::OutList.First());EventClass::OutList.Next();
  if(event.Type==EventType::Produce||event.Type==EventType::Suspend||event.Type==EventType::Abandon||event.Type==EventType::Place){event.Execute();continue;}
  auto* actor=event.Type==EventType::Deploy?event.Deploy.Whom.As_Techno():
      event.Type==EventType::Idle?event.Idle.Whom.As_Techno():event.MegaMission.Whom.As_Techno();
  if(!actor || resolve_map_object(world,object_id(world,actor))!=actor || !actor->IsAlive || actor->Health<=0 || actor->InLimbo)continue;
  if((actor->WhatAmI()==AbstractType::Infantry||actor->WhatAmI()==AbstractType::Unit) && LogicClass::Instance.FindItemIndex(actor)<0
      && !LogicClass::Instance.AddObject(actor,false))continue;
  event.Execute();
 }
 // Compatibility admission for externally supplied objects. Map readers now
 // use Unlimbo, which registers normal actors with Logic during placement.
 // Existing Logic members must never be inserted or updated a second time.
 const auto admit_ordered_actor=[](FootClass* actor){
  if(!actor->IsAlive||actor->Health<=0||actor->InLimbo||!actor->IsOnMap)return;
  if((actor->Destination||(actor->Target&&actor->MissionIsOverriden()))
     &&LogicClass::Instance.FindItemIndex(actor)<0)LogicClass::Instance.AddObject(actor,false);
 };
 for(int i=w.units;i<UnitClass::Array.Count;++i)admit_ordered_actor(UnitClass::Array[i]);
 for(int i=w.infantry;i<InfantryClass::Array.Count;++i)admit_ordered_actor(InfantryClass::Array[i]);
 const bool hadProjectileVisuals=BulletClass::Array.Count>w.bullets||LineTrail::Array.Count>w.line_trails||ParticleSystemClass::Array.Count>w.particle_systems;
 LogicClass::Instance.Update();
 LineTrail::UpdateAll();
 if(hadProjectileVisuals||BulletClass::Array.Count>w.bullets||LineTrail::Array.Count>w.line_trails||ParticleSystemClass::Array.Count>w.particle_systems)map_object_changed();
 AbstractClass::RemoveAllInactive();
 // Compatibility for external presentation-only objects. Map-loaded units
 // already tick FlashData in TechnoClass::Update (0x6F9E50); never advance
 // those Logic members a second time.
 const auto update_display_flash=[&](TechnoClass* actor){
  if(!actor->IsAlive||actor->InLimbo||!actor->IsOnMap||LogicClass::Instance.FindItemIndex(actor)>=0)return;
  const int previous=actor->Flashing.DurationRemaining;
  if(actor->Flashing.Update())actor->Mark(MarkType::Change);
  if((previous&2)!=(actor->Flashing.DurationRemaining&2))map_object_changed();
 };
 for(int i=w.units;i<UnitClass::Array.Count;++i)update_display_flash(UnitClass::Array[i]);
 for(int i=w.infantry;i<InfantryClass::Array.Count;++i)update_display_flash(InfantryClass::Array[i]);
 // Interim display-session animation, NOT a replacement Infantry.Update.
 // Use original objects, timers, RNG and sequence callbacks; no host tween or
 // second animation state. Never execute mission/target/locomotion decisions
 // here. Once an infantry enters Logic it must not be ticked a second time.
 for(int i=w.infantry;i<InfantryClass::Array.Count;++i) {
  auto& infantry=*InfantryClass::Array[i];
  // Foot.Update owns an attached parasite even if the victim has received no
  // player order. Advance that effect without admitting faction/unit AI.
  if(infantry.ParasiteEatingMe&&LogicClass::Instance.FindItemIndex(&infantry)<0)
   infantry.ParasiteEatingMe->ParasiteImUsing->Update();
  // External display-only actors still need to complete a death sequence.
  // Ordinary map occupants now execute the normal Logic path above.
  if(infantry.IsAlive&&!infantry.InLimbo&&infantry.IsOnMap&&infantry.Health<=0
      &&LogicClass::Instance.FindItemIndex(&infantry)<0&&infantry.IsPlayingDeathSequence()){
   infantry.Animation.Update();infantry.Doing_AI();infantry.NeedsRedraw=true;map_object_changed();continue;
  }
  if(!infantry.IsAlive || infantry.InLimbo || !infantry.IsOnMap || infantry.Health<=0
      || LogicClass::Instance.FindItemIndex(&infantry)>=0
      || !infantry.Type || !infantry.Type->Sequence || !infantry.Locomotor
      || infantry.Target || infantry.Destination || infantry.IsFiring || infantry.Crawling
      || infantry.IsFallingDown || infantry.IsInAir() || infantry.IsBeingWarpedOut() || infantry.IsWarpingIn()
      || infantry.Type->Fraidycat || !_strcmpi(infantry.Type->ID,"COW")
      || (infantry.GetCurrentMission()!=Mission::Guard && infantry.GetCurrentMission()!=Mission::Area_Guard)
      || infantry.Locomotor->Is_Moving())continue;
  switch(infantry.SequenceAnim) {
   case Sequence::Nothing:case Sequence::Ready:case Sequence::Guard:
   case Sequence::Idle1:case Sequence::Idle2:case Sequence::Tread:
   case Sequence::WetIdle1:case Sequence::WetIdle2:break;
   default:continue;
  }
  const int before=infantry.GetCurrentFrame();
  if(infantry.SequenceAnim==Sequence::Nothing)infantry.PlayAnim(Sequence::Ready);
  infantry.UpdateIdleAction();
  infantry.Animation.Update();
  infantry.Doing_AI();
  if(infantry.GetCurrentFrame()!=before){infantry.NeedsRedraw=true;map_object_changed();}
 }
 ++world.simulation_tick;
 AbstractClass::RemoveAllInactive();
 refresh_map_world_hover(world);
}
MapObjectId object_id(const MapWorld&w,const ObjectClass*o)noexcept{return o?MapObjectId{w.impl->world_id,std::uint32_t(o->UniqueID)}:MapObjectId{};}
ObjectClass*resolve_map_object(MapWorld&world,MapObjectId id)noexcept{auto&w=*world.impl;if(id.world!=w.world_id)return nullptr;for(int i=w.buildings;i<BuildingClass::Array.Count;++i)if(std::uint32_t(BuildingClass::Array[i]->UniqueID)==id.value)return BuildingClass::Array[i];for(int i=w.terrains;i<TerrainClass::Array.Count;++i)if(std::uint32_t(TerrainClass::Array[i]->UniqueID)==id.value)return TerrainClass::Array[i];for(int i=w.infantry;i<InfantryClass::Array.Count;++i)if(std::uint32_t(InfantryClass::Array[i]->UniqueID)==id.value)return InfantryClass::Array[i];for(int i=w.units;i<UnitClass::Array.Count;++i)if(std::uint32_t(UnitClass::Array[i]->UniqueID)==id.value)return UnitClass::Array[i];for(int i=w.aircraft;i<AircraftClass::Array.Count;++i)if(std::uint32_t(AircraftClass::Array[i]->UniqueID)==id.value)return AircraftClass::Array[i];return nullptr;}
void refresh_map_world_hover(MapWorld&world){
 auto&w=*world.impl;
 // Reassert even when the pointer/ID did not change: Techno::Update resets
 // IsMouseHovering every frame, before the display/input pass.
 set_world_hover(world,w.view.pointer_inside?pick_world_object(world,w.view.pointer):nullptr);
}
void input_map_world(MapWorld&world,const GameInputEvent&e,GameInputResult&result){
 auto&w=*world.impl;
 auto& tactical=w.view.tactical;
 const auto cancel_band=[&]{w.pressed=w.dragging=false;tactical.EndRubberBand();++world.presentation_revision;};
 if(e.kind==GameInputKind::focus_lost||e.kind==GameInputKind::pointer_leave){cancel_band();set_world_hover(world,nullptr);return;}
 if(e.kind==GameInputKind::pointer_move||e.kind==GameInputKind::pointer_button)refresh_map_world_hover(world);
 auto& display=DisplayClass::Instance;
 if(display.RepairMode || display.SellMode) {
  if((e.kind==GameInputKind::key && e.code==27 && e.pressed)
      ||(e.kind==GameInputKind::pointer_button && e.code==2)) {
   display.SetRepairMode(0);display.SetSellMode(0);cancel_band();
   MouseClass::Instance.ResetScrollInput();SidebarClass::Instance.UpdateCommandButtons();result.consumed=true;return;
  }
 }
 if(e.kind==GameInputKind::key && e.code==27 && e.pressed && w.pressed){cancel_band();result.consumed=true;return;}
 auto b=DSurface::ViewBounds;if(b.Width<=0||b.Height<=0)b=TacticalClass::ViewBounds;
 if(b.Width<=0||b.Height<=0)return;
 const auto update_band=[&]{
  const int dx=e.x-w.press_point.X,dy=e.y-w.press_point.Y;
  if(!w.dragging && double(dx)*dx+double(dy)*dy>16.0){
   w.dragging=true;tactical.StartRubberBand({w.press_point.X-b.X,w.press_point.Y-b.Y});
  }
  if(w.dragging){
   tactical.ModifyRubberBand({std::clamp(e.x-b.X,0,b.Width-1),std::clamp(e.y-b.Y,0,b.Height-1)});
   ++world.presentation_revision;
  }
 };
 if(w.pressed && !display.RepairMode && !display.SellMode && e.kind==GameInputKind::pointer_move){update_band();result.consumed=true;return;}
 if(w.pressed && !display.RepairMode && !display.SellMode && e.kind==GameInputKind::pointer_button && e.code==1 && !e.pressed){
  update_band();
  if(w.dragging){
   rebuild_world_selectables(world);
   // YR 0x4AB9B0 keeps the old selection when an empty band catches nothing.
   if(!(e.modifiers&1) && tactical.HasBandObjects())
    while(ObjectClass::CurrentObjects.Count)ObjectClass::CurrentObjects[ObjectClass::CurrentObjects.Count-1]->Deselect();
   tactical.SelectRubberBand(DisplayClass::BandboxSelectionCallback);
   TacticalClass::StartDrawActionLineTimer();
   cancel_band();result.consumed=true;return;
  }
 }
 const bool inside=e.x>=b.X&&e.y>=b.Y&&e.x<b.X+b.Width&&e.y<b.Y+b.Height;
 if(!inside){if(e.kind==GameInputKind::pointer_button&&!e.pressed)cancel_band();return;}
 if(display.RepairMode || display.SellMode) {
  // YR 0x692610 building mode classifier. Repair uses action 10 (Eaten),
  // whereas action 11 belongs to unit/engineer interaction.
  auto* object=pick_world_object(world,{e.x,e.y});
  auto* building=object&&object->WhatAmI()==AbstractType::Building?static_cast<BuildingClass*>(object):nullptr;
  const bool owned=building && building->Owner && building->Owner->IsControlledByCurrentPlayer();
  const auto action=display.RepairMode?(owned&&building->CanBeRepaired()?Action::Eaten:Action::NoRepair):
      (owned&&building->CanBeSold()&&!building->IsStrange()?Action::Sell:Action::NoSell);
  CellStruct cell;const bool cell_found=tactical.PickTerrainCell({e.x,e.y},b,cell);
  if(cell_found)display.ConvertAction(cell,false,object,action,false);
  if(e.kind==GameInputKind::pointer_button && e.code==1) {
   if(e.pressed){cancel_band();w.pressed=true;w.press_point={e.x,e.y};w.press_target=object_id(world,object);}
   else if(w.pressed) {
    w.pressed=false;
    const auto target=object_id(world,object);
    if(cell_found && target.world==w.press_target.world && target.value==w.press_target.value
       && std::abs(e.x-w.press_point.X)<=4 && std::abs(e.y-w.press_point.Y)<=4
       && (action==Action::Eaten || action==Action::Sell))
     display.LeftMouseButtonUp(building->GetCoords(),cell,object,action,0);
   }
  }
  result.consumed=true;return;
 }
 if(e.kind==GameInputKind::pointer_move)return;
 if(e.kind==GameInputKind::pointer_button&&e.code==1){
  if(e.pressed){
   if(result.consumed||GadgetClass::StuckOn)return;
   cancel_band();w.pressed=true;w.press_point={e.x,e.y};w.press_target=object_id(world,pick_world_object(world,w.press_point));result.consumed=true;
  }else if(w.pressed){
   w.pressed=false;result.consumed=true;
   auto*picked=pick_world_object(world,{e.x,e.y});const auto released=object_id(world,picked);
   if(std::abs(e.x-w.press_point.X)>4||std::abs(e.y-w.press_point.Y)>4||released.world!=w.press_target.world||released.value!=w.press_target.value)return;
   // Active_Click 0x4AE750 re-evaluates each selected object's action. Keep
   // selection when giving orders. Both cell and object commands go through
   // the original action classifier and Event queue, never host combat state.
   // Armed defenses are not garrisons: let each building classify its orders.
   if(picked&&ObjectClass::CurrentObjects.Count&&!(e.modifiers&1)){
    bool commanded=false;
    for(auto* selected:ObjectClass::CurrentObjects){
     if(!selected||(selected->WhatAmI()!=AbstractType::Infantry&&selected->WhatAmI()!=AbstractType::Unit&&selected->WhatAmI()!=AbstractType::Building))continue;
     auto* actor=static_cast<TechnoClass*>(selected);
     if(!actor->Owner->IsControlledByCurrentPlayer())continue;
     const auto action=actor->MouseOverObject(picked,false);
     if(action==Action::Attack||action==Action::Self_Deploy||action==Action::AreaAttack
       ||action==Action::Capture||action==Action::Damage||action==Action::GRepair||action==Action::Enter||action==Action::Repair)
      commanded=actor->ObjectClickedAction(action,picked,false)||commanded;
    }
    if(commanded){TacticalClass::StartDrawActionLineTimer();return;}
   }
   if(!picked && ObjectClass::CurrentObjects.Count){
    CellStruct cell;
    if(!w.view.tactical.PickTerrainCell({e.x,e.y},b,cell))return;
    bool commanded=false;
    const bool feedback=Unsorted::MoveFeedback;
    auto restoreFeedback=[&]{Unsorted::MoveFeedback=feedback;};
    try {
     for(auto* selected:ObjectClass::CurrentObjects){
      if(!selected||(selected->WhatAmI()!=AbstractType::Infantry&&selected->WhatAmI()!=AbstractType::Unit&&selected->WhatAmI()!=AbstractType::Building))continue;
      auto* actor=static_cast<TechnoClass*>(selected);
      if(!actor->Owner->IsControlledByCurrentPlayer())continue;
      const auto action=actor->MouseOverCell(&cell,false,false);
      if(action!=Action::Move && action!=Action::NoMove && action!=Action::Attack)continue;
      // Ctrl+Shift attack-move has a separate, still pending firing path.
      if(Game::AttackMoveMode || (e.modifiers&3)==3)continue;
      CellStruct follow=cell;
      commanded=actor->CellClickedAction(action,&cell,&follow,false)||commanded;
      Unsorted::MoveFeedback=false;
     }
    }catch(...){restoreFeedback();throw;}
    restoreFeedback();
    if(commanded){TacticalClass::StartDrawActionLineTimer();return;}
    // An unavailable order is not a request to deselect the current group.
    return;
   }
   if(!(e.modifiers&1))while(ObjectClass::CurrentObjects.Count)ObjectClass::CurrentObjects[ObjectClass::CurrentObjects.Count-1]->Deselect();
   if(picked&&picked->CanBeSelected()){
    if((e.modifiers&1)&&picked->IsSelected)picked->Deselect();else picked->Select();
    TacticalClass::StartDrawActionLineTimer();
   }
  }
 }
 if(e.kind==GameInputKind::pointer_button&&e.code==2&&!result.consumed){
  // Display.RightMouseButtonUp 0x4AAD30, after Mouse has consumed a drag.
  // Pressing right begins scrolling; it must not discard the selected army.
  if(!e.pressed)while(ObjectClass::CurrentObjects.Count)ObjectClass::CurrentObjects[ObjectClass::CurrentObjects.Count-1]->Deselect();
  cancel_band();result.consumed=true;
 }
}
ObjectClass*pick_world_object(MapWorld&world,Point2D p){
 auto b=DSurface::ViewBounds;if(b.Width<=0||b.Height<=0)b=TacticalClass::ViewBounds;
 if(p.X<b.X||p.Y<b.Y||p.X>=b.X+b.Width||p.Y>=b.Y+b.Height)return nullptr;
 p.X-=b.X;p.Y-=b.Y;rebuild_world_selectables(world);
 auto& tactical=world.impl->view.tactical;
 auto* object=tactical.GetSelectableObject(p);
 // Final target rejection in ProcessClickCoords, 0x00692522..0x006925EA,
 // also applies to the cell fallback. Full fog/cursor handling is separate.
 if(object&&(object->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None){
  auto* techno=static_cast<TechnoClass*>(object);
  if(!techno->IsOwnedByCurrentPlayer){
   const auto sensed=[&]{return MapClass::Instance.GetCellAt(techno->GetCoords())->Sensors_InclHouse(HouseClass::CurrentPlayer->ArrayIndex);};
   if((techno->CloakState==CloakState::Cloaked&&!sensed())||techno->GetTechnoType()->Invisible)return nullptr;
   if(object->WhatAmI()==AbstractType::Building){auto* building=static_cast<BuildingClass*>(object);
    if((building->Translucency==15&&!sensed())||building->Type->InvisibleInGame)return nullptr;
   }
  }
 }
 return object;
}

MapWorld* current_map_world() noexcept{return active_world;}
}
