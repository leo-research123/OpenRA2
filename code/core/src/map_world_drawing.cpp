// Host resource lookup, request storage and submission. Scene traversal and
// rendering decisions live in yrpp/TacticalClassRender.cpp.
#include "map_world_internal.hpp"
#include "techno_drawing.hpp"
#include "type_drawing.hpp"
#include "map_view.hpp"
#include "tactical_drawing.hpp"
#include "yrpp/HouseClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/ColorScheme.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace game {
namespace {
thread_local TacticalDrawingFrame* active_frame=nullptr;
bool complete(DrawingStatus s){return s==DrawingStatus::drawn||s==DrawingStatus::skipped;}
}
DrawingStatus resolve_world_drawing_palette(MapWorld& world,DrawingPaletteKind kind,int index,
        const DrawingPaletteHandle*& result) noexcept {
 try{
  auto& w=*world.impl;const BytePalette* palette=nullptr;
  switch(kind){
  case DrawingPaletteKind::normal:palette=&w.selection_palette;break;
  case DrawingPaletteKind::animation:palette=&w.anim_palette;break;
  case DrawingPaletteKind::color_scheme:
   // This host owns palettes through its house remap cache. Resolve the exact
   // requested scheme index; never substitute the current player's scheme.
   for(auto* house:HouseClass::Array)if(house->ColorSchemeIndex==index){palette=world_palette(world,w.unit_palette,house);break;}
   break;
  }
  result=reinterpret_cast<const DrawingPaletteHandle*>(palette);
  return palette?DrawingStatus::drawn:DrawingStatus::unavailable;
 }catch(...){return DrawingStatus::backend_failure;}
}
SHPStruct* world_sprite_data(SHPStruct*image){
 // GetData alone uses one shared scratch buffer (0x0069E580). Resolving the
 // next reference evicts it. Drawing needs the reference-owned allocation,
 // as in the original GetPixels path (0x0069E740), including failed-load null.
 if(!image)return nullptr;
 if(auto*reference=image->AsReference()){reference->Load();return reference->Data;}
 return image;
}
WorldSprite* append_world_sprite(MapWorld&world,SHPStruct*image,int frame,ObjectClass*owner,CellClass*cell,Point2D point,const BytePalette*palette,int sort,bool flat,bool shadow,bool pickable){
 auto*shape=world_sprite_data(image);if(!shape||frame<0||frame>=shape->Frames)return nullptr;auto b=shape->GetFrameBounds(frame);auto&clip=TacticalClass::ViewBounds;
 if(point.X-shape->Width/2+b.X+b.Width<0||point.Y-shape->Height/2+b.Y+b.Height<0||point.X-shape->Width/2+b.X>=clip.Width||point.Y-shape->Height/2+b.Y>=clip.Height)return nullptr;
 WorldSprite s;s.image=image;s.frame=frame;s.owner=owner;s.cell=cell;s.position=point;s.palette=palette;s.sort=sort;
 // Legacy request depth for non-overlay callers; calibrated original paths
 // override this with their scanline depth contract.
 s.depth=std::clamp(0x8000-(point.Y+(cell?15*static_cast<signed char>(cell->Level):0)+15),0,65535);
 s.flat=flat;s.shadow=shadow;s.pickable=pickable;s.intensity=cell?cell->Intensity_Normal:1000;world.impl->sprites.push_back(s);return &world.impl->sprites.back();
}
namespace {
std::array<std::uint32_t,14> voxel_state(const BuildingVoxelPart&part){
 std::array<std::uint32_t,14> state{};const auto&local=part.local;
 for(unsigned row=0;row<3;++row)for(unsigned col=0;col<4;++col)
  state[row*4+col]=std::bit_cast<std::uint32_t>(local.row[row][col]);
 state[12]=part.frame;state[13]=unsigned(part.use_buffer)|(unsigned(part.shadow)<<1)|(unsigned(part.half_shadow)<<2)|(unsigned(part.camera_applied)<<3)|(unsigned(part.shadow_layer)<<8);
 return state;
}
}
DrawingStatus cache_world_voxel(MapWorld& world,const TechnoVoxelRequest& r,std::shared_ptr<VoxelSurface>& voxel) noexcept {try{
 auto& w=*world.impl;
   BuildingVoxelPart part;part.resource=r.resource;part.local=r.local_matrix?*r.local_matrix:r.matrix;part.frame=unsigned(r.frame);
   part.cache_key=r.cache_key;part.shadow=r.shadow;part.shadow_layer=r.shadow_layer;part.half_shadow=r.half_shadow;part.use_buffer=r.use_buffer;part.camera_applied=!r.local_matrix;
   const auto original_key=std::make_pair(part.resource,part.cache_key);
   if(part.cache_key!=-1&&!part.shadow){auto found=w.unit_voxel_surfaces.find(original_key);if(found!=w.unit_voxel_surfaces.end())voxel=found->second;}
   if(!voxel){const auto key=std::make_pair(part.resource,voxel_state(part));auto found=w.voxel_surfaces.find(key);
    if(found==w.voxel_surfaces.end()){
     auto surface=std::make_shared<VoxelSurface>();const auto status=render_building_voxel(part,w.voxel_palette,*surface);
     if(status!=DrawingStatus::drawn){if(status==DrawingStatus::unavailable)++w.missing_voxel_parts;else if(status!=DrawingStatus::skipped)++w.invalid_voxel_parts;return DrawingStatus::skipped;}
     if(w.voxel_surfaces.size()>=512)w.voxel_surfaces.clear();found=w.voxel_surfaces.emplace(key,std::move(surface)).first;
    }
    voxel=found->second;if(part.cache_key!=-1&&!part.shadow)w.unit_voxel_surfaces.emplace(original_key,voxel);
   }
 return DrawingStatus::drawn;
}catch(...){return DrawingStatus::backend_failure;}}
const BytePalette*world_palette(MapWorld&world,const BytePalette&source,HouseClass*owner){
 if(!owner)return &source;
 auto&cache=world.impl->remaps;const auto hsv=world_house_hsv(world,*owner);
 const auto color=(std::uint32_t(hsv.R)<<16)|(std::uint32_t(hsv.G)<<8)|hsv.B;
 const auto key=std::make_pair(&source,color);auto it=cache.find(key);
 if(it!=cache.end())return it->second.get();
 auto palette=std::make_unique<BytePalette>();ColorScheme::BuildPalette(hsv,source,*palette);
 auto*out=palette.get();cache.emplace(key,std::move(palette));return out;
}
DrawingStatus submit_world_sprite(WorldSprite& s,const MapDrawingContext& c,const RectangleStruct& bounds,MapDrawStatistics& stats) noexcept {try{
 const DrawingPaletteHandle* pal=nullptr;DrawingStatus status;
 if(!s.palette)return DrawingStatus::unavailable;
 if(s.color_scheme){if(!c.color_scheme_palette)return DrawingStatus::unavailable;status=c.color_scheme_palette(c.types.backend_context,*s.palette,53,pal);}
 else if(s.cell_tint){if(!c.terrain_palette)return DrawingStatus::unavailable;int r=1000,g=1000,b=1000,shades=53;if(s.cell){r=s.cell->Color2_Red;g=s.cell->Color2_Green;b=s.cell->Color2_Blue;shades=LightConvertClass::PrepareCellTint(r,g,b,c.lighting_quality);}status=c.terrain_palette(c.types.backend_context,*s.palette,r,g,b,shades,pal);}
 else{if(!c.shape_palette)return DrawingStatus::unavailable;status=c.shape_palette(c.types.backend_context,*s.palette,53,pal);}
 if(!complete(status))return status;
 if(s.voxel){IndexedDrawingRequest r;r.target=c.types.target;r.palette=pal;r.position={s.position.X+bounds.X,s.position.Y+bounds.Y};r.clip=bounds;r.width=s.voxel->width;r.height=s.voxel->height;r.pixels=s.voxel->pixels.data();r.pixel_count=std::uint32_t(s.voxel->pixels.size());r.absolute_depth=s.depth;r.depth_mode=ShapeDepthMode::legacy;r.intensity=s.intensity;r.shadow=s.shadow;r.flags=s.flags;r.tint=s.tint;status=submit_type_indexed(c.types,r);if(!complete(status))return record_drawing_failure(status,"Voxel submit",s.owner);if(status==DrawingStatus::drawn)++stats.drawn;else ++stats.skipped;return status;}
 ShapeDrawingRequest req;req.target=c.types.target;req.palette=pal;req.image=s.image;req.frame=s.frame;req.position=s.position;req.clip=bounds;req.flags=s.flags;req.intensity=s.intensity;req.tint=s.tint;req.depth_mode=s.flat?ShapeDepthMode::read:ShapeDepthMode::read_write;req.absolute_depth=s.depth;req.blend_mode=s.shadow?ShapeBlendMode::shadow:ShapeBlendMode::palette;if(s.original_depth){req.depth_mode=ShapeDepthMode::legacy;req.blend_mode=ShapeBlendMode::palette;req.depth_adjustment=s.depth_adjustment;req.gradient=s.gradient;req.depth_image=s.depth_image;req.depth_frame=s.depth_frame;req.depth_offset=s.depth_offset;}status=submit_type_shape(c.types,req);if(!complete(status))return record_drawing_failure(status,"World SHP submit",s.owner,&req);if(status==DrawingStatus::drawn)++stats.drawn;else ++stats.skipped;
 return status;
}catch(...){return DrawingStatus::backend_failure;}}

