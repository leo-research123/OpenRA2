#include "support/test_support.hpp"
#include "techno_drawing.hpp"
#include "yrpp/UnitClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/TacticalClass.h"
#include <array>
#include <fstream>
#include <memory>
#include <vector>
namespace {
struct Input {
 int kind,visual,height,z,z_adjust,gradient,write_depth,intensity,tint,excluded,warp,double_thick,
 terrain,composite,small,large,no_shadow,draw_shadows,clear,disguise,carry,shrouded,light,
 normal_light,terrain_light,foot,iron,iron_stage,airstrike,air_stage,cloak_progress,frame;
};
std::istream& operator>>(std::istream& in,Input& c){
 return in>>c.kind>>c.visual>>c.height>>c.z>>c.z_adjust>>c.gradient>>c.write_depth>>c.intensity>>c.tint
 >>c.excluded>>c.warp>>c.double_thick>>c.terrain>>c.composite>>c.small>>c.large>>c.no_shadow
 >>c.draw_shadows>>c.clear>>c.disguise>>c.carry>>c.shrouded>>c.light>>c.normal_light>>c.terrain_light
 >>c.foot>>c.iron>>c.iron_stage>>c.airstrike>>c.air_stage>>c.cloak_progress>>c.frame;
}
struct Disguise:AnimTypeClass {int kind=16;Disguise():AnimTypeClass("TECHNODISGUISE"){}AbstractType WhatAmI() const override{return AbstractType(kind);}};
template<class T>struct Object:T {
 const Input* input=nullptr;CellClass* cell=nullptr;Disguise* disguise=nullptr;
 template<class Type>explicit Object(Type* type):T(type,nullptr){}
 VisualType VisualCharacter(VARIANT_BOOL,HouseClass*) const override{return VisualType(input->visual);}
 int GetHeight() const override{return input->height;}
 int GetZ() const override{return input->z;}
 int GetZAdjustment() const override{return input->z_adjust;}
 CellClass* GetCell() const override{return cell;}
 CoordStruct* GetCoords(CoordStruct* at) const override{*at={2176,1920,input->z};return at;}
 bool IsClearlyVisibleTo(HouseClass*) const override{return input->clear;}
 ObjectTypeClass* GetDisguise(bool) const override{return disguise;}
 HouseClass* GetDisguiseHouse(bool) const override{return nullptr;}
 int GetFlashingIntensity(int n) const override{return n+33;}
 bool IsIronCurtained() const override{return input->iron;}
};
// Boundary object: only Target is queried. The noinit object owns nothing;
// storage ends without invoking the still-unported EXE destructor.
struct Strike:AirstrikeClass {Strike():AirstrikeClass(noinit_t()) {Target=nullptr;}};
struct Fixture {
 Input input{};SHPStruct shape{};Disguise disguise;
 UnitTypeClass unit_type{"DRAWUNIT"};InfantryTypeClass infantry_type{"DRAWINF"};AircraftTypeClass aircraft_type{"DRAWAIR"};
 BuildingTypeClass building_type{"DRAWBUILDING",BuildingTypeClass::ConstructionDefaults{}};
 Object<UnitClass> unit{&unit_type};Object<InfantryClass> infantry{&infantry_type};
 Object<AircraftClass> aircraft{&aircraft_type};Object<BuildingClass> building{&building_type};
 std::unique_ptr<CellClass,GameDeleter> cell{CellClass::Create()};
 alignas(Strike) std::byte strike_storage[sizeof(Strike)];Strike* strike=new(strike_storage) Strike;
 game::TechnoDrawing drawing;std::vector<std::array<int,17>> requests;
 Fixture(){
  unit.input=infantry.input=aircraft.input=building.input=&input;
  unit.cell=infantry.cell=aircraft.cell=building.cell=cell.get();
  unit.disguise=infantry.disguise=aircraft.disguise=building.disguise=&disguise;
  shape.Width=60;shape.Height=30;shape.Frames=40;
  drawing.context=this;drawing.types.backend_context=this;drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(this);
  drawing.cell_at=[](void* p,const CellStruct&) noexcept{return static_cast<Fixture*>(p)->cell.get();};
  drawing.initialize_light=[](void*,CellClass& c) noexcept {c.LightConvert=reinterpret_cast<LightConvertClass*>(1);c.Intensity_Normal=1101;c.Intensity_Terrain=1202;return game::DrawingStatus::drawn;};
  drawing.height=[](int z) noexcept{return TacticalClass::AdjustForZ(z);};
  drawing.shape_data=[](SHPStruct* image){return image;};
  drawing.frame_bounds=[](SHPStruct*,int){return RectangleStruct{1,2,30,12};};
  drawing.palette=[](void* p,const TechnoClass&,game::TechnoPalette kind,CellClass*,HouseClass*,const game::DrawingPaletteHandle*& out) noexcept {
   auto& f=*static_cast<Fixture*>(p);int id=kind==game::TechnoPalette::cell?5:kind==game::TechnoPalette::eight_bit?6:f.input.clear?3:4;
   out=reinterpret_cast<const game::DrawingPaletteHandle*>(std::uintptr_t(id));return game::DrawingStatus::drawn;
  };
  drawing.shrouded=[](CellClass&) noexcept {return static_cast<Fixture*>(game::techno_drawing()->context)->input.shrouded!=0;};
  drawing.types.backend.shape=[](void* p,const game::ShapeDrawingRequest& r){
   auto& f=*static_cast<Fixture*>(p);
   f.requests.push_back({int(reinterpret_cast<std::uintptr_t>(r.palette)),r.frame,r.position.X,r.position.Y,r.clip.X,r.clip.Y,r.clip.Width,r.clip.Height,int(r.flags),r.depth_adjustment,r.gradient,r.intensity,int(unsigned(r.tint)&0xFFFFu),int(reinterpret_cast<std::uintptr_t>(r.depth_image)),r.depth_frame,r.depth_offset.X,r.depth_offset.Y});
   return game::DrawingStatus::drawn;
  };
 }
 ~Fixture(){building.Airstrike=nullptr;cell->LightConvert=nullptr;}
 void run(){
  requests.clear();disguise.kind=input.disguise;drawing.draw_shadows=input.draw_shadows;
  cell->LightConvert=input.light?reinterpret_cast<LightConvertClass*>(1):nullptr;
  cell->Intensity_Normal=WORD(input.normal_light);cell->Intensity_Terrain=WORD(input.terrain_light);
  building_type.DoubleThick=input.double_thick;building_type.TerrainPalette=input.terrain;
  unit_type.SmallVisceroid=input.small;unit_type.LargeVisceroid=input.large;unit.TerrainPalette=input.composite;
  TechnoClass* object=input.kind==1?static_cast<TechnoClass*>(&unit):input.kind==2?static_cast<TechnoClass*>(&aircraft):input.kind==6?static_cast<TechnoClass*>(&building):static_cast<TechnoClass*>(&infantry);
  object->GetTechnoType()->NoShadow=input.no_shadow;
  object->CloakProgress.Value=input.cloak_progress;object->BeingWarpedOut=input.warp==1;object->WarpingOut=input.warp==2;object->IsOnCarryall=input.carry;
  object->IronTintStage=input.iron_stage;object->AirstrikeTintStage=input.air_stage;
  object->IronTintTimer.StartTime=object->AirstrikeTintTimer.StartTime=-1;object->IronTintTimer.TimeLeft=object->AirstrikeTintTimer.TimeLeft=5;
  building.Airstrike=input.airstrike?strike:nullptr;strike->Target=&building;
  struct Call{Fixture& f;TechnoClass& object;} call{*this,*object};
  const auto status=game::with_techno_drawing(drawing,[](void* p){
   auto& c=*static_cast<Call*>(p);auto& f=c.f;auto& i=f.input;Point2D at{50,60};RectangleStruct clip{11,13,400,300};
   if(i.foot)static_cast<FootClass&>(c.object).Draw_A_SHP(&f.shape,i.frame,&at,&clip,0,256,i.z_adjust,ZGradient(i.gradient),i.write_depth,i.intensity,i.tint,nullptr,0,0,0,i.excluded);
   else c.object.DrawObject(&f.shape,i.frame,&at,&clip,0,256,i.z_adjust,ZGradient(i.gradient),i.write_depth,i.intensity,i.tint,nullptr,0,0,0,i.excluded);
  },&call);
  EXPECT_TRUE(game::techno_drawing_complete(status));
 }
};
}
TEST(TechnoDrawing, OriginalSharedShapeRequests){
 Fixture f;std::ifstream stream(RA2_TECHNO_DRAW_FIXTURE);std::string tag,fields;std::getline(stream,tag);ASSERT_EQ(tag,"TECHNO_DRAW_V1");std::getline(stream,fields);int count=0;
 while(stream>>tag){
  ASSERT_EQ(tag,"CASE");std::string name;stream>>name>>f.input;SCOPED_TRACE(name);
  int size;stream>>tag>>size;ASSERT_EQ(tag,"RESULT");std::vector<std::array<int,17>> expected(size);
  for(auto& r:expected){stream>>tag;ASSERT_EQ(tag,"DRAW");for(auto& v:r)stream>>v;}
  ASSERT_TRUE(stream);f.run();EXPECT_EQ(f.requests,expected);++count;
 }
 EXPECT_EQ(count,1104);
}
TEST(TechnoDrawing, NestedScopeFailureAndExceptionRestore){
 game::TechnoDrawing outer,inner;
 EXPECT_EQ(game::techno_drawing(),nullptr);
 const auto result=game::with_techno_drawing(outer,[](void* p){
  auto& inner=*static_cast<game::TechnoDrawing*>(p);auto* before=game::techno_drawing();
  EXPECT_EQ(game::with_techno_drawing(inner,[](void*){throw 1;},nullptr),game::DrawingStatus::backend_failure);
  EXPECT_EQ(game::techno_drawing(),before);
  game::record_techno_drawing(*before,game::DrawingStatus::invalid_argument);
  game::record_techno_drawing(*before,game::DrawingStatus::drawn);
 },&inner);
 EXPECT_EQ(result,game::DrawingStatus::invalid_argument);EXPECT_EQ(game::techno_drawing(),nullptr);
}
