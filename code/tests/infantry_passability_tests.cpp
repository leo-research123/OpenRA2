#include "support/test_support.hpp"
#include "map_world_fixture.hpp"
#include "map_view.hpp"
#include "yrpp/InfantryClass.h"
#include "yrpp/WalkLocomotionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/AStarClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TerrainClass.h"
#include <bit>
#include <climits>
#include <array>

namespace {
enum Flag : unsigned {
 Ally=1u<<0,Moving=1u<<1,Frozen=1u<<2,Tracks=1u<<3,Nav=1u<<4,Cloak=1u<<5,Disguise=1u<<6,Warp=1u<<7,Iron=1u<<8,
 Dest=1u<<9,Cell=1u<<10,Archive=1u<<11,Engineer=1u<<12,C4=1u<<13,Thief=1u<<14,Target=1u<<15,Tether=1u<<16,
 Bridge=1u<<17,BadGround=1u<<18,Init=1u<<19,Armed=1u<<20,Healer=1u<<21,WallWeapon=1u<<22,RepairHut=1u<<23,
 Gate=1u<<24,Open=1u<<25,Invisible=1u<<26,Laser=1u<<27,Firestorm=1u<<28,FireActive=1u<<29,Human=1u<<30
};
struct World {
 std::filesystem::path root=std::filesystem::temp_directory_path()/("ra2-passability-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 game::ResourceHandle* resources=nullptr;game::MapViewHandle* view=nullptr;
 explicit World(bool retain_navigation=false){std::filesystem::create_directories(root);map_fixture::fixtures(root);std::string error;
  if(!game::create_resources(root.string(),resources,error)||!game::create_map_view(*resources,view)||!game::load_map_view(*view,"world.map",9))throw std::runtime_error("passability fixture load failed");
  // Differential cases supply their own graph and compare that exact input.
  if(!retain_navigation)game::with_map_view(*view,[](void*){auto& map=MapClass::Instance;
   for(auto& zones:map.MovementZones){YRMemory::Deallocate(zones);zones=nullptr;}map.somecount_4C=0;map.ZoneConnections.Clear();
   for(int i=0;i<3;++i){map.SubzoneTracking[i].Clear();map.SubzoneTrackingCounts[i]=0;}
   std::fill_n(map.LevelAndPassability,map.ValidMapCellCount,CellLevelPassabilityStruct{7,0,0});
   std::fill_n(map.LevelAndPassabilityStruct2pointer_70,map.ValidMapCellCount,LevelAndPassabilityStruct2{});
  },nullptr);
 }
 ~World(){game::destroy_map_view(view);FileSystem::ClearNameCache();Destroy_All_Shapes();Unload_All_Shapes();game::destroy_resources(resources);std::error_code error;std::filesystem::remove_all(root,error);}
};
struct Driver:WalkLocomotionClass {bool jumps=false;bool YRPP_STDCALL Will_Jump_Tracks()override{return jumps;}};
struct Body:InfantryClass {
 AbstractType kind=AbstractType::Infantry;
 Body(InfantryTypeClass* type,HouseClass* owner):InfantryClass(type,owner){}
 AbstractType WhatAmI()const override{return kind;}
 bool IsStrange()const override{return kind==AbstractType::Unit;}
};
}

TEST(InfantryPassability, OriginalOccupancyAndTerrainDecisionCorpus) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  struct Globals {int init=Unsorted::ScenarioInit;float cost=GroundType::Array[0].Cost[0];~Globals(){Unsorted::ScenarioInit=init;GroundType::Array[0].Cost[0]=cost;}}globals;
  InfantryTypeClass type("PASS_INF");BuildingTypeClass building_type("PASS_BLDG",BuildingTypeClass::ConstructionDefaults{});
  HouseClass house(nullptr),enemy(nullptr);house.ArrayIndex=0;enemy.ArrayIndex=1;
  WeaponTypeClass weapon("PASS_WEAPON");WarheadTypeClass warhead("PASS_WARHEAD");weapon.Warhead=&warhead;
  OverlayTypeClass overlay("PASS_OVERLAY");overlay.DamageLevels=3;
  InfantryClass actor(&type,&house);Body bodies[]{{&type,&house},{&type,&house},{&type,&house}};
  BuildingClass buildings[]{{&building_type,&house},{&building_type,&house},{&building_type,&house}};
  Driver* drivers[3];
  for(int i=0;i<3;++i){drivers[i]=GameCreate<Driver>();ASSERT_NE(drivers[i],nullptr);
#if !defined(_MSC_VER)
   drivers[i]->AddRef();
#endif
   bodies[i].Locomotor=drivers[i];drivers[i]->Link_To_Object(&bodies[i]);}
  auto*cell=MapClass::Instance.GetCellAt(CellStruct{8,6});auto*source=MapClass::Instance.GetCellAt(CellStruct{7,6});
  cell->Level=source->Level=0;cell->SlopeIndex=source->SlopeIndex=0;source->Flags=static_cast<CellFlags>(0);
  cell->TubeIndex=source->TubeIndex=-1;cell->LandType=LandType::Clear;type.SpeedType=SpeedType::Foot;
  actor.IsInPlayfield=false;actor.QueuedMission=Mission::None;
  std::ifstream input(RA2_INFANTRY_PASSABILITY_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0;int kind;
  while(input>>kind){
   unsigned flags;int mission,count,occupation,owner,overlay_kind,expected;
   ASSERT_TRUE(bool(input>>flags>>mission>>count>>occupation>>owner>>overlay_kind>>expected));
   actor.CurrentMission=static_cast<Mission>(mission);actor.IsTether=flags&Tether;
   type.Engineer=flags&Engineer;type.C4=flags&C4;type.VehicleThief=flags&Thief;
   type.Weapon[0].WeaponType=flags&(Armed|Healer)?&weapon:nullptr;
   weapon.Damage=flags&Healer?-10:10;weapon.AmbientDamage=0;warhead.Wall=flags&WallWeapon;
   GroundType::Array[0].Cost[0]=flags&BadGround?0.0f:1.0f;Unsorted::ScenarioInit=bool(flags&Init);house.IsHumanPlayer=flags&Human;
   cell->Flags=static_cast<CellFlags>(flags&Bridge?0x100:0);cell->OccupationFlags=cell->AltOccupationFlags=occupation;
   cell->InfantryOwnerIndex=cell->AltInfantryOwnerIndex=owner;cell->OverlayTypeIndex=overlay_kind?overlay.ArrayIndex:-1;
   overlay.Crate=overlay_kind==1;overlay.Wall=overlay_kind==2||overlay_kind==3;cell->OverlayData=overlay_kind==3?0x30:0;
   cell->WallOwnerIndex=flags&Ally?0:1;
   constexpr AbstractType rtti[]{AbstractType::None,AbstractType::Infantry,AbstractType::Unit,AbstractType::Aircraft,AbstractType::Building,AbstractType::Terrain};
   ObjectClass* objects[3];
   for(int i=0;i<3;++i){
    bodies[i].kind=rtti[kind];objects[i]=kind==4?static_cast<ObjectClass*>(&buildings[i]):static_cast<ObjectClass*>(&bodies[i]);
    auto*obj=static_cast<TechnoClass*>(objects[i]);obj->AbstractFlags=static_cast<::AbstractFlags>(kind==5?2:kind==4?3:7);
    obj->Owner=kind==5?nullptr:flags&Ally?&house:&enemy;obj->Location={8*256+192,6*256+64,0};
    obj->IsOnMap=true;obj->IsAlive=true;obj->Health=100;obj->CurrentMission=flags&Open?Mission::Open:Mission::Guard;
    obj->CloakState=flags&Cloak?CloakState::Cloaked:CloakState::Uncloaked;obj->Disguised=flags&Disguise;obj->DisguisedAsHouse=&house;
    obj->BeingWarpedOut=flags&Warp;obj->IronCurtainTimer.StartTime=-1;obj->IronCurtainTimer.TimeLeft=flags&Iron?10:0;
    bodies[i].FrozenStill=flags&Frozen;bodies[i].Destination=flags&Nav?source:nullptr;drivers[i]->IsMoving=flags&Moving;drivers[i]->jumps=flags&Tracks;
    buildings[i].UnloadTimer.State1=false;buildings[i].UnloadTimer.State2=flags&Open;buildings[i].LaserFenceFrame=flags&Laser?8:0;
   }
   for(int i=0;i<3;++i)objects[i]->NextObject=i+1<count?objects[i+1]:nullptr;
   actor.Destination=flags&Dest?static_cast<AbstractClass*>(objects[0]):flags&Cell?cell:nullptr;
   actor.Target=flags&Target?objects[0]:nullptr;actor.ArchiveTarget=flags&Archive?objects[0]:nullptr;
   building_type.BridgeRepairHut=flags&RepairHut;building_type.Gate=flags&Gate;building_type.InvisibleInGame=flags&Invisible;
   building_type.LaserFence=flags&Laser;building_type.FirestormWall=flags&Firestorm;house.FirestormActive=enemy.FirestormActive=flags&FireActive;
   cell->FirstObject=cell->AltObject=count?objects[0]:nullptr;
   const auto result=actor.IsCellOccupied(cell,FacingType::None,-1,source,false);
   EXPECT_EQ(int(result),expected)<<"case="<<cases<<" kind="<<kind<<" flags="<<flags<<" mission="<<mission;
   // Do not let the synthetic per-case links become destructor-owned world data.
   cell->FirstObject=cell->AltObject=nullptr;actor.Destination=actor.Target=actor.ArchiveTarget=nullptr;
   for(auto&body:bodies){body.NextObject=nullptr;body.Destination=nullptr;body.IsOnMap=false;}
   for(auto&body:buildings){body.NextObject=nullptr;body.IsOnMap=false;}
   ++cases;
  }
  cell->OverlayTypeIndex=-1;cell->OccupationFlags=cell->AltOccupationFlags=0;
  EXPECT_EQ(cases,18298u);
 },nullptr));
}

