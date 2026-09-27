#include "yrpp/DisplayClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/LocomotionClass.h"
#include "support/test_support.hpp"
// Original-derived contracts and bounded native-session invariants.
// Provenance and limits: tests/fixtures/building_visual_reference.json.
// All synthetic fixtures exercise the real core; no Godot-side object model.
#include "support/map_test_support.hpp"
#include "map_world_internal.hpp"
#include "building_selection.hpp"
#include "building_drawing.hpp"
#include "building_voxel.hpp"
#include "api/software_type_drawing.hpp"
#include "yrpp/Surface.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/Drawing.h"
#include "type_drawing_packets.hpp"
#include <bit>
#include <cmath>
#include <set>
#include "type_resources.hpp"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/ParticleClass.h"
#include "type_drawing.hpp"
#include "tactical_drawing.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/AlphaShapeClass.h"
#include <functional>
#include <sstream>
#include <exception>
#include <algorithm>
#include <cfenv>
#include "yrpp/YRMath.h"
using namespace map_fixture;

TEST(BuildingVisual, TrigInitializationNearestPreservesRounding) {
    const int previous=std::fegetround();
    struct Restore {int mode;~Restore(){std::fesetround(mode);}}restore{previous};
    std::fesetround(FE_TONEAREST);
    EXPECT_EQ(Math::sin(0),0.0);EXPECT_EQ(Math::cos(0),1.0);
    EXPECT_EQ(std::fegetround(),FE_TONEAREST);
}
TEST(BuildingVisual, TrigInitializationChoppedPreservesRounding) {
    const int previous=std::fegetround();
    struct Restore {int mode;~Restore(){std::fesetround(mode);}}restore{previous};
    std::fesetround(FE_TOWARDZERO);
    EXPECT_EQ(Math::sin(0),0.0);EXPECT_EQ(Math::cos(0),1.0);
    EXPECT_EQ(std::fegetround(),FE_TOWARDZERO);
}

namespace {
void health_rules_fixture(const std::filesystem::path& root) {
    // Match the RULESMD.INI thresholds used by the reference data.
    // The original Rules constructor defaults ConditionRed to 0.5.
    std::ofstream(root/"RULESMD.INI",std::ios::app)
        << "\n[AudioVisual]\nConditionRed=25%\nConditionYellow=50%\n";
}
void ticks(game::MapViewHandle& view, int count) {
    for (int i = 0; i < count; ++i) EXPECT_TRUE((game::update_game_view(view, 1.0/60.0))) << "advance simulation tick";
}
struct Draw {
    std::vector<game::ShapeDrawingRequest> shapes;
    std::vector<game::RasterDrawingRequest> rasters;
    std::vector<game::IndexedDrawingRequest> indexed;
    std::vector<std::vector<std::uint32_t>> voxel_pixels;
};
Draw draw(game::MapViewHandle& view, const RectangleStruct* world_bounds = nullptr) {
    EXPECT_TRUE((game::set_map_viewport(view, 640, 480))) << "set viewport";
    Draw out; game::MapDrawingContext context; context.types.backend_context = &out;
    context.types.target = reinterpret_cast<game::DrawingTargetHandle*>(&out);
    context.terrain_palette = [](void*, const BytePalette& p, int,int,int,int,
            const game::DrawingPaletteHandle*& result) noexcept {
        result = reinterpret_cast<const game::DrawingPaletteHandle*>(&p); return game::DrawingStatus::drawn;
    };
    context.shape_palette = [](void*, const BytePalette& p, int, const game::DrawingPaletteHandle*& result) noexcept {
        result = reinterpret_cast<const game::DrawingPaletteHandle*>(&p); return game::DrawingStatus::drawn;
    };
    context.color_scheme_palette=context.shape_palette;
    context.types.backend.tile = [](void*, const game::TileDrawingRequest&) { return game::DrawingStatus::drawn; };
    context.types.backend.shape = [](void* p, const game::ShapeDrawingRequest& r) {
        static_cast<Draw*>(p)->shapes.push_back(r); return game::DrawingStatus::drawn;
    };
    context.types.backend.raster = [](void* p, const game::RasterDrawingRequest& r) {
        // Only metadata is inspected after the callback. Pixel storage is borrowed.
        auto copy = r; copy.pixels = nullptr;
        static_cast<Draw*>(p)->rasters.push_back(copy); return game::DrawingStatus::drawn;
    };
    context.types.backend.indexed=[](void*p,const game::IndexedDrawingRequest&r){auto&d=*static_cast<Draw*>(p);auto copy=r;copy.pixels=nullptr;d.indexed.push_back(copy);d.voxel_pixels.emplace_back(r.pixels,r.pixels+r.pixel_count);return game::DrawingStatus::drawn;};
    game::MapDrawStatistics statistics;
    if(world_bounds)native(view,[&]{
        EXPECT_EQ(game::draw_map_world(*view.world,context,*world_bounds,statistics),game::DrawingStatus::drawn);
    });
    else {
        const auto status=game::draw_map_view(view, context, statistics);
        EXPECT_EQ(status,game::DrawingStatus::drawn) << "draw native map: " << game::map_view_error(view)
            << "; " << game::drawing_failure();
    }
    return out;
}
using Bytes=std::vector<unsigned char>;
void u32(Bytes&b,std::size_t i,std::uint32_t v){for(unsigned k=0;k<4;++k)b[i+k]=v>>(k*8);}
void f32(Bytes&b,std::size_t i,float v){u32(b,i,std::bit_cast<std::uint32_t>(v));}
Bytes motion_file(int frames=3,int layers=1){Bytes b(24+16*layers+48*frames*layers);u32(b,16,frames);u32(b,20,layers);for(int f=0;f<frames;++f)for(int l=0;l<layers;++l){int o=24+16*layers+48*(f*layers+l);for(int k:{0,5,10})f32(b,o+k*4,1);f32(b,o+12,float(f*6));f32(b,o+44,float(8+l));}return b;}
Bytes voxel_file(bool barrel=false){
 const unsigned nx=barrel?20:8,ny=barrel?2:6,nz=barrel?2:6;
 Bytes body(nx*ny*8);const auto spans=nx*ny*8;
 for(unsigned y=0;y<ny;++y)for(unsigned x=0;x<nx;++x){u32(body,4*(y*nx+x),body.size()-spans);body.push_back(0);body.push_back(nz);for(unsigned z=0;z<nz;++z){body.push_back(barrel?81:42+x%4);body.push_back(253);}body.push_back(nz);u32(body,4*nx*ny+4*(y*nx+x),body.size()-spans-1);}
 Bytes b(32+28+body.size()+92);std::memcpy(b.data(),"Voxel Animation",15);u32(b,20,1);u32(b,24,1);u32(b,28,body.size());u32(b,48,0);std::memcpy(b.data()+60,body.data(),body.size());
 const auto t=60+body.size();u32(b,t,0);u32(b,t+4,nx*ny*4);u32(b,t+8,spans);f32(b,t+12,0.5f);for(unsigned k:{0u,5u,10u})f32(b,t+16+k*4,1);
 const float bounds[6]={barrel?0.0f:-4.0f,barrel?-1.0f:-3.0f,0.0f,barrel?20.0f:4.0f,barrel?1.0f:3.0f,float(nz)};
 for(unsigned k=0;k<6;++k)f32(b,t+64+4*k,bounds[k]);b[t+88]=nx;b[t+89]=ny;b[t+90]=nz;b[t+91]=4;return b;
}
void voxel_fixture(const std::filesystem::path&root){
 edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nTurret=yes\nTurretAnim=TESTTUR\nTurretAnimIsVoxel=yes\nBarrelAnimIsVoxel=yes\nBarrelStartPitch=2\nTurretAnimX=9\nTurretAnimY=-22\nTurretAnimZAdjust=-40\n");
 edit(root/"ARTMD.INI","[BLDG]\n","[BLDG]\nHeight=3\nTurretOffset=15\n");
 binary(root/"TESTTUR.VXL",voxel_file());binary(root/"TESTBARL.VXL",voxel_file(true));binary(root/"TESTTUR.HVA",motion_file());binary(root/"TESTBARL.HVA",motion_file());
 Bytes vpl(16+768+32*256);u32(vpl,0,16);u32(vpl,4,31);u32(vpl,8,32);for(int n=0;n<32*256;++n)vpl[16+768+n]=n%256;binary(root/"VOXELS.VPL",vpl);
 Bytes pal(768);pal[15*3]=pal[15*3+1]=pal[15*3+2]=63;pal[12*3]=63;binary(root/"PALETTE.PAL",pal);
}
game::BuildingVoxelPlan plan(){game::BuildingVoxelPlan p;EXPECT_TRUE((game::building_voxel_plan(placed(),p))) << "native voxel plan exists";return p;}
const game::BuildingVoxelPart& part(const game::BuildingVoxelPlan&p,bool barrel){for(unsigned i=0;i<p.count;++i)if(p.parts[i].barrel==barrel)return p.parts[i];throw std::runtime_error("required native voxel part absent");}
// SetCurrent alone cancels a turn but retains DesiredFacing when Current already equals d.
void facing(FacingClass&f,unsigned raw){DirStruct d;d.Raw=raw;f.SetDesired(d);f.SetCurrent(d);}
bool near(float a,float b){return std::abs(a-b)<0.00005f;}
bool matrix_equal(const Matrix3D&a,const Matrix3D&b){for(unsigned i=0;i<12;++i)if(!near(a.Data[i],b.Data[i]))return false;return true;}
std::uint64_t hash_pixels(const Draw&d){std::uint64_t h=0;for(const auto&p:d.voxel_pixels)for(auto v:p)h=(h^v)*1099511628211ull;for(const auto&r:d.indexed){h^=r.position.X;h*=1099511628211ull;h^=r.position.Y;h*=1099511628211ull;}return h;}
#if defined(RA2_VISUAL_SOFTWARE)
struct Software {
 BSurface target{8,8,2};BytePalette palette{};std::unique_ptr<ConvertClass> convert;
 ABuffer alpha{{0,0,8,8}};ZBuffer depth{{0,0,8,8}};game::TypeDrawingContext context;
 Software(){Drawing::SetColorMode(static_cast<RGBMode>(2));ABuffer::Instance=&alpha;ZBuffer::Instance=&depth;
  convert=std::make_unique<ConvertClass>(palette,palette,2,53,false);
  EXPECT_TRUE((convert->FullColorData!=nullptr)) << "allocated native palette";
  for(unsigned i=0;i<53*256;++i)static_cast<WORD*>(convert->FullColorData)[i]=WORD(0x2000+i);
  target.Fill(0x1234);depth.Fill(100);alpha.Fill(127);
  context=game::make_software_type_drawing(&target,convert.get());
 }
 ~Software(){ABuffer::Instance=nullptr;ZBuffer::Instance=nullptr;alpha.ReleaseSurface();depth.ReleaseSurface();}
 WORD pixel(int x,int y){auto*p=static_cast<WORD*>(target.Lock(x,y));EXPECT_TRUE((p!=nullptr)) << "surface lock";WORD v=*p;target.Unlock();return v;}
 WORD&z(int x,int y){return *static_cast<WORD*>(depth.GetBuffer(x,y));}
 WORD&a(int x,int y){return *static_cast<WORD*>(alpha.GetBuffer(x,y));}
 game::IndexedDrawingRequest indexed(const std::vector<std::uint32_t>&p){game::IndexedDrawingRequest r;r.target=context.target;r.palette=context.palette;r.position={1,1};r.clip={0,0,8,8};r.width=int(p.size());r.height=1;r.pixels=p.data();r.pixel_count=p.size();r.absolute_depth=100;return r;}
};
#endif

TEST(TacticalForeground, RearSelectionMatchesOriginalTenSegments) {
 session(voxel_fixture,[](auto& view){native(view,[&]{
  auto& building=placed();building.IsSelected=true;
  std::ifstream file(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_behind_reference.txt");
  ASSERT_TRUE(file);int foundation,height,x,y,z,count,cases=0;
  while(file>>foundation>>height>>x>>y>>z>>count){
   building.Type->Foundation=Foundation(foundation);building.Type->Height=height;
   building.Location={x-building.Type->GetFoundationWidth()*128+128,y-building.Type->GetFoundationHeight(false)*128+128,z};
   game::BuildingSelectionGeometry expected;expected.count=count;
   ASSERT_EQ(count,10);for(auto i=0;i<count;++i){int palette;auto& e=expected.edges[i];file>>e.first.X>>e.first.Y>>e.first.Z>>e.last.X>>e.last.Y>>e.last.Z>>palette;EXPECT_EQ(palette,15);}
   expected.palette_index=building.GetHeight()<-4?12:15;
   std::vector<std::array<int,4>> actual,reference;
   game::TypeDrawingContext drawing;drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(&drawing);
   drawing.backend_context=&actual;drawing.backend.raster=[](void* p,const game::RasterDrawingRequest& r){
    static_cast<std::vector<std::array<int,4>>*>(p)->push_back({r.position.X,r.position.Y,r.line_z,r.color});return game::DrawingStatus::drawn;
   };
   const auto projected=TacticalClass::CoordsToScreen({x,y,z});Point2D camera{projected.X-320,projected.Y-240};RectangleStruct bounds{0,0,640,480};
   game::BuildingHealthDrawing frame{drawing};frame.camera=camera;frame.selection_palette=&view.world->impl->selection_palette;
   struct Call{BuildingClass* b;RectangleStruct bounds;} call{&building,bounds};
   EXPECT_TRUE(game::drawing_completed(game::with_building_health_drawing(frame,[](void* p){auto& c=*static_cast<Call*>(p);Point2D at{};c.b->DrawBehind(&at,&c.bounds);},&call)));
   drawing.backend_context=&reference;
   EXPECT_TRUE(game::drawing_completed(game::draw_building_selection(drawing,bounds,camera,expected,*frame.selection_palette)));
   EXPECT_EQ(actual,reference)<<foundation<<","<<height;ASSERT_TRUE(file);++cases;
  }
  EXPECT_EQ(cases,54);
 });});
}

TEST(TacticalForeground, AircraftRegistersBeforeInputAndDeletionInvalidatesCandidates) {
 session([](const auto& root){
  voxel_fixture(root);
  std::ofstream(root/"RULESMD.INI",std::ios::app)<<"\n[AircraftTypes]\n0=TESTAIR\n[TESTAIR]\nStrength=100\nSelectable=yes\nAmmo=3\nPipScale=Ammo\n";
  std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[TESTAIR]\nVoxel=yes\n";
  binary(root/"TESTAIR.VXL",voxel_file());binary(root/"TESTAIR.HVA",motion_file());
  shp(root/"PIPS.SHP",21,8,8);shp(root/"PIPS2.SHP",21,8,8);shp(root/"PIPBRD.SHP",2,8,8);
 },[](auto& view){
  ASSERT_TRUE(game::set_map_viewport(view,640,480));AircraftClass* plane=nullptr;
  struct PlayerScope{HouseClass* old;~PlayerScope(){HouseClass::CurrentPlayer=old;}} player{HouseClass::CurrentPlayer};
  native(view,[&]{
   auto* type=AircraftTypeClass::Find("TESTAIR");ASSERT_NE(type,nullptr);type->Locomotor=LocomotionClass::CLSIDs::Fly;
   HouseClass::CurrentPlayer=placed().Owner;
   plane=new AircraftClass(type,HouseClass::CurrentPlayer);plane->Location=placed().Location;
   game::attach_map_object(*plane);ASSERT_TRUE(plane->IsOnMap);plane->SetHeight(208);
   DisplayClass::Remove(plane);DisplayClass::Submit(plane);game::map_object_changed();
   game::TacticalDrawingFrame frame;frame.world=view.world.get();
   EXPECT_TRUE(game::drawing_completed(game::with_tactical_drawing(frame,[](void* p){static_cast<TacticalClass*>(p)->BuildSelectableList();},&view.tactical)));
   bool found=false;for(int i=0;i<view.tactical.SelectableCount;++i)found|=TacticalClass::SelectableObjects[i].Object==plane;
   EXPECT_TRUE(found);EXPECT_TRUE(plane->Select());
  });
  auto output=draw(view);
  EXPECT_EQ(std::count_if(output.shapes.begin(),output.shapes.end(),[&](const auto& r){return r.image==view.world->impl->mobile_pips&&r.frame==13;}),3);
  native(view,[&]{delete plane;EXPECT_EQ(view.tactical.SelectableCount,0);});
 });
}

TEST(TacticalBackground, StaticTerrainAndBuildingBodyAreOnlyInBackground) {
 session([](const auto& root){
  edit(root/"RULESMD.INI","IsAnimated=yes","IsAnimated=no");
  turret_fixture(root);
 },[](auto& view){
  ASSERT_TRUE(game::set_map_viewport(view,640,480));
  native(view,[&]{
   auto& building=placed();auto* terrain=TerrainClass::Array[0];
   building.NeedsRedraw=true;terrain->NeedsRedraw=true;
   game::TacticalDrawingFrame frame;frame.world=view.world.get();frame.bounds={0,0,640,480};
   const auto collect=[&](bool foreground){
    struct Call{TacticalClass* tactical;bool foreground;}call{&view.tactical,foreground};
    return game::with_tactical_drawing(frame,[](void* p){auto& c=*static_cast<Call*>(p);
     if(c.foreground)c.tactical->BuildDrawRequests();else c.tactical->BuildBackgroundDrawRequests();
    },&call);
   };
   EXPECT_EQ(collect(false),game::DrawingStatus::drawn);
   ASSERT_TRUE(frame.background_prepared);ASSERT_GT(frame.background_requests,0u);
   EXPECT_FALSE(building.NeedsRedraw);EXPECT_FALSE(terrain->NeedsRedraw);
   const auto background=view.world->impl->sprites;
   const auto terrainCount=std::count_if(background.begin(),background.end(),[&](const auto& r){return r.owner==terrain;});
   const auto bodyCount=std::count_if(background.begin(),background.end(),[&](const auto& r){return r.image==building.Type->Image;});
   EXPECT_GT(terrainCount,0);EXPECT_GT(bodyCount,0);
   EXPECT_EQ(collect(true),game::DrawingStatus::drawn);
   const auto& all=view.world->impl->sprites;ASSERT_GE(all.size(),background.size());
   for(std::size_t i=0;i<background.size();++i){EXPECT_EQ(all[i].image,background[i].image);EXPECT_EQ(all[i].frame,background[i].frame);EXPECT_EQ(all[i].owner,background[i].owner);}
   EXPECT_EQ(std::count_if(all.begin(),all.end(),[&](const auto& r){return r.owner==terrain;}),terrainCount);
   EXPECT_EQ(std::count_if(all.begin(),all.end(),[&](const auto& r){return r.image==building.Type->Image;}),bodyCount);
  });
 });
}

TEST(TacticalBackground, FailureRetriesWholeFrameAndPublishesStateOnlyOnSuccess) {
 session([](const auto&){},[](auto& view){
  ASSERT_TRUE(game::set_map_viewport(view,640,480));
  native(view,[&]{
   struct Sink{bool fail=true;bool sawTile=false;bool lightBeforeTiles=false;int shapes=0;}sink;
   game::MapDrawingContext drawing;drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(&sink);drawing.types.backend_context=&sink;
   drawing.terrain_palette=[](void*,const BytePalette& p,int,int,int,int,const game::DrawingPaletteHandle*& result)noexcept{result=reinterpret_cast<const game::DrawingPaletteHandle*>(&p);return game::DrawingStatus::drawn;};
   drawing.shape_palette=[](void*,const BytePalette& p,int,const game::DrawingPaletteHandle*& result)noexcept{result=reinterpret_cast<const game::DrawingPaletteHandle*>(&p);return game::DrawingStatus::drawn;};
   drawing.color_scheme_palette=drawing.shape_palette;
   drawing.types.backend.tile=[](void* p,const game::TileDrawingRequest&){auto& s=*static_cast<Sink*>(p);s.sawTile=true;return game::DrawingStatus::drawn;};
   drawing.types.backend.raster=[](void* p,const game::RasterDrawingRequest& r){auto& s=*static_cast<Sink*>(p);if(r.blend_mode==game::RasterBlendMode::shroud){EXPECT_FALSE(s.sawTile);s.lightBeforeTiles=true;}return game::DrawingStatus::drawn;};
   drawing.types.backend.shape=[](void* p,const game::ShapeDrawingRequest&){auto& s=*static_cast<Sink*>(p);++s.shapes;return s.fail?game::DrawingStatus::backend_failure:game::DrawingStatus::drawn;};
   game::MapDrawStatistics stats;
   game::TacticalDrawingFrame frame{view.world.get(),&drawing,{0,0,640,480},&stats};
   const auto render=[&]{return game::with_tactical_drawing(frame,[](void* p){static_cast<TacticalClass*>(p)->Render(nullptr,false,0x3);},&view.tactical);};
   view.tactical.LastTacticalPos={-9,-9};view.tactical.VisibleCellCount=1;view.tactical.Redrawing=true;
   // The deprecated mode is rejected at entry; it must never reach the
   // header-only FoggedObject placeholders or partially submit a frame.
   view.scenario.SpecialFlags.FogOfWar=true;
   EXPECT_EQ(render(),game::DrawingStatus::unsupported);EXPECT_EQ(sink.shapes,0);EXPECT_FALSE(sink.sawTile);
   view.scenario.SpecialFlags.FogOfWar=false;
   EXPECT_EQ(render(),game::DrawingStatus::backend_failure);EXPECT_EQ(sink.shapes,1);EXPECT_TRUE(sink.lightBeforeTiles);
   EXPECT_EQ(view.tactical.LastTacticalPos,(Point2D{-9,-9}));EXPECT_EQ(view.tactical.VisibleCellCount,1);EXPECT_TRUE(view.tactical.Redrawing);
   sink={};sink.fail=false;
   EXPECT_EQ(render(),game::DrawingStatus::drawn);EXPECT_GT(sink.shapes,1);EXPECT_TRUE(sink.lightBeforeTiles);
   EXPECT_EQ(view.tactical.LastTacticalPos,view.tactical.TacticalPos);EXPECT_EQ(view.tactical.VisibleCellCount,0);EXPECT_FALSE(view.tactical.Redrawing);
   const int expected=sink.shapes;sink={};sink.fail=false;
   EXPECT_EQ(render(),game::DrawingStatus::drawn);EXPECT_EQ(sink.shapes,expected);
  });
 });
}

TEST(TacticalBackground, CliffAndSlopeCastersUseDifferentOriginalScales) {
 session([](const auto& root){shp(root/"C_SHADOW.SHP",14,180,180);},[](auto& view){
  ASSERT_TRUE(game::set_map_viewport(view,640,480));
  native(view,[&]{
   auto* tile=IsometricTileTypeClass::Array[0];
   const int index=tile->ArrayIndex,oldSlope=IsometricTileTypeClass::SlopeSetPieces;
   const bool oldCaster=tile->ShadowCaster;
   int oldSets[5];std::copy(std::begin(IsometricTileTypeClass::ShadowTileSets),std::end(IsometricTileTypeClass::ShadowTileSets),oldSets);
   auto restore=ra2::test::scope_exit([&]{tile->ArrayIndex=index;tile->ShadowCaster=oldCaster;IsometricTileTypeClass::SlopeSetPieces=oldSlope;
    std::copy(std::begin(oldSets),std::end(oldSets),std::begin(IsometricTileTypeClass::ShadowTileSets));});
   tile->ShadowCaster=true;IsometricTileTypeClass::ShadowTileSets[0]=1000;IsometricTileTypeClass::SlopeSetPieces=2000;
   game::TacticalDrawingFrame frame;frame.world=view.world.get();frame.bounds={0,0,640,480};
   auto& requests=view.world->impl->sprites;requests.clear();
   struct Call{IsometricTileTypeClass* tile;int sub;}call{tile,0};
   const auto cast=[&](int tileIndex,int sub){tile->ArrayIndex=tileIndex;call.sub=sub;
    return game::with_tactical_drawing(frame,[](void* p){auto& c=*static_cast<Call*>(p);c.tile->DrawShadowCaster(c.sub,nullptr,{100,100},{0,0,640,480},28);},&call);};
   cast(1020,0);ASSERT_EQ(requests.size(),1u);EXPECT_EQ(requests.back().position,(Point2D{160,145}));EXPECT_EQ(requests.back().frame,0);
   cast(1022,1);ASSERT_EQ(requests.size(),2u);EXPECT_EQ(requests.back().position,(Point2D{190,130}));EXPECT_EQ(requests.back().frame,1);
   cast(2004,6);ASSERT_EQ(requests.size(),3u);EXPECT_EQ(requests.back().position,(Point2D{178,127}));EXPECT_EQ(requests.back().frame,12);
   cast(2004,0);EXPECT_EQ(requests.size(),3u);
   for(const auto& r:requests){EXPECT_EQ(r.flags,0x4601u);EXPECT_EQ(r.gradient,0);EXPECT_EQ(r.depth_adjustment,28);}
  });
 });
}

TEST(TacticalBackground, AlphaPlacementDrawExpirationAndWorldTeardown) {
 const int initial=AlphaShapeClass::Array.Count;
 session([](const auto& root){
  edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nAlphaImage=TESTALPHA\n");
  // Isolate alpha attachments from the separate building-animation lifecycle.
  edit(root/"ARTMD.INI","ActiveAnim=SPIN\n","ActiveAnim=none\n");
  edit(root/"ARTMD.INI","ActiveAnimDamaged=SPINDAM\n","ActiveAnimDamaged=none\n");
  shp(root/"TESTALPHA.SHP",1,31,17);
 },[&](auto& view){
  ASSERT_TRUE(game::set_map_viewport(view,640,480));
  CoordStruct location;
  native(view,[&]{
   ASSERT_EQ(AlphaShapeClass::Array.Count,initial+1);
   auto& owner=placed();location=owner.Location;
   auto* shape=AlphaShapeClass::Array[initial];
   EXPECT_EQ(shape->AttachedTo,&owner);EXPECT_EQ(shape->AlphaImage,owner.Type->AlphaImage);
   EXPECT_FALSE(shape->IsObjectGone);EXPECT_EQ(shape->WhatAmI(),AbstractType::AlphaShape);
   EXPECT_GE(AbstractClass::Array.FindItemIndex(shape),0);
   const auto absolute=TacticalClass::CoordsToScreen(owner.GetCoords());
   EXPECT_EQ(shape->Rect.X,absolute.X-15);EXPECT_EQ(shape->Rect.Y,absolute.Y-8);
   EXPECT_EQ(shape->Rect.Width,31);EXPECT_EQ(shape->Rect.Height,17);
   CLSID id{};ASSERT_EQ(shape->GetClassID(&id),0);DWORD words[4];std::memcpy(words,&id,sizeof(words));
   EXPECT_EQ(words[0],0x623C7584u);EXPECT_EQ(words[1],0x11D274E7u);
   EXPECT_EQ(words[2],0x6000F5B8u);EXPECT_EQ(words[3],0xED09C808u);
  });
  for(int i=0;i<2;++i){
   const auto output=draw(view);
   EXPECT_EQ(std::count_if(output.rasters.begin(),output.rasters.end(),[](const auto& r){return r.blend_mode==game::RasterBlendMode::alpha_shape;}),1);
   EXPECT_EQ(AlphaShapeClass::Array.Count,initial+1); // repaint does not purge/update
  }
  native(view,[&]{
   auto* shape=AlphaShapeClass::Array[initial];
   ASSERT_TRUE(placed().Limbo());
   EXPECT_TRUE(shape->IsObjectGone);EXPECT_EQ(shape->AttachedTo,&placed());
   EXPECT_EQ(AlphaShapeClass::Array.Count,initial+1); // original deferred removal
  });
  ticks(view,1); // production Logic::Update performs the purge
  native(view,[&]{
   EXPECT_EQ(AlphaShapeClass::Array.Count,initial);
   // This small fixture's structure lies outside its playable rectangle.
   auto& map=MapClass::Instance;const auto visible=map.VisibleRect;
   auto restore=ra2::test::scope_exit([&]{map.VisibleRect=visible;});
   map.VisibleRect=map.MapRect;
   ASSERT_TRUE(placed().Unlimbo(location,DirType::North));
   ASSERT_EQ(AlphaShapeClass::Array.Count,initial+1);
   EXPECT_FALSE(AlphaShapeClass::Array[initial]->IsObjectGone);
  });
  // Leave the live attachment to real map teardown, before SHP resources unload.
 });
 EXPECT_EQ(AlphaShapeClass::Array.Count,initial);
}

#if defined(RA2_VISUAL_SOFTWARE)
TEST(TacticalBackground, LightingWritesKeepZeroAndDistinguishSkipValues) {
 Software sw;
 std::uint16_t values[]{0,127,254,255,0x100};
 game::RasterDrawingRequest r;r.target=sw.context.target;r.position={1,2};r.clip={0,0,8,8};
 r.width=5;r.height=1;r.pitch=5;r.pixels=values;r.pixel_count=5;
 r.blend_mode=game::RasterBlendMode::shroud;
 ASSERT_EQ(game::submit_type_raster(sw.context,r),game::DrawingStatus::drawn);
 const WORD shroud[]{0,127,127,255,127};
 for(int x=0;x<5;++x){EXPECT_EQ(sw.a(x+1,2),shroud[x]);EXPECT_EQ(sw.pixel(x+1,2),0x1234);EXPECT_EQ(sw.z(x+1,2),100);}
 values[0]=63;values[1]=63;values[2]=128;values[3]=0;
 r.blend_mode=game::RasterBlendMode::fog;
 ASSERT_EQ(game::submit_type_raster(sw.context,r),game::DrawingStatus::drawn);
 const WORD fog[]{0,63,127,128,127};for(int x=0;x<5;++x)EXPECT_EQ(sw.a(x+1,2),fog[x]);
 values[0]=254;values[1]=254;values[2]=0;values[3]=255;
 r.blend_mode=game::RasterBlendMode::alpha_shape;
 ASSERT_EQ(game::submit_type_raster(sw.context,r),game::DrawingStatus::drawn);
 const WORD alpha[]{0,126,0,255,127};for(int x=0;x<5;++x)EXPECT_EQ(sw.a(x+1,2),alpha[x]);
 // Clip with a nonzero origin must restrict writes, and the source is strided.
 sw.alpha.Fill(127);r.clip={2,2,2,1};r.blend_mode=game::RasterBlendMode::shroud;
 ASSERT_EQ(game::submit_type_raster(sw.context,r),game::DrawingStatus::drawn);
 EXPECT_EQ(sw.a(1,2),127);EXPECT_EQ(sw.a(2,2),127);EXPECT_EQ(sw.a(3,2),0);EXPECT_EQ(sw.a(4,2),127);
}
#endif

TEST(BuildingVisual, MapSmudgesExpandFootprintBeforeDrawing){
 for(const auto size:{Point2D{1,1},Point2D{2,1},Point2D{1,2},Point2D{2,2},Point2D{3,2}})
 for(bool explicitZero:{false,true}){
  SCOPED_TRACE((std::to_string(size.X)+"x"+std::to_string(size.Y)+(explicitZero?",0":",omitted")));
  session([&](const auto& root){
   shp(root/"MARK.SHP",1,120,60);
   std::ofstream(root/"RULESMD.INI",std::ios::app)
    <<"\n[SmudgeTypes]\n0=MARK\n[MARK]\nTheater=no\nWidth="<<size.X<<"\nHeight="<<size.Y<<"\nBurn=yes\n";
   std::ofstream(root/"world.map",std::ios::app)
    <<"\n[Smudge]\n0=MARK,8,5"<<(explicitZero?",0":"")
    <<"\n1=MARK,9,5,1\n2=MARK,8,6,-1\n";
  },[&](auto& view){
   SHPStruct* image=nullptr;
   native(view,[&]{
    auto* type=SmudgeTypeClass::Find("MARK");ASSERT_NE(type,nullptr);image=type->Image;
    // Include the next row/column: nonzero map records must not create
    // new footprints or overwrite the data assigned by the zero record.
    for(int y=0;y<=size.Y;++y)for(int x=0;x<=size.X;++x){
     auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(8+x),short(5+y)});ASSERT_NE(cell,nullptr);
     if(x<size.X&&y<size.Y){EXPECT_EQ(cell->SmudgeTypeIndex,type->ArrayIndex);EXPECT_EQ(cell->SmudgeData,x+y*size.X);}
     else EXPECT_EQ(cell->SmudgeTypeIndex,-1);
    }
   });
   ASSERT_NE(image,nullptr);
   const auto output=draw(view);
   const Point2D anchor{90-view.tactical.TacticalPos.X,195-view.tactical.TacticalPos.Y};
   int submissions=0;
   for(const auto& request:output.shapes)if(request.image==image){
    ++submissions;EXPECT_EQ(request.position,anchor);EXPECT_EQ(request.frame,0);
    EXPECT_EQ(request.flags,0xE00u);EXPECT_EQ(request.depth_adjustment,-1);
   }
   EXPECT_EQ(submissions,size.X*size.Y);
  });
 }
}