TacticalDrawingFrame* tactical_drawing() noexcept{return active_frame;}
DrawingStatus with_tactical_drawing(TacticalDrawingFrame& frame,void(*call)(void*),void* argument) noexcept {
 auto* previous=active_frame;active_frame=&frame;
 struct Restore{TacticalDrawingFrame* previous;~Restore(){active_frame=previous;}}restore{previous};
 try{if(call)call(argument);else frame.status=DrawingStatus::invalid_argument;}
 catch(...){frame.status=DrawingStatus::backend_failure;}
 return frame.status;
}
DrawingStatus draw_lighting_shape(SHPStruct* source,int index,const Point2D& point,
 const RectangleStruct& clip,RasterBlendMode mode) noexcept {
 auto* frame=active_frame;if(!frame||!frame->drawing)return DrawingStatus::unavailable;
 return submit_type_lighting_shape(frame->drawing->types,{frame->drawing->types.target,source,index,point,clip,mode});
}
void record_tactical_drawing(DrawingStatus status) noexcept {
 if(auto* frame=active_frame;frame&&complete(frame->status)&&status!=DrawingStatus::skipped)frame->status=status;
}
DrawingStatus draw_cell_terrain(CellClass& cell,const MapDrawingContext& drawing,const Point2D& at,
 const RectangleStruct& bounds) noexcept {
 TacticalDrawingFrame frame;frame.drawing=&drawing;frame.bounds=bounds;
 struct Call{CellClass& cell;const Point2D& at;const RectangleStruct& bounds;}call{cell,at,bounds};
 return with_tactical_drawing(frame,[](void* p){auto& c=*static_cast<Call*>(p);c.cell.DrawIt(c.at,c.bounds,false);},&call);
}
DrawingStatus draw_tactical_view(TacticalClass& tactical,MapWorld* world,const MapDrawingContext& drawing,
 const RectangleStruct& bounds,MapDrawStatistics& stats) noexcept {
 TacticalDrawingFrame frame{world,&drawing,bounds,&stats};
 return with_tactical_drawing(frame,[](void* p){static_cast<TacticalClass*>(p)->Render(nullptr,true,0x3);},&tactical);
}
DrawingStatus draw_map_world(MapWorld& world,const MapDrawingContext& drawing,const RectangleStruct& bounds,MapDrawStatistics& stats) noexcept {
 TacticalDrawingFrame frame{&world,&drawing,bounds,&stats};frame.include_terrain=false;
 return with_tactical_drawing(frame,[](void* p){static_cast<TacticalClass*>(p)->Render(nullptr,true,0x3);},&world.impl->view.tactical);
}
void rebuild_world_sprites(MapWorld& world){
 TacticalDrawingFrame frame;frame.world=&world;
 const auto status=with_tactical_drawing(frame,[](void* p){static_cast<TacticalClass*>(p)->BuildDrawRequests();},&world.impl->view.tactical);
 if(!complete(status))throw std::runtime_error(drawing_failure()[0]?drawing_failure():"Tactical draw-request collection failed");
}
void rebuild_world_selectables(MapWorld& world){
 TacticalDrawingFrame frame;frame.world=&world;
 const auto status=with_tactical_drawing(frame,[](void* p){static_cast<TacticalClass*>(p)->BuildSelectableList();},&world.impl->view.tactical);
 if(!complete(status))throw std::runtime_error(drawing_failure()[0]?drawing_failure():"Tactical selectable collection failed");
}
}
