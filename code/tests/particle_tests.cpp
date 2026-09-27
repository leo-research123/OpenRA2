#include "support/test_support.hpp"
#include "support/map_test_support.hpp"
#include "map_world_internal.hpp"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/GameOptionsClass.h"
#include <bit>
#include <cfenv>

TEST(Particles, OriginalSmokeFrames){
 map_fixture::session([](const auto&) {}, [](auto& view) {
 map_fixture::native(view, [] {
  for(auto* building:BuildingClass::Array)building->Limbo();
  std::ifstream in(RA2_SMOKE_FIXTURE);std::string magic;int count;ASSERT_TRUE(bool(in>>magic>>count));ASSERT_EQ(magic,"SMOKE_REFERENCE_V1");
  ParticleTypeClass held("FIXTURE_SMOKE");ParticleSystemTypeClass type("FIXTURE_SMOKE_SYS");
  held.BehavesLike=static_cast<BehavesLike>(1);held.MaxEC=80;held.Velocity=9;held.Deacc=.05f;
  held.EndStateAI=20;held.StateAIAdvance=4;held.DeleteOnStateLimit=true;held.Translucency=50;
  type.BehavesLike=BehavesLike::Smoke;type.HoldsWhat=ParticleTypeClass::Array.FindItemIndex(&held);
  type.SpawnFrames=10;type.SpawnRadius=5;type.ParticleCap=7;type.Slowdown=.0025f;
  type.SpawnCutoff=13;type.SpawnTranslucencyCutoff=12.5;type.Lifetime=50;
  auto& scenario=*ScenarioClass::Instance;const auto oldID=scenario.UniqueID;
  const int oldWind=RulesClass::Instance->WindDirection,oldFrame=Unsorted::CurrentFrame,rounding=std::fegetround();
  auto restore=ra2::test::scope_exit([&]{scenario.UniqueID=oldID;RulesClass::Instance->WindDirection=oldWind;Unsorted::CurrentFrame=oldFrame;std::fesetround(rounding);});
  std::unique_ptr<ParticleSystemClass> system;
  auto clear=ra2::test::scope_exit([&]{system.reset();AbstractClass::RemoveAllInactive();});
  for(int row=0;row<count;++row){
   unsigned seed,spawnBits;int bridge,wind,tick,lifetime,dying,alive,created,next1,next2,particles;
   ASSERT_TRUE(bool(in>>seed>>bridge>>wind>>tick>>spawnBits>>lifetime>>dying>>alive>>created>>next1>>next2>>particles));
   SCOPED_TRACE(::testing::Message()<<seed<<' '<<bridge<<' '<<wind<<' '<<tick);
   if(!tick){
    system.reset();AbstractClass::RemoveAllInactive();scenario.Random=Randomizer(seed);std::fesetround(FE_TOWARDZERO);
    held.WindEffect=wind;RulesClass::Instance->WindDirection=3;
    auto& cells=MapClass::Instance.Cells;
    for(int i=0;i<cells.Capacity;++i)if(auto* cell=cells[i])cell->Flags=static_cast<CellFlags>((unsigned(cell->Flags)&~0x100u)|(bridge?0x100u:0u));
    system=std::make_unique<ParticleSystemClass>(&type,CoordStruct{2176,1664,256},nullptr,nullptr,CoordStruct::Empty,nullptr);
    scenario.UniqueID=100+(seed&1);
   }
   Unsorted::CurrentFrame=tick;if(system->IsAlive)system->Update();
   EXPECT_EQ(std::bit_cast<unsigned>(system->SpawnFrames),spawnBits);EXPECT_EQ(system->Lifetime,lifetime);
   EXPECT_EQ(int(system->TimeToDie),dying);EXPECT_EQ(int(system->IsAlive),alive);
   EXPECT_EQ(scenario.UniqueID-100-int(seed&1),2*created);EXPECT_EQ(scenario.Random.Next1,next1);EXPECT_EQ(scenario.Random.Next2,next2);
   ASSERT_EQ(system->Particles.Count,particles);
   for(int i=0;i<particles;++i){
    unsigned id,speed;int x,y,z,vx,vy,vz,ec,state,translucency,dead;
    ASSERT_TRUE(bool(in>>id>>x>>y>>z>>vx>>vy>>vz>>speed>>ec>>state>>translucency>>dead));
    auto* p=system->Particles[i];EXPECT_EQ(p->UniqueID,id);EXPECT_EQ(p->Location,(CoordStruct{x,y,z}));
    EXPECT_EQ(p->Velocity,(CoordStruct{vx,vy,vz}));EXPECT_EQ(std::bit_cast<unsigned>(p->Speed),speed);
    EXPECT_EQ(p->RemainingEC,ec);EXPECT_EQ(p->StartStateAI,state);EXPECT_EQ(p->Translucency,translucency);EXPECT_EQ(p->IsToDie,dead);
   }
  }
  EXPECT_EQ(count,960);
 });
 });
}