TEST(InfantryPassability, TunnelDirectionAndIndexLifecycle) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  InfantryTypeClass type("TUNNEL_INF");HouseClass house(nullptr);InfantryClass actor(&type,&house);
  house.ArrayIndex=0;actor.IsInPlayfield=false;type.SpeedType=SpeedType::Foot;
  auto* cell=MapClass::Instance.GetCellAt(CellStruct{8,6});auto* source=MapClass::Instance.GetCellAt(CellStruct{7,6});
  cell->Level=source->Level=0;cell->Flags=source->Flags=static_cast<CellFlags>(0);
  cell->TubeIndex=source->TubeIndex=-1;cell->OccupationFlags=0;cell->InfantryOwnerIndex=-1;
  cell->LandType=LandType::Clear;cell->OverlayTypeIndex=-1;cell->FirstObject=nullptr;
  const auto old_cost=GroundType::Array[0].Cost[0];GroundType::Array[0].Cost[0]=1;
  EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::Count,-1,source,false),Move::No);
  const int old_count=TubeClass::Array.Count;
  {
   CellStruct entry=cell->MapCoords;TubeClass tube(&entry,int(FacingType::East));
   EXPECT_EQ(TubeClass::Array.Count,old_count+1);EXPECT_EQ(cell->GetTunnel(),&tube);
   EXPECT_EQ(tube.FaceCount,0);for(int face:tube.Faces)EXPECT_EQ(face,-1);
   EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::Count,-1,source,false),Move::No);
   tube.ExitCell={12,6};
   EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::Count,-1,source,false),Move::OK);
   EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::West,0,source,false),Move::No);
   EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::East,0,source,false),Move::OK);
   EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::None,-1,source,false),Move::OK);
  }
  EXPECT_EQ(TubeClass::Array.Count,old_count);EXPECT_EQ(cell->TubeIndex,-1);EXPECT_EQ(cell->GetTunnel(),nullptr);
  {
   CellStruct entry=source->MapCoords;TubeClass tube(&entry,int(FacingType::East));tube.ExitCell={12,6};
   EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::East,0,source,false),Move::No);
   tube.ExitFace=int(FacingType::West);
   EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::East,0,source,false),Move::OK);
  }
  cell->TubeIndex=static_cast<short>(TubeClass::Array.Count);EXPECT_EQ(cell->GetTunnel(),nullptr);
  cell->TubeIndex=-2;EXPECT_EQ(cell->GetTunnel(),nullptr);cell->TubeIndex=-1;
  GroundType::Array[0].Cost[0]=old_cost;
 },nullptr));
}

TEST(InfantryPassability, SeparateBridgeChainsAndSelfAreNotObstacles) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  InfantryTypeClass type("LAYER_INF");BuildingTypeClass building_type("LAYER_BUILDING",BuildingTypeClass::ConstructionDefaults{});
  HouseClass house(nullptr);house.ArrayIndex=0;InfantryClass actor(&type,&house);BuildingClass obstacle(&building_type,&house);
  auto* cell=MapClass::Instance.GetCellAt(CellStruct{8,6});
  actor.IsInPlayfield=false;type.SpeedType=SpeedType::Foot;cell->LandType=LandType::Clear;cell->Level=0;
  cell->TubeIndex=-1;cell->OverlayTypeIndex=-1;cell->Flags=static_cast<CellFlags>(0x100);
  cell->OccupationFlags=cell->AltOccupationFlags=0;cell->InfantryOwnerIndex=cell->AltInfantryOwnerIndex=-1;
  const auto old_cost=GroundType::Array[0].Cost[0];GroundType::Array[0].Cost[0]=1;
  cell->FirstObject=&obstacle;cell->AltObject=&actor;actor.NextObject=obstacle.NextObject=nullptr;
  EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::None,0,nullptr,false),Move::No);
  EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::None,4,nullptr,false),Move::OK);
  cell->FirstObject=&actor;cell->AltObject=&obstacle;
  EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::None,0,nullptr,false),Move::OK);
  EXPECT_EQ(actor.IsCellOccupied(cell,FacingType::None,4,nullptr,false),Move::No);
  cell->FirstObject=cell->AltObject=nullptr;GroundType::Array[0].Cost[0]=old_cost;
 },nullptr));
}

