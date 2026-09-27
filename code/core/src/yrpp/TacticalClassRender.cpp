// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
// Background traversal derives from OpenTS 44fac744 tactical.cpp, calibrated to YR.
// TacticalClass::Render (0x006D3D10): native whole-frame background order
// is restored through the original class entries. Foreground uses the five
// original display layers and submits object hooks in their original order. Host resource/cache/submit code
// stays in map_world_drawing.cpp; public classes contain no game interfaces.
#include "map_world_internal.hpp"
#include "building_selection.hpp"
#include "building_drawing.hpp"
#include "overlay_drawing.hpp"
#include "sprite_drawing.hpp"
#include "techno_drawing.hpp"
#include "yrpp/ScenarioClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/RulesClass.h"
#include "type_drawing.hpp"
#include "map_view.hpp"
#include "api/images.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/LineTrail.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/BitFont.h"
#include "yrpp/Surface.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/ColorScheme.h"
#include "map_runtime.hpp"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <stdexcept>
#include "tactical_drawing.hpp"
using namespace game;
namespace {
bool complete(DrawingStatus s){return s==DrawingStatus::drawn||s==DrawingStatus::skipped;}
Point2D project(const CoordStruct&loc,const MapViewHandle&v){auto p=TacticalClass::CoordsToScreen(loc);p.X-=v.tactical.TacticalPos.X;p.Y-=v.tactical.TacticalPos.Y;return p;}
int sort_at(const ObjectClass& object,const CoordStruct& position){
 // Attached-animation coordinates are resolved by this display adapter below.
 // Translate the original object's world-space key to that same render origin;
 // never mix the slot's world YSortAdjust with projected screen pixels.
 const auto origin=object.GetRenderCoords();
 return std::bit_cast<std::int32_t>(std::uint32_t(object.GetYSort())
     +std::uint32_t(position.X)-std::uint32_t(origin.X)
     +std::uint32_t(position.Y)-std::uint32_t(origin.Y));
}
void append_original_sprite(MapWorld& world,ObjectClass& object,AnimClass* anim,
        ObjectClass* owner,CellClass* cell,Point2D point,const BytePalette* alternative,
        int sort,bool owner_palette,bool pickable,bool visible=false,bool forced=true,const RectangleStruct* clip=nullptr) {
 struct Sink {MapWorld& world;ObjectClass* owner;CellClass* cell;int sort;bool pickable;
  bool parachute;Layer layer;SpritePalette palette=SpritePalette::animation;};
 const bool parachute=anim&&owner&&owner->Parachute==anim;
 const auto layer=anim?anim->InWhichLayer():Layer::Ground;
 if(layer<Layer::Underground||layer>Layer::Top)return;
 Sink sink{world,owner,cell,sort,pickable,parachute,layer};
 SpriteDrawing drawing;drawing.context=&sink;drawing.types.backend_context=&sink;
 drawing.types.target=reinterpret_cast<DrawingTargetHandle*>(&sink);
 drawing.detail_level=GameOptionsClass::Instance.DetailLevel;
 drawing.tactical_rect=clip?*clip:RectangleStruct{0,0,TacticalClass::ViewBounds.Width,TacticalClass::ViewBounds.Height};
 // This host uses the SHP renderer. The optional original Direct3D RING1
 // triangle path remains available to an environment with that backend.
 drawing.hardware=false;
 drawing.cell_at=[](void*,const CellStruct& at) noexcept {return MapClass::Instance.GetCellAt(at);};
 drawing.cell_at_world=[](void*,const CoordStruct& at) noexcept {return MapClass::Instance.GetCellAt(at);};
 drawing.initialize_light=[](void*,CellClass&) noexcept {return DrawingStatus::drawn;};
 drawing.shape_data=world_sprite_data;
 drawing.frame_bounds=[](SHPStruct* image,int frame){auto* shape=world_sprite_data(image);return shape?shape->GetFrameBounds(frame):RectangleStruct{};};
 drawing.height=[](int z) noexcept {return TacticalClass::AdjustForZ(z);};
 drawing.building=[](CellClass& c) noexcept {return c.GetBuilding();};
 drawing.shrouded=[](CellClass& c) noexcept {return c.IsShrouded();};
 drawing.pixel_format=[]() noexcept {return 2;};
 if(auto* rules=RulesClass::Instance){
  if(unsigned(rules->LaserTargetColor)<16)drawing.laser_color=reinterpret_cast<const unsigned char*>(&rules->ColorAdd[rules->LaserTargetColor]);
  if(unsigned(rules->ForceShieldColor)<16)drawing.shield_color=reinterpret_cast<const unsigned char*>(&rules->ColorAdd[rules->ForceShieldColor]);
 }
 if(anim&&!anim->UseCellLightConvert&&(owner_palette||parachute)){
  drawing.alternative_palette=reinterpret_cast<const DrawingPaletteHandle*>(alternative);
  drawing.alternative_intensity=parachute?anim->TintColor:cell?std::bit_cast<short>(cell->Intensity_Normal):1000;
  if(owner&&owner->WhatAmI()==AbstractType::Building)
   drawing.alternative_intensity=static_cast<BuildingClass*>(owner)->GetFlashingIntensity(drawing.alternative_intensity);
 }
 drawing.palette=[](void* p,SpritePalette kind,const AnimClass*,CellClass* cell,const DrawingPaletteHandle*& result) noexcept {
  try{
   auto& sink=*static_cast<Sink*>(p);auto& w=*sink.world.impl;sink.palette=kind;
   const BytePalette* palette=&w.anim_palette;
   switch(kind){
   case SpritePalette::cell:sink.cell=cell;palette=&FileSystem::ISOx_PAL;break;
   case SpritePalette::tiberium:case SpritePalette::neutral:palette=&w.unit_palette;break;
   case SpritePalette::player:
    if(!HouseClass::CurrentPlayer)return DrawingStatus::unavailable;
    palette=world_palette(sink.world,w.unit_palette,HouseClass::CurrentPlayer);break;
   case SpritePalette::alternative:
    if(sink.owner&&sink.owner->WhatAmI()==AbstractType::Building&&static_cast<BuildingClass*>(sink.owner)->Type->TerrainPalette)
     palette=&FileSystem::ISOx_PAL;
    else palette=world_palette(sink.world,w.unit_palette,sink.owner?sink.owner->GetOwningHouse():nullptr);
    break;
   case SpritePalette::animation:break;
   }
   result=reinterpret_cast<const DrawingPaletteHandle*>(palette);return DrawingStatus::drawn;
  }catch(...){return DrawingStatus::backend_failure;}
 };
 drawing.types.backend.shape=[](void* p,const ShapeDrawingRequest& r) noexcept {
  try{
   if(auto* frame=tactical_drawing();frame&&frame->selection_only)return DrawingStatus::skipped;
   auto& sink=*static_cast<Sink*>(p);
   const bool shadow=(r.flags&1)!=0;
   auto* s=append_world_sprite(sink.world,r.image,r.frame,sink.owner,sink.cell,r.position,
    reinterpret_cast<const BytePalette*>(r.palette),sink.sort,r.gradient==0,shadow,!shadow&&sink.pickable);
   if(!s)return DrawingStatus::skipped;
   s->layer=sink.layer;s->parachute=sink.parachute;s->original_depth=true;
   s->flags=r.flags;s->gradient=r.gradient;s->depth_adjustment=r.depth_adjustment;s->intensity=r.intensity;s->tint=r.tint;
   s->cell_tint=sink.palette==SpritePalette::cell||sink.palette==SpritePalette::alternative;
   s->color_scheme=sink.palette==SpritePalette::neutral||sink.palette==SpritePalette::player
    ||(sink.palette==SpritePalette::alternative&&s->palette!=&FileSystem::ISOx_PAL);
   return DrawingStatus::drawn;
  }catch(...){return DrawingStatus::backend_failure;}
 };
 struct Call{const ObjectClass& object;RectangleStruct clip;bool force;}call{object,drawing.tactical_rect,forced};
 const auto result=visible?with_sprite_drawing(drawing,[](void* p){auto& c=*static_cast<Call*>(p);c.object.DrawIfVisible(&c.clip,c.force,0);},&call)
     :draw_object_sprite(object,drawing,point,drawing.tactical_rect);
 if(!complete(result)){record_drawing_failure(result,"Object sprite DrawIt",&object);throw std::runtime_error(drawing_failure());}
}
void append_animation(MapWorld& world,AnimClass& a,ObjectClass* owner,CellClass* cell,Point2D point,
        const BytePalette* palette,int sort,bool cell_tint,bool pickable){
 append_original_sprite(world,a,&a,owner,cell,point,palette,sort,cell_tint,pickable);
}
void append_world_techno(MapWorld& world,TechnoClass& object,int pass=-1,bool forced=true,const RectangleStruct* clip=nullptr) {
 struct Sink {MapWorld& world;TechnoDrawing* drawing;TechnoPalette palette=TechnoPalette::house;} sink{world,nullptr};
 auto& w=*world.impl;TechnoDrawing drawing;sink.drawing=&drawing;
 drawing.context=&sink;drawing.types.backend_context=&sink;
 drawing.types.target=reinterpret_cast<DrawingTargetHandle*>(&sink);
 drawing.player=HouseClass::CurrentPlayer;drawing.frame=Unsorted::CurrentFrame;
 const auto& runtime=map_runtime();
 drawing.window_active=runtime.has_window&&runtime.has_window();
 drawing.debug_map=runtime.debug_map&&*runtime.debug_map;
 drawing.fog_of_war=ScenarioClass::Instance&&ScenarioClass::Instance->SpecialFlags.FogOfWar;
 drawing.fogged=[](void*,const CoordStruct& at) noexcept {return MapClass::Instance.IsLocationFogged(at);};
 drawing.selectable=[](void* p,ObjectClass& object,const Point2D& at) noexcept {
  static_cast<Sink*>(p)->world.impl->view.tactical.AddSelectable(&object,at.X,at.Y);
 };
 drawing.camera=building_voxel_camera();drawing.building_depth=w.building_zshape;
 drawing.tactical_rect=clip?*clip:RectangleStruct{0,0,TacticalClass::ViewBounds.Width,TacticalClass::ViewBounds.Height};
 drawing.level_height=Unsorted::LevelHeight;drawing.bridge_height=4*Unsorted::LevelHeight;
 if(auto* rules=RulesClass::Instance){drawing.extra_infantry_light=rules->ExtraInfantryLight;drawing.extra_unit_light=rules->ExtraUnitLight;drawing.extra_aircraft_light=rules->ExtraAircraftLight;
  const auto tint=[&](int index){return techno_color_mask(reinterpret_cast<const unsigned char*>(&rules->ColorAdd[index]),2);};
  drawing.laser_tint=tint(rules->LaserTargetColor);drawing.shield_tint=tint(rules->ForceShieldColor);drawing.berserk_tint=tint(rules->BerserkColor);drawing.iron_tint=tint(rules->IronCurtainColor);
 }
 if(auto* scenario=ScenarioClass::Instance)drawing.level_light=scenario->NormalLighting.Level;
 drawing.height=[](int z) noexcept {return TacticalClass::AdjustForZ(z);};
 drawing.cell_at=[](void*,const CellStruct& at) noexcept {return MapClass::Instance.TryGetCellAt(at);};
 drawing.cell_at_world=[](void*,const CoordStruct& at) noexcept {return MapClass::Instance.TryGetCellAt(at);};
 drawing.ground_height=[](void*,const CoordStruct& at) noexcept {return MapClass::Instance.GetCellFloorHeight(at);};
 drawing.project=[](void* p,const CoordStruct& at) noexcept {return project(at,static_cast<Sink*>(p)->world.impl->view);};
 drawing.shape_data=world_sprite_data;
 drawing.load_shape=[](void*,const char* name){return static_cast<SHPStruct*>(FileSystem::LoadFile(name,true));};
 drawing.initialize_light=[](void*,CellClass&) noexcept {return DrawingStatus::drawn;};
 drawing.shrouded=[](CellClass& cell) noexcept {return cell.IsShrouded();};
 drawing.palette=[](void* p,const TechnoClass& object,TechnoPalette kind,CellClass*,HouseClass* house,const DrawingPaletteHandle*& result) noexcept {
  try{auto& sink=*static_cast<Sink*>(p);auto& w=*sink.world.impl;sink.palette=kind;
   const BytePalette* palette=nullptr;
   switch(kind){
    case TechnoPalette::cell:palette=&FileSystem::ISOx_PAL;break;
    case TechnoPalette::animation:palette=&w.anim_palette;break;
    case TechnoPalette::normal:palette=&w.selection_palette;break;
    case TechnoPalette::house:case TechnoPalette::eight_bit:palette=world_palette(sink.world,w.unit_palette,house?house:object.Owner);break;
   }
   result=reinterpret_cast<const DrawingPaletteHandle*>(palette);return DrawingStatus::drawn;
  }catch(...){return DrawingStatus::backend_failure;}
 };
 drawing.types.backend.shape=[](void* p,const ShapeDrawingRequest& r) noexcept {
  try{if(auto* frame=tactical_drawing();frame&&frame->selection_only)return DrawingStatus::skipped;
   auto& sink=*static_cast<Sink*>(p);auto* object=const_cast<TechnoClass*>(sink.drawing->submitting_object);
   if(!object)return DrawingStatus::invalid_argument;
   auto* cell=MapClass::Instance.TryGetCellAt(object->Location);const bool shadow=(r.flags&1)!=0;
   bool pickable=object->GetTechnoType()->Selectable&&!shadow;
   if(object->WhatAmI()==AbstractType::Building&&r.image==static_cast<BuildingClass*>(object)->Type->BibShape)pickable=false;
   auto* sprite=append_world_sprite(sink.world,r.image,r.frame,object,cell,r.position,reinterpret_cast<const BytePalette*>(r.palette),object->GetYSort(),r.gradient==0,shadow,pickable);
   if(!sprite)return DrawingStatus::skipped;
   sprite->original_depth=true;sprite->flags=r.flags;sprite->gradient=r.gradient;sprite->depth_adjustment=r.depth_adjustment;
   sprite->depth_image=r.depth_image;sprite->depth_frame=r.depth_frame;sprite->depth_offset=r.depth_offset;sprite->intensity=r.intensity;sprite->tint=r.tint;
   // Buildings remain registered in the ground display list. A height change
   // does not resubmit them into another list during rendering.
   sprite->layer=object->WhatAmI()==AbstractType::Building?Layer::Ground:object->InWhichLayer();sprite->cell_tint=sink.palette==TechnoPalette::cell;sprite->color_scheme=sink.palette==TechnoPalette::house;
   return DrawingStatus::drawn;
  }catch(...){return DrawingStatus::backend_failure;}
 };
 drawing.voxel=[](void* p,const TechnoVoxelRequest& r) noexcept {
  try{if(auto* frame=tactical_drawing();frame&&frame->selection_only)return DrawingStatus::skipped;
   auto& sink=*static_cast<Sink*>(p);auto& w=*sink.world.impl;
   std::shared_ptr<VoxelSurface> voxel;
   const auto cached=cache_world_voxel(sink.world,r,voxel);
   if(cached!=DrawingStatus::drawn)return cached;
   WorldSprite sprite;sprite.owner=const_cast<TechnoClass*>(r.object);sprite.cell=MapClass::Instance.TryGetCellAt(r.object->Location);
   sprite.voxel=std::move(voxel);sprite.palette=reinterpret_cast<const BytePalette*>(r.palette);
   sprite.color_scheme=sink.palette==TechnoPalette::house;sprite.cell_tint=sink.palette==TechnoPalette::cell;
   sprite.position={r.position.X+sprite.voxel->offset.X,r.position.Y+sprite.voxel->offset.Y};
   sprite.layer=r.shadow?Layer::Ground:r.object->InWhichLayer();sprite.sort=r.object->GetYSort();sprite.depth=r.depth;
   sprite.shadow=r.shadow;sprite.flags=r.flags;sprite.intensity=r.intensity;sprite.tint=r.tint;
   sprite.pickable=r.object->GetTechnoType()->Selectable&&!r.shadow;w.sprites.push_back(std::move(sprite));return DrawingStatus::drawn;
  }catch(...){return DrawingStatus::backend_failure;}
 };
 drawing.animation=[](void* p,const TechnoClass& object,AnimClass& anim,const Point2D&,const RectangleStruct&) noexcept {
  try{auto& sink=*static_cast<Sink*>(p);auto& w=*sink.world.impl;auto& building=static_cast<const BuildingClass&>(object);
   const auto at=anim.GetRenderCoords();
   auto* cell=MapClass::Instance.TryGetCellAt(object.Location);const bool owner_palette=anim.Type->ShouldUseCellDrawer;
   auto* palette=owner_palette?(building.Type->TerrainPalette?&FileSystem::ISOx_PAL:world_palette(sink.world,w.unit_palette,object.Owner)):anim.Type->AltPalette?&w.unit_palette:&w.anim_palette;
   append_animation(sink.world,anim,const_cast<TechnoClass*>(&object),cell,project(at,w.view),palette,sort_at(anim,at),owner_palette,object.GetTechnoType()->Selectable);
   return DrawingStatus::drawn;
  }catch(...){return DrawingStatus::backend_failure;}
 };
 const auto at=project(object.GetRenderCoords(),w.view);
 DrawingStatus status;
 if(pass<0)status=draw_techno_object(object,drawing,at,drawing.tactical_rect);
 else {
  struct Call{TechnoClass& object;RectangleStruct clip;bool force;int pass;BuildingDrawing* building;} call{object,drawing.tactical_rect,forced,pass,nullptr};
  const auto invoke=[](void* p){auto& c=*static_cast<Call*>(p);c.object.DrawIfVisible(&c.clip,c.force,DWORD(c.pass));};
  if(object.WhatAmI()==AbstractType::Building){
   auto* cell=MapClass::Instance.TryGetCellAt(object.Location);
   BuildingDrawing parts{drawing.types,w.building_zshape,cell?std::bit_cast<short>(cell->Intensity_Normal):1000};call.building=&parts;
   status=with_techno_drawing(drawing,[](void* p){auto& c=*static_cast<Call*>(p);
    const auto result=with_building_drawing(*c.building,[](void* q){auto& c=*static_cast<Call*>(q);c.object.DrawIfVisible(&c.clip,c.force,DWORD(c.pass));},p);
    record_techno_drawing(*techno_drawing(),result);
   },&call);
  }else status=with_techno_drawing(drawing,invoke,&call);
 }
 if(!complete(status)){record_drawing_failure(status,"Techno DrawIt",&object);throw std::runtime_error(drawing_failure());}
}

}
namespace game {
DrawingStatus draw_tactical_object(ObjectClass& object,bool forced,const RectangleStruct& clip,bool upper) noexcept {
 auto* frame=tactical_drawing();if(!frame||!frame->world)return DrawingStatus::unavailable;
 try{
  auto& world=*frame->world;auto& w=*world.impl;
  if((object.AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None)
   append_world_techno(world,static_cast<TechnoClass&>(object),upper?1:0,forced,&clip);
  else if(object.WhatAmI()==AbstractType::Bullet){
   auto& bullet=static_cast<BulletClass&>(object);
   struct Sink{MapWorld& world;BulletClass& object;DrawingPaletteKind palette=DrawingPaletteKind::normal;}sink{world,bullet};
   TypeDrawingContext parts;parts.target=reinterpret_cast<DrawingTargetHandle*>(&sink);parts.backend_context=&sink;
   DrawingResources resources;resources.context=&sink;
   resources.palette=[](void* p,DrawingPaletteKind kind,int index,const DrawingPaletteHandle*& result) noexcept {
    auto& sink=*static_cast<Sink*>(p);sink.palette=kind;return resolve_world_drawing_palette(sink.world,kind,index,result);
   };
   parts.backend.shape=[](void* p,const ShapeDrawingRequest& r) noexcept {try{
    auto& sink=*static_cast<Sink*>(p);auto& object=sink.object;
    auto* s=append_world_sprite(sink.world,r.image,r.frame,&object,object.GetCell(),r.position,reinterpret_cast<const BytePalette*>(r.palette),object.GetYSort(),r.gradient==0,bool(r.flags&1));
    if(!s)return DrawingStatus::skipped;
    s->layer=object.InWhichLayer();s->original_depth=true;s->flags=r.flags;s->gradient=r.gradient;
    s->depth_adjustment=r.depth_adjustment;s->intensity=r.intensity;s->cell_tint=false;
    s->color_scheme=sink.palette==DrawingPaletteKind::color_scheme;return DrawingStatus::drawn;
   }catch(...){return DrawingStatus::backend_failure;}};
   struct Call{ObjectClass& object;RectangleStruct clip;bool forced;}call{object,clip,forced};
   return with_drawing_resources(resources,parts,[](void* p){auto& c=*static_cast<Call*>(p);c.object.DrawIfVisible(&c.clip,c.forced,0);},&call);
  }else if(object.WhatAmI()==AbstractType::Particle||object.WhatAmI()==AbstractType::ParticleSystem){
   if(!frame->submit_objects){WorldSprite s;s.owner=&object;s.draw_object=true;s.layer=object.InWhichLayer();s.sort=object.GetYSort();s.position=project(object.GetRenderCoords(),w.view);w.sprites.push_back(s);return DrawingStatus::drawn;}
   auto drawing=frame->drawing->types;
   if(object.WhatAmI()==AbstractType::Particle&&int(static_cast<ParticleClass&>(object).Type->BehavesLike)==1){
    const auto status=frame->drawing->shape_palette(drawing.backend_context,w.anim_palette,53,drawing.palette);
    if(!complete(status))return status;
   }
   struct Call{ObjectClass& object;RectangleStruct clip;bool forced;}call{object,clip,forced};
   return with_type_drawing(drawing,[](void* p){auto& c=*static_cast<Call*>(p);c.object.DrawIfVisible(&c.clip,c.forced,0);},&call);
  }else if(object.WhatAmI()==AbstractType::Anim){
   auto& anim=static_cast<AnimClass&>(object);auto* owner=anim.OwnerObject;
   auto* cell=MapClass::Instance.TryGetCellAt(anim.GetRenderCoords());
   const bool owner_palette=owner&&owner->WhatAmI()==AbstractType::Building&&anim.Type->ShouldUseCellDrawer;
   const bool parachute=owner&&owner->Parachute==&anim;
   const BytePalette* palette=parachute?world_palette(world,w.unit_palette,owner->GetOwningHouse()):&w.anim_palette;
   if(owner_palette)palette=static_cast<BuildingClass*>(owner)->Type->TerrainPalette?&FileSystem::ISOx_PAL:world_palette(world,w.unit_palette,owner->GetOwningHouse());
   append_original_sprite(world,anim,&anim,owner,cell,project(anim.GetRenderCoords(),w.view),palette,anim.GetYSort(),owner_palette,false,true,forced,&clip);
  }else {
   auto* cell=MapClass::Instance.TryGetCellAt(object.Location);
   append_original_sprite(world,object,nullptr,&object,cell,project(object.GetRenderCoords(),w.view),nullptr,object.GetYSort(),false,object.GetType()->Selectable,true,forced,&clip);
  }
  return DrawingStatus::drawn;
 }catch(...){return DrawingStatus::backend_failure;}
}
DrawingStatus dispatch_tactical_object(ObjectClass& object,TacticalObjectPass pass,Point2D* point,RectangleStruct* clip,bool forced) noexcept {
 auto* frame=tactical_drawing();if(!frame||!clip)return DrawingStatus::unavailable;
 try{
  // Without a host world, keep the original virtual calls available to direct
  // bindings and instruction probes; no parallel object list is constructed.
  if(!frame->world){
   if(pass==TacticalObjectPass::behind)object.DrawBehind(point,clip);
   else if(pass==TacticalObjectPass::extras)object.DrawExtras(point,clip);
   else if(pass==TacticalObjectPass::building_info)static_cast<BuildingClass&>(object).DrawInfoTipAndSpiedSelection(point,clip);
   else object.DrawIfVisible(clip,forced,pass==TacticalObjectPass::building_upper);
   return DrawingStatus::drawn;
  }
  auto& world=*frame->world;auto& w=*world.impl;
  if(frame->selection_only&&((object.AbstractFlags&AbstractFlags::Techno)==AbstractFlags::None
    ||(pass!=TacticalObjectPass::body&&pass!=TacticalObjectPass::building_upper)))return DrawingStatus::skipped;
  if(pass==TacticalObjectPass::body||pass==TacticalObjectPass::building_upper){
   const auto begin=w.sprites.size();
   const auto result=draw_tactical_object(object,forced,*clip,pass==TacticalObjectPass::building_upper);
   if(!complete(result))return record_drawing_failure(result,"DrawIfVisible",&object);
   if(frame->submit_objects)for(std::size_t n=begin;n<w.sprites.size();++n){
    ++frame->statistics->visited;
    const auto status=submit_world_sprite(w.sprites[n],*frame->drawing,frame->bounds,*frame->statistics);
    if(!complete(status))return status;
   }
   return result;
  }
  // Diagnostics collect body requests without owning a destination. Production
  // hooks submit now, between the same body calls as the original DrawObjects.
  if(!frame->submit_objects)return DrawingStatus::skipped;
  auto* extras=building_health_drawing();if(!extras)return DrawingStatus::unavailable;
  if(pass==TacticalObjectPass::behind)object.DrawBehind(point,clip);
  else if(pass==TacticalObjectPass::extras)object.DrawExtras(point,clip);
  else static_cast<BuildingClass&>(object).DrawInfoTipAndSpiedSelection(point,clip);
  return complete(extras->status)?extras->status:record_drawing_failure(extras->status,"Object drawing hook",&object);
 }catch(...){return DrawingStatus::backend_failure;}
}
}
void TacticalClass::DrawOverlays(const RectangleStruct& area){
 auto* frame=tactical_drawing();if(!frame||!frame->world)return;
 try{auto& world=*frame->world;auto& w=*world.impl;
 // CellClass owns all overlay decisions. This adapter only retains requests
 // and translates converter identities into the existing palette cache.
 struct OverlaySink {MapWorld& world;CellClass* cell=nullptr;OverlayPalette palette=OverlayPalette::cell;} sink{world};
 OverlayDrawing overlays;
 overlays.context=&sink;overlays.types.backend_context=&sink;
 overlays.types.target=reinterpret_cast<DrawingTargetHandle*>(&sink);
 overlays.frame=Unsorted::CurrentFrame;
 // Rebuilding a full target is a new redraw, even when simulation is paused.
 overlays.redraws=static_cast<unsigned char>(++MapClass::Instance.Redraws);
 overlays.tactical_rect=frame->bounds.Width>0?frame->bounds:TacticalClass::ViewBounds;
 std::copy(std::begin(w.overlay_slope_depth),std::end(w.overlay_slope_depth),overlays.slope_depth);
 overlays.initialize_light=[](void*,CellClass&) noexcept {
  // Native map loading already computes Cell's light/tint fields. GPU palette
  // conversion is deferred until draw_map_world; no fake LightConvert object.
  return DrawingStatus::drawn;
 };
 overlays.palette=[](void* p,CellClass& cell,OverlayPalette kind,const DrawingPaletteHandle*& result) noexcept {
  try {
   auto& sink=*static_cast<OverlaySink*>(p);sink.cell=&cell;sink.palette=kind;
   const BytePalette* palette=&FileSystem::ISOx_PAL;
   if(kind==OverlayPalette::theater)palette=&FileSystem::TEMPERAT_PAL;
   else if(kind==OverlayPalette::wall){
    if(!HouseClass::CurrentPlayer)return DrawingStatus::unavailable;
    palette=world_palette(sink.world,sink.world.impl->unit_palette,HouseClass::CurrentPlayer);
   }
   result=reinterpret_cast<const DrawingPaletteHandle*>(palette);return DrawingStatus::drawn;
  }catch(...){return DrawingStatus::backend_failure;}
 };
 overlays.types.backend.shape=[](void* p,const ShapeDrawingRequest& r) noexcept {
  try {
   auto& sink=*static_cast<OverlaySink*>(p);auto* cell=sink.cell;
   auto* s=append_world_sprite(sink.world,r.image,r.frame,nullptr,cell,r.position,
    reinterpret_cast<const BytePalette*>(r.palette),256*(cell->MapCoords.X+cell->MapCoords.Y)+256,
    r.gradient==0,(r.flags&1)!=0);
   if(!s)return DrawingStatus::skipped;
   s->original_depth=true;s->flags=r.flags;s->gradient=r.gradient;
   s->depth_adjustment=r.depth_adjustment;s->intensity=r.intensity;
   s->depth_image=r.depth_image;s->depth_offset=r.depth_offset;
   s->cell_tint=sink.palette==OverlayPalette::cell;
   s->color_scheme=sink.palette==OverlayPalette::wall;
   return DrawingStatus::drawn;
  }catch(...){return DrawingStatus::backend_failure;}
 };
 // YR 0x006D6D10 differs from OpenTS: base (-4,-2), last row
 // height/15+21 inclusive. Reverse rows, ascending columns; bodies then shadows.
 const auto bounds=overlays.tactical_rect,clip=Drawing::Intersect(bounds,area);
 auto at=ApplyMatrix_Pixel({TacticalPos.X+area.X-bounds.X,TacticalPos.Y+area.Y-bounds.Y});
 at.X=std::max(at.X,0);at.Y=std::max(at.Y,0);
 const CellStruct base{short(at.X/256-4),short(at.Y/256-2)};
 for(bool shadow:{false,true})for(int row=area.Height/15+21;row>=0;--row){
  CellStruct cellAt{short(base.X+row/2),short(base.Y+(row+1)/2)};
  for(int col=0;col<area.Width/60+4;++col,++cellAt.X,--cellAt.Y){
   if(!MapClass::Instance.CoordinatesLegal(cellAt))continue;
   auto* cell=MapClass::Instance.TryGetCellAt(cellAt);if(!cell||cell->OverlayTypeIndex==-1)continue;
   RectangleStruct rect;if(shadow)cell->ShapeRect(&rect);else cell->GetContainingRect(&rect);
   rect=Drawing::Intersect(rect,clip);if(rect.Width<=0||rect.Height<=0)continue;
   auto point=project(CoordStruct{cellAt.X*256,cellAt.Y*256,0},w.view);point.X-=30;
   const auto status=draw_cell_overlay(*cell,overlays,point,clip,shadow);
   if(!complete(status)){
    record_tactical_drawing(record_drawing_failure(status,shadow?"Cell DrawOverlayShadow":"Cell DrawOverlay"));return;
   }
  }
 }
 }catch(...){record_tactical_drawing(DrawingStatus::backend_failure);}
}
void TacticalClass::BuildBackgroundDrawRequests() noexcept {
 auto* frame=tactical_drawing();if(!frame||!frame->world)return;
 auto& world=*frame->world;auto& w=*world.impl;
 frame->status=DrawingStatus::skipped;frame->background_prepared=false;frame->background_requests=0;
 // No semantic frame cache: a failed collection cannot turn into a cache hit.
 w.sprite_revision=~std::uint64_t{};
 w.sprites.clear();w.missing_voxel_parts=w.invalid_voxel_parts=0;
 if(w.voxel_asset_revision!=voxel_resource_revision()){w.voxel_surfaces.clear();w.unit_voxel_surfaces.clear();w.voxel_asset_revision=voxel_resource_revision();}
 // 记忆雾已排除：本体地图不使用 FoggedObjectClass（原 0x004D1890）。
 // 仅保留原类头文件，不创建、遍历或提交快照；Render 入口拒绝启用该模式。
 const auto bounds=frame->bounds.Width>0?frame->bounds:TacticalClass::ViewBounds;
 DrawOverlays(bounds);if(!complete(frame->status))return;
 DrawTerrain(true,bounds,bounds);if(!complete(frame->status))return;
 DrawTileShadows(bounds,bounds);if(!complete(frame->status))return;
 DrawBuildings(true,bounds,bounds);if(!complete(frame->status))return;
 frame->background_requests=unsigned(w.sprites.size());frame->background_prepared=true;
}
void TacticalClass::BuildDrawRequests() noexcept {
 auto* frame=tactical_drawing();if(!frame||!frame->world)return;
 if(!frame->background_prepared)BuildBackgroundDrawRequests();
 if(!complete(frame->status))return;
 try{
  auto& world=*frame->world;auto& w=*world.impl;
  w.sprites.resize(frame->background_requests);
  SelectableCount=0;AddBuildingsToSelectables(frame->bounds.Width>0?frame->bounds:TacticalClass::ViewBounds);
  DrawObjects(true);
  if(complete(frame->status)){
   w.sprite_revision=world.presentation_revision;
   w.selectable_revision=world.presentation_revision;w.selectable_camera=TacticalPos;w.selectable_viewport={ViewBounds.Width,ViewBounds.Height};
  }else {SelectableCount=0;w.selectable_revision=~std::uint64_t{};}
 }catch(...){frame->status=DrawingStatus::backend_failure;SelectableCount=0;frame->world->impl->selectable_revision=~std::uint64_t{};}
}
void TacticalClass::BuildSelectableList() noexcept {
 auto* frame=tactical_drawing();if(!frame||!frame->world)return;
 auto& world=*frame->world;auto& w=*world.impl;
 if(w.selectable_revision==world.presentation_revision&&w.selectable_camera==TacticalPos
    &&w.selectable_viewport==Point2D{ViewBounds.Width,ViewBounds.Height})return;
 // Before the first draw, or after input changes the world, run the SAME
 // visibility/DrawIt registration path with request sinks disabled. Input never
 // invents a second candidate list from type arrays or uses deleted pointers.
 const bool previous=frame->selection_only;frame->selection_only=true;
 SelectableCount=0;AddBuildingsToSelectables(TacticalClass::ViewBounds);
 try{DrawObjects(true);
  if(complete(frame->status)){w.selectable_revision=world.presentation_revision;w.selectable_camera=TacticalPos;w.selectable_viewport={ViewBounds.Width,ViewBounds.Height};}
  else {SelectableCount=0;w.selectable_revision=~std::uint64_t{};}
 }catch(...){frame->status=DrawingStatus::backend_failure;SelectableCount=0;w.selectable_revision=~std::uint64_t{};}
 frame->selection_only=previous;
}
void TacticalClass::Render(DSurface* surface,bool force,int mode) {
 // 分层约定：恢复原版的对象选择、调用顺序、状态变化和像素规则；目标、
 // 资源缓存及提交由自有后端承接，不要求复刻 DDraw 的锁定/复制/滚屏机制。
 // 现代实时渲染通常复用 GPU 资源和静态绘制数据，筛选可见内容并有序批量重绘，
 // 利用 GPU 并行能力，减少屏幕滚移缓存的跨帧状态维护；背景像素缓存是否有收益
 // 应由性能测量决定。此处“整帧”仅指绘制范围，渲染频率、逻辑节拍和插值另行处理。
 // 下列“预留”只有职责定位，尚未执行；“过渡”保留当前行为，待逐段归位。
 // 编号描述本函数当前布局；标明的原版归属才是后续调度目标。
 // 原版顺序依据：0x006D3D10。

 // ===== 0. 帧入口与目标绑定 [宿主适配，已接入] ===========================
 // 借用当前目标、裁剪区域和统计；GPU 目标不伪装成 DSurface。
 auto* frame=tactical_drawing();
 if(!frame)return;
 // Ownership migration: the typed host currently requests the existing whole
 // frame path (mode 0x3). Original split/cached modes are not silently emulated.
 if(mode!=0x3){frame->status=DrawingStatus::unsupported;return;}
 if(!frame->drawing||!frame->statistics){frame->status=DrawingStatus::unavailable;return;}
 (void)surface;(void)force; // No fabricated DSurface is used for a GPU target.
 const auto& c=*frame->drawing;const auto& bounds=frame->bounds;auto& stats=*frame->statistics;
 try{frame->status=[&]() -> DrawingStatus {

 // ===== 1. 帧准备与绘制状态 [整帧模式已接入] ============================
 // 原 0x006D3D10 的选择候选每帧重建；绘制中的原 DrawIt 登记移动对象，原入口登记建筑。
 // 宿主已经清理整张目标，故背景总以 forced=true 执行；不能使用场景请求
 // 缓存跳过 NeedsRedraw 等原类副作用。SHP/VXL 资源缓存继续保留。
 if(bounds.Width<=0||bounds.Height<=0||!c.types.target)return DrawingStatus::unavailable;
 // 废弃的 FogOfWar 记忆模式不实现；与普通 Shroud 黑幕绘制分开。
 // 已核查的本体地图全部关闭此项，不为未支持模式调用头文件中的空实现。
 if(ScenarioClass::Instance&&ScenarioClass::Instance->SpecialFlags.FogOfWar)
  return record_drawing_failure(DrawingStatus::unsupported,"deprecated FogOfWar mode");
 if(frame->include_terrain)stats={};
 SelectableCount=0;frame->status=DrawingStatus::skipped;
 frame->background_prepared=false;frame->background_requests=0;

 // ===== 2. 背景阶段 [整帧路径已拆回，2.3 记忆雾明确不实现] =============
 // 2.1 深度/遮罩：原 0x006D2B60 → 0x006D3660。
 // WipeDepth 的整帧分支不做脏矩形擦除；目标在宿主帧入口清 Z / ABuffer。
 // 原版 ABuffer 顺序为 shroud → fog → AlphaShape；本体路径关闭记忆雾，
 // 此处只执行 shroud → AlphaShape，且必须先于任何地表消费者。
 if(frame->include_terrain){
  if(!MapClass::Instance.Cells.Items)return DrawingStatus::unavailable;
  DrawShroud(bounds);if(!complete(frame->status))return frame->status;

 // 2.2 TMP 地表：0x006D2DE0 → DrawTiles 0x006D7560 → Cell::DrawIt 0x00480350。
  DrawTiles(bounds,bounds);if(!complete(frame->status))return frame->status;
 }

 // 2.3 雾中记忆对象 [已废弃，不实现]：原 0x006D3470 → 0x004D1890。
 // 已核查的本体战役、合作和多人地图均为 FogOfWar=no；只保留
 // FoggedObjectClass 的 YRpp 空实现头文件，此处不调度、不预留迁移任务。
 // 2.4 覆盖物 → 静态 Terrain → 地表阴影 → 建筑本体。
 // 顺序及对象筛选在各原类方法中；本体走 DrawIfVisible(..., false)，
 // 建筑上层留在第 4 层。背景先提交；前景严格沿显示层顺序继续提交。
 if(frame->world&&frame->world->impl->loaded){
  BuildBackgroundDrawRequests();if(!complete(frame->status))return frame->status;
  for(auto& request:frame->world->impl->sprites){
   ++stats.visited;
   const auto status=submit_world_sprite(request,c,bounds,stats);
   if(!complete(status))return status;
  }
 }

 // ===== 3. 前景准备 [世界绑定已接入，原阶段预留] =========================
 if(!frame->world)return stats.drawn?DrawingStatus::drawn:DrawingStatus::skipped;
 auto& world=*frame->world;
 auto&w=*world.impl;
 if(!w.loaded)return frame->include_terrain&&stats.drawn?DrawingStatus::drawn:DrawingStatus::skipped;
 // 建筑选择登记 0x006D9CE0 在第 4 层入口执行。预留航路/集结点/放置标记后层（参数 false），
 // 以及原版位于 DrawObjects 之前的效果。先核对状态与依赖，再接入实际调用。
 // 背景复制到前景目标是后端适配点；整帧目标可复用同一目标继续绘制。

 // ===== 4. 场景对象调度与提交 [原五层遍历已接入] =======================
 // DrawObjects 0x006D8DB0 owns IsVisible, the body hooks, Ground building
 // information and the second Techno Extras pass. Flush each object's own
 // requests immediately; there is no type sweep or global fragment sort.
 // Resolve hook resources once per frame, outside the two object passes.
 if(!c.shape_palette)return record_drawing_failure(DrawingStatus::unavailable,"Tactical shape palette callback");
 BuildingHealthDrawing extras{c.types,w.health_pips};
 extras.wrench=w.repair_wrench;extras.selection_palette=&w.selection_palette;
 extras.camera=TacticalPos;extras.pip_border=w.pip_border;extras.mobile_pips=w.mobile_pips;
 auto resource_status=c.shape_palette(c.types.backend_context,w.selection_palette,1,extras.palette);
 if(!complete(resource_status))return resource_status;
 resource_status=c.shape_palette(c.types.backend_context,w.anim_palette,53,extras.wrench_palette);
 if(!complete(resource_status))return resource_status;
 frame->submit_objects=true;
 const auto object_status=with_building_health_drawing(extras,[](void* p){static_cast<TacticalClass*>(p)->BuildDrawRequests();},this);
 frame->submit_objects=false;
 if(!complete(frame->status))return frame->status;
 if(!complete(object_status))return object_status;

 // ===== 5. 前景效果 [部分已接入] ========================================
 // 5.1 灯光：保留当前 Spotlight/BuildingLight 调用，后续核对原调度的对应关系。
 // Air-layer spotlight effects run after world sprites, before selection/UI.
 struct LightPass{RectangleStruct bounds;};LightPass light_pass{bounds};
 auto light_status=with_type_drawing(c.types,[](void* p){auto&pass=*static_cast<LightPass*>(p);SpotlightClass::DrawAll();for(auto*light:BuildingLightClass::Array){Point2D point{};light->DrawIt(&point,&pass.bounds);}},&light_pass);
 if(!complete(light_status))return light_status;
 // 5.2 激光、电弧等效果 [预留于原对应位置]；拖尾绘制 [已接入]。
 // 原 LineTrail 0x00556D40 合并绘制/更新；当前 UpdateAll 在逻辑阶段，
 // 恢复调度时须核对次数，不能因一次宿主重绘再次推进模拟状态。
 const auto trail_status=with_type_drawing(c.types,[](void*){LineTrail::DrawAll();},nullptr);
 if(!complete(trail_status))return trail_status;

 // ===== 6. 悬停提示 [已归回 GScreenClass / CCToolTip] =================
 // 原 0x004F4570 在战场及界面完成后绘制；此处不再直接绘制建筑名称。

 // ===== 7. 范围标记、框选与战术标记前层 [部分已接入] =====================
 // 预留框选之前的原范围/信标等阶段；DrawRubberBand 0x006DA180 已接入。
 DrawingResources resources;resources.normal_palette=&w.selection_palette;resources.clip=bounds;
 const auto band_status=with_drawing_resources(resources,c.types,[](void* p){static_cast<TacticalClass*>(p)->DrawRubberBand();},&w.view.tactical);
 if(!complete(band_status))return band_status;
 // 预留：航路/集结点/放置标记的前层（参数 true），与第 3 层形成原先后关系。

 // ===== 8. 规划、行动线、控制连接与 Temporal [部分已接入] ================
 // 当前 helper 只覆盖原 Render 的 0x006D46DD..0x006D47F6 子集；
 // 完整规划、其他对象和 Temporal 分支仍预留，不能将此 helper 当作完整阶段。
 const auto action_status=with_type_drawing(c.types,[](void* p){static_cast<TacticalClass*>(p)->DrawActionLinesAndLinks();},&w.view.tactical);
 if(!complete(action_status))return action_status;

 // ===== 9. 画面收尾与战术文字 [预留] ====================================
 // 原规划收尾后调用 0x006D7840，其完整像素语义待校准，不预设为“地图外填黑”。
 // 随后计时器 0x006D4B50、屏幕文字 0x006D4E20 及原帧状态收尾待接回。
 // 原 Surface 恢复/解锁由宿主目标生命周期承接，保留所需的提交完成边界。

 // ===== 10. 宿主结果与异常边界 [已接入] =================================
 // DrawingStatus/统计属于内部适配，不改变原 Render 的公开签名；异常在本侧收束。
 // The whole-frame entry reports submitted tiles as well as objects. Keep
 // the object-only adapter's existing result contract for private callers.
 if(frame->include_terrain)return stats.drawn?DrawingStatus::drawn:DrawingStatus::skipped;
 return w.sprites.empty()?DrawingStatus::skipped:DrawingStatus::drawn;
 }();
 if(complete(frame->status)){LastTacticalPos=TacticalPos;VisibleCellCount=0;field_D7C=false;Redrawing=false;}
 }catch(...){frame->status=DrawingStatus::backend_failure;}
}