TEST(BuildingVisual, ScenarioReadersSharePlacementAndPreserveSourceIndices) {
 session([](const auto& root){
  shp(root/"GI.SHP",16,24,24);shp(root/"MARK.SHP",1,60,30);
  std::ofstream(root/"RULESMD.INI",std::ios::app)
   <<"\n[VehicleTypes]\n0=TANK\n[TANK]\nStrength=100\nSpeed=4\n"
     "Locomotor={4A582741-9839-11D1-B709-00A024DDAFD1}\nMovementZone=Normal\nSpeedType=Track\n"
     "[Clear]\nTrack=100%\nWheel=100%\nFoot=100%\n"
     "[InfantryTypes]\n0=E1\n[E1]\nImage=GI\nStrength=125\nSpeed=4\n"
     "Locomotor={4A582744-9839-11D1-B709-00A024DDAFD1}\nMovementZone=Infantry\n"
     "[SmudgeTypes]\n0=MARK\n[MARK]\nTheater=no\nWidth=1\nHeight=1\n";
  std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[GI]\nSequence=GISEQ\n[GISEQ]\nReady=0,1,1\nWalk=0,1,1\n";
  std::ofstream(root/"world.map",std::ios::app)
   <<"\n[Units]\n0=Neutral,MISSING,256,8,6,0,Guard,None,0,-1,0,-1,1,1\n"
     "1=Neutral,TANK,256,8,6,64,Guard,None,0,-1,0,2,1,1\n"
     "2=Neutral,TANK,256,10,6,0,Guard,None,0,-1,0,-1,1,1\n"
     "[Infantry]\n0=Neutral,E1,256,9,8,2,Guard,0,None,0,-1,0,1,1\n"
     "[Smudge]\n0=MARK,8,3,0\n";
 },[](auto& view){native(view,[&]{
  ASSERT_EQ(UnitClass::Array.Count,2);ASSERT_EQ(InfantryClass::Array.Count,1);
  auto& unit=*UnitClass::Array[0];auto& follower=*UnitClass::Array[1];auto& building=placed();
  EXPECT_EQ(unit.FollowerCar,&follower);EXPECT_TRUE(follower.IsFollowerCar);
  EXPECT_LT(unit.UniqueID,InfantryClass::Array[0]->UniqueID);
  EXPECT_LT(InfantryClass::Array[0]->UniqueID,building.UniqueID);
  EXPECT_GE(LogicClass::Instance.FindItemIndex(&unit),0);
  EXPECT_GE(LogicClass::Instance.FindItemIndex(InfantryClass::Array[0]),0);
  EXPECT_EQ(unit.Type->FireAngle,8);EXPECT_EQ(unit.BarrelFacing.Desired().Raw,0x3800);
  unit.Type->ThreatPosed=11;
  EXPECT_EQ(view.world->impl->unknown_records,1u);
  auto* marked=MapClass::Instance.GetCellAt(CellStruct{8,3});
  ASSERT_EQ(marked->SmudgeTypeIndex,SmudgeTypeClass::Find("MARK")->ArrayIndex);
  const auto location=unit.Location;const auto cell=unit.GetMapCoords();BYTE neighbours[8];
  for(int i=0;i<8;++i){const auto delta=Unsorted::AdjacentCell[i];
   neighbours[i]=MapClass::Instance.GetCellAt(CellStruct{short(cell.X+delta.X),short(cell.Y+delta.Y)})->BlockedNeighbours;
  }
  EXPECT_TRUE(unit.Limbo());EXPECT_LT(LogicClass::Instance.FindItemIndex(&unit),0);
  EXPECT_EQ(unit.ThreatPosed,0u);
  EXPECT_TRUE(unit.Unlimbo(location,DirType::East));
  EXPECT_EQ(unit.ThreatPosed,11u);EXPECT_GE(LogicClass::Instance.FindItemIndex(&unit),0);
  for(int i=0;i<8;++i){const auto delta=Unsorted::AdjacentCell[i];
   EXPECT_EQ(MapClass::Instance.GetCellAt(CellStruct{short(cell.X+delta.X),short(cell.Y+delta.Y)})->BlockedNeighbours,neighbours[i]);
  }
  // The same Building.Mark clears a smudge on runtime placement. The map
  // retains it because its smudge reader ran after buildings, as in YR.
  // This tiny map loads a structure outside its clamped playable rectangle.
  // Runtime placement now enforces that rectangle: expose the fixture's full
  // map while checking Mark, then restore it for the remaining session.
  auto& map=MapClass::Instance;const auto visible=map.VisibleRect;
  struct RestoreVisible {MapClass& map;RectangleStruct rect;~RestoreVisible(){map.VisibleRect=rect;}} restore_visible{map,visible};
  map.VisibleRect=map.MapRect;
  const auto origin=building.Location;EXPECT_TRUE(building.Limbo());EXPECT_TRUE(building.Unlimbo(origin,DirType::North));
  EXPECT_EQ(marked->SmudgeTypeIndex,-1);
 });});
}