TEST(InfantryPassability, ReadOnlyScriptExitPermissionUsesMovingNotLeavingFlag) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  ScriptTypeClass type("EXIT_SCRIPT");TeamTypeClass team_type("EXIT_TEAM");team_type.ScriptType=&type;
  const int teams=TeamClass::Array.Count,scripts=ScriptClass::Array.Count;
  {
  // Team owns its Script and member chain. The real constructor/destructor
  // must be paired; noinit_t leaves the destructor's FirstUnit indeterminate.
  TeamClass team(&team_type,nullptr,0);ASSERT_NE(team.CurrentScript,nullptr);
  auto& script=*team.CurrentScript;
  script.CurrentMission=0;type.ActionsCount=1;type.ScriptActions[0]={3,0};
  team.IsMoving=false;team.IsLeavingMap=true;
  auto* scenario=ScenarioClass::Instance;const auto old_waypoint=scenario->Waypoints[0];
  auto restore=ra2::test::scope_exit([&]{scenario->Waypoints[0]=old_waypoint;});
  scenario->Waypoints[0]={100,100};ASSERT_FALSE(MapClass::Instance.IsWithinUsableArea(scenario->Waypoints[0],true));
  EXPECT_FALSE(team.IsLeavingMapNow());team.IsMoving=true;team.IsLeavingMap=false;EXPECT_TRUE(team.IsLeavingMapNow());
  scenario->Waypoints[0]={8,6};ASSERT_TRUE(MapClass::Instance.IsWithinUsableArea(scenario->Waypoints[0],true));
  EXPECT_FALSE(team.IsLeavingMapNow());scenario->Waypoints[0]={100,100};
  type.ScriptActions[0].Action=4;EXPECT_FALSE(team.IsLeavingMapNow());type.ScriptActions[0].Action=3;
  script.CurrentMission=1;EXPECT_FALSE(team.IsLeavingMapNow());script.CurrentMission=-1;
  EXPECT_TRUE(script.HasCurrentMission());EXPECT_FALSE(team.IsLeavingMapNow());
  ScriptActionNode action;script.GetCurrentAction(&action);EXPECT_EQ(action.Action,-1);EXPECT_EQ(action.Argument,0);
  }
  EXPECT_EQ(TeamClass::Array.Count,teams);EXPECT_EQ(ScriptClass::Array.Count,scripts);
  EXPECT_EQ(team_type.cntInstances,0);
 },nullptr));
}

TEST(InfantryPassability, WeaponDamageAndAllianceQueryBoundaries) {
 InfantryTypeClass type("DAMAGE_INF");HouseClass house(nullptr),other(nullptr);InfantryClass actor(&type,&house);
 WeaponTypeClass primary("PASS_PRIMARY"),secondary("PASS_SECONDARY"),elite("PASS_ELITE");
 primary.Damage=10;primary.AmbientDamage=4;secondary.Damage=-30;secondary.AmbientDamage=1;elite.Damage=40;elite.AmbientDamage=2;
 type.Weapon[0].WeaponType=&primary;type.Weapon[1].WeaponType=&secondary;
 EXPECT_EQ(actor.CombatDamage(0),14);EXPECT_EQ(actor.CombatDamage(1),-29);EXPECT_EQ(actor.CombatDamage(-1),-7);
 // 0x006F3970/0x0070E1A0 select the weapon field at 0x138; the visual
 // turret field at 0x124 can deliberately differ (e.g. IFV turret mappings).
 type.TurretCount=2;actor.CurrentTurretNumber=0;actor.CurrentWeaponNumber=1;
 EXPECT_EQ(actor.CombatDamage(-1),-29);EXPECT_EQ(actor.GetTurretWeapon()->WeaponType,&secondary);
 type.IsGattling=true;EXPECT_EQ(actor.CombatDamage(-1),-7);type.IsGattling=false;
 type.EliteWeapon[1].WeaponType=&elite;actor.Veterancy.Veterancy=2;EXPECT_EQ(actor.CombatDamage(-1),42);
 primary.Damage=INT_MAX;primary.AmbientDamage=1;EXPECT_EQ(actor.CombatDamage(0),INT_MIN);
 type.Weapon[1].WeaponType=nullptr;type.EliteWeapon[1].WeaponType=nullptr;EXPECT_FALSE(actor.IsArmed());
 type.TurretCount=0;EXPECT_TRUE(actor.IsArmed());
 house.ArrayIndex=other.ArrayIndex=-1;house.Allies.data=0;
 EXPECT_TRUE(house.IsAlliedWith(-1));EXPECT_TRUE(house.IsAlliedWith(&house));EXPECT_TRUE(house.IsAlliedWith(&other));
 house.ArrayIndex=0;EXPECT_FALSE(house.IsAlliedWith(-1));EXPECT_FALSE(house.IsAlliedWith(&other));
 EXPECT_FALSE(house.IsAlliedWith(static_cast<HouseClass*>(nullptr)));
 house.Allies.data=2;other.ArrayIndex=33;EXPECT_TRUE(house.IsAlliedWith(33));EXPECT_TRUE(house.IsAlliedWith(&other));
 other.ArrayIndex=-31;EXPECT_TRUE(house.IsAlliedWith(&other));
}

TEST(AStarCosts, OriginalStepAndTrafficCostCorpus) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  InfantryTypeClass type("COST_INF");HouseClass house(nullptr);Body body(&type,&house);AStarClass finder{};
  struct Saved {CellClass* cell;CellFlags flags;ObjectClass* ground;ObjectClass* bridge;};
  std::vector<Saved> saved;
  for(int y=3;y<10;++y)for(int x=5;x<12;++x){auto*c=MapClass::Instance.GetCellAt(CellStruct{short(x),short(y)});saved.push_back({c,c->Flags,c->FirstObject,c->AltObject});}
  auto restore=ra2::test::scope_exit([&]{for(auto s:saved){s.cell->Flags=s.flags;s.cell->FirstObject=s.ground;s.cell->AltObject=s.bridge;}});
  body.Location={8*256+128,6*256+128,0};body.OnBridge=false;body.PrimaryFacing.SetCurrent(DirStruct(0x4000));
  std::ifstream input(RA2_ASTAR_COST_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0;
  int move,mode,predicted,bridge,avoid,orientation,direction,near,far,traffic;std::uint64_t bits;
  constexpr CellStruct offsets[]{{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};
  constexpr int across[]{-2,-2,0,1,1,1,0,-2},along[]{0,-1024,-1024,-1024,0,512,512,512};
  auto* dest=MapClass::Instance.GetCellAt(CellStruct{8,6});
  auto** to=MapClass::Instance.Cells.Items+MapClass::GetCellIndex(dest->MapCoords);
  while(input>>move){
   ASSERT_TRUE(bool(input>>mode>>predicted>>bridge>>avoid>>orientation>>direction>>near>>far>>traffic>>bits));
   for(auto s:saved){s.cell->Flags=static_cast<CellFlags>(0);s.cell->FirstObject=s.cell->AltObject=nullptr;}
   const auto* table=orientation?along:across;
   if(near)to[table[direction]]->Flags=static_cast<CellFlags>(0x100);
   if(far)to[table[(direction-4)&7]]->Flags=static_cast<CellFlags>(0x100);
   dest->Flags=static_cast<CellFlags>(static_cast<unsigned>(dest->Flags)|(predicted?0x40000:0)|(orientation?0x800:0));
   finder.FindMode=mode;finder.FindBridgeDir=avoid;
   body.AbstractFlags=static_cast<::AbstractFlags>(traffic==1?2:7);body.PathDirections[0]=traffic==2?-1:traffic==4?8:2;
   body.SpeedPercentage=traffic==5?1.0:0.0;
   if(traffic){if(bridge)dest->AltObject=&body;else dest->FirstObject=&body;}
   if(traffic==6)MapClass::Instance.GetCellAt(CellStruct{9,6})->FirstObject=&body;
   const CellStruct start{short(8-offsets[direction].X),short(6-offsets[direction].Y)};
   auto** from=MapClass::Instance.Cells.Items+MapClass::GetCellIndex(start);
   EXPECT_EQ(std::bit_cast<std::uint64_t>(finder.GetMovementCost(from,to,bridge,static_cast<Move>(move),&body)),bits)<<"case="<<cases;
   ++cases;
  }
  EXPECT_EQ(cases,12360u);
 },nullptr));
}

