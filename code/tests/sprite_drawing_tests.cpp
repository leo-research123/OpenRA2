#include "support/test_support.hpp"
#include "sprite_drawing.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/TacticalClass.h"
#include <array>
#include <bit>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
namespace {
struct Input { int hardware,depth,ring,fps,reduced,detail,detail_type,trans_detail,invisible,fogged,fog_remove,image,frame,start,end,flags,temporal,double_thick,translucent,translucency,level,veins,normal,cell_light,alternate,alt_palette,light,extras,building,has_building,airstrike,iron,shield,shrouded,pixel_format,tiled,flat,shadow,x,y,z,owner,owner_z,height,adjust,y_offset,count,frame_height,rate,timer,left,now,terrain_animated,crumbling,health,spawner,draw_shadows,normal_light,terrain_light,tint_light; };
std::istream& operator>>(std::istream& in,Input& c){return in>>c.hardware>>c.depth>>c.ring>>c.fps>>c.reduced>>c.detail>>c.detail_type>>c.trans_detail>>c.invisible>>c.fogged>>c.fog_remove>>c.image>>c.frame>>c.start>>c.end>>c.flags>>c.temporal>>c.double_thick>>c.translucent>>c.translucency>>c.level>>c.veins>>c.normal>>c.cell_light>>c.alternate>>c.alt_palette>>c.light>>c.extras>>c.building>>c.has_building>>c.airstrike>>c.iron>>c.shield>>c.shrouded>>c.pixel_format>>c.tiled>>c.flat>>c.shadow>>c.x>>c.y>>c.z>>c.owner>>c.owner_z>>c.height>>c.adjust>>c.y_offset>>c.count>>c.frame_height>>c.rate>>c.timer>>c.left>>c.now>>c.terrain_animated>>c.crumbling>>c.health>>c.spawner>>c.draw_shadows>>c.normal_light>>c.terrain_light>>c.tint_light;}
struct ObservedAnim : AnimClass {
 bool missing=false;int height=0;
 explicit ObservedAnim(AnimTypeClass* t):AnimClass(t,{2176,1920,104}){}
 SHPStruct* GetImage() const override {return missing?nullptr:Type->Image;}
 int GetHeight() const override {return height;}
 CoordStruct* GetCoords(CoordStruct* p) const override {*p=Location;return p;}
 CoordStruct* GetRenderCoords(CoordStruct* p) const override {*p=Location;return p;}
};
struct Terrain : TerrainClass {
 bool missing=false;
 explicit Terrain(TerrainTypeClass* t):TerrainClass(t,{8,7}){}
 SHPStruct* GetImage() const override {return missing?nullptr:Type->Image;}
 CellStruct* GetMapCoords(CellStruct* p) const override {*p={8,7};return p;}
};
struct Building : BuildingClass {
 bool iron=false;
 explicit Building(BuildingTypeClass* t):BuildingClass(t,nullptr){}
 bool IsIronCurtained() const override {return iron;}
};
struct Fixture {
 SHPStruct shape{};
 AnimTypeClass anim_type{"SPRITEDRAW"};
 TerrainTypeClass terrain_type{"SPRITETERRAIN"};
 BuildingTypeClass building_type{"SPRITEBUILDING",BuildingTypeClass::ConstructionDefaults{}};
 ObservedAnim anim{&anim_type};Terrain terrain{&terrain_type};Building building{&building_type};
 std::unique_ptr<CellClass,GameDeleter> cell{CellClass::Create()};
 game::SpriteDrawing drawing;
 std::vector<game::ShapeDrawingRequest> requests;
 std::vector<game::SpriteTriangle> triangles;
 Input input{};int token=0;
 unsigned char laser[3]{21,43,17},shield[3]{5,32,28};
 Fixture(){
  anim_type.Image=terrain_type.Image=&shape;
  drawing.context=this;drawing.types.backend_context=this;drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(this);
  drawing.cell_at=[](void* p,const CellStruct&) noexcept {return static_cast<Fixture*>(p)->cell.get();};
  drawing.cell_at_world=[](void* p,const CoordStruct&) noexcept {return static_cast<Fixture*>(p)->cell.get();};
  drawing.initialize_light=[](void* p,CellClass& c) noexcept {auto& f=*static_cast<Fixture*>(p);c.LightConvert=reinterpret_cast<LightConvertClass*>(&f.token);c.Intensity_Normal=1101;c.Intensity_Terrain=1202;return game::DrawingStatus::drawn;};
  drawing.palette=[](void*,game::SpritePalette p,const AnimClass*,CellClass*,const game::DrawingPaletteHandle*& result) noexcept {
   constexpr unsigned ids[]{5,1,2,3,4,6};result=reinterpret_cast<const game::DrawingPaletteHandle*>(std::uintptr_t(ids[int(p)]));return game::DrawingStatus::drawn;
  };
  drawing.shape_data=[](SHPStruct* p){return p;};
  drawing.frame_bounds=[](SHPStruct*,int){const auto* d=game::sprite_drawing();auto& f=*static_cast<Fixture*>(d->context);return RectangleStruct{1,2,30,f.input.frame_height};};
  drawing.height=[](int z) noexcept {return TacticalClass::AdjustForZ(z);};
  drawing.reduce_effects=[](void* p) noexcept {auto& f=*static_cast<Fixture*>(p);return unsigned(f.input.fps)<unsigned(f.input.reduced?(f.input.fps>=20?15:20):(f.input.fps<15?20:15));};
  drawing.building=[](CellClass&) noexcept -> BuildingClass* {auto& f=*static_cast<Fixture*>(game::sprite_drawing()->context);return f.input.has_building?&f.building:nullptr;};
  drawing.shrouded=[](CellClass&) noexcept {return static_cast<Fixture*>(game::sprite_drawing()->context)->input.shrouded!=0;};
  drawing.pixel_format=[]() noexcept {return static_cast<Fixture*>(game::sprite_drawing()->context)->input.pixel_format;};
  drawing.is_ring=[](const AnimClass&) noexcept {return static_cast<Fixture*>(game::sprite_drawing()->context)->input.ring!=0;};
  drawing.triangle=[](void* p,const game::SpriteTriangle& t) noexcept {try{static_cast<Fixture*>(p)->triangles.push_back(t);return game::DrawingStatus::drawn;}catch(...){return game::DrawingStatus::backend_failure;}};
  drawing.laser_color=laser;drawing.shield_color=shield;
  drawing.types.backend.shape=[](void* p,const game::ShapeDrawingRequest& r){static_cast<Fixture*>(p)->requests.push_back(r);return game::DrawingStatus::drawn;};
 }
 ~Fixture(){anim.OwnerObject=nullptr;anim.LightConvert=nullptr;building.Airstrike=nullptr;cell->LightConvert=nullptr;anim_type.Image=terrain_type.Image=nullptr;}
 void setup(const Input& c){
  input=c;requests.clear();triangles.clear();
  drawing.hardware=c.hardware;drawing.depth_available=c.depth;drawing.depth_origin=0x8007;drawing.frame=c.now;drawing.detail_level=c.detail;drawing.draw_shadows=c.draw_shadows;drawing.tactical_rect={5,7,1280,720};
  anim.missing=terrain.missing=!c.image;anim.height=c.height;shape.Frames=short(c.count);
  anim.Location={2176,1920,c.z};terrain.Location.Z=c.z;building.Location.Z=c.owner_z;anim.OwnerObject=c.owner?&building:nullptr;
  anim.Animation.Value=terrain.Animation.Value=c.frame;anim.Animation.Rate=c.rate;anim.Animation.Timer.StartTime=c.timer;anim.Animation.Timer.TimeLeft=c.left;
  anim.TintColor=c.tint_light;anim.ZAdjust=c.adjust;anim.AnimFlags=BlitterFlags(c.flags);
  anim.IsBuildingAnim=c.building;anim.UnderTemporal=c.temporal;anim.TranslucencyLevel=byte(c.level);anim.HasExtras=c.extras;anim.UseCellLightConvert=c.cell_light;anim.IsFogged=c.fogged;anim.Invisible=c.invisible;
  anim.LightConvert=c.alternate?reinterpret_cast<LightConvertClass*>(&token):nullptr;
  anim_type.Start=c.start;anim_type.End=c.end;anim_type.DetailLevel=c.detail_type;anim_type.TranslucencyDetailLevel=c.trans_detail;anim_type.Translucency=c.translucency;anim_type.YDrawOffset=c.y_offset;
  anim_type.IsVeins=c.veins;anim_type.Tiled=c.tiled;anim_type.UseNormalLight=c.normal;anim_type.AltPalette=c.alt_palette;anim_type.DoubleThick=c.double_thick;anim_type.Flat=c.flat;anim_type.Translucent=c.translucent;anim_type.Shadow=c.shadow;anim_type.ShouldFogRemove=c.fog_remove;
  terrain_type.IsAnimated=c.terrain_animated;terrain_type.SpawnsTiberium=c.spawner;terrain.IsCrumbling=c.crumbling;terrain.Health=c.health;
  cell->LightConvert=c.light?reinterpret_cast<LightConvertClass*>(&token):nullptr;cell->Intensity_Normal=WORD(c.normal_light);cell->Intensity_Terrain=WORD(c.terrain_light);
  building.Airstrike=c.airstrike?reinterpret_cast<AirstrikeClass*>(&token):nullptr;building.iron=c.iron;building.ForceShielded=c.shield;
 }
};
std::string hex(const std::vector<game::SpriteTriangle>& triangles){
 if(triangles.empty())return "-";
 constexpr char digits[]="0123456789abcdef";std::string out;
 const auto* bytes=reinterpret_cast<const unsigned char*>(triangles.data());
 for(std::size_t i=0;i<triangles.size()*sizeof(game::SpriteTriangle);++i){out+=digits[bytes[i]>>4];out+=digits[bytes[i]&15];}return out;
}
}
TEST(SpriteDrawing, OriginalInstructionRequestsAndTriangles){
 Fixture f;std::ifstream input(RA2_SPRITE_DRAW_FIXTURE);std::string tag,fields;
 std::getline(input,tag);ASSERT_EQ(tag,"SPRITE_DRAW_V1");std::getline(input,fields);int cases=0;
 while(input>>tag){
  ASSERT_EQ(tag,"CASE");std::string name,triangles;int terrain,count;Input c{};input>>name>>terrain>>c;
  SCOPED_TRACE(name);input>>tag>>count>>triangles;ASSERT_EQ(tag,"RESULT");ASSERT_TRUE(input);
  f.setup(c);const ObjectClass& object=terrain?static_cast<const ObjectClass&>(f.terrain):f.anim;
  const auto status=game::draw_object_sprite(object,f.drawing,{c.x,c.y},{11,13,400,300});
  ASSERT_TRUE(game::sprite_drawing_complete(status));ASSERT_EQ(f.requests.size(),std::size_t(count));
  for(const auto& r:f.requests){
   std::array<long long,18> expected{};input>>tag;ASSERT_EQ(tag,"DRAW");for(auto& v:expected)input>>v;
   const std::array<long long,18> actual{static_cast<long long>(reinterpret_cast<std::uintptr_t>(r.palette)),r.frame,r.position.X,r.position.Y,r.clip.X,r.clip.Y,r.clip.Width,r.clip.Height,r.flags,0,r.depth_adjustment,r.gradient,r.intensity,r.tint&0xFFFF,0,0,0,0};
   EXPECT_EQ(actual,expected);EXPECT_EQ(r.image,&f.shape);EXPECT_EQ(r.target,f.drawing.types.target);
  }
  EXPECT_EQ(hex(f.triangles),triangles);EXPECT_EQ(game::sprite_drawing(),nullptr);++cases;
 }
 EXPECT_TRUE(input.eof());EXPECT_EQ(cases,1690);
}
TEST(SpriteDrawing, FailureRestoresScopeAndStopsFollowingPasses){
 Fixture f;Input c{};c.image=1;c.light=1;c.detail=2;c.count=36;c.shadow=1;c.end=20;f.setup(c);
 f.drawing.types.backend.shape=[](void*,const game::ShapeDrawingRequest&)->game::DrawingStatus{throw 7;};
 EXPECT_EQ(game::draw_object_sprite(f.anim,f.drawing,{50,50},{0,0,100,100}),game::DrawingStatus::backend_failure);
 EXPECT_EQ(game::sprite_drawing(),nullptr);
 f.drawing.types.backend.shape=[](void*,const game::ShapeDrawingRequest&){return game::DrawingStatus::backend_failure;};
 EXPECT_EQ(game::draw_object_sprite(f.terrain,f.drawing,{50,50},{0,0,100,100}),game::DrawingStatus::backend_failure);
 EXPECT_EQ(game::sprite_drawing(),nullptr);
}
