#include "api/map_objects.hpp"
#include "map_world_internal.hpp"
#include "map_view.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/MapClass.h"
#include <algorithm>
#include <cstdio>
namespace game {
namespace {
bool valid(MapViewHandle&v){MapViewInfo i{};return !v.operation_depth&&v.world&&get_map_view_info(v,i)&&i.state==MapViewState::ready;}
void utf8(const wchar_t*in,char*out,std::size_t cap)noexcept{if(!cap)return;std::size_t used=0;for(std::size_t i=0;in&&in[i]&&i<256;++i){std::uint32_t c=std::uint32_t(in[i]);if(c>=0xd800&&c<=0xdbff&&in[i+1]>=0xdc00&&in[i+1]<=0xdfff)c=0x10000+((c-0xd800)<<10)+(std::uint32_t(in[++i])-0xdc00);if(c>0x10ffff||(c>=0xd800&&c<=0xdfff))c=0xfffd;std::size_t n=c<0x80?1:c<0x800?2:c<0x10000?3:4;if(used+n>=cap)break;if(n==1)out[used++]=char(c);else{out[used++]=char((n==2?0xc0:n==3?0xe0:0xf0)|(c>>(6*(n-1))));for(int j=int(n)-2;j>=0;--j)out[used++]=char(0x80|((c>>(6*j))&63));}}out[used]=0;}
}
void snapshot_object(MapWorld&world,ObjectClass&o,MapObjectSnapshot&out)noexcept{
 out={};out.id=object_id(world,&o);out.kind=o.WhatAmI()==AbstractType::Building?MapObjectKind::building:o.WhatAmI()==AbstractType::Infantry?MapObjectKind::infantry:o.WhatAmI()==AbstractType::Unit?MapObjectKind::unit:o.WhatAmI()==AbstractType::Aircraft?MapObjectKind::aircraft:MapObjectKind::terrain;
 auto*t=o.GetType();out.type_index=t?t->GetArrayIndex():-1;out.health=o.Health;out.max_health=t?t->Strength:0;
 out.alive=!o.IsDead();out.selected=o.IsSelected;out.hovered=out.id.world==world.impl->hover.world&&out.id.value==world.impl->hover.value;
 if(out.kind==MapObjectKind::building||out.kind==MapObjectKind::infantry||out.kind==MapObjectKind::unit||out.kind==MapObjectKind::aircraft){
  const float rank=static_cast<TechnoClass&>(o).Veterancy.Veterancy;
  out.level=rank>=2.0f?2:rank>=1.0f?1:0;
 }
 out.health_band=!out.alive?MapHealthBand::dead:o.IsRedHP()?MapHealthBand::red:o.IsYellowHP()?MapHealthBand::yellow:MapHealthBand::green;
 out.world_x=o.Location.X;out.world_y=o.Location.Y;out.world_z=o.Location.Z;out.cell_x=o.Location.X/256;out.cell_y=o.Location.Y/256;out.has_image=t&&t->Image;
 utf8(o.GetUIName(),out.name,sizeof(out.name));if(t){std::snprintf(out.type_id,sizeof(out.type_id),"%s",t->ID);if(!*out.name)std::snprintf(out.name,sizeof(out.name),"%s",*t->Name?t->Name:t->ID);}
 if(out.kind==MapObjectKind::building){auto&b=static_cast<BuildingClass&>(o);out.frame=b.GetCurrentFrame();out.powered=b.IsPowerOnline();if(b.Owner){out.owner_index=b.Owner->ArrayIndex;std::snprintf(out.owner_name,sizeof(out.owner_name),"%s",b.Owner->PlainName);}}
 else if(out.kind==MapObjectKind::infantry){auto&i=static_cast<InfantryClass&>(o);out.frame=i.GetCurrentFrame();if(i.Owner){out.owner_index=i.Owner->ArrayIndex;std::snprintf(out.owner_name,sizeof(out.owner_name),"%s",i.Owner->PlainName);}}
 else if(out.kind==MapObjectKind::unit||out.kind==MapObjectKind::aircraft){auto& unit=static_cast<FootClass&>(o);auto* type=unit.GetTechnoType();out.frame=unit.WalkedFramesSoFar;out.has_image=type&&(type->Image||(type->MainVoxel.VXL&&type->MainVoxel.HVA));if(unit.Owner){out.owner_index=unit.Owner->ArrayIndex;std::snprintf(out.owner_name,sizeof(out.owner_name),"%s",unit.Owner->PlainName);}}
 else out.frame=static_cast<TerrainClass&>(o).Animation.Value;
}
bool get_map_world_snapshot(MapViewHandle&v,MapWorldSnapshot&output)noexcept{
 if(!valid(v))return false;MapWorldSnapshot out{};struct Query{MapViewHandle&v;MapWorldSnapshot&o;}q{v,out};
 if(!with_map_view(v,[](void*p){auto&q=*static_cast<Query*>(p);auto&world=*q.v.world;auto&w=*world.impl;auto&o=q.o;
 o.presentation_revision=world.presentation_revision;o.resource_revision=world.resource_revision;o.simulation_tick=world.simulation_tick;
 o.buildings=BuildingClass::Array.Count-w.buildings;o.terrain_objects=TerrainClass::Array.Count-w.terrains;o.animations=AnimClass::Array.Count-w.animations;
 o.infantry=InfantryClass::Array.Count-w.infantry;o.units=UnitClass::Array.Count-w.units;
 for(auto*c:w.resource_cells)if(c->GetContainedTiberiumIndex()>=0)++o.resource_cells;
 o.selected=ObjectClass::CurrentObjects.Count;o.hovered=w.hover;o.missing_images=w.missing_images;o.unknown_records=w.unknown_records;o.missing_palettes=w.missing_palettes;o.objects_loaded=w.loaded;
 },&q))return false;output=out;return true;
}
bool copy_map_objects(MapViewHandle&v,MapObjectSnapshot*out,std::uint32_t cap,std::uint32_t&required)noexcept{
 required=0;if(!valid(v)||(!out&&cap))return false;auto&w=*v.world->impl;required=BuildingClass::Array.Count-w.buildings+TerrainClass::Array.Count-w.terrains+InfantryClass::Array.Count-w.infantry+UnitClass::Array.Count-w.units+AircraftClass::Array.Count-w.aircraft;if(!out)return cap==0;if(cap<required)return false;
 struct Query{MapWorld&w;MapObjectSnapshot*out;}q{*v.world,out};return with_map_view(v,[](void*p){auto&q=*static_cast<Query*>(p);auto&w=*q.w.impl;for(int i=w.buildings;i<BuildingClass::Array.Count;++i)snapshot_object(q.w,*BuildingClass::Array[i],*q.out++);for(int i=w.terrains;i<TerrainClass::Array.Count;++i)snapshot_object(q.w,*TerrainClass::Array[i],*q.out++);for(int i=w.infantry;i<InfantryClass::Array.Count;++i)snapshot_object(q.w,*InfantryClass::Array[i],*q.out++);for(int i=w.units;i<UnitClass::Array.Count;++i)snapshot_object(q.w,*UnitClass::Array[i],*q.out++);for(int i=w.aircraft;i<AircraftClass::Array.Count;++i)snapshot_object(q.w,*AircraftClass::Array[i],*q.out++);},&q);
}
bool get_map_object(MapViewHandle&v,MapObjectId id,MapObjectSnapshot&output)noexcept{
 if(!valid(v))return false;MapObjectSnapshot out{};struct Query{MapWorld&w;MapObjectId id;MapObjectSnapshot&o;bool ok=false;}q{*v.world,id,out};if(!with_map_view(v,[](void*p){auto&q=*static_cast<Query*>(p);if(auto*o=resolve_map_object(q.w,q.id)){snapshot_object(q.w,*o,q.o);q.ok=true;}},&q)||!q.ok)return false;output=out;return true;
}
bool get_map_resource(MapViewHandle&v,int x,int y,MapResourceSnapshot&output)noexcept{
 if(!valid(v)||x<0||x>=512||y<0||y>=512)return false;MapResourceSnapshot out{};struct Query{int x,y;MapResourceSnapshot&o;bool ok=false;}q{x,y,out};
 if(!with_map_view(v,[](void*p){auto&q=*static_cast<Query*>(p);auto*c=MapClass::Instance.TryGetCellAt(CellStruct{short(q.x),short(q.y)});if(!c)return;auto&o=q.o;o.cell_x=q.x;o.cell_y=q.y;o.overlay_index=c->OverlayTypeIndex;o.frame=c->OverlayData;o.type_index=c->GetContainedTiberiumIndex();if(auto*t=TiberiumClass::Array.GetItemOrDefault(o.type_index)){o.units=c->OverlayData+1;o.value_per_unit=std::max(t->Value,0);o.total_value=std::int64_t(o.units)*o.value_per_unit;}q.ok=true;},&q)||!q.ok)return false;output=out;return true;
}
bool set_map_object_health(MapViewHandle&v,MapObjectId id,int hp)noexcept{
 if(!valid(v)||hp<0)return false;struct Command{MapWorld&w;MapObjectId id;int hp;bool ok=false;}q{*v.world,id,hp};
 return with_map_view(v,[](void*p){auto&q=*static_cast<Command*>(p);auto*o=resolve_map_object(q.w,q.id);if(!o||o->IsDead()||!o->GetType())return;if(q.hp==0&&o->WhatAmI()==AbstractType::Building){auto*b=static_cast<BuildingClass*>(o);b->Destory(nullptr,nullptr,false,b->Type->FoundationData);refresh_map_world_hover(q.w);q.ok=true;return;}
 o->Health=o->EstimatedHealth=std::clamp(q.hp,0,std::max(o->GetType()->Strength,0));
 if(!o->Health){o->Deselect();o->IsAlive=false;o->NotifyObjectExpired(true);detach_map_object(*o);if(o->WhatAmI()==AbstractType::Building){auto*b=static_cast<BuildingClass*>(o);for(int i=0;i<21;++i)b->DestroyNthAnim(static_cast<BuildingAnimSlot>(i));}}
 else if(o->WhatAmI()==AbstractType::Building)static_cast<BuildingClass*>(o)->ToggleDamagedAnims(!o->IsGreenHP());if(o->WhatAmI()==AbstractType::Building){auto*b=static_cast<BuildingClass*>(o);if(b->Owner)b->Owner->RecheckPower=true;}o->NeedsRedraw=true;map_object_changed();refresh_map_world_hover(q.w);q.ok=true;
 },&q)&&q.ok;
}
bool set_map_object_level(MapViewHandle&v,MapObjectId id,int level)noexcept{
 if(!valid(v)||level<0||level>2)return false;
 struct Command{MapWorld&w;MapObjectId id;int level;bool ok=false;}q{*v.world,id,level};
 return with_map_view(v,[](void*p){auto&q=*static_cast<Command*>(p);auto*o=resolve_map_object(q.w,q.id);
  if(!o||o->IsDead())return;
  const auto kind=o->WhatAmI();
  if(kind!=AbstractType::Building&&kind!=AbstractType::Infantry&&kind!=AbstractType::Unit&&kind!=AbstractType::Aircraft)return;
  auto& techno=*static_cast<TechnoClass*>(o);
  techno.Veterancy.Veterancy=static_cast<float>(q.level);
  techno.NeedsRedraw=true;
  map_object_changed();q.ok=true;
 },&q)&&q.ok;
}
bool set_map_unit_cheat_health(MapViewHandle&v,MapObjectId id,int hp)noexcept{
 if(!valid(v)||hp<0||hp>1000000)return false;
 struct Command{MapWorld&w;MapObjectId id;int hp;bool ok=false;}q{*v.world,id,hp};
 return with_map_view(v,[](void*p){auto&q=*static_cast<Command*>(p);auto*o=resolve_map_object(q.w,q.id);
  if(!o||o->IsDead())return;
  const auto kind=o->WhatAmI();
  if(kind!=AbstractType::Infantry&&kind!=AbstractType::Unit&&kind!=AbstractType::Aircraft)return;
  if(q.hp==0){o->Deselect();o->IsAlive=false;o->Health=o->EstimatedHealth=0;
   o->NotifyObjectExpired(true);detach_map_object(*o);refresh_map_world_hover(q.w);}
  else o->Health=o->EstimatedHealth=q.hp;
  o->NeedsRedraw=true;map_object_changed();q.ok=true;
 },&q)&&q.ok;
}
bool grant_map_player_power(MapViewHandle&v)noexcept{
 if(!valid(v))return false;
 struct Command{MapWorld&w;bool ok=false;}q{*v.world};
 return with_map_view(v,[](void*p){auto&q=*static_cast<Command*>(p);
  auto*house=HouseClass::CurrentPlayer;if(!house)return;
  q.w.impl->player_power_bonus=1000000;
  house->RecheckPower=true;
  house->UpdatePower();
  q.ok=true;
 },&q)&&q.ok;
}
bool set_map_building_enabled(MapViewHandle&v,MapObjectId id,bool enabled)noexcept{
 if(!valid(v))return false;struct Command{MapWorld&w;MapObjectId id;bool enabled,ok=false;}q{*v.world,id,enabled};return with_map_view(v,[](void*p){auto&q=*static_cast<Command*>(p);auto*o=resolve_map_object(q.w,q.id);if(!o||o->IsDead()||o->WhatAmI()!=AbstractType::Building)return;auto*b=static_cast<BuildingClass*>(o);b->StuffEnabled=q.enabled;b->HasPower=q.enabled;if(b->Owner)b->Owner->RecheckPower=true;b->UpdateAnimations();map_object_changed();q.ok=true;},&q)&&q.ok;
}
bool harvest_map_resource(MapViewHandle&v,int x,int y,int requested,int&units,std::int64_t&value)noexcept{
 units=0;value=0;if(!valid(v)||x<0||x>=512||y<0||y>=512||requested<=0)return false;struct Command{int x,y,n,units=0;std::int64_t value=0;bool ok=false;}q{x,y,requested};
 bool done=with_map_view(v,[](void*p){auto&q=*static_cast<Command*>(p);auto*c=MapClass::Instance.TryGetCellAt(CellStruct{short(q.x),short(q.y)});if(!c)return;if(auto*t=TiberiumClass::Find(c->OverlayTypeIndex)){q.units=c->ReduceTiberium(q.n);q.value=std::int64_t(q.units)*std::max(t->Value,0);}q.ok=true;},&q);if(!done||!q.ok)return false;units=q.units;value=q.value;return true;
}
}