TEST(AStarCosts, PredictedPathsToggleSymmetricallyAndMovingReservationsAreQueried) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  InfantryTypeClass type("PREDICT_INF");HouseClass house(nullptr);InfantryClass actor(&type,&house),blocker(&type,&house);
  AStarClass finder{};finder.CanFindPath=true;finder.FindMode=2;
  actor.Location={8*256+128,6*256+128,0};actor.PrimaryFacing.SetCurrent(DirStruct(0x4000));
  blocker.Location={9*256+128,6*256+128,0};blocker.CurrentMapCoords={9,6};blocker.NextObject=nullptr;
  for(int& direction:blocker.PathDirections)direction=-1;
  blocker.PathDirections[0]=blocker.PathDirections[1]=blocker.PathDirections[2]=2;
  auto* driver=GameCreate<Driver>();ASSERT_NE(driver,nullptr);
#if !defined(_MSC_VER)
  driver->AddRef();
#endif
  blocker.Locomotor=driver;driver->Link_To_Object(&blocker);
  struct Saved {CellClass* cell;CellFlags flags;ObjectClass* ground;ObjectClass* bridge;unsigned occupation;};
  std::vector<Saved> saved;
  for(int y=3;y<10;++y)for(int x=5;x<14;++x){auto*c=MapClass::Instance.GetCellAt(CellStruct{short(x),short(y)});
   saved.push_back({c,c->Flags,c->FirstObject,c->AltObject,c->OccupationFlags});c->Flags=static_cast<CellFlags>(0);c->FirstObject=c->AltObject=nullptr;c->OccupationFlags=0;}
  auto restore=ra2::test::scope_exit([&]{for(auto s:saved){s.cell->Flags=s.flags;s.cell->FirstObject=s.ground;s.cell->AltObject=s.bridge;s.cell->OccupationFlags=s.occupation;}});
  auto* next=MapClass::Instance.GetCellAt(CellStruct{9,6});next->FirstObject=&blocker;
  auto predicted=[](int x,int y){return(static_cast<unsigned>(MapClass::Instance.GetCellAt(CellStruct{short(x),short(y)})->Flags)&0x40000u)!=0;};
  finder.ApplyPathCollisionAvoidance(&actor);
  EXPECT_TRUE(predicted(9,6));EXPECT_TRUE(predicted(10,6));EXPECT_TRUE(predicted(11,6));EXPECT_TRUE(predicted(12,6));EXPECT_FALSE(predicted(8,6));
  finder.ApplyPathCollisionAvoidance(&actor);
  for(auto s:saved)EXPECT_EQ(static_cast<unsigned>(s.cell->Flags),0u);
  finder.FindMode=1;finder.ApplyPathCollisionAvoidance(&actor);EXPECT_EQ(finder.FindMode,0);
  for(auto s:saved)EXPECT_EQ(static_cast<unsigned>(s.cell->Flags),0u);
  driver->HeadToCoord={8*256+192,6*256+64,0};
  EXPECT_EQ(AStarClass::FindMovingBlocker({8,6},0),&blocker);
  EXPECT_EQ(AStarClass::FindMovingBlocker({8,6},4),nullptr);
  driver->HeadToCoord=CoordStruct::Empty;
  CellStruct start{8,6},result;int directions[]{2,8,4};
  {CellStruct entry{9,6};TubeClass tube(&entry,2);tube.ExitCell={11,7};
   AStarClass::FollowPath(&result,&start,3,directions);EXPECT_EQ(result,(CellStruct{11,8}));}
  AStarClass::FollowPath(&result,&start,2,directions);EXPECT_EQ(result,(CellStruct{0,0}));
  AStarClass::FollowPath(&result,&start,0,nullptr);EXPECT_EQ(result,start);
 },nullptr));
}

TEST(AStarRegular, OriginalSearchAndFinalPathCorpus) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  InfantryTypeClass type("PATH_INF");HouseClass house(nullptr);house.ArrayIndex=0;InfantryClass actor(&type,&house);AStarClass finder;
  actor.IsInPlayfield=false;actor.CurrentMission=Mission::Move;type.SpeedType=SpeedType::Foot;
  finder.UpdateMapDimensions(MapClass::Instance.MapRect);finder.Reset();
  struct Saved {CellClass* cell;CellFlags flags;ObjectClass* ground;ObjectClass* bridge;LandType land;int overlay;unsigned occupation,alt_occupation;int owner,alt_owner;};
  std::vector<Saved> saved;
  for(int i=0;i<MapClass::Instance.Cells.Capacity;++i)if(auto* c=MapClass::Instance.Cells.Items[i])
   saved.push_back({c,c->Flags,c->FirstObject,c->AltObject,c->LandType,c->OverlayTypeIndex,c->OccupationFlags,c->AltOccupationFlags,c->InfantryOwnerIndex,c->AltInfantryOwnerIndex});
  const float clear_cost=GroundType::Array[0].Cost[0],water_cost=GroundType::Array[2].Cost[0];
  auto restore=ra2::test::scope_exit([&]{
   GroundType::Array[0].Cost[0]=clear_cost;GroundType::Array[2].Cost[0]=water_cost;
   for(auto s:saved){auto*c=s.cell;c->Flags=s.flags;c->FirstObject=s.ground;c->AltObject=s.bridge;c->LandType=s.land;c->OverlayTypeIndex=s.overlay;
    c->OccupationFlags=s.occupation;c->AltOccupationFlags=s.alt_occupation;c->InfantryOwnerIndex=s.owner;c->AltInfantryOwnerIndex=s.alt_owner;}
  });
  GroundType::Array[0].Cost[0]=1;GroundType::Array[2].Cost[0]=0;
  std::ifstream input(RA2_ASTAR_REGULAR_FIXTURE);ASSERT_TRUE(input.good());int seed,limit,cost,length;unsigned cases=0;
  while(input>>seed){
   ASSERT_TRUE(bool(input>>limit>>cost>>length));
   std::vector<int> expected(length?length+1:0),levels(length?length-1:0);
   for(int& value:expected)ASSERT_TRUE(bool(input>>value));for(int& value:levels)ASSERT_TRUE(bool(input>>value));
   const CellStruct start{8,6},end=seed?CellStruct{short(7+seed%7),short(7+seed%8)}:start;
   actor.ThreatAvoidanceCoefficient=seed>=48?0.125:0.0;actor.OnBridge=seed>=64;
   for(int ty=0;ty<7;++ty)for(int tx=0;tx<7;++tx)house.ThreatPosedEstimates[ty+1][tx+1]=seed>=48 && (tx*5+ty*7+seed)%7==0?4:0;
   for(auto s:saved){auto*c=s.cell;const auto p=c->MapCoords;const int hashed=(p.X*17+p.Y*31+seed*13)%19;
    c->LandType=seed>=8 && hashed<5 && p!=start?LandType::Water:LandType::Clear;
    c->Flags=static_cast<CellFlags>((seed>=16 && hashed==7?0x40000:0)|(seed>=64?0x300:0));c->FirstObject=c->AltObject=nullptr;c->OverlayTypeIndex=-1;
    c->OccupationFlags=c->AltOccupationFlags=0;c->InfantryOwnerIndex=c->AltInfantryOwnerIndex=-1;
   }
   std::array<int,2002> directions;directions.fill(0x77777777);
   finder.Clear();finder.FindMode=0;
   auto* result=finder.FindPathRegular(start,end,&actor,directions.data(),limit,false);
   ASSERT_EQ(result!=nullptr,length!=0)<<"seed="<<seed<<" limit="<<limit;
   if(result){
    EXPECT_EQ(result->TotalDistance,cost)<<"seed="<<seed<<" limit="<<limit;ASSERT_EQ(result->PathLength,length)<<"seed="<<seed<<" limit="<<limit;
    EXPECT_EQ(result->StartCell,start);EXPECT_EQ(result->Directions,directions.data());
    for(int i=0;i<=length;++i)EXPECT_EQ(directions[i],expected[i])<<"seed="<<seed<<" limit="<<limit<<" direction="<<i;
    for(int i=0;i<length-1;++i)EXPECT_EQ(result->Levels[i],levels[i])<<"seed="<<seed<<" level="<<i;
   }
   ++cases;
  }
  EXPECT_EQ(cases,480u);
  finder.SearchID=-1;finder.Clear();EXPECT_EQ(finder.SearchID,1);
  finder.SearchID=INT_MAX;finder.Clear();EXPECT_EQ(finder.SearchID,INT_MIN);
 },nullptr));
}