TEST(Particles, OriginalSparkFrames){
 map_fixture::session([](const auto&) {}, [](auto& view) {
 map_fixture::native(view, [] {
  // The executable corpus fixtures a flat, unobstructed map cell.
  for(auto* building:BuildingClass::Array)building->Limbo();
  std::ifstream in(RA2_SPARK_FIXTURE);std::string magic;int count=0;ASSERT_TRUE(bool(in>>magic>>count));ASSERT_EQ(magic,"SPARK_REFERENCE_V1");
  ParticleTypeClass held("FIXTURE_SPARK");ParticleSystemTypeClass type("FIXTURE_WELD");
  held.BehavesLike=BehavesLike::Spark;held.MaxEC=500;held.ColorSpeed=.13;held.XVelocity=held.YVelocity=16;held.MinZVelocity=40;held.ZVelocityRange=15;
  held.ColorList.AddItem(RGBClass(255,255,255));held.ColorList.AddItem(RGBClass(100,150,200));held.ColorList.AddItem(RGBClass(0,0,0));
  type.BehavesLike=BehavesLike::Spark;type.HoldsWhat=ParticleTypeClass::Array.FindItemIndex(&held);type.ParticleCap=15;
  type.SparkSpawnFrames=20;type.OneFrameLight=true;type.LightSize=25;type.SpawnSparkPercentage=.4;
  const int oldGravity=RulesClass::Instance->Gravity;RulesClass::Instance->Gravity=6;
  const int rounding=std::fegetround();auto restore=ra2::test::scope_exit([&]{RulesClass::Instance->Gravity=oldGravity;std::fesetround(rounding);});
  std::unique_ptr<ParticleSystemClass> system;int start=0;
  auto clear=ra2::test::scope_exit([&]{system.reset();AbstractClass::RemoveAllInactive();});
  for(int row=0;row<count;++row){
   unsigned seed;int direction,tick,frames,radius,dying,created,next1,next2,particles;
   ASSERT_TRUE(bool(in>>seed>>direction>>tick>>frames>>radius>>dying>>created>>next1>>next2>>particles));
   SCOPED_TRACE(row);
   if(tick==0){system.reset();AbstractClass::RemoveAllInactive();
    ScenarioClass::Instance->Random=Randomizer(seed);std::fesetround(FE_TOWARDZERO);start=ParticleClass::Array.Count;
    system=std::make_unique<ParticleSystemClass>(&type,CoordStruct{2176,1664,256},nullptr,nullptr,CoordStruct::Empty,nullptr);
    system->unknown_bool_F9=direction;
   }
   system->SparkAI();
   ASSERT_EQ(system->Particles.Count,particles);EXPECT_EQ(system->SparkSpawnFrames,frames);EXPECT_EQ(system->SpotlightRadius,radius);
   EXPECT_EQ(int(system->TimeToDie),dying);EXPECT_EQ(ParticleClass::Array.Count-start,created);
   EXPECT_EQ(ScenarioClass::Instance->Random.Next1,next1);EXPECT_EQ(ScenarioClass::Instance->Random.Next2,next2);
   for(int i=0;i<particles;++i){
    int x,y,z,index,ec;unsigned vx,vy,vz;std::uint64_t accum;ASSERT_TRUE(bool(in>>x>>y>>z>>vx>>vy>>vz>>index>>accum>>ec));
    const auto* p=system->Particles[i];SCOPED_TRACE(i);
    EXPECT_EQ(p->Location,(CoordStruct{x,y,z}));
    EXPECT_EQ(std::bit_cast<unsigned>(p->MovementDirection.X),vx);EXPECT_EQ(std::bit_cast<unsigned>(p->MovementDirection.Y),vy);EXPECT_EQ(std::bit_cast<unsigned>(p->MovementDirection.Z),vz);
    EXPECT_EQ(p->ColorIndex,index);EXPECT_EQ(std::bit_cast<std::uint64_t>(p->ColorAccum),accum);EXPECT_EQ(p->RemainingEC,ec);
   }
  }
  system.reset();AbstractClass::RemoveAllInactive();EXPECT_EQ(count,280);
 });
 });
}
