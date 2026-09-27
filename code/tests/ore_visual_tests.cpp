#include "support/test_support.hpp"
#include "map_world_internal.hpp"
#include "map_view.hpp"
#include "api/map_view.hpp"
#include "api/filesystem.hpp"
#include "api/software_type_drawing.hpp"
#include "yrpp/InfantryClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Surface.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/DrawingBuffers.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>

namespace {
struct OreScene {
 game::ResourceHandle* resources=nullptr;
 game::MapViewHandle* view=nullptr;
 ~OreScene(){game::destroy_map_view(view);FileSystem::ClearNameCache();Destroy_All_Shapes();Unload_All_Shapes();game::destroy_resources(resources);}
};
void save_pixels(const char* directory,const char* name,const std::vector<WORD>& pixels,int size){
 if(!directory)return;
 std::filesystem::create_directories(directory);
 std::ofstream file(std::filesystem::path(directory)/name,std::ios::binary);
 file<<"P6\n"<<size<<" "<<size<<"\n255\n";
 for(auto pixel:pixels){const char rgb[]{char(((pixel>>11)&31)*255/31),char(((pixel>>5)&63)*255/63),char((pixel&31)*255/31)};file.write(rgb,3);}
}
}

TEST(OreVisual, OriginalBackgroundPassBeforeWalkingInfantry){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 OreScene scene;std::string error;
 ASSERT_TRUE(game::create_resources(data,scene.resources,error))<<error;
 ASSERT_EQ(game::load_resources(*scene.resources,{},error),game::ResourceLoadResult::complete)<<error;
 ASSERT_TRUE(game::create_map_view(*scene.resources,scene.view));
 ASSERT_TRUE(game::load_map_view(*scene.view,"ALL01UMD.MAP",12));
 ASSERT_TRUE(game::set_map_viewport(*scene.view,640,480));
 const bool complete=game::with_map_view(*scene.view,[](void* context){
  auto& view=*static_cast<game::MapViewHandle*>(context);auto& world=*view.world;
  auto* cell=MapClass::Instance.GetCellAt(CellStruct{106,117});
  ASSERT_EQ(cell->OverlayTypeIndex,102);ASSERT_EQ(cell->SlopeIndex,0);
  InfantryClass* actor=nullptr;for(auto* p:InfantryClass::Array)if(!std::strcmp(p->Type->ID,"E1")){actor=p;break;}
  ASSERT_NE(actor,nullptr);game::detach_map_object(*actor);
  const CoordStruct center{106*256+128,117*256+128,static_cast<signed char>(cell->Level)*104};
  const auto screen=TacticalClass::CoordsToScreen(center);view.tactical.TacticalPos={screen.X-320,screen.Y-240};
  actor->SequenceAnim=Sequence::Walk;actor->IsSelected=false;
  constexpr int size=96;BSurface target(size,size,2);ABuffer light({0,0,size,size});ZBuffer depth({0,0,size,size});
  auto* oldLight=ABuffer::Instance;auto* oldDepth=ZBuffer::Instance;
  ABuffer::Instance=&light;ZBuffer::Instance=&depth;
  auto restore=ra2::test::scope_exit([&]{ABuffer::Instance=oldLight;ZBuffer::Instance=oldDepth;light.ReleaseSurface();depth.ReleaseSurface();});
  int gradients[5][6]{};std::ifstream table(RA2_ORE_Z_GRADIENTS);std::string comment;std::getline(table,comment);
  for(auto& row:gradients)for(auto& value:row)table>>value;ASSERT_TRUE(bool(table));
  const auto* oldGradients=Drawing::ZGradientTable;Drawing::ZGradientTable=gradients;
  auto restoreGradients=ra2::test::scope_exit([&]{Drawing::ZGradientTable=oldGradients;});
  std::map<const BytePalette*,std::unique_ptr<ConvertClass>> palettes;
  auto drawing=game::make_software_type_drawing(&target,nullptr,nullptr);
  const auto render=[&](const std::vector<const game::WorldSprite*>& sprites){
   target.Fill(0x39E7);depth.Fill(0xFFFF);light.Fill(127);
   for(const auto* s:sprites){
    auto& palette=palettes[s->palette];if(!palette)palette=std::make_unique<ConvertClass>(*s->palette,*s->palette,2,53,false);
    game::ShapeDrawingRequest r;r.target=drawing.target;r.palette=reinterpret_cast<const game::DrawingPaletteHandle*>(palette.get());
    r.image=s->image;r.frame=s->frame;r.position={s->position.X-320+size/2,s->position.Y-240+size/2};r.clip={0,0,size,size};
    r.flags=s->flags;r.intensity=s->intensity;r.depth_mode=game::ShapeDepthMode::legacy;
    r.gradient=s->gradient;r.depth_adjustment=s->depth_adjustment;
    const auto status=game::submit_type_shape(drawing,r);EXPECT_TRUE(status==game::DrawingStatus::drawn||status==game::DrawingStatus::skipped);
   }
   std::vector<WORD> pixels(size*size);const auto* bytes=static_cast<const unsigned char*>(target.Lock(0,0));
   for(int y=0;y<size;++y)std::memcpy(pixels.data()+y*size,bytes+y*target.GetPitch(),size*sizeof(WORD));target.Unlock();return pixels;
  };
  constexpr Point2D offsets[]{{128,128},{64,64},{192,64},{64,192},{192,192}};
  unsigned cases=0,differentCases=0,differentPixels=0;bool saved=false;
  for(int density:{0,5,11})for(const auto offset:offsets)for(int facing=0;facing<8;++facing)for(int frame:{0,3}){
   for(auto* resource:world.impl->resource_cells)resource->OverlayData=static_cast<BYTE>(density);
   if(actor->IsOnMap)game::detach_map_object(*actor);
   actor->Location={106*256+offset.X,117*256+offset.Y,center.Z};game::attach_map_object(*actor);
   actor->PrimaryFacing.SetCurrent(DirStruct(facing*0x2000));actor->Animation.Value=frame;
   game::map_object_changed();game::rebuild_world_sprites(world);
   std::vector<const game::WorldSprite*> actual,background,infantry;
   for(const auto& sprite:world.impl->sprites){
    const auto* overlay=sprite.cell?OverlayTypeClass::Array.GetItemOrDefault(sprite.cell->OverlayTypeIndex):nullptr;
    if(!sprite.owner&&overlay&&overlay->Tiberium){actual.push_back(&sprite);background.push_back(&sprite);}
    else if(sprite.owner==actor){actual.push_back(&sprite);infantry.push_back(&sprite);}
   }
   ASSERT_FALSE(background.empty());ASSERT_EQ(infantry.size(),2u);
   // Original Tactical.Render_Overlays (0x6D3290) runs before Draw_Objects
   // (0x6D8DB0). Compare actual pixels, keeping every original depth flag.
   auto expectedOrder=background;expectedOrder.insert(expectedOrder.end(),infantry.begin(),infantry.end());
   const auto expected=render(expectedOrder),observed=render(actual);
   unsigned mismatches=0;for(unsigned i=0;i<observed.size();++i)mismatches+=observed[i]!=expected[i];
   if(mismatches){++differentCases;differentPixels+=mismatches;}
   if(!saved&&(mismatches||offset==Point2D{64,64})){
    save_pixels(std::getenv("RA2_ORE_CAPTURE"),"actual.ppm",observed,size);
    save_pixels(std::getenv("RA2_ORE_CAPTURE"),"original-order.ppm",expected,size);saved=true;
   }
   ++cases;
  }
  std::cout<<"ORE_OCCLUSION cases="<<cases<<" different_cases="<<differentCases<<" different_pixels="<<differentPixels<<"\n";
  EXPECT_EQ(differentPixels,0u);EXPECT_EQ(cases,240u);
 },scene.view);
 ASSERT_TRUE(complete);
}