TEST(AStarRegular, ThreatArrayIncludesBorderAndTeamQueryDoesNotIssueOrders) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  InfantryTypeClass type("THREAT_INF");HouseClass house(nullptr);InfantryClass actor(&type,&house);
  house.ThreatPosedEstimates[1][2]=99;house.ThreatPosedEstimates[2][3]=7;
  EXPECT_EQ(MapClass::Instance.GetThreatPosed({8,6},&house),7);
  EXPECT_EQ(MapClass::Instance.GetThreatPosed({11,7},&house),7);
  house.ThreatPosedEstimates[2][3]=0xFFFFFFFFu;EXPECT_EQ(MapClass::Instance.GetThreatPosed({8,6},&house),-1);
  TeamTypeClass team_type("THREAT_TEAM");TeamClass team(&team_type,&house,0);
  auto detach=ra2::test::scope_exit([&]{actor.Team=nullptr;});
  actor.ThreatAvoidanceCoefficient=0.25;EXPECT_DOUBLE_EQ(actor.ThreatAvoidanceValue(),0.25);
  actor.Team=&team;team_type.AvoidThreats=false;EXPECT_DOUBLE_EQ(actor.ThreatAvoidanceValue(),0.25);
  team_type.AvoidThreats=true;EXPECT_DOUBLE_EQ(actor.ThreatAvoidanceValue(),1.0);actor.Team=nullptr;
 },nullptr));
}

TEST(MapPathQueries, OriginalRegionThreatCorpus) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  auto& map=MapClass::Instance;HouseClass house(nullptr);
  ASSERT_EQ(map.SubzoneTracking[1].Count,0);ASSERT_EQ(map.SubzoneTracking[2].Count,0);
  auto restore=ra2::test::scope_exit([&]{map.SubzoneTracking[1].Clear();map.SubzoneTracking[2].Clear();});
  for(int level=1;level<=2;++level)for(int i=0;i<2;++i)ASSERT_TRUE(map.SubzoneTracking[level].AddItem(SubzoneTrackingStruct{}));
  std::ifstream input(RA2_MAP_REGION_THREAT_FIXTURE);ASSERT_TRUE(input.good());int seed,level,fx,fy,tx,ty,expected,previous=-1;unsigned count=0;
  while(input>>seed){
   ASSERT_TRUE(bool(input>>level>>fx>>fy>>tx>>ty>>expected));
   if(seed!=previous){for(int y=0;y<130;++y)for(int x=0;x<130;++x)house.ThreatPosedEstimates[y][x]=static_cast<unsigned>((x*37+y*53+seed*17)%257-128);previous=seed;}
   for(int l=1;l<=2;++l){map.SubzoneTracking[l][0].ThreatRegion=fx+1+130*(fy+1);map.SubzoneTracking[l][1].ThreatRegion=tx+1+130*(ty+1);}
   EXPECT_EQ(MapClass::RegionThreat(&house,level,0,1),expected)<<"case="<<count;++count;
  }
  EXPECT_EQ(count,3600u);
 },nullptr));
}