TEST(BuildingVisual, OriginalWorldSortAndDamageFrames){
    session([](const auto&root){
        edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nTechLevel=-1\nCanBeOccupied=yes\n");
    },[](auto&view){
        native(view,[]{
            auto&b=placed();ASSERT_EQ(b.Type->TechLevel,-1);
            auto*a=b.Anims[3];ASSERT_NE(a,nullptr);
            std::ifstream input(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_scene_reference.txt");
            std::string tag;input>>tag;ASSERT_EQ(tag,"BUILDING_SCENE_V1");
            int sorts=0,frames=0;
            while(input>>tag){
                if(tag=="S"){
                    int kind,x,y,z,adjust,gate,voxel,expected;
                    input>>kind>>x>>y>>z>>adjust>>gate>>voxel>>expected;
                    b.Location={x,y,z};b.Type->Gate=gate;b.Type->TurretAnimIsVoxel=voxel;
                    // The original fixture supplies absolute GetCoords.
                    // This scene animation is attached, so Location is relative.
                    const auto owner=b.GetCoords();
                    const auto relative=[](int value,int origin){return std::bit_cast<int>(unsigned(value)-unsigned(origin));};
                    a->Location={relative(x,owner.X),relative(y,owner.Y),relative(z,owner.Z)};a->YSortAdjust=adjust;
                    const int actual=kind==0?a->ObjectClass::GetYSort():kind==1?b.GetYSort():a->GetYSort();
                    EXPECT_EQ(actual,expected)<<"original sort kind="<<kind;
                    ++sorts;
                }else if(tag=="F"){
                    int red,yellow,occupied,tech,hp,occupants,gate,expected;
                    input>>red>>yellow>>occupied>>tech>>hp>>occupants>>gate>>expected;
                    auto&r=*RulesClass::Instance;r.ConditionRed=red/100.0;r.ConditionYellow=yellow/100.0;
                    b.Type->CanBeOccupied=occupied;b.Type->TechLevel=tech;b.Type->Gate=gate;b.Type->GateStages=7;
                    b.Health=hp;b.Animation.Value=0;b.BState=int(BStateType::Idle);
                    b.Occupants.Clear();for(int i=0;i<occupants;++i)b.Occupants.AddItem(nullptr);
                    EXPECT_EQ(b.GetOccupantCount(),occupants);
                    EXPECT_EQ(b.GetCurrentFrame(),expected)<<"hp="<<hp<<" tech="<<tech<<" occupied="<<occupied;
                    ++frames;
                }else{std::string rest;std::getline(input,rest);}
            }
            EXPECT_EQ(sorts,1080);EXPECT_EQ(frames,1344);
        });
    });
}

TEST(BuildingVisual, AnimationDrawIniFieldsReachOriginalMethod){
 session([](const auto& root){
  for(const std::string section:{"[SPIN]\n","[SPINDAM]\n"})
   edit(root/"ARTMD.INI",section,section+"IsVeins=yes\nTiled=yes\nShouldFogRemove=yes\nDetailLevel=3\nTranslucencyDetailLevel=3\n");
 },[](auto& view){
  ASSERT_TRUE(game::set_map_viewport(view,640,480));
  auto* anim=placed().Anims[3];ASSERT_NE(anim,nullptr);
  auto* image=anim->Type->Image;
  EXPECT_TRUE(anim->Type->IsVeins);EXPECT_TRUE(anim->Type->Tiled);EXPECT_TRUE(anim->Type->ShouldFogRemove);
  EXPECT_EQ(anim->Type->DetailLevel,3);EXPECT_EQ(anim->Type->TranslucencyDetailLevel,3);
  auto hidden=draw(view);
  EXPECT_EQ(std::count_if(hidden.shapes.begin(),hidden.shapes.end(),[&](const auto& r){return r.image==image;}),0);
  native(view,[&]{anim->Type->IsVeins=false;anim->Type->DetailLevel=0;game::map_object_changed();});
  auto tiled=draw(view);int copies=0;
  for(const auto& r:tiled.shapes)if(r.image==image){++copies;EXPECT_EQ(r.flags&7u,0u);EXPECT_EQ(r.gradient,2);}
  EXPECT_GT(copies,1);
 });
}

TEST(BuildingVisual, CivilianDamageThresholdReachesDrawing){
    session([](const auto&root){
        health_rules_fixture(root);
        edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nTechLevel=-1\nCanBeOccupied=yes\n");
        shp(root/"BLDG.SHP",8);
    },[](auto&view){
        game::MapObjectSnapshot object[2];std::uint32_t count=0;
        ASSERT_TRUE(game::copy_map_objects(view,object,2,count));
        const auto id=object[0].id;
        for(int hp:{51,50,26,25,24,26,100}){
            ASSERT_TRUE(game::set_map_object_health(view,id,hp));
            const auto output=draw(view);
            EXPECT_EQ(body_request(output).frame,hp<=25?1:0);
            game::MapObjectSnapshot state;ASSERT_TRUE(game::get_map_object(view,id,state));
            EXPECT_EQ(state.health,hp);
        }
    });
}

TEST(BuildingVisual, OriginalGarrisonPowerAndImageContracts){
    session([](const auto&root){
        shp(root/"PIPS.SHP",20,8,8);shp(root/"CONSTR.SHP",8);
        edit(root/"ARTMD.INI","[BLDG]\n","[BLDG]\nBuildup=CONSTR\n");
        edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nCanOccupyFire=yes\nShowOccupantPips=no\nPipsDrawForAll=yes\nPipScale=Passengers\nNeedsEngineer=yes\n");
    },[](auto&view){
        native(view,[&]{
            auto&b=placed();auto&t=*b.Type;auto&owner=*b.Owner;
            ASSERT_TRUE(t.CanOccupyFire);ASSERT_FALSE(t.ShowOccupantPips);ASSERT_TRUE(t.PipsDrawForAll);
            ASSERT_TRUE(t.NeedsEngineer);ASSERT_EQ(t.PipScale,PipScale::Passengers);
            HouseTypeClass country("PIPCOUNTRY");country.ArrayIndex=3;
            HouseClass player(&country);player.ArrayIndex=17;
            auto* saved_player=HouseClass::CurrentPlayer;HouseClass::CurrentPlayer=&player;
            struct Restore {HouseClass* player;~Restore(){HouseClass::CurrentPlayer=player;}} restore{saved_player};
            std::ifstream input(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_gaps_reference.txt");
            std::string tag;input>>tag;ASSERT_EQ(tag,"BUILDING_GAPS_V1");
            int g=0,p=0,o=0,images=0;
            while(input>>tag){
                if(tag=="G"){
                    int foundation,height,capacity,occupied,mode,count;input>>foundation>>height>>capacity>>occupied>>mode>>count;
                    t.Foundation=static_cast<Foundation>(foundation);t.Height=height;t.MaxNumberOccupants=capacity;
                    t.CanBeOccupied=mode==1||mode==5||mode==6;t.ShowOccupantPips=mode!=5;
                    t.PipsDrawForAll=mode==4;t.PipScale=mode==6?PipScale::Tiberium:PipScale::None;
                    b.Health=50;b.Occupants.Clear();for(int i=0;i<occupied;++i)b.Occupants.AddItem(nullptr);
                    owner.Allies.Clear();if(mode==2)owner.Allies.Add(player.ArrayIndex);
                    b.DisplayProductionTo.Clear();if(mode==3)b.DisplayProductionTo.Add(3);if(mode==7)b.DisplayProductionTo.Add(2);
                    std::vector<game::ShapeDrawingRequest>draws;
                    game::TypeDrawingContext context;context.target=reinterpret_cast<game::DrawingTargetHandle*>(&draws);
                    context.backend_context=&draws;
                    context.backend.shape=[](void*context,const game::ShapeDrawingRequest&r){static_cast<std::vector<game::ShapeDrawingRequest>*>(context)->push_back(r);return game::DrawingStatus::drawn;};
                    auto*pips=view.world->impl->health_pips;
                    const auto status=game::draw_building_health(b,context,pips,reinterpret_cast<const game::DrawingPaletteHandle*>(pips),{320,240},{17,21,640,480});
                    EXPECT_TRUE(status==game::DrawingStatus::drawn||status==game::DrawingStatus::skipped);
                    std::vector<game::ShapeDrawingRequest>capacity_draws;
                    for(const auto&r:draws)if(r.frame==6)capacity_draws.push_back(r);
                    ASSERT_EQ(capacity_draws.size(),std::size_t(count))<<"foundation="<<foundation<<" mode="<<mode;
                    for(const auto&r:capacity_draws){int f,x,y,flags,intensity;input>>f>>x>>y>>flags>>intensity;
                        EXPECT_EQ(r.frame,f);EXPECT_EQ(r.position,(Point2D{x,y}));EXPECT_EQ(r.flags,unsigned(flags));EXPECT_EQ(r.intensity,intensity);
                    }
                    ++g;
                }else if(tag=="P"){
                    int bits,support,mode,mission,expected;input>>bits>>support>>mode>>mission>>expected;
                    b.HasPower=bits&1;b.EMPLockRemaining=(bits>>1)&1;b.Health=(bits&4)?100:0;
                    t.Powered=bits&8;t.PoweredSpecial=bits&16;t.NeedsEngineer=bits&32;b.HasEngineer=bits&64;
                    b.Overpowerers.Clear();for(int i=0;i<support;++i)b.Overpowerers.AddItem(nullptr);
                    t.PowerDrain=mode==0?0:10;owner.PowerOutput=0;owner.PowerDrain=10;
                    owner.PowerBlackoutTimer.StartTime=-1;owner.PowerBlackoutTimer.TimeLeft=mode==2?1:0;owner.IsBeingDrained=mode==3;
                    b.CurrentMission=static_cast<Mission>(mission);b.QueuedMission=Mission::Construction;
                    EXPECT_EQ(b.IsPowerOnline(),expected!=0)<<"bits="<<bits<<" support="<<support<<" mode="<<mode<<" mission="<<mission;
                    ++p;
                }else if(tag=="O"){
                    int allowed,fire,occupied,expected;input>>allowed>>fire>>occupied>>expected;
                    t.CanBeOccupied=allowed;t.CanOccupyFire=fire;b.Occupants.Clear();
                    for(int i=0;i<occupied;++i)b.Occupants.AddItem(nullptr);
                    EXPECT_EQ(b.CanOccupyFire(),expected!=0);++o;
                }else if(tag=="I"){
                    int placed,state,expected;input>>placed>>state>>expected;
                    b.ActuallyPlacedOnMap=placed;b.BState=state;
                    EXPECT_EQ(b.GetImage(),expected==1?t.Image:t.Buildup);++images;
                }else FAIL()<<"unknown reference row "<<tag;
                ASSERT_TRUE(bool(input));
            }
            EXPECT_EQ(g,512);EXPECT_EQ(p,6144);EXPECT_EQ(o,12);EXPECT_EQ(images,12);
        });
    });
}

TEST(BuildingVisual, GarrisonPipsReachWorldSelectionAndHover){
    session([](const auto&root){
        shp(root/"PIPS.SHP",20,8,8);
        edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nCanBeOccupied=yes\nMaxNumberOccupants=5\n");
    },[](auto&view){
        const auto point=body_request(draw(view)).position;
        game::GameInputResult input;
        auto count=[&]{int result=0;for(const auto&r:draw(view).shapes)if(r.image==view.world->impl->health_pips&&r.frame==6)++result;return result;};
        // 0x6D8DB0: buildings receive Extras in both object passes.
        native(view,[]{placed().Select();});EXPECT_EQ(count(),10);
        native(view,[]{placed().Deselect();});
        ASSERT_TRUE(game::submit_game_input(view,{game::GameInputKind::pointer_move,point.X,point.Y},input));
        native(view,[]{EXPECT_TRUE(placed().IsMouseHovering);EXPECT_FALSE(placed().IsSelected);});EXPECT_EQ(count(),10);
        native(view,[&]{placed().Type->Height=9;placed().Type->PixelSelectionBracketDelta=200;placed().Health=10;});EXPECT_EQ(count(),10);
        native(view,[&]{placed().Type->ShowOccupantPips=false;});EXPECT_EQ(count(),0);
        native(view,[]{placed().Type->ShowOccupantPips=true;});
        ASSERT_TRUE(game::submit_game_input(view,{game::GameInputKind::pointer_leave},input));
        native(view,[]{EXPECT_FALSE(placed().IsMouseHovering);});EXPECT_EQ(count(),0);
    });
}

TEST(BuildingVisual, PowerGateReachesAnimationAndSnapshot){
    session([](const auto&root){
        edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nPowered=yes\nPower=-10\n");
        edit(root/"ARTMD.INI","ActiveAnim=SPIN\n","ActiveAnim=SPIN\nActiveAnimPowered=yes\n");
    },[](auto&view){
        game::MapObjectId id;
        native(view,[&]{auto&b=placed();id=game::object_id(*view.world,&b);b.Owner->PowerOutput=0;b.Owner->PowerDrain=10;b.Update();ASSERT_NE(b.Anims[3],nullptr);EXPECT_TRUE(b.Anims[3]->PowerOff);});
        game::MapObjectSnapshot snapshot;ASSERT_TRUE(game::get_map_object(view,id,snapshot));EXPECT_FALSE(snapshot.powered);
        native(view,[]{auto&b=placed();b.Owner->PowerOutput=10;b.Update();EXPECT_FALSE(b.Anims[3]->PowerOff);});
        ASSERT_TRUE(game::get_map_object(view,id,snapshot));EXPECT_TRUE(snapshot.powered);
        native(view,[]{auto&b=placed();b.EMPLockRemaining=1;b.Update();EXPECT_TRUE(b.Anims[3]->PowerOff);});
        ASSERT_TRUE(game::get_map_object(view,id,snapshot));EXPECT_FALSE(snapshot.powered);
    });
}

TEST(BuildingVisual, PoweredSpecialBlackoutRestoresAttachments){
    session([](const auto&root){
        edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nPoweredSpecial=yes\n");
        edit(root/"ARTMD.INI","ActiveAnim=SPIN\n","ActiveAnim=SPIN\nActiveAnimPoweredSpecial=yes\nLowPower=BLACKOUT\nLowPowerDamaged=BLACKOUT\n");
        std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[BLACKOUT]\nStart=0\nEnd=2\nLoopEnd=2\nShadow=no\n";
        shp(root/"BLACKOUT.SHP",2);
    },[](auto&view){native(view,[]{
        auto&b=placed();ASSERT_NE(b.Anims[3],nullptr);EXPECT_EQ(b.Anims[19],nullptr);
        b.HasPower=false;b.Update();EXPECT_EQ(b.Anims[19],nullptr);
        b.HasPower=true;b.Update();
        b.Owner->PowerBlackoutTimer.StartTime=-1;b.Owner->PowerBlackoutTimer.TimeLeft=5;b.Update();
        EXPECT_EQ(b.Anims[3],nullptr);ASSERT_NE(b.Anims[19],nullptr);
        b.Owner->PowerBlackoutTimer.Stop();b.Update();ASSERT_NE(b.Anims[3],nullptr);EXPECT_EQ(b.Anims[19],nullptr);
        b.Owner->IsBeingDrained=true;b.Update();EXPECT_EQ(b.Anims[3],nullptr);ASSERT_NE(b.Anims[19],nullptr);
        b.Owner->IsBeingDrained=false;b.Update();EXPECT_NE(b.Anims[3],nullptr);EXPECT_EQ(b.Anims[19],nullptr);
    });});
}

TEST(BuildingVisual, WallBodyAndShadowOriginalRequests){
    session([](const auto&root){
        edit(root/"RULESMD.INI","[ORE102]","114=WALL\n[WALL]\nWall=yes\nImage=WALL\n[ORE102]");
        shp(root/"WALL.SHP",96);
    },[](auto&view){
        ASSERT_TRUE(game::set_map_viewport(view,640,480));
        native(view,[&]{
            auto*t=OverlayTypeClass::Find("WALL");ASSERT_NE(t,nullptr);
            struct PlayerScope {HouseClass* previous;~PlayerScope(){HouseClass::CurrentPlayer=previous;}} player_scope{HouseClass::CurrentPlayer};
            // Original wall drawing requires the current player's converter.
            // This synthetic map otherwise has no active player.
            HouseClass::CurrentPlayer=placed().Owner;ASSERT_NE(HouseClass::CurrentPlayer,nullptr);
            auto*c=MapClass::Instance.TryGetCellAt(CellStruct{8,4});ASSERT_NE(c,nullptr);
            c->OverlayTypeIndex=t->ArrayIndex;c->SlopeIndex=0;c->Flags=CellFlags{};c->Intensity_Normal=800;
            game::changed_world_cell(*view.world,*c);
            std::ifstream input(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_scene_reference.txt");
            std::string tag;int cases=0;
            while(input>>tag){
                if(tag!="W"){std::string rest;std::getline(input,rest);continue;}
                int land,level,frame;input>>land>>level>>frame;
                t->LandType=static_cast<LandType>(land);c->Level=level;c->OverlayData=frame;
                view.tactical.TacticalPos={-20,-30};game::map_object_changed();game::rebuild_world_sprites(*view.world);
                std::vector<const game::WorldSprite*> requests;
                for(const auto&s:view.world->impl->sprites)if(s.cell==c&&s.image==t->GetImage())requests.push_back(&s);
                ASSERT_EQ(requests.size(),2u);
                for(int j=0;j<2;++j){int f,x,y,z,gradient,intensity;unsigned flags;
                    input>>f>>x>>y>>std::hex>>flags>>std::dec>>z>>gradient>>intensity;
                    const auto&s=*requests[j];EXPECT_EQ(s.frame,f);EXPECT_EQ(s.flags,flags);
                    EXPECT_EQ(s.position,(Point2D{x+20,y+30}));EXPECT_EQ(s.depth_adjustment,z);
                    EXPECT_EQ(s.gradient,gradient);EXPECT_EQ(s.intensity,intensity);EXPECT_EQ(s.shadow,j==1);
                    EXPECT_TRUE(s.original_depth);
                    EXPECT_EQ(s.color_scheme,j==0);EXPECT_EQ(s.cell_tint,j==1);
                    if(j==0)EXPECT_EQ(s.palette,game::world_palette(*view.world,
                        view.world->impl->unit_palette,HouseClass::CurrentPlayer));
                }
                ++cases;
            }
            EXPECT_EQ(cases,288);
        });
    });
}

namespace {
void occlusion_fixture(const std::filesystem::path&root){
    edit(root/"ARTMD.INI","ActiveAnimYSort=11","ActiveAnimYSort=362");
    edit(root/"ARTMD.INI","ActiveAnimX=1","ActiveAnimX=0");
    edit(root/"ARTMD.INI","Shadow=yes","Shadow=no");
    edit(root/"ARTMD.INI","Translucency=50","Translucency=0");
    edit(root/"ARTMD.INI","YDrawOffset=7","YDrawOffset=0");
    std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[CATIME]\nFoundation=1x1\nImage=FRONT\n";
    shp(root/"SPIN.SHP",2,64,256);shp(root/"FRONT.SHP",2,64,256);
}
BuildingClass* occlusion_front(){
    auto&rear=placed();rear.Health=100;rear.UpdateAnimations();
    auto*front=new BuildingClass(BuildingTypeClass::Find("CATIME"),rear.Owner);
    // Relative placement of ALL07 YAPOWR (187,154) / YAPSYT (191,157).
    front->Location={rear.Location.X+4*256,rear.Location.Y+3*256,rear.Location.Z};
    front->Type->NoShadow=true;game::attach_map_object(*front);EXPECT_TRUE(front->IsOnMap);front->ActuallyPlacedOnMap=true;
    RectangleStruct rect;front->GetRenderDimensions(&rect);EXPECT_GT(rect.Width,0);EXPECT_FALSE(front->IsFogged);EXPECT_EQ(MapClass::ObjectsInLayers[int(Layer::Ground)].FindItemIndex(front)>=0,true);
    game::map_object_changed();return front;
}
}

TEST(BuildingVisual, AttachedAnimationUsesLayersAfterBackground){
    session(occlusion_fixture,[](auto&view){
        ASSERT_TRUE(game::set_map_viewport(view,640,480));
        native(view,[&]{
            auto*rear=&placed();auto*front=occlusion_front();auto*a=rear->Anims[3];ASSERT_NE(a,nullptr);
            // Flat and large drawing offsets must not replace original Y sorting.
            front->Type->Foundation=static_cast<Foundation>(21);
            for(int height:{0,104,728})for(int adjustment:{-362,0,1,362,2000})for(bool flat:{false,true}){
                rear->Location.Z=front->Location.Z=height;a->YSortAdjust=adjustment;a->Type->Flat=flat;
                for(Point2D camera:{Point2D{-100,-100},Point2D{-85,-80}}){
                    view.tactical.TacticalPos=camera;game::map_object_changed();game::rebuild_world_sprites(*view.world);
                    int animation=-1,body=-1;
                    auto&sprites=view.world->impl->sprites;
                    for(unsigned i=0;i<sprites.size();++i){const auto&s=sprites[i];
                        if(s.shadow)continue;
                        if(s.owner==rear&&s.image==a->Type->Image){animation=i;EXPECT_EQ(s.sort,2816+adjustment);}
                        if(s.owner==front&&s.image==front->Type->Image){body=i;EXPECT_EQ(s.sort,4608);}
                    }
                    ASSERT_GE(animation,0);ASSERT_GE(body,0);
                    // Building body belongs to the background. YSort changes only
                    // the ordering inside its actual display layer.
                    EXPECT_LT(body,animation);
                }
            }
        });
    });
}

#if defined(RA2_VISUAL_SOFTWARE)
TEST(BuildingVisual, BackgroundAnimationOrderAndLegacyDepthConsumer){
    session(occlusion_fixture,[](auto&view){
        BuildingClass*rear=nullptr;BuildingClass*front=nullptr;
        native(view,[&]{rear=&placed();front=occlusion_front();});
        const auto output=draw(view);std::vector<game::ShapeDrawingRequest>pair;
        for(auto r:output.shapes)if(!(r.flags&1)&&(r.image==front->Type->Image||r.image==rear->Anims[3]->Type->Image))pair.push_back(r);
        ASSERT_EQ(pair.size(),2u);ASSERT_EQ(pair[0].image,front->Type->Image);ASSERT_EQ(pair[1].image,rear->Anims[3]->Type->Image);
        // This test now asserts the original stage order above. The following
        // isolated primitive checks are intentionally in both orders; they
        // do not claim that the old front-building pixel is unchanged in
        // the complete scene after removing the host fragment reordering.
        std::swap(pair[0],pair[1]);
        int gradients[5][6]{};std::ifstream table(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"game_ui_z_gradients.txt");
        std::string comment;std::getline(table,comment);for(auto&row:gradients)for(auto&v:row)table>>v;ASSERT_TRUE(bool(table));
        auto*saved=Drawing::ZGradientTable;Drawing::ZGradientTable=gradients;
        struct Restore {const int (*saved)[6];~Restore(){Drawing::ZGradientTable=saved;}} restore{saved};
        Software f;const Point2D pixel{pair[1].position.X-10,pair[1].position.Y-80};
        const auto submit=[&](game::ShapeDrawingRequest r){r.target=f.context.target;r.palette=f.context.palette;
            r.position.X+=4-pixel.X;r.position.Y+=4-pixel.Y;r.clip={0,0,8,8};
            EXPECT_EQ(game::submit_type_shape(f.context,r),game::DrawingStatus::drawn);};
        f.depth.Fill(0xFFFF);submit(pair[1]);const WORD expected=f.pixel(4,4);ASSERT_NE(expected,0x1234);
        f.target.Fill(0x1234);f.depth.Fill(0xFFFF);submit(pair[0]);submit(pair[1]);EXPECT_EQ(f.pixel(4,4),expected);
        f.target.Fill(0x1234);f.depth.Fill(0xFFFF);submit(pair[1]);submit(pair[0]);EXPECT_NE(f.pixel(4,4),expected);
    });
}

TEST(BuildingVisual, WallShadowOriginalPixelsAndGroundDepth){
    session([](const auto&root){
        edit(root/"RULESMD.INI","[ORE102]","114=WALL\n[WALL]\nWall=yes\nImage=WALL\n[ORE102]");
        shp(root/"WALL.SHP",96);
    },[](auto&view){
        struct PlayerScope {HouseClass* previous;~PlayerScope(){HouseClass::CurrentPlayer=previous;}} player_scope{HouseClass::CurrentPlayer};
        SHPStruct*image=nullptr;
        native(view,[&]{auto*t=OverlayTypeClass::Find("WALL");image=t->GetImage();
            HouseClass::CurrentPlayer=placed().Owner;ASSERT_NE(HouseClass::CurrentPlayer,nullptr);
            auto*c=MapClass::Instance.TryGetCellAt(CellStruct{8,4});c->OverlayTypeIndex=t->ArrayIndex;c->OverlayData=12;
            game::changed_world_cell(*view.world,*c);game::map_object_changed();});
        const auto output=draw(view);
        auto it=std::find_if(output.shapes.begin(),output.shapes.end(),[&](const auto&r){return r.image==image&&(r.flags&1);});
        ASSERT_NE(it,output.shapes.end());EXPECT_EQ(it->frame,60);
        int gradients[5][6]{};std::ifstream table(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"game_ui_z_gradients.txt");
        std::string comment;std::getline(table,comment);for(auto&row:gradients)for(auto&v:row)table>>v;ASSERT_TRUE(bool(table));
        auto*saved=Drawing::ZGradientTable;Drawing::ZGradientTable=gradients;
        struct Restore {const int (*saved)[6];~Restore(){Drawing::ZGradientTable=saved;}} restore{saved};
        Software f;auto r=*it;r.target=f.context.target;r.palette=f.context.palette;r.position={4,4};r.clip={0,0,8,8};
        std::ifstream reference(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_scene_reference.txt");
        std::string tag;int cases=0;
        while(reference>>tag){
            if(tag!="P"){std::string rest;std::getline(reference,rest);continue;}
            unsigned initial_z,color,depth;reference>>std::hex>>initial_z>>color>>depth>>std::dec;
            f.target.Fill(0xFFFF);f.depth.Fill(initial_z);
            EXPECT_EQ(game::submit_type_shape(f.context,r),game::DrawingStatus::drawn);
            EXPECT_EQ(f.pixel(4,4),color);EXPECT_EQ(f.z(4,4),depth);++cases;
        }
        EXPECT_EQ(cases,2);
    });
}
#endif
std::uint32_t texel(unsigned color,int dz=0,bool coverage=true){return (coverage?0x1000000u:0u)|(std::uint32_t(std::uint16_t(dz))<<8)|color;}
// Independent fixed-EXE matrices also cover every 16-bit facing interval.
std::array<Matrix3D,32> original_yaw_matrices() {
 std::ifstream input(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_voxel_transform_reference.txt");
 std::string format;unsigned count=0;input>>format>>count;
 EXPECT_TRUE((format=="BUILDING_VOXEL_TRANSFORMS_V1")) << "original transform fixture";
 std::array<Matrix3D,32> matrices;std::array<bool,32> found{};
 for(unsigned i=0;i<count;++i){std::string name;int mode,facing,pitch,motion,mission,animation,spotlight,parts;
  input>>name>>mode>>facing>>pitch>>motion>>mission>>animation>>spotlight>>parts;
  for(int p=0;p<parts;++p){int barrel,frame;std::string bytes;input>>barrel>>frame>>bytes;
   if(mode||pitch||motion||mission||barrel||facing%2048)continue;
   EXPECT_TRUE((bytes.size()==96)) << "original matrix size";
   for(unsigned n=0;n<12;++n){std::uint32_t word=0;for(unsigned b=0;b<4;++b)word|=std::uint32_t(std::stoul(bytes.substr(n*8+b*2,2),nullptr,16))<<(b*8);matrices[facing/2048].Data[n]=std::bit_cast<float>(word);}
   found[facing/2048]=true;
  }
 }
 EXPECT_TRUE((bool(input)&&std::all_of(found.begin(),found.end(),[](bool b){return b;}))) << "all 32 original yaw matrices";return matrices;
}

 
TEST(BuildingVisual, OriginalSelectableDistanceAndCellFallback) {
 session([](const auto&){},[](auto&view){
  draw(view);
  native(view,[&]{
   auto& tactical=view.tactical;auto& building=placed();
   struct PlayerScope{HouseClass* previous;~PlayerScope(){HouseClass::CurrentPlayer=previous;}} playerScope{HouseClass::CurrentPlayer};
   ASSERT_NE(building.Owner,nullptr);HouseClass::CurrentPlayer=building.Owner;
   const auto projected=TacticalClass::CoordsToScreen(building.Location);
   const Point2D point{projected.X-tactical.TacticalPos.X,projected.Y-tactical.TacticalPos.Y};
   BuildingClass candidate(building.Type,building.Owner);candidate.InLimbo=false;
   candidate.Location=building.Location;candidate.IsOwnedByCurrentPlayer=true;
   const auto at=[&](int x,int y){tactical.SelectableCount=0;ASSERT_TRUE(tactical.AddSelectable(&candidate,point.X+x,point.Y+y));};
   tactical.SelectableCount=0;
   EXPECT_EQ(tactical.GetSelectableObject(point),&building); // real cell fallback
   at(14,2);EXPECT_EQ(tactical.GetSelectableObject(point),&candidate); // 198
   at(14,3);EXPECT_EQ(tactical.GetSelectableObject(point),&building); // trunc(200.5), excluded
   at(0,20);EXPECT_EQ(tactical.GetSelectableObject(point),&building); // exactly 200, excluded
   at(0,19);EXPECT_EQ(tactical.GetSelectableObject(point),&candidate); // 180 after truncation
   at(-10,0);tactical.AddSelectable(&building,point.X+10,point.Y);
   EXPECT_EQ(tactical.GetSelectableObject(point),&candidate); // first equal-distance candidate
   at(0,0);candidate.InLimbo=true;EXPECT_EQ(tactical.GetSelectableObject(point),&building);
   candidate.InLimbo=false;candidate.IsAlive=false;EXPECT_EQ(tactical.GetSelectableObject(point),&building);
   candidate.IsAlive=true;candidate.IsOwnedByCurrentPlayer=false;candidate.CloakState=CloakState::Cloaked;
   // 0x6DA380 queries virtual GetCoords: a building's foundation center.
   auto* cell=MapClass::Instance.GetCellAt(candidate.GetCoords());const auto house=HouseClass::CurrentPlayer->ArrayIndex;
   ASSERT_FALSE(cell->Sensors_InclHouse(house));
   EXPECT_EQ(tactical.GetSelectableObject(point),&building);
   cell->Sensors_AddOfHouse(house);EXPECT_EQ(tactical.GetSelectableObject(point),&candidate);
   cell->Sensors_RemOfHouse(house);
   tactical.SelectableCount=0;
  });
 });
}

TEST(BuildingVisual, CombatEffectsPreserveHoveredHealth) {
 session([](const auto&root){
  shp(root/"PIPS.SHP",8,8,8);shp(root/"IMPACT.SHP",4,96,96);
  std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[IMPACT]\nEnd=4\nLoopEnd=4\nRate=1\nShadow=no\nLayer=air\n";
 },[](auto&view){
  const auto initial=draw(view);const auto point=body_request(initial).position;
  game::GameInputResult result;
  ASSERT_TRUE(game::submit_game_input(view,{game::GameInputKind::pointer_move,point.X,point.Y},result));
  const auto pips=view.world->impl->health_pips;
  const auto health=[&](const Draw& output){return std::count_if(output.shapes.begin(),output.shapes.end(),[&](const auto&r){return r.image==pips;});};
  const auto count=health(draw(view));ASSERT_GT(count,0);
  AnimClass* impact=nullptr;
  native(view,[&]{
   auto& building=placed();ASSERT_TRUE(building.IsMouseHovering);ASSERT_FALSE(building.IsSelected);
   auto* type=AnimTypeClass::FindOrAllocate("IMPACT");ASSERT_TRUE(type->LoadFromINI(&view.world->rules_ini));
   impact=new AnimClass(type,building.GetRenderCoords(),0,1,0x600);
   game::map_object_changed();game::refresh_map_world_hover(*view.world);
   EXPECT_TRUE(building.IsMouseHovering);EXPECT_EQ(game::pick_world_object(*view.world,point),&building);
  });
  EXPECT_EQ(health(draw(view)),count);
  native(view,[&]{placed().Health=25;game::map_object_changed();game::refresh_map_world_hover(*view.world);});
  EXPECT_EQ(health(draw(view)),count);
  native(view,[&]{delete impact;game::map_object_changed();game::refresh_map_world_hover(*view.world);});
  EXPECT_EQ(health(draw(view)),count);
  ASSERT_TRUE(game::submit_game_input(view,{game::GameInputKind::pointer_leave},result));
  EXPECT_EQ(health(draw(view)),0);
 });
}

TEST(BuildingVisual, MobileVeterancySurvivesDeselection) {
 session([](const auto& root){
  voxel_fixture(root);
  shp(root/"PIPS.SHP",20,8,8);shp(root/"PIPBRD.SHP",2,8,8);shp(root/"GI.SHP",16,24,24);
  std::ofstream(root/"RULESMD.INI",std::ios::app)<<
   "\n[VehicleTypes]\n0=TANK\n[TANK]\nImage=TESTTUR\nStrength=100\nSpeed=4\nROT=5\n"
   "Locomotor={4A582741-9839-11d1-B709-00A024DDAFD1}\nMovementZone=Normal\n"
   "[InfantryTypes]\n0=E1\n[E1]\nImage=GI\nStrength=125\nSpeed=4\nMovementZone=Infantry\n"
   "Locomotor={4A582744-9839-11D1-B709-00A024DDAFD1}\n";
  std::ofstream(root/"ARTMD.INI",std::ios::app)<<
   "\n[TESTTUR]\nVoxel=yes\n[GI]\nSequence=GISequence\n[GISequence]\nReady=0,1,1\nWalk=0,1,1\n";
  std::ofstream(root/"world.map",std::ios::app)<<
   "\n[Units]\n0=Neutral,TANK,256,8,6,0,Guard,None,0,-1,0,-1,1,1\n"
   "[Infantry]\n0=Neutral,E1,256,9,6,0,Guard,0,None,0,-1,0,1,1\n";
 },[](auto& view){
  FootClass* actors[2]{};
  native(view,[&]{
   ASSERT_EQ(UnitClass::Array.Count,1);ASSERT_EQ(InfantryClass::Array.Count,1);
   actors[0]=UnitClass::Array[0];actors[1]=InfantryClass::Array[0];
  });
  ASSERT_NE(actors[0],nullptr);ASSERT_NE(actors[1],nullptr);
  draw(view); // Populate the ordinary world rendering cache before promotion.
  const auto* pips=view.world->impl->health_pips;
  const auto* border=view.world->impl->pip_border;
  ASSERT_NE(pips,nullptr);ASSERT_NE(border,nullptr);
  native(view,[&]{
   for(auto* actor:actors){
    actor->IsSelected=false;actor->IsMouseHovering=false;actor->Veterancy.Veterancy=0;
   }
  });
  for(bool infantry:{false,true}){
   SCOPED_TRACE(infantry?"infantry":"tank");
   auto* actor=actors[infantry?1:0];
   // Original 0x70A990 frames/anchors. Repeated state 0 verifies both
   // deselection and mouse leave, including promotion while unselected.
   for(const auto [rank,frame]:{std::pair{0,-1},std::pair{1,14},std::pair{2,15}}){
    SCOPED_TRACE(rank);
    native(view,[&]{actor->Veterancy.Veterancy=float(rank);});
    for(int state:{0,1,0,2,0}){
     SCOPED_TRACE(state);
     native(view,[&]{actor->IsSelected=state==1;actor->IsMouseHovering=state==2;});
     const auto output=draw(view);
     Point2D anchor;
     native(view,[&]{anchor=TacticalClass::CoordsToScreen(actor->GetRenderCoords());});
     anchor.X+= (infantry?5:10)-view.tactical.TacticalPos.X;
     anchor.Y+= (infantry?2:6)-view.tactical.TacticalPos.Y;
     int insignia=0,health=0,brackets=0;
     for(const auto& r:output.shapes){
      if(r.image==pips&&(r.frame==14||r.frame==15)){
       ++insignia;EXPECT_EQ(r.frame,frame);EXPECT_EQ(r.position,anchor);
       EXPECT_EQ(r.flags,0xE00u);EXPECT_EQ(r.depth_adjustment,-2);EXPECT_EQ(r.intensity,1000);
      }
      if(r.image==pips&&r.frame>=16&&r.frame<=18)++health;
      if(r.image==border)++brackets;
     }
     EXPECT_EQ(insignia,rank?1:0);
     EXPECT_EQ(health,state?(infantry?8:17):0);
     EXPECT_EQ(brackets,state==1?1:0);
    }
   }
   // Removing the selection gate must retain original visibility and
   // off-map/dead/underground removal from the real display list. Setting
   // a field alone does not perform the original lifecycle transition.
   for(int excluded=0;excluded<5;++excluded){
    SCOPED_TRACE(excluded);
    native(view,[&]{
     if(excluded==0)actor->IsSinking=true;
     if(excluded==1){actor->IsOwnedByCurrentPlayer=false;actor->GetTechnoType()->Invisible=true;}
     if(excluded>=2)DisplayClass::Remove(actor);
     if(excluded==2)actor->IsOnMap=false;
     if(excluded==3)actor->Health=0;
     if(excluded==4)actor->TubeIndex=0;
    });
    const auto output=draw(view);
    EXPECT_EQ(std::count_if(output.shapes.begin(),output.shapes.end(),[&](const auto& r){return r.image==pips;}),0);
    native(view,[&]{
     actor->IsSinking=false;actor->GetTechnoType()->Invisible=false;
     actor->IsOnMap=true;actor->Health=actor->GetTechnoType()->Strength;actor->TubeIndex=-1;
     if(excluded>=2)DisplayClass::Submit(actor);
    });
   }
   native(view,[&]{actor->Veterancy.Veterancy=0;});
  }
 });
}

TEST(BuildingVisual, H02_world_anchor_and_visibility) {
    SCOPED_TRACE("0x006D8C60 center / 0x006F64A0 independent of SHP crop");

      session([](const auto&r){shp(r/"PIPS.SHP",8,8,8);},[](auto&v){
       native(v,[]{placed().Select();game::map_object_changed();});
       const auto pips=v.world->impl->health_pips;
       const auto collect=[&](const Draw& d){std::vector<std::array<int,3>> result;for(const auto&r:d.shapes)if(r.image==pips)result.push_back({r.frame,r.position.X,r.position.Y});return result;};
       const auto selected=draw(v);const auto expected=collect(selected);EXPECT_TRUE((!expected.empty())) << "selected health pips";
       const auto point=body_request(selected).position;
       game::GameInputResult input;
       native(v,[&]{auto&b=placed();const auto center=TacticalClass::CoordsToScreen(b.GetCenterCoords());
        const auto left=TacticalClass::CoordsToScreen({-256,256,b.Type->Height*104});
        EXPECT_TRUE((expected.front()[1]==center.X-v.tactical.TacticalPos.X+left.X+63&&expected.front()[2]==center.Y-v.tactical.TacticalPos.Y+left.Y-26)) << "health origin uses foundation center";
        b.Type->Image=nullptr;b.Type->PixelSelectionBracketDelta=500;b.Type->Bib=true;game::map_object_changed();});
       EXPECT_TRUE((collect(draw(v))==expected)) << "missing/cropped body, Bib and bracket delta do not move building pips";
       for(const auto&r:selected.rasters)if(r.blend_mode==game::RasterBlendMode::copy)EXPECT_TRUE((r.original_line)) << "selection uses original lines, no horizontal HP rectangle";
       native(v,[]{placed().Deselect();game::map_object_changed();});
       ASSERT_TRUE(game::submit_game_input(v,{game::GameInputKind::pointer_move,point.X,point.Y},input));
       native(v,[]{EXPECT_TRUE(placed().IsMouseHovering);EXPECT_FALSE(placed().IsSelected);});
       const auto hovered=draw(v);EXPECT_TRUE((collect(hovered)==expected&&std::none_of(hovered.rasters.begin(),hovered.rasters.end(),[](const auto& r){return r.original_line;}))) << "hover shows health without selection brackets";
       ASSERT_TRUE(game::submit_game_input(v,{game::GameInputKind::pointer_leave},input));
       native(v,[]{EXPECT_FALSE(placed().IsMouseHovering);});
       EXPECT_TRUE((collect(draw(v)).empty())) << "unselected and unhovered building hides health";
      });
}

 
TEST(BuildingVisual, H01_original_health_and_selection) {
    SCOPED_TRACE("0x006F64A0 / 0x006F5190 instruction observations");

      session([](const auto&r){health_rules_fixture(r);voxel_fixture(r);shp(r/"PIPS.SHP",8,8,8);},[](auto&v){native(v,[]{
       auto&building=placed();auto&type=*building.Type;type.Strength=1000;
       auto*pips=static_cast<SHPStruct*>(FileSystem::LoadFile("PIPS.SHP",true));EXPECT_TRUE((pips!=nullptr)) << "pips fixture";
       std::ifstream input(RA2_HEALTH_FIXTURE);EXPECT_TRUE((bool(input))) << "original health/selection observations";
       int health_cases=0,selection_cases=0;char kind;
       while(input>>kind){
        int foundation,height;input>>foundation>>height;type.Foundation=static_cast<Foundation>(foundation);type.Height=height;
        if(kind=='H'){
         int health,count;input>>health>>count;building.Health=health;
         std::vector<game::ShapeDrawingRequest> out;game::TypeDrawingContext context;
         context.target=reinterpret_cast<game::DrawingTargetHandle*>(&out);context.backend_context=&out;
         context.backend.shape=[](void*p,const game::ShapeDrawingRequest&r){static_cast<std::vector<game::ShapeDrawingRequest>*>(p)->push_back(r);return game::DrawingStatus::drawn;};
         const auto status=game::draw_building_health(building,context,pips,reinterpret_cast<const game::DrawingPaletteHandle*>(pips),{320,240},{17,21,640,480});
         if(status!=(count?game::DrawingStatus::drawn:game::DrawingStatus::skipped))throw std::runtime_error("health entry foundation="+std::to_string(foundation)+" height="+std::to_string(height)+" hp="+std::to_string(health)+": "+game::drawing_status_name(status));
         EXPECT_TRUE((int(out.size())==count)) << "original health pip count";
         for(const auto&r:out){int frame,x,y,flags,intensity;input>>frame>>x>>y>>flags>>intensity;
          EXPECT_TRUE((r.image==pips&&r.frame==frame&&r.position==Point2D{x,y}&&r.flags==unsigned(flags)&&r.intensity==intensity)) << "original pip frame, position, flags and intensity";
          EXPECT_TRUE((r.clip.X==17&&r.clip.Y==21&&r.clip.Width==640&&r.clip.Height==480)) << "clip origin remains in the backend contract";}
         ++health_cases;
        }else{
         EXPECT_TRUE((kind=='S')) << "selection row tag";CoordStruct center;int count;input>>center.X>>center.Y>>center.Z>>count;
         building.Location={center.X-type.GetFoundationWidth()*128+128,center.Y-type.GetFoundationHeight(false)*128+128,center.Z};
         game::BuildingSelectionGeometry geometry;EXPECT_TRUE((game::building_selection_geometry(building,geometry))) << "building selection geometry";
         std::multiset<std::array<int,7>> expected,actual;
         for(int i=0;i<count;++i){std::array<int,7> values;for(auto&n:values)input>>n;expected.insert(values);}
         for(unsigned i=0;i<geometry.count;++i){const auto&e=geometry.edges[i];actual.insert({e.first.X,e.first.Y,e.first.Z,e.last.X,e.last.Y,e.last.Z,int(geometry.palette_index)});}
         EXPECT_TRUE((actual==expected)) << "original selected building segment endpoints, directions and color index";++selection_cases;
        }
        EXPECT_TRUE((bool(input))) << "complete original observation row";
       }
       EXPECT_TRUE((health_cases==792&&selection_cases==54)) << "all original building display observations";
      });});
}

 
TEST(BuildingVisual, S01_original_11_world_edges) {
    SCOPED_TRACE("0x006F5190 / 0x006F5EF0");
    session(voxel_fixture,[](auto&v){native(v,[]{auto&b=placed();game::BuildingSelectionGeometry g;EXPECT_TRUE((game::building_selection_geometry(b,g)&&g.count==11)) << "11 segments";std::ifstream f(RA2_SELECTION_FIXTURE);EXPECT_TRUE((bool(f))) << "independent geometry fixture";int width,length,height,lx,ly,lz;while(f>>width>>length>>height>>lx>>ly>>lz){int foundation=-1;for(int candidate=0;candidate<22;++candidate){b.Type->Foundation=static_cast<Foundation>(candidate);if(b.Type->GetFoundationWidth()==width&&b.Type->GetFoundationHeight(false)==length){foundation=candidate;break;}}EXPECT_TRUE((foundation>=0)) << "fixture foundation supported";b.Type->Height=height;b.Location={lx,ly,lz};game::building_selection_geometry(b,g);for(unsigned i=0;i<11;++i){CoordStruct a,z;f>>a.X>>a.Y>>a.Z>>z.X>>z.Y>>z.Z;EXPECT_TRUE((g.edges[i].first==a&&g.edges[i].last==z)) << "original endpoints and signed quarter rounding";}}});});
}

 
TEST(BuildingVisual, S02_crop_and_bib_independent) {
    SCOPED_TRACE("original native dimensions, not SHP bounds");
    session(voxel_fixture,[](auto&v){native(v,[]{auto&b=placed();game::BuildingSelectionGeometry a,z;game::building_selection_geometry(b,a);b.Type->Bib=!b.Type->Bib;auto*image=b.Type->Image;b.Type->Image=nullptr;b.Type->PixelSelectionBracketDelta=125;game::building_selection_geometry(b,z);b.Type->Image=image;EXPECT_TRUE((a.count==z.count)) << "same count";for(unsigned i=0;i<a.count;++i)EXPECT_TRUE((a.edges[i].first==z.edges[i].first&&a.edges[i].last==z.edges[i].last)) << "crop/Bib/HP delta cannot move selection geometry";});});
}

 
TEST(BuildingVisual, S03_missing_body_still_selects) {
    SCOPED_TRACE("selection independent of body resource");
    session([](const auto&r){voxel_fixture(r);std::filesystem::remove(r/"BLDG.SHP");},[](auto&v){native(v,[]{placed().Select();game::map_object_changed();});auto d=draw(v);EXPECT_TRUE((d.rasters.size()>11)) << "outline submits without SHP body";});
}

 
TEST(BuildingVisual, S04_original_palette_indices) {
    SCOPED_TRACE("0x005F5F40 / 0x006F5190");
    session(voxel_fixture,[](auto&v){native(v,[]{game::BuildingSelectionGeometry g;auto&b=placed();game::building_selection_geometry(b,g);EXPECT_TRUE((g.palette_index==15)) << "ordinary selection index15";b.Location.Z=-5;game::building_selection_geometry(b,g);EXPECT_TRUE((g.palette_index==12)) << "below-floor index12";});});
}

 
TEST(BuildingVisual, S05_outline_clipping) {
    SCOPED_TRACE("clipped core raster submission");
    session(voxel_fixture,[](auto&v){native(v,[]{game::BuildingSelectionGeometry g;game::building_selection_geometry(placed(),g);BytePalette p{};p.Entries[15]={255,255,255};std::vector<game::RasterDrawingRequest> out;game::TypeDrawingContext c;c.target=reinterpret_cast<game::DrawingTargetHandle*>(&out);c.backend_context=&out;c.backend.raster=[](void*p,const game::RasterDrawingRequest&r){static_cast<std::vector<game::RasterDrawingRequest>*>(p)->push_back(r);return game::DrawingStatus::drawn;};RectangleStruct clip{17,21,19,23};auto screen=TacticalClass::CoordsToScreen(g.edges[0].first);screen.X-=9;screen.Y-=11;game::draw_building_selection(c,clip,screen,g,p);EXPECT_TRUE((!out.empty())) << "partly visible outline";for(const auto&r:out)EXPECT_TRUE((r.position.X>=clip.X&&r.position.Y>=clip.Y&&r.position.X+r.width<=clip.X+clip.Width&&r.position.Y+r.height<=clip.Y+clip.Height)) << "no raster outside clip";});});
}

 
TEST(BuildingVisual, S06_no_unselected_outline) {
    SCOPED_TRACE("no four-white-strip placeholder");
    session(voxel_fixture,[](auto&v){auto unselected=draw(v);native(v,[]{placed().Select();game::map_object_changed();});auto selected=draw(v);EXPECT_TRUE((selected.rasters.size()>unselected.rasters.size()+11)) << "real sloping segments";});
}

 

 
TEST(BuildingVisual, V02_HVA_scaled_once) {
    SCOPED_TRACE("0x005BD730 through 0x0045FA90");
    session(voxel_fixture, [](auto& view) { native(view, [] {
        auto& type = *placed().Type;
        EXPECT_TRUE((type.TurretVoxel.VXL && type.TurretVoxel.HVA &&
                     type.BarrelVoxel.VXL && type.BarrelVoxel.HVA)) << "native original VoxelStruct resources";
        EXPECT_TRUE((!placed().Anims[9])) << "no fake SHP turret";
        EXPECT_TRUE((plan().count==2)) << "turret and barrel ready";
        EXPECT_TRUE((type.TurretVoxel.HVA->GetLayerMatrix(0,1).row[0][3]==3.0f)) << "HVA multiplier translation";
        EXPECT_TRUE((type.TurretVoxel.HVA->GetLayerMatrix(0,1).row[0][0]==1.0f)) << "basis not scaled";
        EXPECT_TRUE((game::load_building_voxels(type))) << "reload resources";
        EXPECT_TRUE((type.TurretVoxel.HVA->GetLayerMatrix(0,1).row[0][3]==3.0f)) << "reload does not scale twice";
    }); });
}

 
TEST(BuildingVisual, V03_explicit_barrel_only) {
    SCOPED_TRACE("0x0045FA90 no TUR naming branch");
    session([](const auto&r){voxel_fixture(r);edit(r/"RULESMD.INI","TurretAnim=TESTTUR","TurretAnim=SHPSLOT\nVoxelBarrelFile=TESTBARL");},[](auto&v){native(v,[]{EXPECT_TRUE((!placed().Type->TurretVoxel.VXL&&placed().Type->BarrelVoxel.VXL)) << "explicit barrel name";auto p=plan();EXPECT_TRUE((p.count==1&&p.parts[0].barrel)) << "barrel-only plan";});auto d=draw(v);EXPECT_TRUE((d.indexed.size()==1)) << "barrel-only actual drawing";});
}

 
TEST(BuildingVisual, V04_TUR_prefix_not_matched) {
    SCOPED_TRACE("0x0045FA90 search begins after character4");
    session([](const auto&r){voxel_fixture(r);edit(r/"RULESMD.INI","TurretAnim=TESTTUR","TurretAnim=TURX\nVoxelBarrelFile=TESTBARL");},[](auto&v){native(v,[]{EXPECT_TRUE((!placed().Type->TurretVoxel.VXL)) << "prefix TUR is not suffix marker";EXPECT_TRUE((placed().Type->BarrelVoxel.VXL)) << "explicit barrel fallback";});});
}

 
TEST(BuildingVisual, V05_missing_HVA_safe) {
    SCOPED_TRACE("failure flag checked before matrix access");
    session([](const auto&r){voxel_fixture(r);std::filesystem::remove(r/"TESTTUR.HVA");},[](auto&v){native(v,[]{auto p=plan();EXPECT_TRUE((p.count==1&&p.parts[0].barrel)) << "bad HVA skipped; barrel retained";});auto d=draw(v);EXPECT_TRUE((d.indexed.size()==1)) << "no crash or fake turret";});
}

 
TEST(BuildingVisual, V06_original_direction_and_order) {
    SCOPED_TRACE("0x0043DA80 32 directions and four-way part order");
    session(voxel_fixture,[](auto&v){native(v,[]{auto&b=placed();const auto original=original_yaw_matrices();for(unsigned raw=0;raw<65536;raw+=257){facing(b.PrimaryFacing,raw);auto p=plan();auto&m=part(p,false).local;unsigned d=(((raw>>10)+1)>>1)&31;EXPECT_TRUE((m.row[0][0]==original[d].row[0][0]&&m.row[1][0]==original[d].row[1][0])) << "original lookup-table yaw matrix";unsigned q=(((raw>>13)+1)>>1)&3;EXPECT_TRUE((p.parts[0].barrel==(q==0||q==3))) << "original barrel submission order";EXPECT_TRUE((m.row[0][3]==original[d].row[0][3])) << "TurretOffset=15 becomes integer1, not1.875";}});});
}

 
TEST(BuildingVisual, V07_pitch_only_barrel) {
    SCOPED_TRACE("0x0043DA80 independent pitch");
    session(voxel_fixture,[](auto&v){native(v,[]{auto p0=plan();facing(placed().BarrelFacing,0x2000);auto p1=plan();EXPECT_TRUE((matrix_equal(part(p0,false).local,part(p1,false).local))) << "pitch does not rotate turret";EXPECT_TRUE((!matrix_equal(part(p0,true).local,part(p1,true).local))) << "pitch rotates barrel";EXPECT_TRUE((placed().Type->BarrelStartPitch==64)) << "rules pitch2 maps to native64";});});
}

 
TEST(BuildingVisual, V08_native_recoil_transforms) {
    SCOPED_TRACE("0x0043DA80");
    session(voxel_fixture,[](auto&v){native(v,[]{auto p0=plan();placed().TurretRecoil.State=RecoilData::RecoilState::Compressing;placed().TurretRecoil.TravelSoFar=2;auto p1=plan();EXPECT_TRUE((!matrix_equal(part(p0,false).local,part(p1,false).local))) << "turret recoil visible";EXPECT_TRUE((matrix_equal(part(p0,true).local,part(p1,true).local))) << "turret recoil not copied into independent barrel";placed().BarrelRecoil.State=RecoilData::RecoilState::Compressing;placed().BarrelRecoil.TravelSoFar=4;auto p2=plan();EXPECT_TRUE((!matrix_equal(part(p1,true).local,part(p2,true).local))) << "barrel recoil visible";});});
}

 
TEST(BuildingVisual, V09_frame_arity) {
    SCOPED_TRACE("paired barrel frame0 and turret HVA modulo");
    session(voxel_fixture,[](auto&v){native(v,[]{placed().TurretAnimFrame=5;auto p=plan();EXPECT_TRUE((part(p,false).frame==2&&part(p,true).frame==0)) << "paired HVA frame selection";});});
}

 
TEST(BuildingVisual, V10_construction_and_sale_visibility) {
    SCOPED_TRACE("0x0043DA80 mission gates");
    session(voxel_fixture,[](auto&v){native(v,[]{auto&b=placed();b.Type->BuildingAnimFrame[0].FrameCount=8;b.CurrentMission=static_cast<Mission>(18);b.Animation.Value=2;game::BuildingVoxelPlan p;EXPECT_TRUE((!game::building_voxel_plan(b,p))) << "construction hides voxel until final frame";b.Animation.Value=7;EXPECT_TRUE((game::building_voxel_plan(b,p))) << "construction final frame draws voxel";b.CurrentMission=static_cast<Mission>(19);b.Animation.Value=1;EXPECT_TRUE((!game::building_voxel_plan(b,p))) << "selling hidden";b.Type->HasSpotlight=true;EXPECT_TRUE((game::building_voxel_plan(b,p))) << "spotlight exception retained";});});
}

 
TEST(BuildingVisual, V11_real_pixels_not_placeholder) {
    SCOPED_TRACE("span decoding and map indexed requests");
    session(voxel_fixture,[](auto&v){auto d=draw(v);EXPECT_TRUE((d.indexed.size()==2)) << "two native indexed draw requests";std::size_t covered=0;std::set<unsigned>colors;for(auto&image:d.voxel_pixels)for(auto p:image)if(p&0x1000000){++covered;colors.insert(p&255);}EXPECT_TRUE((covered>40&&colors.count(81))) << "real barrel samples and coverage";EXPECT_TRUE((colors.count(42)||colors.count(43))) << "real turret palette indices";});
}

 
TEST(BuildingVisual, V12_facing_and_HVA_cache_invalidation) {
    SCOPED_TRACE("native presentation state, no Godot clone");
    session(voxel_fixture,[](auto&v){auto a=draw(v);native(v,[]{facing(placed().PrimaryFacing,0x4000);});auto b=draw(v);EXPECT_TRUE((hash_pixels(a)!=hash_pixels(b))) << "orientation invalidates without synthetic object callback";native(v,[]{placed().TurretAnimFrame=1;});auto c=draw(v);EXPECT_TRUE((hash_pixels(b)!=hash_pixels(c))) << "native HVA frame invalidates cache";EXPECT_TRUE((hash_pixels(c)==hash_pixels(draw(v)))) << "stable frame reproducible";});
}

 
TEST(BuildingVisual, V13_invalid_span_rejected) {
    SCOPED_TRACE("bounded sparse decoder");
    session(voxel_fixture,[](auto&v){native(v,[&]{auto p=plan();auto*vox=placed().Type->TurretVoxel.VXL;const auto saved=*vox->TailerData[0].span_start_off;*vox->TailerData[0].span_start_off=0x7fffffff;game::VoxelSurface image;EXPECT_TRUE((game::render_building_voxel(part(p,false),v.world->impl->voxel_palette,image)==game::DrawingStatus::invalid_argument)) << "reject bad span before dereference";*vox->TailerData[0].span_start_off=saved;});});
}

 
TEST(BuildingVisual, V14_missing_VPL_reported) {
    SCOPED_TRACE("no embedded-palette fallback");
    session([](const auto&r){voxel_fixture(r);std::filesystem::remove(r/"VOXELS.VPL");},[](auto&v){auto d=draw(v);EXPECT_TRUE((d.indexed.empty())) << "missing VPL cannot silently invent colors";EXPECT_TRUE((v.world->impl->missing_voxel_parts>0)) << "missing palette is observable";});
}

 
TEST(BuildingVisual, V15_mixed_pivot_matrix) {
    SCOPED_TRACE("0x00458810");
    session(voxel_fixture,[](auto&v){native(v,[]{auto&b=placed();auto&t=*b.Type;t.TurretAnimIsVoxel=false;t.VoxelBarrelScale=2;t.VoxelBarrelOffsetToBuildingPivotPoint={1,2,3};t.VoxelBarrelOffsetToRotatePivotPoint={4,5,6};t.VoxelBarrelOffsetToPitchPivotPoint={7,8,9};facing(b.PrimaryFacing,0x4000);facing(b.BarrelFacing,0x4000);auto p=plan();EXPECT_TRUE((p.mixed&&p.count==1)) << "mixed SHP/VXL branch";const auto&m=p.parts[0].local;EXPECT_TRUE((near(m.row[0][0],2)&&near(m.row[1][1],2)&&near(m.row[2][2],2))) << "three basis columns scaled";EXPECT_TRUE((near(m.row[0][3],12)&&near(m.row[1][3],15)&&near(m.row[2][3],18))) << "scale does not multiply pivot translation";});});
}

 
TEST(BuildingVisual, V16_destroyed_building_no_voxel) {
    SCOPED_TRACE("native lifecycle");
    session(voxel_fixture,[](auto&v){auto a=draw(v);EXPECT_TRUE((!a.indexed.empty())) << "initially visible";native(v,[]{placed().Health=0;game::detach_map_object(placed());game::map_object_changed();});auto b=draw(v);EXPECT_TRUE((b.indexed.empty())) << "dead object cannot submit stale cached voxel";});
}

 
TEST(BuildingVisual, B01_indexed_callback_boundary) {
    SCOPED_TRACE("versioned borrowed-pixel contract");
    game::TypeDrawingContext c;c.target=reinterpret_cast<game::DrawingTargetHandle*>(&c);c.palette=reinterpret_cast<game::DrawingPaletteHandle*>(&c);game::IndexedDrawingRequest r;r.target=c.target;r.palette=c.palette;r.width=r.height=1;r.clip={0,0,10,10};std::uint32_t pixel=0x100002a;r.pixels=&pixel;r.pixel_count=1;EXPECT_TRUE((game::submit_type_indexed(c,r)==game::DrawingStatus::unavailable)) << "absent backend explicitly unavailable";c.backend.indexed=[](void*,const game::IndexedDrawingRequest&)->game::DrawingStatus{throw std::runtime_error("test");};EXPECT_TRUE((game::submit_type_indexed(c,r)==game::DrawingStatus::backend_failure)) << "exception contained";r.pixel_count=0;EXPECT_TRUE((game::submit_type_indexed(c,r)==game::DrawingStatus::invalid_argument)) << "buffer length validated";
}

 
TEST(BuildingVisual, B02_GPU_packet_signed_depth) {
    SCOPED_TRACE("same 20-word host/shader stride");
    game::IndexedDrawingRequest r;r.position={11,17};r.clip={0,0,640,480};r.width=r.height=1;r.absolute_depth=0x8000;std::uint32_t p=0x1000000u|(std::uint32_t(std::uint16_t(-37))<<8)|42;r.pixels=&p;r.pixel_count=1;game::TypeGpuPacket out;EXPECT_TRUE((game::prepare_type_indexed_parameters(r,{640,480,53,0,0x8000,true},out)==game::DrawingStatus::drawn)) << "indexed packet prepared";EXPECT_TRUE((out.parameters[19]==3&&out.parameters[13]==1&&out.parameters[14]==0x8000)) << "mode3 and depth write";EXPECT_TRUE(((std::int32_t(p<<8)>>16)==-37)) << "shader sign-extension agrees with core packed format";
}

#if defined(RA2_VISUAL_SOFTWARE)
 
TEST(BuildingVisual, B03_software_indexed_depth) {
    SCOPED_TRACE("native Surface/ZBuffer pixel outputs");
    Software f;std::vector<std::uint32_t>p={texel(42,-7),texel(81,1)};auto r=f.indexed(p);EXPECT_TRUE((game::submit_type_indexed(f.context,r)==game::DrawingStatus::drawn)) << "software indexed submit";EXPECT_TRUE((f.z(1,1)==93&&f.z(2,1)==100)) << "near writes Z; far rejected";const unsigned shade=52u*(261u*1000/2048)*127u/32258u;EXPECT_TRUE((f.pixel(1,1)==WORD(0x2000+shade*256+42))) << "exact native shade lookup";EXPECT_TRUE((f.pixel(2,1)==0x1234)) << "far color rejected";
}

 
TEST(BuildingVisual, B04_transparent_does_not_write_depth) {
    SCOPED_TRACE("coverage bit and index0 independently");
    Software f;std::vector<std::uint32_t>p={texel(0,-80),texel(42,-80,false),texel(81,-1)};auto r=f.indexed(p);EXPECT_TRUE((game::submit_type_indexed(f.context,r)==game::DrawingStatus::drawn)) << "transparent pixels draw";EXPECT_TRUE((f.pixel(1,1)==0x1234&&f.pixel(2,1)==0x1234&&f.z(1,1)==100&&f.z(2,1)==100)) << "transparent leaves both buffers intact";EXPECT_TRUE((f.z(3,1)==99)) << "covered neighbor writes";
}

 
TEST(BuildingVisual, B05_readonly_and_no_depth) {
    SCOPED_TRACE("explicit backend depth modes");
    Software f;std::vector<std::uint32_t>p={texel(42,-2)};auto r=f.indexed(p);r.depth_mode=game::ShapeDepthMode::read;EXPECT_TRUE((game::submit_type_indexed(f.context,r)==game::DrawingStatus::drawn)) << "readonly submit";EXPECT_TRUE((f.z(1,1)==100&&f.pixel(1,1)!=0x1234)) << "readonly changes color only";p[0]=texel(81,50);r.depth_mode=game::ShapeDepthMode::none;EXPECT_TRUE((game::submit_type_indexed(f.context,r)==game::DrawingStatus::drawn)) << "depth disabled";EXPECT_TRUE((f.z(1,1)==100)) << "depth disabled preserves buffer";
}

 
TEST(BuildingVisual, B06_indexed_clip_and_light) {
    SCOPED_TRACE("clip and ABuffer per-pixel samples");
    Software f;std::vector<std::uint32_t>p={texel(42),texel(43)};auto r=f.indexed(p);r.clip={2,1,1,1};f.a(2,1)=64;EXPECT_TRUE((game::submit_type_indexed(f.context,r)==game::DrawingStatus::drawn)) << "clipped indexed draw";const unsigned shade=52u*(261u*1000/2048)*64u/32258u;EXPECT_TRUE((f.pixel(1,1)==0x1234&&f.pixel(2,1)==WORD(0x2000+shade*256+43))) << "clip leaves outside; inside uses own light";
}

 
TEST(BuildingVisual, LineTrailAlphaDepthAndLighting) {
    Software f;game::RasterDrawingRequest r;r.target=f.context.target;r.position={1,1};r.clip={0,0,8,8};r.width=r.height=1;
    r.blend_mode=game::RasterBlendMode::depth_alpha;r.line_rgb=0x63AD81;r.line_opacity=239;r.line_z=-2;
    const WORD depth=0x8000-1-2;
    for(unsigned light:{0u,64u,127u,128u,255u})for(int opacity:{7,8,127,239,255}){
        f.target.Fill(0x369A);f.depth.Fill(0xffff);f.a(1,1)=light;r.line_opacity=opacity;
        const auto result=game::submit_type_raster(f.context,r);
        if(!light||opacity<8){EXPECT_EQ(result,game::DrawingStatus::skipped);EXPECT_EQ(f.pixel(1,1),0x369A);continue;}
        const auto channel=[&](unsigned source,unsigned old){return (light*(((unsigned(opacity)*source)>>8)+(((256-unsigned(opacity))*old)>>8)))>>7;};
        const WORD expected=WORD((channel(0x81,((0x369A>>11)&31)*8)>>3)<<11|(channel(0xAD,((0x369A>>5)&63)*4)>>2)<<5|(channel(0x63,(0x369A&31)*8)>>3));
        EXPECT_EQ(result,game::DrawingStatus::drawn);EXPECT_EQ(f.pixel(1,1),expected);EXPECT_EQ(f.z(1,1),0xffff);
    }
    f.target.Fill(0x369A);f.z(1,1)=depth;f.a(1,1)=128;r.line_opacity=255;
    EXPECT_EQ(game::submit_type_raster(f.context,r),game::DrawingStatus::skipped);EXPECT_EQ(f.pixel(1,1),0x369A);
}

TEST(BuildingVisual, OriginalSparkPixels){
 session([](const auto&){},[](auto& view){native(view,[]{
  Software f;ParticleTypeClass type("PIXEL_SPARK");type.BehavesLike=BehavesLike::Spark;
  type.ColorList.AddItem(RGBClass(255,255,255));type.ColorList.AddItem(RGBClass(100,150,200));type.ColorList.AddItem(RGBClass(0,0,0));
  CoordStruct at{2176,1664,256};ParticleClass particle(&type,&at,&at,nullptr);particle.Location={0,0,0};particle.Color=RGBClass(249,179,101);
  auto* tactical=TacticalClass::Instance;const auto camera=tactical->TacticalPos;
  auto restore=ra2::test::scope_exit([&]{tactical->TacticalPos=camera;});
  std::ifstream in(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"spark_pixels.txt");
  std::string magic;int count;ASSERT_TRUE(bool(in>>magic>>count));ASSERT_EQ(magic,"SPARK_PIXELS_V1");
  std::ofstream gpu;if(const char* path=std::getenv("RA2_SPARK_GPU_PACKETS"))gpu.open(path);
  const auto hex=[&](const auto& data){const auto* bytes=reinterpret_cast<const unsigned char*>(data.data());constexpr char digits[]="0123456789abcdef";
   for(std::size_t i=0;i<data.size()*sizeof(data[0]);++i)gpu<<digits[bytes[i]>>4]<<digits[bytes[i]&15];gpu<<'\n';};
  std::vector<WORD> initial(64,0x1234),depths(64),lights(64),palette(512,0);
  constexpr WORD lightValues[]={0,1,64,126,127,128,255,65535};
  for(int y=0;y<8;++y)for(int x=0;x<8;++x){const WORD depthValues[]={0,WORD(0x8000-y-50),WORD(0x8000-y-49),65535};depths[y*8+x]=depthValues[y%4];lights[y*8+x]=lightValues[x];}
  if(gpu){gpu<<"SHP_GPU_PACKETS_V1\n8 8\n";hex(initial);hex(depths);hex(lights);hex(palette);gpu<<count<<'\n';}
  for(int i=0;i<count;++i){
   std::uint64_t accum;int index,x,y,light,depth,expected;ASSERT_TRUE(bool(in>>index>>accum>>x>>y>>light>>depth>>expected));SCOPED_TRACE(i);
   particle.ColorIndex=index;particle.ColorAccum=std::bit_cast<double>(accum);tactical->TacticalPos={-x,-y};
   f.target.Fill(0x1234);f.z(x,y)=WORD(depth);f.a(x,y)=WORD(light);
   Point2D point{};RectangleStruct bounds{0,0,8,8};
   struct Draw{ParticleClass* particle;Point2D* point;RectangleStruct* bounds;}draw{&particle,&point,&bounds};
   const auto invoke=[](void* p){auto&d=*static_cast<Draw*>(p);d.particle->DrawIt(d.point,d.bounds);};
   EXPECT_TRUE(game::drawing_completed(game::with_type_drawing(f.context,invoke,&draw)));
   EXPECT_EQ(f.pixel(x,y),expected);EXPECT_EQ(f.z(x,y),depth);
   if(gpu){
    game::TypeGpuPacket packet;game::TypeDrawingContext capture;capture.target=f.context.target;capture.backend_context=&packet;
    capture.backend.raster=[](void* p,const game::RasterDrawingRequest&r){return game::prepare_type_particle_parameters(r,{8,8,1,0,0x8000,true},*static_cast<game::TypeGpuPacket*>(p));};
    ASSERT_TRUE(game::drawing_completed(game::with_type_drawing(capture,invoke,&draw)));
    auto expectedPixels=initial;expectedPixels[y*8+x]=WORD(expected);
    gpu<<"spark_"<<i<<'\n';hex(packet.parameters);hex(std::array<std::uint32_t,1>{0});hex(expectedPixels);hex(depths);
   }
  }
  EXPECT_EQ(count,320);
 });});
}

TEST(BuildingVisual, S07_selection_strict_depth_and_lighting) {
    SCOPED_TRACE("0x004BFD30 equal Z hidden / no Z write / light127");
    Software f;game::RasterDrawingRequest r;r.target=f.context.target;r.position={1,1};r.clip={0,0,8,8};r.width=r.height=1;r.original_line=true;r.color=0xffff;r.line_z=14;f.depth.Fill(0xffff);const WORD z=0x8000-1+14;EXPECT_TRUE((game::submit_type_raster(f.context,r)==game::DrawingStatus::drawn)) << "line visible in front";EXPECT_TRUE((f.pixel(1,1)==0xffff&&f.z(1,1)==0xffff)) << "line unchanged at127; no Z write";f.target.Fill(0x1234);f.z(1,1)=z;EXPECT_TRUE((game::submit_type_raster(f.context,r)==game::DrawingStatus::skipped&&f.pixel(1,1)==0x1234)) << "equal Z is hidden";f.z(1,1)=z+1;f.a(1,1)=64;EXPECT_TRUE((game::submit_type_raster(f.context,r)==game::DrawingStatus::drawn&&f.pixel(1,1)==0x7bef)) << "native RGB half intensity";f.a(1,1)=0;f.target.Fill(0x1234);EXPECT_TRUE((game::submit_type_raster(f.context,r)==game::DrawingStatus::skipped&&f.pixel(1,1)==0x1234)) << "zero light invisible";
}

#endif
 
TEST(BuildingVisual, S08_original_line_tie_and_terminal) {
    SCOPED_TRACE("0x004BFD30 >0 tie, terminal excluded");
    session(voxel_fixture,[](auto&v){native(v,[]{game::BuildingSelectionGeometry g;g.count=1;g.edges[0]={{0,0,0},{256,0,0}};BytePalette p{};p.Entries[15]={252,252,252};std::vector<game::RasterDrawingRequest>out;game::TypeDrawingContext c;c.target=reinterpret_cast<game::DrawingTargetHandle*>(&out);c.backend_context=&out;c.backend.raster=[](void*p,const game::RasterDrawingRequest&r){static_cast<std::vector<game::RasterDrawingRequest>*>(p)->push_back(r);return game::DrawingStatus::drawn;};EXPECT_TRUE((game::draw_building_selection(c,{17,21,100,100},{0,0},g,p)==game::DrawingStatus::drawn)) << "known original line";EXPECT_TRUE((out.size()==30)) << "terminal at30,15 excluded";for(unsigned i=0;i<out.size();++i)EXPECT_TRUE((out[i].position==Point2D{17+int(i),21+int(i)/2}&&out[i].line_z==14&&out[i].original_line)) << "literal line pixels and Z, not bounding rectangle";});});
}

 
TEST(BuildingVisual, V17_asset_reload_invalidates_surfaces) {
    SCOPED_TRACE("native resources may reload without object callback");
    session(voxel_fixture,[](auto&v){auto a=draw(v);std::uint64_t before=0;native(v,[&]{before=game::voxel_resource_revision();EXPECT_TRUE((game::load_building_voxels(*placed().Type))) << "resource reload";EXPECT_TRUE((game::voxel_resource_revision()!=before)) << "resource generation changed";placed().Type->TurretVoxel.HVA->Matrixes[0].row[0][3]+=20;});auto b=draw(v);EXPECT_TRUE((hash_pixels(a)!=hash_pixels(b))) << "cached surface must not survive asset replacement";});
}

 
TEST(BuildingVisual, V18_HVA_lighting_does_not_rotate) {
    SCOPED_TRACE("0x00753D00 lighting computed before HVA");
    session(voxel_fixture,[](auto&v){native(v,[&]{auto p=plan();auto part0=part(p,false);auto*vox=placed().Type->TurretVoxel.VXL;const auto&t=vox->TailerData[0];for(unsigned x=0;x<unsigned(std::uint8_t(t.size_X))*std::uint8_t(t.size_Y);++x){auto*span=t.span_data_off+t.span_start_off[x];for(unsigned z=0;z<span[1];++z)span[3+2*z]=80;}game::VoxelPalette pal;pal.sections=32;pal.lighting.resize(32*256);for(unsigned shade=0;shade<32;++shade)for(unsigned color=0;color<256;++color)pal.lighting[shade*256+color]=color?shade+1:0;game::VoxelSurface a,b;EXPECT_TRUE((game::render_building_voxel(part0,pal,a)==game::DrawingStatus::drawn)) << "lit native model";auto&m=placed().Type->TurretVoxel.HVA->Matrixes[0];m.row[0][0]=-1;m.row[1][1]=-1;EXPECT_TRUE((game::render_building_voxel(part0,pal,b)==game::DrawingStatus::drawn)) << "HVA rotated model";std::set<unsigned>ca,cb;for(auto p:a.pixels)if(p&0x1000000)ca.insert(p&255);for(auto p:b.pixels)if(p&0x1000000)cb.insert(p&255);EXPECT_TRUE((ca==cb&&ca.size()==1)) << "HVA cannot rotate the normal lookup light";});});
}



}

namespace {
std::optional<int> legacy_command(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--emit-fixture") {
        const std::filesystem::path root = argv[2];
        if (std::filesystem::exists(root)) throw std::runtime_error("fixture destination must not exist");
        std::filesystem::create_directories(root);
        fixtures(root);
        voxel_fixture(root);
        std::cout << "Synthetic fixture written: " << root << '\n';
        return 0;
    }
    return std::nullopt;
}
const ra2::test::CommandRegistration command(legacy_command);
}

// Fixed-EXE projection samples cover every attachment slot, including turret 9.
// Test the final body/shadow requests so camera, YDrawOffset and crop cannot
// accidentally apply a second anchor correction after the coordinate helper.
TEST(BuildingVisual, AllAttachmentScreenAnchors) {
    session(turret_fixture,[](auto&view){
        ASSERT_TRUE(game::set_map_viewport(view,640,480));
        std::ifstream input(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_attachment_anchors.txt");
        std::string tag;input>>tag;ASSERT_EQ(tag,"BUILDING_ATTACHMENT_ANCHORS_V1");
        auto&b=placed();auto*a=b.Anims[9];ASSERT_NE(a,nullptr);
        int height,x,y,px,py,cases=0;
        while(input>>height>>x>>y>>px>>py){
            for(int slot=0;slot<21;++slot)for(const Point2D camera:{Point2D{-100,-100},Point2D{-83,-75}}){
                auto*saved_slot=b.Anims[slot];
                struct RestoreSlot {BuildingClass& b;int slot;AnimClass* saved;AnimClass* turret;
                    ~RestoreSlot(){b.Anims[slot]=saved;b.Anims[9]=turret;}
                } restore{b,slot,saved_slot,a};
                native(view,[&]{
                    b.Anims[9]=nullptr;b.Anims[slot]=a;b.Location={2176,896,height};
                    b.Type->BuildingAnim[slot].Position={x,y};
                    b.SetAnimCoords();
                    a->Type->YDrawOffset=7;a->Type->Shadow=true;a->Type->Flat=false;
                    a->Animation.Value=0;view.tactical.TacticalPos=camera;game::map_object_changed();
                });
                const auto output=draw(view);int found=0;
                for(const auto&r:output.shapes)if(r.image==a->Type->Image){
                    const bool shadow=(r.flags&1)!=0;
                    EXPECT_EQ(r.position,(Point2D{px-camera.X,py-camera.Y+(shadow?0:7)}))<<"slot="<<slot<<" offset="<<x<<','<<y;
                    ++found;
                }
                EXPECT_EQ(found,2);
                ++cases;
            }
        }
        EXPECT_EQ(cases,1134);
    });
}

TEST(BuildingVisual, VoxelFinalScreenAnchors) {
    session(voxel_fixture,[](auto&view){
        ASSERT_TRUE(game::set_map_viewport(view,640,480));
        std::ifstream input(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_voxel_anchors.txt");
        std::string tag;input>>tag;ASSERT_EQ(tag,"BUILDING_VOXEL_ANCHORS_V1");
        int mode,x,y,px,py,count,cases=0;
        while(input>>mode>>x>>y>>px>>py>>count){
            struct Expected {int barrel,x,y;};std::vector<Expected> expected(count);
            for(auto&e:expected)input>>e.barrel>>e.x>>e.y;
            ASSERT_TRUE(bool(input));
            auto&b=placed();auto&t=*b.Type;auto*saved_voxel=t.TurretVoxel.VXL;
            struct RestoreVoxel {BuildingTypeClass& type;VoxLib* saved;
                ~RestoreVoxel(){type.TurretVoxel.VXL=saved;}
            } restore{t,saved_voxel};
            game::BuildingVoxelPlan p;
            native(view,[&]{
                t.TurretAnimIsVoxel=mode!=2;t.TurretVoxel.VXL=mode==0?saved_voxel:nullptr;
                t.BuildingAnim[9].Position={x,y};b.CurrentMission=Mission::Guard;b.Animation.Value=0;
                const auto anchor=TacticalClass::CoordsToScreen(b.GetRenderCoords());
                view.tactical.TacticalPos={anchor.X-px,anchor.Y-py};
                game::map_object_changed();game::building_voxel_plan(b,p);
            });
            EXPECT_EQ(p.count,unsigned(count));
            // Indexed requests use absolute target coordinates; shape requests
            // use relative coordinates. A nonzero clip origin catches mixing them.
            const RectangleStruct clip{37,19,640,480};
            const auto output=draw(view,&clip);
            EXPECT_EQ(output.indexed.size(),std::size_t(count));
            for(unsigned n=0;n<p.count&&n<expected.size()&&n<output.indexed.size();++n){
                game::VoxelSurface surface;
                ASSERT_EQ(game::render_building_voxel(p.parts[n],view.world->impl->voxel_palette,surface),game::DrawingStatus::drawn);
                EXPECT_EQ(p.parts[n].barrel,bool(expected[n].barrel));
                EXPECT_EQ(output.indexed[n].position,(Point2D{expected[n].x+surface.offset.X+clip.X,expected[n].y+surface.offset.Y+clip.Y}));
            }
            ++cases;
        }
        EXPECT_EQ(cases,45);
    });
}

TEST(BuildingVisual, HousePaletteDoesNotDependOnRemapable) {
    session([](const auto&){},[](auto&view){
        ASSERT_TRUE(game::set_map_viewport(view,640,480));
        native(view,[&]{
            auto&b=placed();auto&t=*b.Type;ASSERT_NE(b.Owner,nullptr);
            view.world->rules_ini.WriteString(b.Owner->PlainName,"Color","DarkBlue");
            view.world->rules_ini.WriteString("Colors","DarkBlue","153,214,212");
            const BytePalette* first=nullptr;
            for(bool remapable:{false,true}){
                t.Remapable=remapable;game::map_object_changed();game::rebuild_world_sprites(*view.world);
                const BytePalette* palette=nullptr;
                for(const auto&s:view.world->impl->sprites)if(s.owner==&b&&s.image==t.Image&&!s.shadow)palette=s.palette;
                ASSERT_NE(palette,nullptr);EXPECT_NE(palette,&view.world->impl->unit_palette);
                if(first)EXPECT_EQ(palette,first);first=palette;
                EXPECT_GT(palette->Entries[16].B,palette->Entries[16].R);
                EXPECT_EQ(palette->Entries[40],view.world->impl->unit_palette.Entries[40]);
            }
            t.TerrainPalette=true;game::map_object_changed();game::rebuild_world_sprites(*view.world);
            for(const auto&s:view.world->impl->sprites)if(s.owner==&b&&s.image==t.Image&&!s.shadow)
                EXPECT_EQ(s.palette,&FileSystem::ISOx_PAL);
        });
    });
}

namespace {
void damage_fire_fixture(const std::filesystem::path&root){
    edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nCanBeOccupied=yes\n");
    std::ofstream(root/"RULESMD.INI",std::ios::app)<<"\n[General]\nDamageFireTypes=FIREA,FIREB,FIREC\n";
    edit(root/"ARTMD.INI","[BLDG]\n","[BLDG]\nDamageFireOffset0=1,27\nDamageFireOffset1=24,-17\nDamageFireOffset2=33,47\nDamageFireOffset4=99,99\n");
    std::ofstream art(root/"ARTMD.INI",std::ios::app);
    int n=0;for(const char*name:{"FIREA","FIREB","FIREC"}){
        shp(root/(std::string(name)+".SHP"),16);
        art<<'['<<name<<"]\nShadow=no\nRate=900\nLoopCount=-1\nEnd="<<8+4*n<<"\nLoopEnd="<<8+4*n<<"\n";++n;
    }
}
int fire_count(const BuildingClass&b){int n=0;for(auto*a:b.DamageFireAnims)n+=a!=nullptr;return n;}
}

TEST(BuildingVisual, DamageFireOriginalCoordinatesDepthAndRandom){
    session(damage_fire_fixture,[](auto&view){
        std::ifstream in(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_damage_fire_reference.txt");
        std::string tag;in>>tag;ASSERT_EQ(tag,"BUILDING_DAMAGE_FIRES_V1");
        int foundation,height,seed,mode,count,cases=0;unsigned next;
        while(in>>foundation>>height>>seed>>mode>>count>>next){
            native(view,[&]{
                auto&b=placed();auto&t=*b.Type;auto&r=*RulesClass::Instance;
                ASSERT_EQ(r.DamageFireTypes.Count,3);
                for(auto*&a:b.DamageFireAnims){auto*old=a;a=nullptr;delete old;}
                t.Foundation=static_cast<Foundation>(foundation);b.Location={2176,896,height};
                const Point2D offsets[]={{1,27},{24,-17},{33,47},{-3,-2},{5,0},{0,10},{120,-200},{-7,41}};
                std::copy(std::begin(offsets),std::end(offsets),t.DamageFireOffset);
                if(mode==1)t.DamageFireOffset[2]={0,0};
                if(mode==2)b.DamageFireAnims[2]=new AnimClass(r.DamageFireTypes[0],b.Location);
                view.scenario.Random=Randomizer(seed);b.CreateDamageFires();
                EXPECT_EQ(fire_count(b),count+(mode==2?1:0));
                for(int n=0;n<count;++n){
                    int type,x,y,z,adjust,frame,delay,loops,flags,force,reverse;
                    in>>type>>x>>y>>z>>adjust>>frame>>delay>>loops>>flags>>force>>reverse;
                    auto*a=b.DamageFireAnims[n];ASSERT_NE(a,nullptr);
                    EXPECT_EQ(a->Type,r.DamageFireTypes[type]);EXPECT_EQ(a->Location,(CoordStruct{x,y,z}));
                    EXPECT_EQ(a->ZAdjust,adjust);EXPECT_EQ(a->Animation.Value,frame);
                    EXPECT_EQ(int(a->AnimFlags),flags);EXPECT_EQ(a->LoopDelay,delay);
                    EXPECT_EQ(loops,1);EXPECT_EQ(force,0);EXPECT_EQ(int(a->Reverse),reverse);
                    EXPECT_EQ(a->OwnerObject,nullptr);EXPECT_FALSE(a->IsBuildingAnim);
                }
                EXPECT_EQ(static_cast<unsigned>(view.scenario.Random.Random()),next);
            });
            ASSERT_TRUE(bool(in));++cases;
        }
        EXPECT_EQ(cases,396);
    });
}

TEST(BuildingVisual, DamageFireHealthTransitionsAndDrawing){
    session([](const auto&root){health_rules_fixture(root);damage_fire_fixture(root);},[](auto&view){
        native(view,[]{auto&b=placed();
            EXPECT_EQ(b.Type->DamageFireOffset[0],(Point2D{1,27}));
            EXPECT_EQ(b.Type->DamageFireOffset[2],(Point2D{33,47}));
            EXPECT_EQ(b.Type->DamageFireOffset[3],(Point2D{0,0}));
            EXPECT_EQ(b.Type->DamageFireOffset[4],(Point2D{0,0}));
            b.Health=26;b.Update();EXPECT_EQ(fire_count(b),0);
            b.Health=25;b.Update();EXPECT_EQ(fire_count(b),3);EXPECT_TRUE(b.RequiresDamageFires);
            auto*first=b.DamageFireAnims[0];b.Update();EXPECT_EQ(b.DamageFireAnims[0],first);
        });
        const auto output=draw(view);int drawn=0;
        for(auto*a:placed().DamageFireAnims)if(a){
            for(const auto&r:output.shapes)if(r.image==a->Type->Image){
                auto expected=TacticalClass::CoordsToScreen(a->Location);
                expected.X-=view.tactical.TacticalPos.X;expected.Y-=view.tactical.TacticalPos.Y;
                EXPECT_EQ(r.position,expected);EXPECT_EQ(r.depth_adjustment,a->ZAdjust-TacticalClass::AdjustForZ(a->Location.Z)-2);
                EXPECT_EQ(r.flags,0x2E00u);++drawn;
            }
        }
        EXPECT_EQ(drawn,3);
        native(view,[]{auto&b=placed();
            b.Health=26;b.Update();EXPECT_EQ(fire_count(b),0);EXPECT_FALSE(b.RequiresDamageFires);
            b.Type->CanBeOccupied=false;b.Health=51;b.Update();EXPECT_EQ(fire_count(b),0);
            b.Health=50;b.Update();EXPECT_EQ(fire_count(b),3);
            b.Health=100;b.Update();EXPECT_EQ(fire_count(b),0);
            b.Health=25;b.Update();EXPECT_EQ(fire_count(b),3);
            // A free animation can expire without its building being removed.
            auto*a=b.DamageFireAnims[0];delete a;EXPECT_EQ(b.DamageFireAnims[0],nullptr);
            b.Update();EXPECT_EQ(fire_count(b),2); // original only creates on the transition
            game::detach_map_object(b);EXPECT_EQ(fire_count(b),0);EXPECT_FALSE(b.RequiresDamageFires);
            game::attach_map_object(b);b.Update();EXPECT_EQ(fire_count(b),3);
            b.Health=0;b.UpdateDamageFires();EXPECT_EQ(fire_count(b),0);
        });
    });
}

TEST(BuildingVisual, DamageFireExpirationAndMapReload){
    session(damage_fire_fixture,[](auto&view){
        native(view,[]{
            for(auto*t:RulesClass::Instance->DamageFireTypes)t->LoopCount=1;
            placed().Health=25;placed().Update();ASSERT_EQ(fire_count(placed()),3);
        });
        ticks(view,80);
        native(view,[]{
            EXPECT_EQ(fire_count(placed()),0);EXPECT_TRUE(placed().RequiresDamageFires);
            placed().Health=100;placed().Update();
            placed().Health=25;placed().Update();EXPECT_EQ(fire_count(placed()),3);
        });
        // Live fire references and the rule type list must not survive a map load.
        ASSERT_TRUE(game::load_map_view(view,"world.map",9));
        native(view,[]{
            EXPECT_EQ(fire_count(placed()),0);EXPECT_EQ(RulesClass::Instance->DamageFireTypes.Count,3);
            placed().Health=25;placed().Update();EXPECT_EQ(fire_count(placed()),3);
        });
    });
}

TEST(BuildingVisual, AnimationLayersPrecedeScreenSorting){
    session([](const auto&root){
        damage_fire_fixture(root);
        edit(root/"ARTMD.INI","[FIREA]\n","[FIREA]\nLayer=Top\n");
        edit(root/"ARTMD.INI","[FIREB]\n","[FIREB]\nLayer=Air\n");
        edit(root/"ARTMD.INI","[FIREC]\n","[FIREC]\nLayer=Surface\n");
        edit(root/"ARTMD.INI","[SPINDAM]\n","[SPINDAM]\nLayer=Top\n");
    },[](auto&view){
        ASSERT_TRUE(game::set_map_viewport(view,640,480));
        native(view,[&]{
            auto&b=placed();b.Health=25;b.Update();ASSERT_EQ(fire_count(b),3);
            EXPECT_EQ(AnimTypeClass::Find("FIREA")->Layer,Layer::Top);
            EXPECT_EQ(AnimTypeClass::Find("FIREB")->Layer,Layer::Air);
            EXPECT_EQ(AnimTypeClass::Find("FIREC")->Layer,Layer::Surface);
            for(auto*a:b.DamageFireAnims)if(a){
                // Deliberately conflicting Y order cannot cross an object layer.
                a->YSortAdjust=a->Type->Layer==Layer::Top?-10000:10000;
                EXPECT_EQ(a->InWhichLayer(),a->Type->Layer);
            }
            ASSERT_NE(b.Anims[3],nullptr);EXPECT_EQ(b.Anims[3]->Type->Layer,Layer::Top);
            EXPECT_EQ(b.Anims[3]->InWhichLayer(),Layer::Ground);
            game::map_object_changed();
            game::TacticalDrawingFrame frame;frame.world=view.world.get();frame.bounds={0,0,640,480};
            ASSERT_EQ(game::with_tactical_drawing(frame,[](void* p){static_cast<TacticalClass*>(p)->BuildDrawRequests();},&view.tactical),game::DrawingStatus::drawn);
            Layer previous=Layer::None;int fires=0,attached=0;
            // Background precedes every foreground layer, including Surface.
            for(std::size_t i=frame.background_requests;i<view.world->impl->sprites.size();++i){
                const auto& s=view.world->impl->sprites[i];
                EXPECT_GE(int(s.layer),int(previous));previous=s.layer;
                for(auto*a:b.DamageFireAnims)if(a&&s.image==a->Type->Image){EXPECT_EQ(s.layer,a->Type->Layer);++fires;}
                if(s.image==b.Anims[3]->Type->Image){EXPECT_EQ(s.layer,Layer::Ground);++attached;}
            }
            EXPECT_EQ(fires,3);EXPECT_GT(attached,0);
        });
    });
}

#if defined(RA2_VISUAL_SOFTWARE)
TEST(BuildingVisual, DamageFireOverBuildingPixels){
    session(damage_fire_fixture,[](auto&view){
        native(view,[]{
            auto&b=placed();b.Type->DamageFireOffset[0]={0,-1};b.Type->DamageFireOffset[1]={0,0};
            b.Type->NoShadow=true;b.Health=25;b.Update();ASSERT_EQ(fire_count(b),1);
            b.DamageFireAnims[0]->Animation.Value=5;game::map_object_changed();
        });
        const auto output=draw(view);auto&b=placed();auto*fire=b.DamageFireAnims[0];
        const auto origin=body_request(output).position;
        std::vector<game::ShapeDrawingRequest> pair;
        for(const auto&r:output.shapes)if(r.image==b.Type->Image||r.image==fire->Type->Image)pair.push_back(r);
        ASSERT_EQ(pair.size(),2u);EXPECT_EQ(pair[0].image,b.Type->Image);EXPECT_EQ(pair[1].image,fire->Type->Image);
        int gradients[5][6]{};
        std::ifstream table(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"game_ui_z_gradients.txt");
        std::string comment;std::getline(table,comment);
        for(auto&row:gradients)for(auto&v:row)table>>v;
        ASSERT_TRUE(bool(table));
        auto*saved_gradients=Drawing::ZGradientTable;Drawing::ZGradientTable=gradients;
        struct RestoreGradients {const int (*saved)[6];~RestoreGradients(){Drawing::ZGradientTable=saved;}} restore{saved_gradients};
        Software f;
        const auto submit=[&](game::ShapeDrawingRequest r){
            r.target=f.context.target;r.palette=f.context.palette;
            r.position.X+=4-origin.X;r.position.Y+=4-origin.Y;r.clip={0,0,8,8};
            EXPECT_EQ(game::submit_type_shape(f.context,r),game::DrawingStatus::drawn);
        };
        auto fire_request=*std::find_if(pair.begin(),pair.end(),[&](const auto&r){return r.image==fire->Type->Image;});
        // Measure the fire's visible pixel, then composite the actual production
        // request sequence. Previously the later Ground body erased this pixel.
        f.depth.Fill(0xFFFF);submit(fire_request);const WORD expected=f.pixel(4,4);
        ASSERT_NE(expected,0x1234);
        f.target.Fill(0x1234);f.depth.Fill(0xFFFF);
        submit(pair[0]);const WORD building_pixel=f.pixel(4,4),building_z=f.z(4,4);
        submit(pair[1]);EXPECT_NE(building_pixel,expected);EXPECT_EQ(f.pixel(4,4),expected);
        EXPECT_EQ(f.z(4,4),building_z); // fire still reads, never overwrites scene Z
        f.target.Fill(0x1234);f.depth.Fill(0);
        submit(fire_request);EXPECT_EQ(f.pixel(4,4),0x1234); // nearer geometry still occludes it
    });
}
#endif

TEST(BuildingVisual, FactoryGateAndConstructionOriginalRequests){
    session([](const auto&root){
        edit(root/"ARTMD.INI","[BLDG]\n","[BLDG]\nBibShape=BBIB\nDeployingAnim=DEPLOY\nUnderDoorAnim=UNDER\nRoofDeployingAnim=ROOF\nUnderRoofDoorAnim=UNDERROOF\nDoorAnim=DOOR\nDoorStages=8\nDamagedDoor=yes\n");
        for(const char* name:{"BLDG","BBIB","DEPLOY","UNDER","ROOF","UNDERROOF","DOOR"})shp(root/(std::string(name)+".SHP"),32);
    },[](auto&view){native(view,[&]{
        auto&b=placed();auto&t=*b.Type;
        ASSERT_EQ(t.DoorStages,8);ASSERT_TRUE(t.DamagedDoor);
        SHPStruct*images[]={t.Image,t.BibShape,t.DeployingAnim,t.UnderDoorAnim,t.RoofDeployingAnim,t.UnderRoofDoorAnim,t.DoorAnim};
        for(auto* image:images)ASSERT_NE(image,nullptr);
        b.RadioLinks.SetCapacity(1);b.Location.Z=104;t.Foundation=static_cast<Foundation>(3);
        t.NormalZAdjust=-7;t.ZShapePointMove={3,-2};t.ExtraLight=120;t.GateStages=8;
        std::ifstream in(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_parts_reference.txt");std::string tag;in>>tag;ASSERT_EQ(tag,"BUILDING_PARTS_V1");
        int mission,roof,parts,health,flags,elapsed,frame,no_shadow,count,cases=0;
        while(in>>mission>>roof>>parts>>health>>flags>>elapsed>>frame>>no_shadow>>count){
            SCOPED_TRACE(::testing::Message()<<"case "<<cases<<" mission "<<mission<<" roof "<<roof<<" states "<<flags<<" time "<<elapsed);
            b.CurrentMission=static_cast<Mission>(mission);b.QueuedMission=Mission::None;b.BState=1;b.Animation.Value=frame;
            b.Health=health;t.Strength=100;t.NoShadow=no_shadow;t.JumpJet=roof==1;t.ConsideredAircraft=roof==2;b.RadioLinks[0]=roof?&b:nullptr;
            t.BibShape=parts?images[1]:nullptr;t.DeployingAnim=parts?images[2]:nullptr;t.UnderDoorAnim=parts?images[3]:nullptr;
            t.RoofDeployingAnim=parts?images[4]:nullptr;t.UnderRoofDoorAnim=parts?images[5]:nullptr;t.DoorAnim=parts?images[6]:nullptr;
            // This reference doubles GetCurrentFrame. Use the neutral body state
            // while retaining real health for lower-door and damaged-door paths.
            if(health<=50)b.Animation.Value=frame-1;
            Unsorted::CurrentFrame=100;b.UnloadTimer.Rate1=30;b.UnloadTimer.Rate2=30;
            b.UnloadTimer.ActionTimer.StartTime=100-elapsed;b.UnloadTimer.ActionTimer.TimeLeft=30;
            b.UnloadTimer.State1=flags&1;b.UnloadTimer.State2=flags&2;
            std::vector<game::ShapeDrawingRequest> actual;game::TypeDrawingContext c;
            c.target=reinterpret_cast<game::DrawingTargetHandle*>(&actual);c.palette=reinterpret_cast<const game::DrawingPaletteHandle*>(&actual);c.backend_context=&actual;
            c.backend.shape=[](void* context,const game::ShapeDrawingRequest&r){static_cast<std::vector<game::ShapeDrawingRequest>*>(context)->push_back(r);return game::DrawingStatus::drawn;};
            for(bool upper:{false,true}){const auto status=game::draw_building_parts(b,c,view.world->impl->building_zshape,800,{50,50},{0,0,128,128},upper);ASSERT_TRUE(status==game::DrawingStatus::drawn||status==game::DrawingStatus::skipped);}
            ASSERT_EQ(actual.size(),std::size_t(count));
            for(int j=0;j<count;++j){int image,f,x,y,z,gradient,intensity,aux,dx,dy;std::string draw_flags;
                in>>image>>f>>x>>y>>draw_flags>>z>>gradient>>intensity>>aux>>dx>>dy;
                const auto&r=actual[j];EXPECT_EQ(r.image,images[image]);EXPECT_EQ(r.frame,f);EXPECT_EQ(r.position,(Point2D{x,y}));EXPECT_EQ(r.flags,std::stoul(draw_flags,nullptr,0));
                EXPECT_EQ(r.depth_adjustment,z);EXPECT_EQ(r.gradient,gradient);EXPECT_EQ(r.intensity,intensity);EXPECT_EQ(r.depth_image!=nullptr,aux!=0);if(aux)EXPECT_EQ(r.depth_offset,(Point2D{dx,dy}));
            }
            ASSERT_TRUE(bool(in));++cases;
        }
        EXPECT_EQ(cases,410);
        // Type owns these images; restore every borrowed pointer before teardown.
        t.BibShape=images[1];t.DeployingAnim=images[2];t.UnderDoorAnim=images[3];t.RoofDeployingAnim=images[4];t.UnderRoofDoorAnim=images[5];t.DoorAnim=images[6];b.RadioLinks[0]=nullptr;
    });});
}

TEST(BuildingVisual, TransitionTimerOriginalStateAndFloatingResults){
    std::ifstream in(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"transition_timer_reference.txt");std::string tag;in>>tag;ASSERT_EQ(tag,"TRANSITION_TIMER_V1");
    const int saved=Unsorted::CurrentFrame;Unsorted::CurrentFrame=100;
    struct Restore {int frame;~Restore(){Unsorted::CurrentFrame=frame;}} restore{saved};
    int states,elapsed,op,start,left,finished,s1,s2,cases=0;unsigned total;std::string time,rate,percent;
    while(in>>states>>elapsed>>time>>op>>rate>>start>>left>>total>>s1>>s2>>percent>>finished){
        SCOPED_TRACE(::testing::Message()<<"case "<<cases<<" op "<<op<<" state "<<states<<" time "<<elapsed);
        TransitionTimer timer;timer.Rate1=30;timer.Rate2=30;timer.State1=states&1;timer.State2=states&2;
        timer.ActionTimer.StartTime=elapsed==-1?-1:100-elapsed;timer.ActionTimer.TimeLeft=30;
        const double duration=std::bit_cast<double>(std::uint64_t(std::stoull(time,nullptr,0)));
        switch(op){case 0:timer.StartTimer11(duration);break;case 1:timer.StartTimer10(duration);break;case 2:timer.Update();break;case 3:timer.SetToDone();break;}
        EXPECT_EQ(std::bit_cast<std::uint64_t>(timer.Rate1),std::stoull(rate,nullptr,0));
        EXPECT_EQ(timer.ActionTimer.StartTime,start);EXPECT_EQ(timer.ActionTimer.TimeLeft,left);EXPECT_EQ(timer.Rate2,total);
        EXPECT_EQ(timer.State1,s1!=0);EXPECT_EQ(timer.State2,s2!=0);EXPECT_EQ(timer.IsTimerFinished(),finished!=0);
        EXPECT_EQ(std::bit_cast<std::uint64_t>(timer.PercentageDone()),std::stoull(percent,nullptr,0));++cases;
    }
    EXPECT_EQ(cases,784);
}

TEST(BuildingVisual, BuildingBeginModeAndSaleOriginalStates){
    session([](const auto&){},[](auto&view){native(view,[&]{
        auto&b=placed();auto&t=*b.Type;const int saved_speed=GameOptionsClass::Instance.GameSpeed;
        struct Restore {int speed;~Restore(){GameOptionsClass::Instance.GameSpeed=speed;}}restore{saved_speed};
        Unsorted::CurrentFrame=100;Unsorted::ScenarioInit=0;
        for(int i=0;i<6;++i){t.BuildingAnimFrame[i].dwUnknown=i*3;t.BuildingAnimFrame[i].FrameCount=7;t.BuildingAnimFrame[i].FrameDuration=i+1;}
        std::ifstream in(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_stage_reference.txt");std::string tag;in>>tag;ASSERT_EQ(tag,"BUILDING_STAGE_V1");int cases=0;
        while(in>>tag){
            if(tag=="M"){
                int old,state,normalized,speed,bs,qs,value,start,left,rate;in>>old>>state>>normalized>>speed>>bs>>qs>>value>>start>>left>>rate;
                b.BState=old;b.QueueBState=2;b.Animation.Value=73;b.Animation.Timer.StartTime=50;b.Animation.Timer.TimeLeft=12;b.Animation.Rate=13;
                t.Normalized=normalized;GameOptionsClass::Instance.GameSpeed=speed;b.BeginMode(static_cast<BStateType>(state));
                EXPECT_EQ(b.BState,bs);EXPECT_EQ(b.QueueBState,qs);EXPECT_EQ(b.Animation.Value,value);EXPECT_EQ(b.Animation.Rate,rate);
                EXPECT_EQ(b.Animation.Timer.StartTime,start);EXPECT_EQ(b.Animation.Timer.TimeLeft,left);
            }else{
                ASSERT_EQ(tag,"F");int gate,mission,start,length,frame,expected;in>>gate>>mission>>start>>length>>frame>>expected;
                b.BState=0;b.CurrentMission=static_cast<Mission>(mission);b.Animation.Value=frame;t.Gate=gate;
                t.BuildingAnimFrame[0].dwUnknown=start;t.BuildingAnimFrame[0].FrameCount=length;
                EXPECT_EQ(b.GetCurrentFrame(),expected);
            }
            ASSERT_TRUE(bool(in));++cases;
        }
        EXPECT_EQ(cases,432);
    });});
}

TEST(BuildingVisual, BuildupResourceTimingAndOfflineProgress){
    session([](const auto&root){
        edit(root/"ARTMD.INI","[BLDG]\n","[BLDG]\nBuildup=CONSTR\n");shp(root/"CONSTR.SHP",30);
    },[](auto&view){native(view,[&]{
        auto&b=placed();auto&t=*b.Type;auto&sequence=t.BuildingAnimFrame[0];
        ASSERT_EQ(sequence.FrameCount,15);ASSERT_EQ(sequence.FrameDuration,3);ASSERT_EQ(sequence.dwUnknown,0u);
        b.CurrentMission=Mission::Construction;b.BeginMode(BStateType::Construction);b.IsReadyToCommence=false;b.HasPower=false;
        ASSERT_FALSE(b.IsPowerOnline());ASSERT_EQ(b.GetImage(),t.Buildup);
        const int begin=Unsorted::CurrentFrame;
        for(int i=1;i<=42;++i){Unsorted::CurrentFrame=begin+i;b.Update();EXPECT_EQ(b.Animation.Value,i/3);EXPECT_EQ(b.IsReadyToCommence,i==42);}
        // A pending idle state consumes the queue after the last construction
        // frame, before the next frame could wrap back to the beginning.
        b.BeginMode(BStateType::Idle);EXPECT_EQ(b.BState,0);EXPECT_EQ(b.QueueBState,1);
        ++Unsorted::CurrentFrame;b.Update();EXPECT_EQ(b.BState,1);EXPECT_EQ(b.QueueBState,-1);EXPECT_EQ(b.GetImage(),t.Image);
        // The door runs on CurrentFrame and invalidates cached world sprites.
        const auto revision=view.world->presentation_revision;b.UnloadTimer.StartTimer11(1.0/30.0);
        Unsorted::CurrentFrame+=30;b.Update();EXPECT_TRUE(b.UnloadTimer.AreStates01());EXPECT_GT(view.world->presentation_revision,revision);
    });});
}

TEST(BuildingVisual, RepairWrenchWithoutSelectionAndShroud){
    session([](const auto&root){shp(root/"WRENCH.SHP",7);},[](auto&view){
        const int saved_speed=GameOptionsClass::Instance.GameSpeed;
        struct RestoreSpeed {int value;~RestoreSpeed(){GameOptionsClass::Instance.GameSpeed=value;}}restore_speed{saved_speed};
        auto*wrench=view.world->impl->repair_wrench;ASSERT_NE(wrench,nullptr);
        native(view,[]{placed().IsBeingRepaired=true;placed().IsSelected=false;});
        for(int speed=0;speed<7;++speed)for(int frame=0;frame<30;++frame){
            native(view,[&]{GameOptionsClass::Instance.GameSpeed=speed;Unsorted::CurrentFrame=frame;});
            const auto output=draw(view);std::vector<game::ShapeDrawingRequest>requests;
            for(const auto&r:output.shapes)if(r.image==wrench)requests.push_back(r);
            ASSERT_EQ(requests.size(),2u);const auto&r=requests[0];
            // Literal original period table for rate 14 (0x005FB2E0).
            constexpr int periods[]={28,14,9,7,5,4,4};
            EXPECT_EQ(r.frame,6*(frame%periods[speed])/(periods[speed]-1));EXPECT_EQ(r.flags,0xE00u);EXPECT_EQ(r.depth_adjustment,0);EXPECT_EQ(r.gradient,0);EXPECT_EQ(r.intensity,1000);
            auto p=TacticalClass::CoordsToScreen(placed().GetRenderCoords());p.X-=view.tactical.TacticalPos.X;p.Y-=view.tactical.TacticalPos.Y;EXPECT_EQ(r.position,p);
        }
        native(view,[]{placed().IsSinking=true;});
        for(const auto&r:draw(view).shapes)EXPECT_NE(r.image,wrench);
        native(view,[]{placed().IsSinking=false;auto*c=MapClass::Instance.TryGetCellAt(placed().Location);c->AltFlags=AltCellFlags{};c->ShroudCounter=1;});
        for(const auto&r:draw(view).shapes)EXPECT_NE(r.image,wrench);
    });
}

TEST(BuildingVisual, PoweredLightEffectAndRestoreAreDistinct){
    session([](const auto&root){
        edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nPowered=yes\nPower=-10\n");
        edit(root/"ARTMD.INI","[BLDG]\n","[BLDG]\nActiveAnimTwo=SPIN\nActiveAnimTwoPowered=no\nActiveAnimTwoPoweredLight=yes\nActiveAnimThree=SPIN\nActiveAnimThreePowered=no\nActiveAnimThreePoweredEffect=yes\nSuperAnimThree=SPIN\nSuperAnimThreePowered=no\nSuperAnimThreePoweredEffect=yes\nSuperLowPower=SPIN\n");
    },[](auto&view){native(view,[&]{
        auto&b=placed();auto&h=*b.Owner;h.PowerOutput=100;h.PowerDrain=10;b.UpdateAnimations();
        b.PlayNthAnim(static_cast<BuildingAnimSlot>(16),false,false);
        ASSERT_NE(b.Anims[3],nullptr);ASSERT_NE(b.Anims[4],nullptr);ASSERT_NE(b.Anims[5],nullptr);ASSERT_NE(b.Anims[16],nullptr);
        auto*paused=b.Anims[3];b.Anims[3]->Animation.Value=1;
        h.PowerOutput=0;b.UpdateAnimations();
        EXPECT_EQ(b.Anims[3],paused);EXPECT_TRUE(paused->PowerOff);EXPECT_EQ(paused->Animation.Value,1);
        EXPECT_EQ(b.Anims[4],nullptr);EXPECT_FALSE(b.AnimStates[4]);
        EXPECT_EQ(b.Anims[5],nullptr);EXPECT_TRUE(b.AnimStates[5]);
        EXPECT_EQ(b.Anims[16],nullptr);EXPECT_TRUE(b.AnimStates[16]);EXPECT_NE(b.Anims[20],nullptr);
        // Repeated offline/health updates must not respawn an erased effect.
        b.UpdateAnimations();EXPECT_EQ(b.Anims[5],nullptr);EXPECT_EQ(b.Anims[16],nullptr);
        h.PowerOutput=100;b.UpdateAnimations();
        EXPECT_EQ(b.Anims[3],paused);EXPECT_FALSE(paused->PowerOff);EXPECT_EQ(paused->Animation.Value,1);
        EXPECT_NE(b.Anims[4],nullptr);EXPECT_NE(b.Anims[5],nullptr);EXPECT_NE(b.Anims[16],nullptr);
        EXPECT_FALSE(b.AnimStates[5]);EXPECT_FALSE(b.AnimStates[16]);EXPECT_EQ(b.Anims[20],nullptr);
    });});
}

#include "yrpp/LightSourceClass.h"
#include "yrpp/SpotlightClass.h"
TEST(BuildingVisual, L01_static_light_lifecycle_and_overlap) {
 const int before=LightSourceClass::Array.Count;
 session([](const auto&root){edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nLightVisibility=768\nLightIntensity=0.5\nLightRedTint=0.2\nLightGreenTint=-0.1\nLightBlueTint=0\n");},[](auto&v){native(v,[&]{
  auto&b=placed();ASSERT_TRUE(b.LightSource);EXPECT_TRUE(b.LightSource->Activated);
  EXPECT_EQ(b.Type->LightIntensity,500);EXPECT_EQ(b.Type->LightGreenTint,-99);
  auto*c=MapClass::Instance.TryGetCellAt(b.Location);ASSERT_TRUE(c);
  const auto ambient=c->Ambient;EXPECT_TRUE(ambient>0);
  auto*source=b.LightSource;source->Deactivate();EXPECT_EQ(c->Ambient,0);
  source->Activate();EXPECT_EQ(c->Ambient,ambient);
  b.StuffEnabled=false;b.UpdateAnimations();EXPECT_EQ(c->Ambient,0);
  b.StuffEnabled=true;b.UpdateAnimations();EXPECT_EQ(c->Ambient,ambient);
  {LightSourceClass dark(source->Location,768,-250,{0,0,0});dark.Activate();EXPECT_TRUE(c->Ambient<ambient);}
  EXPECT_EQ(c->Ambient,ambient);
  GameOptionsClass::Instance.DetailLevel=0;LightSourceClass::UpdateLightConverts(0,true);EXPECT_EQ(c->Ambient,0);
  GameOptionsClass::Instance.DetailLevel=2;LightSourceClass::UpdateLightConverts(0);EXPECT_EQ(c->Ambient,ambient);
  game::detach_map_object(b);EXPECT_FALSE(b.LightSource);EXPECT_EQ(c->Ambient,0);
 });});EXPECT_EQ(LightSourceClass::Array.Count,before);
}
TEST(BuildingVisual, L02_spotlight_motion_power_and_cleanup) {
 const int before=BuildingLightClass::Array.Count;
 session([](const auto&root){edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nHasSpotlight=yes\n");},[](auto&v){
  CoordStruct first{};native(v,[&]{auto&b=placed();ASSERT_TRUE(b.Spotlight);first=b.Spotlight->Location;});
  ticks(v,8);native(v,[&]{EXPECT_TRUE(placed().Spotlight->Location!=first);});
  native(v,[&]{auto&b=placed();b.Spotlight->Location=b.GetCenterCoords();EXPECT_TRUE(b.IsPowerOnline());EXPECT_FALSE(b.IsFogged);EXPECT_EQ(b.Spotlight->OwnerObject,&b);EXPECT_TRUE(TacticalClass::Instance);});
  auto output=draw(v);EXPECT_TRUE(std::any_of(output.rasters.begin(),output.rasters.end(),[](const auto&r){return r.blend_mode==game::RasterBlendMode::spotlight;}));
  native(v,[&]{placed().HasPower=false;placed().StuffEnabled=false;});
  output=draw(v);EXPECT_FALSE(std::any_of(output.rasters.begin(),output.rasters.end(),[](const auto&r){return r.blend_mode==game::RasterBlendMode::spotlight;}));
  native(v,[&]{game::detach_map_object(placed());EXPECT_FALSE(placed().Spotlight);});
 });EXPECT_EQ(BuildingLightClass::Array.Count,before);
}
#if defined(RA2_VISUAL_SOFTWARE)
TEST(BuildingVisual, L03_RGB565_glow_depth_and_channel_masks) {
 Software f;f.target.Fill(0x4208);f.depth.Fill(0xffff);
 game::RasterDrawingRequest r;r.target=f.context.target;r.position={1,1};r.clip={0,0,8,8};r.width=r.height=1;r.color=128;r.blend_mode=game::RasterBlendMode::spotlight;
 EXPECT_EQ(game::submit_type_raster(f.context,r),game::DrawingStatus::drawn);EXPECT_EQ(f.pixel(1,1),0x630c);
 f.target.Fill(0x4208);r.spotlight_flags=unsigned(SpotlightFlags::NoRed);game::submit_type_raster(f.context,r);EXPECT_EQ(f.pixel(1,1),0x430c);
 f.target.Fill(0x4208);r.spotlight_flags=unsigned(SpotlightFlags::NoColor);game::submit_type_raster(f.context,r);EXPECT_EQ(f.pixel(1,1),0x2104);
 r.blend_mode=game::RasterBlendMode::depth_glow;r.light_strength=128;r.spotlight_flags=0;r.line_z=-15;f.z(1,1)=0x8000-1-15;
 f.target.Fill(0x4208);EXPECT_EQ(game::submit_type_raster(f.context,r),game::DrawingStatus::skipped);EXPECT_EQ(f.pixel(1,1),0x4208);
 f.z(1,1)++;EXPECT_EQ(game::submit_type_raster(f.context,r),game::DrawingStatus::drawn);EXPECT_EQ(f.pixel(1,1),0x630c);
 f.target.Fill(0x4208);r.light_strength=-3;EXPECT_EQ(game::submit_type_raster(f.context,r),game::DrawingStatus::drawn);EXPECT_EQ(f.pixel(1,1),0x39e7);
}
#endif
TEST(BuildingVisual, D01_destruction_animation_delay_rubble_and_lights) {
 session([](const auto&root){
  std::string overlays;
  for(int i=114;i<239;++i)overlays+=std::to_string(i)+"=ORE"+std::to_string(i)+"\n";
  overlays+="239=RUBBLE_OVERLAY\n";
  edit(root/"RULESMD.INI","[ORE102]",overlays+"[ORE102]");
  std::ofstream(root/"RULESMD.INI",std::ios::app)<<"\n[RUBBLE_OVERLAY]\nImage=SROCK01\nTheater=no\nDrawFlat=yes\nIsRubble=yes\n";
  shp(root/"SROCK01.SHP",2);
  edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nHasSpotlight=yes\nLightIntensity=0.5\nLightRedTint=0\nLightGreenTint=0\nLightBlueTint=0\nLeaveRubble=yes\nExplosion=DEATHFX\nDestroyAnim=FALLFX\n");
  std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[DEATHFX]\nEnd=8\nRate=1\nLoopEnd=8\nShadow=no\n\n[FALLFX]\nEnd=8\nRate=1\nLoopEnd=8\nShadow=no\n";
  shp(root/"DEATHFX.SHP",8);shp(root/"FALLFX.SHP",8);shp(root/"BLDG.SHP",8);
 },[](auto&v){
  game::MapObjectId id{};int previous=0;BuildingTypeClass* type=nullptr;CoordStruct location{};
  native(v,[&]{id=game::object_id(*v.world,&placed());previous=AnimClass::Array.Count;type=placed().Type;location=placed().Location;});
  ASSERT_TRUE(game::set_map_object_health(v,id,0));
  native(v,[&]{auto&b=placed();EXPECT_EQ(b.Health,0);EXPECT_TRUE(b.IsOnMap);EXPECT_EQ(b.C4Timer.GetTimeLeft(),8);EXPECT_FALSE(b.LightSource);EXPECT_FALSE(b.Spotlight);EXPECT_TRUE(AnimClass::Array.Count>previous);});
  auto dying=draw(v);EXPECT_TRUE(std::any_of(dying.shapes.begin(),dying.shapes.end(),[](const auto&r){return r.image==placed().Type->Image;}));
  native(v,[&]{Unsorted::CurrentFrame+=7;placed().Update();EXPECT_TRUE(placed().IsOnMap);++Unsorted::CurrentFrame;placed().Update();EXPECT_EQ(BuildingClass::Array.Count,0);EXPECT_FALSE(game::resolve_map_object(*v.world,id));auto*c=MapClass::Instance.TryGetCellAt(location);ASSERT_TRUE(c);EXPECT_EQ(c->Rubble,type);EXPECT_EQ(c->OverlayTypeIndex,239);});
  auto output=draw(v);EXPECT_TRUE(std::any_of(output.shapes.begin(),output.shapes.end(),[&](const auto&r){return r.image==type->Image&&r.frame==3;}));
  EXPECT_TRUE(std::any_of(output.shapes.begin(),output.shapes.end(),[&](const auto&r){return r.image==type->Image&&r.frame==7&&(r.flags&1);}));
  // YR 0x0047F6A0 / 0x0047F510: non-anchor rubble cells have no
  // BuildingType pointer and draw nothing, including the SROCK01 placeholder.
  native(v,[&]{auto*overlay=OverlayTypeClass::Array[239];ASSERT_TRUE(overlay->IsRubble);
   EXPECT_FALSE(std::any_of(output.shapes.begin(),output.shapes.end(),[&](const auto&r){return r.image==overlay->GetImage();}));
  });
 });
}
TEST(BuildingVisual, DestructionUsesUnsignedAnimationIndex) {
 session([](const auto& root){
  std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[DEATHFX]\nEnd=8\nLoopEnd=8\nShadow=no\n";
  shp(root/"DEATHFX.SHP",8);
 },[](auto& v){native(v,[&]{
  auto& building=placed();auto* effect=AnimTypeClass::FindOrAllocate("DEATHFX");
  ASSERT_TRUE(effect->LoadFromINI(&v.world->art_ini));
  auto& random=ScenarioClass::Instance->Random;
  // Width 1 avoids scorch randomness; one random heading, then delay 0,
  // then high-bit-set selection. The original DIV selects item 1 of 3.
  building.Type->Foundation=static_cast<Foundation>(0);building.Type->Explosion.Clear();
  for(int i=0;i<3;++i)building.Type->Explosion.AddItem(effect);
  random.Next1=0;random.Next2=103;for(auto& value:random.Table)value=0;
  random.Table[2]=0xFFFFFFFEu;
  const CellStruct footprint[]={{0,0},{0x7FFF,0x7FFF}};
  building.Destory(nullptr,nullptr,false,footprint);
  int count=0;for(auto* anim:AnimClass::Array)if(anim->Type==effect)++count;
  EXPECT_EQ(count,1);
 });});
}
TEST(BuildingVisual, L04_original_binary_cell_light_reference) {
 session([](const auto&){},[](auto&v){native(v,[&]{
  std::ifstream in(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"building_lights_reference.txt");std::string magic;in>>magic;ASSERT_EQ(magic,"BUILDING_LIGHTS_V1");
  auto*scenario=ScenarioClass::Instance;const auto previous=scenario->NormalLighting;const auto oldAmbient=scenario->AmbientCurrent;const auto oldDetail=GameOptionsClass::Instance.DetailLevel;
  scenario->AmbientCurrent=75;scenario->NormalLighting.Tint={100,80,120};scenario->NormalLighting.Ground=20;scenario->NormalLighting.Level=30;
  LightSourceClass light({2688,2688,999},768,0,{200,-300,100});
  int x,level,intensity,activated,detail,count=0;
  while(in>>x>>level>>intensity>>activated>>detail){
   light.LightIntensity=intensity;light.Activated=activated;GameOptionsClass::Instance.DetailLevel=detail;
   struct LightTestCell:CellClass{LightTestCell():CellClass(){}};LightTestCell cell;cell.MapCoords={short(x),10};cell.Level=BYTE(level);int actual[8],expected[8];for(auto&n:expected)in>>n;
   cell.CalculateLightSourceLighting(actual[0],actual[1],actual[2],actual[3],actual[4],actual[5],actual[6],actual[7]);
   for(int i=0;i<8;++i)EXPECT_EQ(actual[i],expected[i])<<"case "<<count<<" output "<<i;
   ++count;
  }
  light.Activated=false;scenario->NormalLighting=previous;scenario->AmbientCurrent=oldAmbient;GameOptionsClass::Instance.DetailLevel=oldDetail;EXPECT_EQ(count,120);
 });});
}
TEST(BuildingVisual, L05_original_binary_spotlight_motion) {
 session([](const auto&){},[](auto&v){native(v,[&]{
  std::ifstream in(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"spotlight_motion_reference.txt");std::string magic;in>>magic;ASSERT_EQ(magic,"SPOTLIGHT_MOTION_V1");
  auto&r=*RulesClass::Instance;r.SpotlightSpeed=0.05;r.SpotlightAcceleration=0.005;r.SpotlightAngle=0.5;
  BuildingLightClass light(&placed());placed().Location={1700,2200,0};
  int mode,direction,tick,x,y,z,endDirection,count=0;std::string speed,acceleration;
  while(in>>mode>>direction>>tick>>x>>y>>z>>speed>>acceleration>>endDirection){
   if(!tick){light.SetBehaviour(static_cast<SpotlightBehaviour>(mode));light.Speed=0;light.Acceleration=0;light.Direction=direction;light.field_B8={1000,3000,0};light.field_C4={2000,1000,0};}
   light.Update();EXPECT_EQ(light.Location,(CoordStruct{x,y,z}))<<"frame "<<count;
   EXPECT_NEAR(light.Speed,std::stod(speed),1e-12);EXPECT_NEAR(light.Acceleration,std::stod(acceleration),1e-12);EXPECT_EQ(light.Direction,bool(endDirection));++count;
  }
  EXPECT_EQ(count,640);
 });});
}
TEST(BuildingVisual, D02_exploding_death_is_once_and_releases_power) {
 session([](const auto&){},[](auto&v){native(v,[&]{
  auto&b=placed();b.Type->Explodes=true;b.Type->PowerDrain=20;b.Type->DestroyAnim.AddItem(AnimTypeClass::Find("SPIN"));
  b.Owner->UpdatePower();EXPECT_EQ(b.Owner->PowerDrain,20);
  b.Destory(nullptr,nullptr,true,b.Type->FoundationData);const int animations=AnimClass::Array.Count;
  b.Destory(nullptr,nullptr,true,b.Type->FoundationData);EXPECT_EQ(AnimClass::Array.Count,animations);
  EXPECT_EQ(b.C4Timer.GetTimeLeft(),0);EXPECT_TRUE(b.NoCrew);auto*owner=b.Owner;b.Update();EXPECT_EQ(BuildingClass::Array.Count,0);EXPECT_TRUE(owner->RecheckPower);
  owner->UpdatePower();EXPECT_EQ(owner->PowerDrain,0);
 });});
}
TEST(BuildingVisual, L06_temporary_spotlight_expiry_and_reload) {
 const int baseline=SpotlightClass::Array.Count;
 session([](const auto&){},[&](auto&v){native(v,[&]{
  auto*light=new SpotlightClass(placed().Location,16);EXPECT_EQ(SpotlightClass::Array.Count,baseline+1);
  for(int i=0;i<9;++i)light->Update();EXPECT_EQ(light->MovementRadius,72);light->Update();EXPECT_EQ(SpotlightClass::Array.Count,baseline);
  new SpotlightClass(placed().Location,16);
 });});EXPECT_EQ(SpotlightClass::Array.Count,baseline);
}

TEST(BuildingVisual, MapUnitLightAndColorSchemeRouting) {
    session([](const auto&root){
        voxel_fixture(root);
        std::ofstream(root/"RULESMD.INI",std::ios::app)<<
            "\n[AudioVisual]\nExtraUnitLight=0.1\nExtraInfantryLight=0.2\nExtraAircraftLight=0.3\n"
            "[VehicleTypes]\n0=TANK\n[TANK]\nImage=TESTTUR\nStrength=100\nSpeed=4\nROT=5\n"
            "Locomotor={4A582741-9839-11d1-B709-00A024DDAFD1}\nMovementZone=Normal\n";
        std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[TESTTUR]\nVoxel=yes\n";
        std::ofstream(root/"world.map",std::ios::app)<<
            "\n[AudioVisual]\nExtraUnitLight=0.2\n[Units]\n0=Neutral,TANK,256,8,6,0,Guard,None,0,-1,0,-1,1,1\n";
    },[](auto&view){
        ASSERT_TRUE(game::set_map_viewport(view,640,480));
        native(view,[&]{
            EXPECT_EQ(RulesClass::Instance->ExtraUnitLight,200);
            EXPECT_EQ(RulesClass::Instance->ExtraInfantryLight,200);
            EXPECT_EQ(RulesClass::Instance->ExtraAircraftLight,300);
            ASSERT_EQ(UnitClass::Array.Count,1);auto* unit=UnitClass::Array[0];
            MapClass::Instance.GetCellAt(unit->Location)->Intensity_Normal=782;
            game::rebuild_world_sprites(*view.world);
            int parts=0,buildings=0;
            for(const auto& sprite:view.world->impl->sprites){
                if(sprite.owner==unit&&sprite.voxel){++parts;EXPECT_EQ(sprite.intensity,sprite.shadow?1000:982);EXPECT_TRUE(sprite.color_scheme);}
                if(sprite.owner==&placed()&&sprite.image==placed().Type->Image){++buildings;EXPECT_TRUE(sprite.color_scheme);}
            }
            EXPECT_GT(parts,0);EXPECT_GT(buildings,0);
        });
    });
}

TEST(BuildingVisual, GenericOverlayOriginalDepthAndPlacement) {
    session([](const auto& root) {
        std::ofstream(root/"RULESMD.INI",std::ios::app)
            << "\n[ORE74]\nImage=LOWBRIDGE\nTheater=no\nDrawFlat=yes\nLand=Road\n";
        shp(root/"LOWBRIDGE.SHP",6,60,30);
    },[](auto& view) {
        CellClass* cell=nullptr;OverlayTypeClass* type=nullptr;SHPStruct* image=nullptr;
        native(view,[&] {
            cell=MapClass::Instance.GetCellAt(CellStruct{8,7});
            type=OverlayTypeClass::Array[74];image=type->GetImage();
            ASSERT_NE(image,nullptr);
            cell->OverlayTypeIndex=74;
            auto& decorated=view.world->impl->decorated_cells;
            if(std::find(decorated.begin(),decorated.end(),cell)==decorated.end())decorated.push_back(cell);
        });
        std::ifstream input(std::filesystem::path(RA2_SELECTION_FIXTURE).parent_path()/"overlay_draw_reference.txt");
        std::string tag;input>>tag;ASSERT_EQ(tag,"OVERLAY_DRAW_V1");
        int flat,rock,crate,land,level,frame,intensity,expected_frame,x,y,z,gradient,light,cases=0;
        std::string flags;
        while(input>>flat>>rock>>crate>>land>>level>>frame>>intensity
            >>expected_frame>>x>>y>>flags>>z>>gradient>>light) {
            SCOPED_TRACE(::testing::Message()<<"overlay case "<<cases);
            native(view,[&] {
                type->DrawFlat=flat;type->IsARock=rock;type->Crate=crate;
                type->LandType=static_cast<LandType>(land);
                cell->Level=static_cast<BYTE>(level);cell->OverlayData=static_cast<BYTE>(frame);
                cell->Intensity_Terrain=static_cast<WORD>(intensity);
                cell->Intensity_Normal=333; // Detect accidentally using object rather than terrain light.
                ++view.world->presentation_revision;
            });
            const auto output=draw(view);
            const auto found=std::find_if(output.shapes.begin(),output.shapes.end(),
                [&](const auto& request){return request.image==image && !(request.flags&1);});
            ASSERT_NE(found,output.shapes.end());
            const auto& request=*found;
            EXPECT_EQ(request.frame,expected_frame);
            EXPECT_EQ(request.flags,std::stoul(flags,nullptr,0));
            EXPECT_EQ(request.depth_mode,game::ShapeDepthMode::legacy);
            EXPECT_EQ(request.depth_adjustment,z);
            EXPECT_EQ(request.gradient,gradient);
            EXPECT_EQ(request.intensity,light);
            native(view,[&] {
                // Original input point is the unraised TMP top-left. The x86
                // fixture captures Cell::Overlay_Draw_Offset relative to it.
                const auto point=TacticalClass::CoordsToScreen(CoordStruct{8*256,7*256,0});
                EXPECT_EQ(request.position.X,point.X-30-view.tactical.TacticalPos.X+x);
                EXPECT_EQ(request.position.Y,point.Y-view.tactical.TacticalPos.Y+y);
            });
            game::TypeGpuTarget target{640,480,53,0,0x8000,true};
            game::TypeGpuPacket packet;
            ASSERT_EQ(game::prepare_type_shape(request,target,packet),game::DrawingStatus::drawn);
            // Original SHP scanline Z, strict test, and Z write. The previous
            // fallback encoded a fixed per-cell value and read-only Z instead.
            EXPECT_EQ(packet.parameters[13]&0x100FF,0x10001);
            ++cases;
        }
        EXPECT_TRUE(input.eof());EXPECT_EQ(cases,144);
    });
}

TEST(BuildingVisual, OverlaySlopeResourcesAndSeparateShadowPass) {
    session([](const auto& root) {
        std::string entries;
        for(int i=114;i<122;++i)entries+=std::to_string(i)+"=ORE"+std::to_string(i)+"\n";
        edit(root/"RULESMD.INI","[ORE102]\n",entries+"[ORE102]\n");
        std::ofstream rules(root/"RULESMD.INI",std::ios::app);
        for(int i=114;i<122;++i) {
            rules<<"[ORE"<<i<<"]\nTiberium=yes\nTheater=no\n";
            shp(root/("ORE"+std::to_string(i)+".SHP"),12);
        }
        for(int i=1;i<=4;++i)shp(root/("SLOP0"+std::to_string(i)+"Z.TEM"),1);
    },[](auto& view) {
        std::array<SHPStruct*,4> expected{};
        native(view,[&] {
            auto& world=*view.world;auto& w=*world.impl;
            ASSERT_EQ(OverlayTypeClass::Array.Count,122);
            for(int slope=1;slope<=4;++slope) {
                expected[slope-1]=w.overlay_slope_depth[slope];ASSERT_NE(expected[slope-1],nullptr);
                auto* cell=MapClass::Instance.GetCellAt(CellStruct{short(7+slope%2),short(6+slope/2)});
                ASSERT_NE(cell,nullptr);cell->OverlayTypeIndex=102;cell->OverlayData=1;cell->SlopeIndex=BYTE(slope);
                if(std::find(w.decorated_cells.begin(),w.decorated_cells.end(),cell)==w.decorated_cells.end())
                    w.decorated_cells.push_back(cell);
            }
            ++world.presentation_revision;
        });
        const auto output=draw(view);
        bool shadow_started=false;int bodies=0,shadows=0;
        std::set<SHPStruct*> depth_images;
        native(view,[&] {
            std::set<SHPStruct*> overlay_images;
            for(int i=102;i<122;++i)overlay_images.insert(OverlayTypeClass::Array[i]->GetImage());
            for(const auto& r:output.shapes)if(overlay_images.contains(r.image)) {
                if(r.flags&1) {
                    shadow_started=true;++shadows;
                    EXPECT_EQ(r.image,OverlayTypeClass::Array[102]->GetImage());
                    EXPECT_EQ(r.frame,7);EXPECT_EQ(r.depth_image,nullptr);
                } else {
                    EXPECT_FALSE(shadow_started);++bodies;
                    EXPECT_NE(r.depth_image,nullptr);depth_images.insert(r.depth_image);
                    EXPECT_EQ(r.depth_offset,(Point2D{}));EXPECT_EQ(r.gradient,0);
                }
                EXPECT_EQ(r.depth_mode,game::ShapeDepthMode::legacy);
            }
        });
        EXPECT_EQ(bodies,4);EXPECT_EQ(shadows,4);
        EXPECT_EQ(depth_images,(std::set<SHPStruct*>(expected.begin(),expected.end())));
    });
}