TEST(MapPathQueries, BridgeConnectionsPreserveLaneAndBrokenEndChoice) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  auto& map=MapClass::Instance;ASSERT_EQ(map.ZoneConnections.Count,0);
  const auto old_visible=map.VisibleRect;map.VisibleRect=map.MapRect;
  const int old_bridge=IsometricTileTypeClass::BridgeSet,old_wood=IsometricTileTypeClass::WoodBridgeSet;
  auto* old_zones=map.MovementZones[0];std::array<unsigned short,7> zones{100,101,102,103,104,105,106};map.MovementZones[0]=zones.data();
  IsometricTileTypeClass::BridgeSet=100;IsometricTileTypeClass::WoodBridgeSet=200;
  const std::vector<CellLevelPassabilityStruct> passability(map.LevelAndPassability,map.LevelAndPassability+map.ValidMapCellCount);
  struct Saved{CellClass* cell;CellFlags flags;int tile;LandType land;};std::vector<Saved> saved;
  for(int i=0;i<map.Cells.Capacity;++i)if(auto*c=map.Cells.Items[i]){saved.push_back({c,c->Flags,c->IsoTileTypeIndex,c->LandType});c->Flags=static_cast<CellFlags>(0);c->IsoTileTypeIndex=0;c->LandType=LandType::Clear;}
  auto restore=ra2::test::scope_exit([&]{map.VisibleRect=old_visible;map.MovementZones[0]=old_zones;map.ZoneConnections.Clear();
   IsometricTileTypeClass::BridgeSet=old_bridge;IsometricTileTypeClass::WoodBridgeSet=old_wood;
   std::copy(passability.begin(),passability.end(),map.LevelAndPassability);
   for(auto s:saved){s.cell->Flags=s.flags;s.cell->IsoTileTypeIndex=s.tile;s.cell->LandType=s.land;}
  });
  for(int i=0;i<map.ValidMapCellCount;++i)map.LevelAndPassability[i].ZoneArrayIndex=i%7;
  ASSERT_TRUE(map.ZoneConnections.AddItem(ZoneConnectionClass{{4,6},{12,6},true,0}));
  for(int y=5;y<=7;++y){for(int x=5;x<=11;++x)map.GetCellAt(CellStruct{short(x),short(y)})->Flags=static_cast<CellFlags>(0x900);
   map.GetCellAt(CellStruct{4,short(y)})->IsoTileTypeIndex=100;map.GetCellAt(CellStruct{12,short(y)})->IsoTileTypeIndex=200;}
  EXPECT_EQ(map.GetCellZoneIndex({-10,-10}),0);EXPECT_EQ(map.GetCellZoneIndex({100,100}),map.ValidMapCellCount-1);
  EXPECT_EQ(map.ZoneConnectionIndex({8,7},1,0),0);EXPECT_EQ(map.ZoneConnectionIndex({8,8},1,0),-1);
  CellStruct result;auto*middle=map.GetCellAt(CellStruct{8,7});
  MapClass::GetBridgeZoneConnectionCell(&result,middle,true);EXPECT_EQ(result,(CellStruct{12,7})); // Equal distances prefer To.
  MapClass::GetBridgeZoneConnectionCell(&result,map.GetCellAt(CellStruct{6,7}),true);EXPECT_EQ(result,(CellStruct{4,7}));
  MapClass::GetBridgeZoneConnectionCell(&result,middle,false);EXPECT_EQ(result,middle->MapCoords);
  EXPECT_EQ(map.GetMovementZoneType({8,7},MovementZone::Normal,true),zones[map.GetCellZoneIndex({4,6})%7]);
  EXPECT_EQ(map.GetMovementZoneType({8,7},MovementZone::Normal,false),zones[map.GetCellZoneIndex({8,7})%7]);
  map.ZoneConnections[0].IsPassable=false;
  MapClass::GetBridgeZoneConnectionCell(&result,middle,true);EXPECT_EQ(result,(CellStruct{12,7}));
  auto& end_subzone=map.LevelAndPassabilityStruct2pointer_70[map.GetCellZoneIndex({12,7})];
  end_subzone.SubzoneIDs[0]=11;map.FindBridgeEndCellForSubzone(&result,{8,7},0,11);EXPECT_EQ(result,(CellStruct{12,7}));
  map.FindBridgeEndCellForSubzone(&result,{8,7},0,12);EXPECT_EQ(result,(CellStruct{0,0}));end_subzone.SubzoneIDs[0]=0;
  map.FindBridgeEndCellForSubzone(&result,{4,7},0,12);EXPECT_EQ(result,(CellStruct{4,7}));
  // Original connection builder recognizes start/end tile variants and subtile
  // IDs; a missing middle deck marks the connection broken, not absent.
  const auto start_height=map.GetCellAt(CellStruct{4,6})->Height,end_height=map.GetCellAt(CellStruct{12,6})->Height;
  map.GetCellAt(CellStruct{4,6})->Height=7;map.GetCellAt(CellStruct{12,6})->Height=4;map.GetCellAt(CellStruct{12,6})->IsoTileTypeIndex=202;
  map.ComputeZoneConnections();ASSERT_EQ(map.ZoneConnections.Count,1);EXPECT_EQ(map.ZoneConnections[0].FromMapCoords,(CellStruct{4,6}));EXPECT_EQ(map.ZoneConnections[0].ToMapCoords,(CellStruct{12,6}));EXPECT_TRUE(map.ZoneConnections[0].IsPassable);
  map.GetCellAt(CellStruct{8,6})->Flags=static_cast<CellFlags>(0);map.ComputeZoneConnections();ASSERT_EQ(map.ZoneConnections.Count,1);EXPECT_FALSE(map.ZoneConnections[0].IsPassable);
  map.GetCellAt(CellStruct{4,6})->Height=start_height;map.GetCellAt(CellStruct{12,6})->Height=end_height;
  EXPECT_EQ(map.GetMovementZoneType({8,7},MovementZone::Normal,true),zones[map.GetCellZoneIndex({12,6})%7]);
  map.GetCellAt(CellStruct{12,7})->LandType=LandType::Rock;
  MapClass::GetBridgeZoneConnectionCell(&result,middle,true);EXPECT_EQ(result,(CellStruct{4,7}));
  EXPECT_EQ(map.GetMovementZoneType({8,7},MovementZone::Normal,true),zones[map.GetCellZoneIndex({4,6})%7]);
  map.ZoneConnections[0].ConnectionType=1;
  EXPECT_EQ(map.ZoneConnectionIndex({8,7},2,0),-1);EXPECT_EQ(map.GetMovementZoneType({8,7},MovementZone::Normal,true),-1);
  MapClass::FindBridgeSpanEndCell(&result,{8,7},{11,7});EXPECT_EQ(result,(CellStruct{4,7}));
  map.GetCellAt(CellStruct{12,7})->LandType=LandType::Clear;
  MapClass::FindBridgeSpanEndCell(&result,{8,7},{11,7});EXPECT_EQ(result,(CellStruct{12,7}));
  MapClass::GetBridgeZoneConnectionCell(&result,middle,true);EXPECT_EQ(result,(CellStruct{12,7}));
 },nullptr));
}

TEST(AStarHierarchy, OriginalThreeLevelGraphCorpus) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  auto& map=MapClass::Instance;InfantryTypeClass type("HIER_INF");HouseClass house(nullptr);InfantryClass actor(&type,&house);
  auto restore=ra2::test::scope_exit([&]{for(int level=0;level<3;++level){map.SubzoneTracking[level].Clear();map.SubzoneTrackingCounts[level]=0;}
   for(int i=0;i<map.ValidMapCellCount;++i)map.LevelAndPassabilityStruct2pointer_70[i]={};});
  for(int level=0;level<3;++level){ASSERT_EQ(map.SubzoneTracking[level].Count,0);map.SubzoneTrackingCounts[level]=8;
   for(int i=0;i<8;++i)ASSERT_TRUE(map.SubzoneTracking[level].AddItem(SubzoneTrackingStruct{}));
   map.LevelAndPassabilityStruct2pointer_70[map.GetCellZoneIndex({8,6})].SubzoneIDs[level]=1;
   map.LevelAndPassabilityStruct2pointer_70[map.GetCellZoneIndex({9,7})].SubzoneIDs[level]=6;}
  AStarClass finder;finder.UpdateMapDimensions(map.MapRect);finder.Reset();
  for(int y=0;y<130;++y)for(int x=0;x<130;++x)house.ThreatPosedEstimates[y][x]=(x*37+y*53)%127;
  std::ifstream input(RA2_ASTAR_HIERARCHY_FIXTURE);ASSERT_TRUE(input.good());int seed,movement,avoid,ban,result,previous=-1;unsigned cases=0;
  while(input>>seed){
   ASSERT_TRUE(bool(input>>movement>>avoid>>ban>>result));
   if(seed!=previous){
    for(int level=0;level<3;++level)for(int node=0;node<8;++node){auto& record=map.SubzoneTracking[level][node];record.SubzoneConnections.Clear();
     for(int n=1;n<8;++n)if(n!=node && ((node*11+n*7+seed*5)%9<3 || n==node+1))ASSERT_TRUE(record.SubzoneConnections.AddItem({unsigned(n),static_cast<BYTE>((node+n+seed)%2)}));
     record.ParentSubzoneID=node==1 || node==6 || seed%2==0?node:2+node%3;
     record.Passability=node==1 || node==6?0:(node+seed)%8;record.ThreatRegion=node%4+3+130*(node/4+3);}
    previous=seed;
   }
   finder.Clear();finder.SearchID=7;actor.ThreatAvoidanceCoefficient=avoid?0.25:0;
   for(int level=0;level<3;++level){
    finder.ZoneIndices[level].Clear();if(ban)ASSERT_TRUE(finder.ZoneIndices[level].AddItem(ban==1?(1u<<16|2u):(2u<<16|6u)));
    std::fill_n(finder.LevelVisitedMarkers[level],8,0);std::fill_n(finder.OpenSetMarkers[level],8,0);std::fill_n(finder.GCostArray[level],8,0.0f);
    std::fill_n(finder.PassabilityData[level].Indices,500,0);finder.PassabilityCounts[level]=0;
   }
   EXPECT_EQ(finder.FindPathHierarchical({8,6},{9,7},static_cast<MovementZone>(movement),&actor),result!=0)<<"case="<<cases;
   for(int level=0;level<3;++level){int count;ASSERT_TRUE(bool(input>>count));ASSERT_EQ(finder.PassabilityCounts[level],count)<<"case="<<cases<<" level="<<level;
    for(int i=0;i<count;++i){int id;ASSERT_TRUE(bool(input>>id));EXPECT_EQ(finder.PassabilityData[level].Indices[i],id)<<"case="<<cases<<" level="<<level<<" node="<<i;}
    for(int i=0;i<8;++i){int mark;ASSERT_TRUE(bool(input>>mark));EXPECT_EQ(finder.LevelVisitedMarkers[level][i],mark)<<"case="<<cases<<" level="<<level<<" mark="<<i;}}
   ++cases;
  }
  EXPECT_EQ(cases,936u);
  finder.Clear();EXPECT_TRUE(finder.FindPathHierarchical({8,6},{8,6},MovementZone::Infantry,nullptr));
  for(int level=0;level<3;++level){EXPECT_EQ(finder.PassabilityCounts[level],1);EXPECT_EQ(finder.PassabilityData[level].Indices[0],1);}
  finder.IsSearching=true;finder.BanNeighbourhoodSubzoneEdges(1,0);EXPECT_FALSE(finder.IsSearching);
 },nullptr));
}

TEST(AStarHierarchy, MainEntryUsesZonesAndRetriesBlockedCorridor) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  auto& map=MapClass::Instance;InfantryTypeClass type("MAIN_PATH_INF");HouseClass house(nullptr);house.ArrayIndex=0;InfantryClass actor(&type,&house);
  // Foot.GetDestination queries the retained locomotor, even for a null
  // path-output buffer. A directly constructed test actor has not run Init.
  type.Locomotor=LocomotionClass::CLSIDs::Walk;ASSERT_TRUE(actor.InitializeLocomotor());
  actor.CurrentMission=Mission::Move;actor.IsInPlayfield=true;type.MovementZone=MovementZone::Infantry;type.SpeedType=SpeedType::Foot;
  auto* old_zones=map.MovementZones[7];const auto old_visible=map.VisibleRect;map.VisibleRect=map.MapRect;
  const float old_clear=GroundType::Array[0].Cost[0],old_water=GroundType::Array[2].Cost[0];GroundType::Array[0].Cost[0]=1;GroundType::Array[2].Cost[0]=0;
  std::array<unsigned short,3> zones{0,1,2};map.MovementZones[7]=zones.data();
  std::vector<LandType> old_land;old_land.reserve(map.Cells.Capacity);
  for(int i=0;i<map.Cells.Capacity;++i)old_land.push_back(map.Cells.Items[i]?map.Cells.Items[i]->LandType:LandType::Clear);
  auto restore=ra2::test::scope_exit([&]{map.MovementZones[7]=old_zones;map.VisibleRect=old_visible;
   GroundType::Array[0].Cost[0]=old_clear;GroundType::Array[2].Cost[0]=old_water;
   for(int i=0;i<map.Cells.Capacity;++i)if(auto*c=map.Cells.Items[i])c->LandType=old_land[i];
   for(int level=0;level<3;++level){map.SubzoneTracking[level].Clear();map.SubzoneTrackingCounts[level]=0;}
   for(int i=0;i<map.ValidMapCellCount;++i){map.LevelAndPassabilityStruct2pointer_70[i]={};map.LevelAndPassability[i].ZoneArrayIndex=0;}
  });
  for(int level=0;level<3;++level){ASSERT_EQ(map.SubzoneTracking[level].Count,0);map.SubzoneTrackingCounts[level]=2;
   ASSERT_TRUE(map.SubzoneTracking[level].AddItem(SubzoneTrackingStruct{}));ASSERT_TRUE(map.SubzoneTracking[level].AddItem(SubzoneTrackingStruct{}));
   for(int i=0;i<map.ValidMapCellCount;++i)map.LevelAndPassabilityStruct2pointer_70[i].SubzoneIDs[level]=1;
  }
  AStarClass finder;finder.UpdateMapDimensions(map.MapRect);finder.Reset();std::array<int,2002> directions{};
  CellStruct start{8,6},end{10,7};auto* path=finder.FindPath(&start,&end,&actor,directions.data(),-1,MovementZone::None,0);
  ASSERT_NE(path,nullptr);CellStruct reached;AStarClass::FollowPath(&reached,&start,path->PathLength-1,directions.data());EXPECT_EQ(reached,end);
  EXPECT_EQ(finder.PassabilityCounts[0],1);
  EXPECT_EQ(finder.AttemptPath(&start,&end,&actor,false,false),2);
  actor.Location={start.X*256+128,start.Y*256+128,0};AStarClass::Instance.Reset();
  EXPECT_EQ(actor.FindPath(&end,nullptr,0,0,0,0),nullptr);
  auto* foot_path=actor.FindPath(&end,directions.data(),0,0,0,0);ASSERT_NE(foot_path,nullptr);EXPECT_EQ(foot_path->StartCell,start);
  actor.PathDirections[0]=2;foot_path=actor.FindPath(&end,directions.data(),0,0,1,0);ASSERT_NE(foot_path,nullptr);EXPECT_EQ(foot_path->StartCell,(CellStruct{9,6}));
  map.LevelAndPassability[map.GetCellZoneIndex(end)].ZoneArrayIndex=1;
  EXPECT_EQ(finder.FindPath(&start,&end,&actor,directions.data(),-1,MovementZone::None,0),nullptr);
  map.LevelAndPassability[map.GetCellZoneIndex(end)].ZoneArrayIndex=0;
  auto* source=map.GetCellAt(start);for(int face=0;face<8;++face)source->GetNeighbourCell(static_cast<FacingType>(face))->LandType=LandType::Water;
  EXPECT_EQ(finder.FindPath(&start,&end,&actor,directions.data(),-1,MovementZone::None,0),nullptr);
 },nullptr));
}

TEST(MapZones, OriginalFullMapZoneAndSubzoneGraphs) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  auto& map=MapClass::Instance;const auto old_visible=map.VisibleRect;map.VisibleRect=map.MapRect;
  const std::vector<CellLevelPassabilityStruct> saved(map.LevelAndPassability,map.LevelAndPassability+map.ValidMapCellCount);
  std::vector<char> heights(map.Cells.Capacity);for(int i=0;i<map.Cells.Capacity;++i)if(auto*c=map.Cells.Items[i])heights[i]=c->Level;
  auto restore=ra2::test::scope_exit([&]{map.VisibleRect=old_visible;std::copy(saved.begin(),saved.end(),map.LevelAndPassability);
   for(int i=0;i<map.Cells.Capacity;++i)if(auto*c=map.Cells.Items[i])c->Level=heights[i];
   for(auto& zones:map.MovementZones){YRMemory::Deallocate(zones);zones=nullptr;}map.somecount_4C=0;
   for(int level=0;level<3;++level){map.SubzoneTracking[level].Clear();map.SubzoneTrackingCounts[level]=0;}
   std::fill_n(map.LevelAndPassabilityStruct2pointer_70,map.ValidMapCellCount,LevelAndPassabilityStruct2{});
  });
  ASSERT_EQ(map.ValidMapCellCount,441);std::ifstream input(RA2_MAP_ZONES_FIXTURE);ASSERT_TRUE(input.good());int seed,best,count;unsigned cases=0;
  while(input>>seed){
   ASSERT_TRUE(bool(input>>best>>count));SCOPED_TRACE(seed);
   for(int y=0;y<21;++y)for(int x=0;x<21;++x){const bool valid=x+y>8 && x-y<8 && y-x<8 && x+y<=32;
    const int hashed=(x*17+y*31+seed*13)%19,pass=valid?(seed<4?0:(hashed+seed)%8):7;
    const char h=static_cast<char>(seed<8?0:(x/(2+seed%3)+y/3+seed)%5);map.LevelAndPassability[x+21*y]={static_cast<char>(pass),h,0};if(valid)map.GetCellAt(CellStruct{short(x),short(y)})->Level=h;}
   EXPECT_EQ(map.ResetAllZones(),best);ASSERT_EQ(map.somecount_4C,static_cast<unsigned>(count));
   for(int i=0;i<441;++i){int expected;ASSERT_TRUE(bool(input>>expected));ASSERT_EQ(map.LevelAndPassability[i].ZoneArrayIndex,expected)<<"cell="<<i;}
   for(int movement=0;movement<13;++movement)for(int i=0;i<count;++i){int expected;ASSERT_TRUE(bool(input>>expected));ASSERT_EQ(static_cast<unsigned short*>(map.MovementZones[movement])[i],expected)<<"movement="<<movement<<" zone="<<i;}
   for(int level=2;level>=0;--level){map.SubzoneTracking[level].Clear();map.ResetSubzone(level);int size;ASSERT_TRUE(bool(input>>size));ASSERT_EQ(map.SubzoneTrackingCounts[level],size)<<"level="<<level;
    for(int i=0;i<441;++i){int expected;ASSERT_TRUE(bool(input>>expected));ASSERT_EQ(map.LevelAndPassabilityStruct2pointer_70[i].SubzoneIDs[level],expected)<<"level="<<level<<" cell="<<i;}
    ASSERT_EQ(map.SubzoneTracking[level].Count,size);
    for(int i=0;i<size;++i){int parent,pass,region,links;ASSERT_TRUE(bool(input>>parent>>pass>>region>>links));const auto& entry=map.SubzoneTracking[level][i];
     EXPECT_EQ(entry.ParentSubzoneID,parent);EXPECT_EQ(entry.Passability,static_cast<unsigned>(pass));EXPECT_EQ(entry.ThreatRegion,static_cast<unsigned>(region));ASSERT_EQ(entry.SubzoneConnections.Count,links)<<"level="<<level<<" zone="<<i;
     for(int n=0;n<links;++n){int id,cross;ASSERT_TRUE(bool(input>>id>>cross));EXPECT_EQ(entry.SubzoneConnections[n].SubzoneID,static_cast<unsigned>(id))<<"level="<<level<<" zone="<<i<<" link="<<n;EXPECT_EQ(entry.SubzoneConnections[n].IsCrossBlock,cross);}
    }
   }
   ++cases;
  }
  EXPECT_EQ(cases,40u);
 },nullptr));
}

TEST(MapZones, OriginalCellPassabilityCorpus) {
 World world;
 ASSERT_TRUE(game::with_map_view(*world.view,[](void*){
  auto& map=MapClass::Instance;auto* cell=map.GetCellAt(CellStruct{8,6});const auto old_visible=map.VisibleRect;map.VisibleRect=map.MapRect;
  const auto old_land=cell->LandType;const auto old_theater=ScenarioClass::Instance->Theater;const auto old_pass=cell->Passability;const auto old_overlay=cell->OverlayTypeIndex;auto* old_first=cell->FirstObject;
  float costs[12];for(int i=0;i<12;++i)costs[i]=GroundType::Array[i].Cost[2];
  HouseClass house(nullptr);BuildingTypeClass building_type("PASS_BUILDING",BuildingTypeClass::ConstructionDefaults{});BuildingClass building(&building_type,&house);
  TerrainTypeClass terrain_type("PASS_TREE");TerrainClass terrain(&terrain_type,{8,6});OverlayTypeClass overlay("PASS_OVERLAY");
  auto restore=ra2::test::scope_exit([&]{map.VisibleRect=old_visible;cell->LandType=old_land;cell->Passability=old_pass;cell->OverlayTypeIndex=old_overlay;cell->FirstObject=old_first;ScenarioClass::Instance->Theater=old_theater;for(int i=0;i<12;++i)GroundType::Array[i].Cost[2]=costs[i];});
  std::ifstream input(RA2_CELL_PASSABILITY_FIXTURE);ASSERT_TRUE(input.good());int land,flags,cost,occupier,theater,expected;unsigned cases=0;
  while(input>>land){ASSERT_TRUE(bool(input>>flags>>cost>>occupier>>theater>>expected));
   cell->LandType=static_cast<LandType>(land);cell->OverlayTypeIndex=flags?overlay.ArrayIndex:-1;cell->FirstObject=occupier?(occupier<4?static_cast<ObjectClass*>(&building):static_cast<ObjectClass*>(&terrain)):nullptr;
   overlay.Crushable=flags==1;overlay.Wall=flags==2;overlay.IsVeins=flags==3;overlay.IsVeinholeMonster=flags==4;overlay.LandType=LandType::Clear;
   constexpr float values[]{1.0f,0.0f,0.01f,0.0101f};for(int i=0;i<12;++i)GroundType::Array[i].Cost[2]=values[cost];
   building_type.FirestormWall=occupier==1 || occupier==2;house.FirestormActive=occupier==2;building_type.LaserFence=occupier==3;building.LaserFenceFrame=0;
   terrain_type.TemperateOccupationBits=occupier==5?7:3;terrain_type.SnowOccupationBits=occupier==6?7:3;ScenarioClass::Instance->Theater=static_cast<TheaterType>(theater);
   cell->RecalcPassability();EXPECT_EQ(static_cast<int>(cell->Passability),expected)<<"case="<<cases;++cases;
  }
  EXPECT_EQ(cases,5376u);
 },nullptr));
}
