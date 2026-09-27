#include "support/test_support.hpp"
#include "map_world_fixture.hpp"
#include "map_world_internal.hpp"
#include "building_selection.hpp"
#include "yrpp/InfantryClass.h"
#include "yrpp/WalkLocomotionClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/AircraftTrackerClass.h"
#include "yrpp/FlyLocomotionClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/AStarClass.h"
#include "yrpp/TargetClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/TEventClass.h"
#include "yrpp/TActionClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/AITriggerTypeClass.h"
#include "yrpp/SwizzleManagerClass.h"
#include "scenario_loading.hpp"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/Unsorted.h"
#include "api/type_resources.hpp"
#include "scenario_runtime.hpp"
#include "yrpp/WaypointPathClass.h"
#include "yrpp/CellSpread.h"
#include "map_runtime.hpp"
#include "api/clock.hpp"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/WarheadTypeClass.h"
#include <cstdlib>
#include <array>
#include <cfenv>
#include <cmath>
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/BuildingLightClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/LineTrail.h"
#include "type_drawing.hpp"
#include "tactical_drawing.hpp"
#include "building_voxel.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/CommandClass.h"
#include "player_commands.hpp"
#include "game_ui_runtime.hpp"
#include "yrpp/MouseClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/FactoryClass.h"

namespace {
struct View {
 game::ResourceHandle* resources=nullptr;game::MapViewHandle* view=nullptr;
 explicit View(const std::string& path,bool bootstrap=false){std::string error;
  if(!game::create_resources(path,resources,error))throw std::runtime_error(error);
  if(bootstrap&&game::load_resources(*resources,{},error)!=game::ResourceLoadResult::complete){game::destroy_resources(resources);throw std::runtime_error(error);}
  if(!game::create_map_view(*resources,view)){game::destroy_resources(resources);throw std::runtime_error("Map allocation failed");}}
 ~View(){game::destroy_map_view(view);FileSystem::ClearNameCache();Destroy_All_Shapes();Unload_All_Shapes();game::destroy_resources(resources);}
 void load(const char* name){if(!game::load_map_view(*view,name,std::strlen(name)))throw std::runtime_error(game::map_view_error(*view));}
 std::vector<game::MapObjectSnapshot> objects(){std::uint32_t count=0;EXPECT_TRUE(game::copy_map_objects(*view,nullptr,0,count));
  std::vector<game::MapObjectSnapshot> out(count);EXPECT_TRUE(game::copy_map_objects(*view,out.data(),count,count));return out;}
};

InfantryClass* infantry_near_building(const char* id,HouseClass* owner,BuildingClass* building){
 auto* type=InfantryTypeClass::Find(id);if(!type)return nullptr;
 auto* actor=new InfantryClass(type,owner);if(!actor->InitializeLocomotor()){delete actor;return nullptr;}
 const auto center=building->GetMapCoords();
 for(int radius=2;radius<10;++radius)for(int y=-radius;y<=radius;++y)for(int x=-radius;x<=radius;++x){
  if(std::abs(x)!=radius&&std::abs(y)!=radius)continue;
  auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(center.X+x),short(center.Y+y)});
  if(!cell||!MapClass::Instance.IsWithinUsableArea(cell->MapCoords,true)||cell->GetBuilding())continue;
  if(actor->IsCellOccupied(cell,FacingType(-1),-1,nullptr,true)!=Move::OK)continue;
  if(actor->Unlimbo(cell->GetCoords(),DirType::North))return actor;
 }
 delete actor;return nullptr;
}

// Exercise the same picking and mouse down/up boundary used by the host.
// Passing an Action directly misses both input dispatch and modifier handling.
bool click_world_object(game::MapViewHandle& view,ObjectClass* target,unsigned modifiers=0){
 const auto center=TacticalClass::CoordsToScreen(target->Location);
 if(!game::center_map_view(view,center.X,center.Y))return false;
 struct Pick{game::MapViewHandle& view;ObjectClass* target;Point2D point{};bool found=false;}pick{view,target};
 if(!game::with_map_view(view,[](void* p){auto& pick=*static_cast<Pick*>(p);auto& world=*pick.view.world;
  game::rebuild_world_sprites(world);
  for(const auto& sprite:world.impl->sprites)if(sprite.owner==pick.target&&!sprite.shadow){
   const Point2D anchor=sprite.voxel?Point2D{sprite.position.X-sprite.voxel->offset.X,sprite.position.Y-sprite.voxel->offset.Y}:sprite.position;
   for(int y=-120;y<=10&&!pick.found;++y)for(int x=-70;x<=70&&!pick.found;++x){
    const Point2D at{anchor.X+x,anchor.Y+y};
    if(at.X<0||at.Y<0||at.X>=TacticalClass::ViewBounds.Width||at.Y>=TacticalClass::ViewBounds.Height)continue;
    if(game::pick_world_object(world,at)==pick.target){pick.point=at;pick.found=true;}
   }
  }
 },&pick)||!pick.found)return false;
 game::GameInputResult result;
 for(bool down:{true,false})if(!game::submit_game_input(view,{game::GameInputKind::pointer_button,pick.point.X,pick.point.Y,1,modifiers,down},result))return false;
 return true;
}
}

TEST(InfantryTransport, OriginalGunnerModeAndWeaponQueries){
 UnitTypeClass kind("GUNNER_CARRIER_TEST");InfantryTypeClass held("GUNNER_PASSENGER_TEST");
 kind.TurretCount=4;UnitClass carrier(&kind,nullptr);InfantryClass passenger(&held,nullptr);
 for(int i=0;i<18;++i)kind.TurretWeapon[i]=(i*7+2)%4;
 std::ifstream in(RA2_UNIT_GUNNER_FIXTURE);std::string magic;int count=0;in>>magic>>count;ASSERT_EQ(magic,"UNIT_GUNNER_V1");
 for(int i=0;i<count;++i){int mode,charge,weapon,turret,selected,empty_weapon,empty_turret;
  ASSERT_TRUE(bool(in>>mode>>charge>>weapon>>turret>>selected>>empty_weapon>>empty_turret));
  SCOPED_TRACE(i);held.IFVMode=mode;kind.IsChargeTurret=charge;carrier.CurrentWeaponNumber=13;carrier.CurrentTurretNumber=3;
  carrier.ReceiveGunner(&passenger);EXPECT_EQ(carrier.CurrentWeaponNumber,weapon);EXPECT_EQ(carrier.CurrentTurretNumber,turret);
  EXPECT_EQ(carrier.GetTurretWeapon(),&kind.Weapon[selected]);
  carrier.RemoveGunner(&passenger);EXPECT_EQ(carrier.CurrentWeaponNumber,empty_weapon);EXPECT_EQ(carrier.CurrentTurretNumber,empty_turret);
 }
 EXPECT_EQ(count,42);
}

TEST(VehicleCombat, FirstMapLethalShellRemovesTankAndIFV){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 for(const char* targetType:{"MTNK","FV"}){
  SCOPED_TRACE(targetType);
  View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
  struct State{game::MapViewHandle* view;const char* type;UnitClass* source=nullptr;UnitClass* target=nullptr;
   game::MapObjectId id{};TargetClass token{};CellStruct cell{};unsigned units=0;}s{view.view,targetType};
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   for(auto* unit:UnitClass::Array)if(unit->Owner==HouseClass::CurrentPlayer){
    if(!s.source&&!std::strcmp(unit->Type->ID,"MTNK"))s.source=unit;
    else if(!s.target&&!std::strcmp(unit->Type->ID,s.type))s.target=unit;
   }
   ASSERT_NE(s.source,nullptr);ASSERT_NE(s.target,nullptr);
   s.id=game::object_id(*s.view->world,s.target);s.token=TargetClass(s.target);s.cell=s.target->GetMapCoords();s.units=UnitClass::Array.Count;
   // Keep the original weapon/projectile/damage path; shorten only the target's health.
   s.target->Health=s.target->EstimatedHealth=1;
   ASSERT_TRUE(s.source->Select());
   EXPECT_GE(LogicClass::Instance.FindItemIndex(s.target),0); // Placed units join Logic before receiving orders.
  },&s));
  ASSERT_NE(s.source,nullptr);ASSERT_NE(s.target,nullptr);
  ASSERT_TRUE(click_world_object(*view.view,s.target,2));
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& world=*s.view->world;
   bool fired=false;int frames=0;
   for(;frames<600&&game::resolve_map_object(world,s.id);++frames){
    ++Unsorted::CurrentFrame;game::update_map_world(world);
    for(auto* bullet:BulletClass::Array)fired|=bullet->Owner==s.source;
   }
   EXPECT_TRUE(fired);
   auto* remaining=game::resolve_map_object(world,s.id);
   ASSERT_EQ(remaining,nullptr)<<"Lethal shell left vehicle alive="<<remaining->IsAlive<<" health="<<remaining->Health;
   EXPECT_EQ(UnitClass::Array.Count,int(s.units)-1);
   EXPECT_EQ(s.source->Target,nullptr);
   EXPECT_EQ(s.token.As_Object(),nullptr);
   auto* cell=MapClass::Instance.GetCellAt(s.cell);
   for(auto* object=cell->FirstObject;object;object=object->NextObject)EXPECT_NE(object,s.target);
   EXPECT_EQ(cell->OccupationFlags&0x20u,0u);
   game::rebuild_world_sprites(world);
   for(const auto& sprite:world.impl->sprites)EXPECT_NE(sprite.owner,s.target);
   for(int i=0;i<30;++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
   EXPECT_EQ(game::resolve_map_object(world,s.id),nullptr);
   std::cout<<"VEHICLE_DEATH "<<s.type<<" removed_after="<<frames<<"\n";
  },&s));
 }
}

TEST(VehicleCombat, RemovalWithoutEffectsInvalidatesCachedSprites){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& world=*static_cast<game::MapViewHandle*>(p)->world;
  UnitClass* victim=nullptr;
  for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"MTNK")){victim=unit;break;}
  ASSERT_NE(victim,nullptr);victim->Type->Explosion.Count=victim->Type->DestroyAnim.Count=0;
  victim->Type->MaxDebris=0;victim->Type->Crewed=false;victim->Deselect();
  game::rebuild_world_sprites(world);
  ASSERT_TRUE(std::any_of(world.impl->sprites.begin(),world.impl->sprites.end(),[&](const auto& sprite){return sprite.owner==victim;}));
  const auto id=game::object_id(world,victim);const auto revision=world.presentation_revision;
  int damage=victim->Health;
  ASSERT_EQ(victim->ReceiveDamage(&damage,0,RulesClass::Instance->C4Warhead,nullptr,true,true,nullptr),DamageState::NowDead);
  AbstractClass::RemoveAllInactive();
  EXPECT_EQ(game::resolve_map_object(world,id),nullptr);EXPECT_GT(world.presentation_revision,revision);
  game::rebuild_world_sprites(world);
  for(const auto& sprite:world.impl->sprites)EXPECT_NE(sprite.owner,victim);
 },view.view));
}

TEST(VehicleCombat, DestroyedIFVResolvesPassengerBeforeDeletion){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 for(bool preventEscape:{false,true}){
  SCOPED_TRACE(preventEscape);
  View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1024,768));
  struct State{game::MapViewHandle* view;UnitClass* carrier=nullptr;InfantryClass* engineer=nullptr;bool prevent;}s{view.view,nullptr,nullptr,preventEscape};
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"FV")&&unit->Owner==HouseClass::CurrentPlayer){s.carrier=unit;break;}
   for(auto* unit:InfantryClass::Array)if(!std::strcmp(unit->Type->ID,"ENGINEER")&&unit->Owner==HouseClass::CurrentPlayer){s.engineer=unit;break;}
   ASSERT_NE(s.carrier,nullptr);ASSERT_NE(s.engineer,nullptr);ASSERT_TRUE(s.engineer->Select());
  },&s));
  ASSERT_NE(s.carrier,nullptr);ASSERT_NE(s.engineer,nullptr);
  ASSERT_TRUE(click_world_object(*view.view,s.carrier));
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& world=*s.view->world;
   for(int i=0;i<1200&&s.carrier->Passengers.NumPassengers!=1;++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
   ASSERT_EQ(s.carrier->Passengers.FirstPassenger,s.engineer);ASSERT_TRUE(s.engineer->InLimbo);
   ASSERT_TRUE(s.carrier->Select());
   const auto carrierId=game::object_id(world,s.carrier),engineerId=game::object_id(world,s.engineer);
   const int animations=AnimClass::Array.Count;
   int damage=s.carrier->Health;
   EXPECT_EQ(s.carrier->ReceiveDamage(&damage,0,RulesClass::Instance->C4Warhead,nullptr,true,s.prevent,nullptr),DamageState::NowDead);
   EXPECT_FALSE(s.carrier->IsAlive);EXPECT_TRUE(s.carrier->InLimbo);
   EXPECT_EQ(s.carrier->Passengers.NumPassengers,0);EXPECT_GT(AnimClass::Array.Count,animations);
   if(!s.prevent){EXPECT_TRUE(s.engineer->IsAlive);EXPECT_TRUE(s.engineer->IsOnMap);EXPECT_FALSE(s.engineer->InLimbo);
    EXPECT_EQ(s.engineer->Transporter,nullptr);EXPECT_TRUE(s.engineer->IsSelected);}
   AbstractClass::RemoveAllInactive();
   EXPECT_EQ(game::resolve_map_object(world,carrierId),nullptr);
   EXPECT_EQ(game::resolve_map_object(world,engineerId)!=nullptr,!s.prevent);
   for(int i=0;i<30;++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
  },&s));
 }
}

TEST(VehicleParasite, InitializesFromWeaponAndReleasesManager){
 const int before=ParasiteClass::Array.Count;
 WarheadTypeClass warhead("VEHICLE_PARASITE_WH");WeaponTypeClass weapon("VEHICLE_PARASITE_WEAPON");
 UnitTypeClass type("VEHICLE_PARASITE_TEST");weapon.Warhead=&warhead;
 {
  UnitClass unarmed(&type,nullptr);EXPECT_EQ(unarmed.ParasiteImUsing,nullptr);
  type.Weapon[0].WeaponType=&weapon;
  UnitClass ordinary(&type,nullptr);EXPECT_EQ(ordinary.ParasiteImUsing,nullptr);
  warhead.Parasite=true;
  UnitClass parasite(&type,nullptr);
  ASSERT_NE(parasite.ParasiteImUsing,nullptr);
  EXPECT_EQ(parasite.ParasiteImUsing->Owner,&parasite);
  EXPECT_EQ(parasite.ParasiteImUsing->Victim,nullptr);
  EXPECT_EQ(ParasiteClass::Array.Count,before+1);
 }
 EXPECT_EQ(ParasiteClass::Array.Count,before);
}

TEST(VehicleParasite, RealRulesInitializeDroneAndSquid){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 const int before=ParasiteClass::Array.Count;
 {
  View view(data,true);view.load("ALL01UMD.MAP");
  ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
   const int baseline=ParasiteClass::Array.Count;
   for(const char* id:{"DRON","SQD"}){
    SCOPED_TRACE(id);auto* type=UnitTypeClass::Find(id);ASSERT_NE(type,nullptr);
    UnitClass unit(type,HouseClass::CurrentPlayer);
    ASSERT_TRUE(unit.GetWeapon(0)->WeaponType->Warhead->Parasite);
    ASSERT_NE(unit.ParasiteImUsing,nullptr);EXPECT_EQ(unit.ParasiteImUsing->Owner,&unit);
    EXPECT_FALSE(unit.ParasiteImUsing->CanInfect(nullptr));
   }
   EXPECT_EQ(ParasiteClass::Array.Count,baseline);
  },nullptr));
 }
 EXPECT_EQ(ParasiteClass::Array.Count,before);
}

TEST(VehicleParasite, AreaGuardInfectsVehicleAndReturnsAfterKill){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 const int before=ParasiteClass::Array.Count;
 {
  View view(data,true);view.load("ALL01UMD.MAP");
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& world=*static_cast<game::MapViewHandle*>(p)->world;
   UnitClass* target=nullptr;
   for(auto* unit:UnitClass::Array)if(unit->Owner==HouseClass::CurrentPlayer&&!std::strcmp(unit->Type->ID,"MTNK")){target=unit;break;}
   ASSERT_NE(target,nullptr);
   HouseClass* enemy=nullptr;
   for(auto* house:HouseClass::Array)if(!house->Type->MultiplayPassive&&!house->IsAlliedWith(target->Owner)){enemy=house;break;}
   ASSERT_NE(enemy,nullptr);
   // Keep the target stationary and prevent unrelated retaliation; acquisition,
   // launch, parasite damage, destruction and exit all use normal game paths.
   for(auto* unit:UnitClass::Array){unit->SetTarget(nullptr);unit->ForceMission(Mission::Sleep);}
   for(auto* actor:InfantryClass::Array){actor->SetTarget(nullptr);actor->ForceMission(Mission::Sleep);}
   target->Health=target->EstimatedHealth=100;
   auto* drone=new UnitClass(UnitTypeClass::Find("DRON"),enemy);
   ASSERT_NE(drone->ParasiteImUsing,nullptr);ASSERT_TRUE(drone->InitializeLocomotor());
   bool placed=false;const auto center=target->GetMapCoords();
   for(int radius=1;radius<=3&&!placed;++radius)for(int y=-radius;y<=radius&&!placed;++y)for(int x=-radius;x<=radius&&!placed;++x){
    if(std::abs(x)!=radius&&std::abs(y)!=radius)continue;
    auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(center.X+x),short(center.Y+y)});
    if(!cell||cell->SlopeIndex||cell->GetBuilding()||!MapClass::Instance.IsWithinUsableArea(cell->MapCoords,true)
       ||drone->IsCellOccupied(cell,FacingType(-1),-1,nullptr,true)!=Move::OK)continue;
    placed=drone->Unlimbo(cell->GetCoords(),DirType::North);
   }
   ASSERT_TRUE(placed);drone->ForceMission(Mission::Area_Guard);drone->SetTarget(nullptr);drone->TargetingTimer.Start(0);
   EXPECT_NE(drone->GetFireErrorWithoutRange(target,0),FireError::ILLEGAL);
   drone->Mission_AreaGuard();ASSERT_NE(drone->Target,nullptr);
   const auto droneId=game::object_id(world,drone),targetId=game::object_id(world,target);
   bool infected=false,damaged=false,fired=false;int frame=0;
   for(;frame<1800&&game::resolve_map_object(world,targetId);++frame){
    ++Unsorted::CurrentFrame;game::update_map_world(world);
    ASSERT_NE(game::resolve_map_object(world,droneId),nullptr)<<"Drone died before parasite completed";
    fired|=drone->LastFireBulletFrame>0;
    if(auto* victim=game::resolve_map_object(world,targetId)){
     infected|=static_cast<FootClass*>(victim)->ParasiteEatingMe==drone;
     damaged|=victim->Health<100;
    }
   }
   EXPECT_TRUE(fired);EXPECT_TRUE(infected);EXPECT_TRUE(damaged);
   ASSERT_EQ(game::resolve_map_object(world,targetId),nullptr)<<"Parasite did not finish within "<<frame<<" frames";
   ASSERT_EQ(game::resolve_map_object(world,droneId),drone);
   EXPECT_TRUE(drone->IsAlive);EXPECT_FALSE(drone->InLimbo);EXPECT_TRUE(drone->IsOnMap);
   EXPECT_EQ(drone->ParasiteImUsing->Victim,nullptr);
   std::cout<<"DRONE parasite completed frame="<<frame<<" hp="<<drone->Health<<"\n";
  },view.view));
 }
 EXPECT_EQ(ParasiteClass::Array.Count,before);
}

TEST(VehicleFiring, OriginalTurretFacingAndMuzzleCoordinates){
 struct FlatDrive final:DriveLocomotionClass{
  Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override{return LocomotionClass::Draw_Matrix(key);}
 }drive;
 UnitTypeClass type("MUZZLE_TEST");UnitClass unit(&type,nullptr);
 unit.Locomotor=&drive;drive.LinkedTo=&unit;unit.Location={25216,28800,416};
 auto cleanup=ra2::test::scope_exit([&]{unit.Locomotor=nullptr;drive.LinkedTo=nullptr;});
 std::ifstream input(RA2_UNIT_FIRE_COORDINATES_FIXTURE);std::string format;int count=0;
 ASSERT_TRUE(bool(input>>format>>count));ASSERT_EQ(format,"UNIT_FIRE_COORDINATES_V1");
 int different=0;
 for(int i=0;i<count;++i){
  int turreted,hull,turret,offset,burst,facing;CoordStruct flh,base,expected,actual;
  ASSERT_TRUE(bool(input>>turreted>>hull>>turret>>offset>>burst>>flh.X>>flh.Y>>flh.Z>>base.X>>base.Y>>base.Z>>facing>>expected.X>>expected.Y>>expected.Z));
  type.Turret=turreted;type.TurretOffset=offset;type.Weapon[0].FLH=flh;
  unit.PrimaryFacing.SetCurrent(DirStruct(hull));unit.SecondaryFacing.SetCurrent(DirStruct(turret));unit.CurrentBurstIndex=burst;
  const TechnoClass& actor=unit;actor.GetFLH(&actual,0,base);
  const bool mismatch=actor.TurretFacing().Raw!=facing||actual!=expected;
  if(mismatch&&different++<8)std::cout<<"MUZZLE_DIFF row="<<i<<" facing="<<actor.TurretFacing().Raw<<"/"<<facing
   <<" xyz="<<actual.X<<','<<actual.Y<<','<<actual.Z<<" expected="<<expected.X<<','<<expected.Y<<','<<expected.Z<<'\n';
 }
 EXPECT_EQ(count,2240);EXPECT_EQ(different,0);
}

TEST(BuildingFiring, OriginalTurretAndPixelMuzzleCoordinates){
 TacticalClass tactical({}, {},0,1,0,1,1);
 BuildingTypeClass type("BUILDING_MUZZLE",BuildingTypeClass::ConstructionDefaults{});
 BuildingClass building(&type,nullptr);building.Location={25216,28800,416};
 std::ifstream input(RA2_BUILDING_FIRE_COORDINATES_FIXTURE);std::string format;int count=0;
 ASSERT_TRUE(bool(input>>format>>count));ASSERT_EQ(format,"BUILDING_FIRE_COORDINATES_V1");
 int different=0;
 for(int i=0;i<count;++i){
  int mode,x,y,facing,burst;CoordStruct fire,mount,actualFire,actualMount;
  ASSERT_TRUE(bool(input>>mode>>x>>y>>facing>>burst>>fire.X>>fire.Y>>fire.Z>>mount.X>>mount.Y>>mount.Z));
  type.TurretAnimIsVoxel=mode==1;type.PrimaryFireDualOffset=mode==3;
  type.BuildingAnim[int(BuildingAnimSlot::Turret)].Position={x,y};
  type.PrimaryFirePixelOffset=mode>=2?Point2D{x,y}:Point2D{0xFFFF,0xFFFF};
  type.Weapon[0].FLH={250,burst?17:0,230};
  building.PrimaryFacing.SetCurrent(DirStruct(facing));building.CurrentBurstIndex=burst;
  building.GetFLH(&actualFire,0,{3,5,7});building.vt_entry_300(&actualMount,0);
  if(actualFire!=fire||actualMount!=mount){
   if(different++<8)std::cout<<"BUILDING_MUZZLE_DIFF mode="<<mode<<" facing="<<facing
    <<" actual="<<actualFire.X<<','<<actualFire.Y<<','<<actualFire.Z
    <<" expected="<<fire.X<<','<<fire.Y<<','<<fire.Z<<'\n';
  }
 }
 EXPECT_EQ(count,3072);EXPECT_EQ(different,0);
}

TEST(BuildingFiring, OriginalBarrelMatrixAndBranchPriority){
 TacticalClass tactical({}, {},0,1,0,1,1);
 BuildingTypeClass type("BARREL_MUZZLE",BuildingTypeClass::ConstructionDefaults{});
 WeaponTypeClass weapon("BARREL_WEAPON");type.Weapon[0].WeaponType=&weapon;type.Weapon[0].FLH={250,17,230};
 BuildingClass building(&type,nullptr);building.Location={25216,28800,416};building.FiringOccupantIndex=2;
 ASSERT_TRUE(building.Occupants.AddItem(nullptr));
 auto cleanup=ra2::test::scope_exit([&]{building.Occupants.Count=0;});
 std::ifstream input(RA2_BUILDING_FIRE_EXTENDED_FIXTURE);std::string format;int count=0;
 ASSERT_TRUE(bool(input>>format>>count));ASSERT_EQ(format,"BUILDING_FIRE_EXTENDED_V1");
 int different=0,matrixDifferent=0;
 constexpr Point2D pixels[]{{3,28},{-17,23},{0,0}};
 constexpr CoordStruct pivots[3][3]{{{0,0,0},{0,0,0},{0,0,0}},{{1,2,3},{4,5,6},{7,8,9}},{{-13,27,5},{12,-19,8},{7,22,-11}}};
 constexpr CoordStruct ends[]{{250,30,230},{135,-17,32},{61,19,-12}};
 constexpr double scales[]{1.0,1.3,0.75};constexpr int bursts[]{1,2,4};
 for(int i=0;i<count;++i){
  int flags,yaw,pitch,burst,config;CoordStruct fire,mount,actualFire,actualMount;std::string bits;
  ASSERT_TRUE(bool(input>>flags>>yaw>>pitch>>burst>>config>>fire.X>>fire.Y>>fire.Z>>mount.X>>mount.Y>>mount.Z>>bits));
  ASSERT_GE(config,0);ASSERT_LT(config,3);ASSERT_EQ(bits.size(),96u);
  const auto pixel=pixels[config];type.TurretAnimIsVoxel=flags&1;type.BarrelAnimIsVoxel=flags&2;
  type.PrimaryFirePixelOffset=flags&4?pixel:Point2D{0xFFFF,0xFFFF};type.PrimaryFireDualOffset=flags&8;
  type.CanBeOccupied=flags&16;building.Occupants.Count=(flags&32)?1:0;
  type.BuildingAnim[int(BuildingAnimSlot::Turret)].Position=pixel;type.MuzzleFlash[2]={pixel.X+9,pixel.Y-7};
  type.VoxelBarrelScale=scales[config];type.VoxelBarrelOffsetToBuildingPivotPoint=pivots[config][0];
  type.VoxelBarrelOffsetToRotatePivotPoint=pivots[config][1];type.VoxelBarrelOffsetToPitchPivotPoint=pivots[config][2];
  type.VoxelBarrelOffsetToBarrelEnd=ends[config];weapon.Burst=bursts[config];building.CurrentBurstIndex=burst;
  building.PrimaryFacing.SetCurrent(DirStruct(yaw));building.BarrelFacing.SetCurrent(DirStruct(pitch));
  building.GetFLH(&actualFire,0,{3,5,7});building.vt_entry_300(&actualMount,0);
  if(actualFire!=fire||actualMount!=mount){
   if(different++<8)std::cout<<"BARREL_COORD_DIFF row="<<i<<" flags="<<flags<<" yaw="<<yaw<<" pitch="<<pitch
    <<" actual="<<actualFire.X<<','<<actualFire.Y<<','<<actualFire.Z<<" expected="<<fire.X<<','<<fire.Y<<','<<fire.Z<<'\n';
  }
  Matrix3D matrix;EXPECT_EQ(building.GetVoxelBarrelOffsetMatrix(matrix),&matrix);
  for(unsigned n=0;n<12;++n){std::uint32_t expected=0;
   for(unsigned b=0;b<4;++b)expected|=std::uint32_t(std::stoul(bits.substr(n*8+b*2,2),nullptr,16))<<(b*8);
   if(std::bit_cast<std::uint32_t>(matrix.Data[n])!=expected){
    if(matrixDifferent++<8)std::cout<<"BARREL_MATRIX_DIFF row="<<i<<" word="<<n<<" actual=0x"<<std::hex
     <<std::bit_cast<std::uint32_t>(matrix.Data[n])<<" expected=0x"<<expected<<std::dec<<'\n';
   }
  }
 }
 EXPECT_EQ(count,9216);EXPECT_EQ(different,0);EXPECT_EQ(matrixDifferent,0);
}

TEST(BuildingFiring, GrandCannonLaunchAndMuzzleFlash){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL02UMD.MAP");
 CoordStruct reference{};bool found=false;
 std::ifstream input(RA2_BUILDING_FIRE_COORDINATES_FIXTURE);std::string format;int count;
 ASSERT_TRUE(bool(input>>format>>count));
 for(int i=0;i<count;++i){int mode,x,y,facing,burst;CoordStruct fire,mount;
  ASSERT_TRUE(bool(input>>mode>>x>>y>>facing>>burst>>fire.X>>fire.Y>>fire.Z>>mount.X>>mount.Y>>mount.Z));
  if(mode==1&&x==3&&y==28&&facing==0&&burst==0){reference=fire;found=true;}
 }
 ASSERT_TRUE(found);
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){
  const auto reference=*static_cast<CoordStruct*>(p);
  auto* type=BuildingTypeClass::Find("GTGCAN");ASSERT_NE(type,nullptr);
  ASSERT_TRUE(type->TurretAnimIsVoxel);ASSERT_FALSE(type->BarrelAnimIsVoxel);
  ASSERT_EQ(type->BuildingAnim[int(BuildingAnimSlot::Turret)].Position,(Point2D{3,28}));
  ASSERT_EQ(type->Weapon[0].FLH,(CoordStruct{250,0,230}));
  ASSERT_EQ(type->PrimaryFirePixelOffset,(Point2D{0xFFFF,0xFFFF}));
  CellClass* origin=nullptr;CellClass* target=nullptr;auto& map=MapClass::Instance;
  for(int i=0;i<map.Cells.Capacity&&!origin;++i)if(auto* cell=map.Cells[i]){
   const auto at=cell->MapCoords;
   if(!map.IsWithinUsableArea(at,true))continue;
   bool free=true;
   for(int y=-1;y<=2;++y)for(int x=-1;x<=2;++x){
    auto* c=map.TryGetCellAt(CellStruct{short(at.X+x),short(at.Y+y)});
    if(!c||!map.IsWithinUsableArea(c->MapCoords,true)||c->OccupationFlags||c->Level!=cell->Level||c->LandType==LandType::Water)free=false;
   }
   auto* aim=map.TryGetCellAt(CellStruct{at.X,short(at.Y-6)});
   if(free&&aim&&map.IsWithinUsableArea(aim->MapCoords,true)){origin=cell;target=aim;}
  }
  ASSERT_NE(origin,nullptr);ASSERT_NE(target,nullptr);
  auto* cannon=new BuildingClass(type,HouseClass::CurrentPlayer);
  ++Unsorted::ScenarioInit;const bool placed=cannon->Unlimbo(origin->GetCoords(),DirType::North);--Unsorted::ScenarioInit;
  ASSERT_TRUE(placed);cannon->ForceMission(Mission::Guard);
  cannon->SetTarget(target);
  const CoordStruct expected{cannon->Location.X+reference.X-25216,cannon->Location.Y+reference.Y-28800,cannon->Location.Z+reference.Z-416};
  const int animations=AnimClass::Array.Count;
  auto* bullet=cannon->Fire(target,0);ASSERT_NE(bullet,nullptr);
  EXPECT_EQ(bullet->SourceCoords,expected);EXPECT_EQ(bullet->Location,expected);
  bool flash=false;
  for(int i=animations;i<AnimClass::Array.Count;++i)if(auto* anim=AnimClass::Array[i];anim->Type&&!std::strcmp(anim->Type->ID,"GCMUZZLE")){
   flash=true;EXPECT_EQ(anim->Location,expected);
  }
  EXPECT_TRUE(flash);
 },&reference));
}

TEST(VehicleFiring, OriginalHullAndTurretFacingAdmission){
 struct FacingUnit final:UnitClass{
  using UnitClass::UnitClass;
  int SelectWeapon(AbstractClass*) const override{return 0;}
  FireError GetFireError(AbstractClass*,int,bool) const override{return FireError::FACING;}
  bool CanDeployNow() const override{return false;}
 };
 struct MovingDrive final:DriveLocomotionClass{
  bool moving=false;
  bool YRPP_STDCALL Is_Moving() override{return moving;}
 }drive;
 UnitTypeClass type("FIRING_FACING_TEST");WeaponTypeClass weapon("FIRING_FACING_WEAPON");
 FacingUnit unit(&type,nullptr);UnitClass target(&type,nullptr);type.Weapon[0].WeaponType=&weapon;
 unit.Locomotor=&drive;drive.LinkedTo=&unit;unit.Target=&target;unit.Location={0,0,0};
 const int savedFrame=Unsorted::CurrentFrame;
 auto cleanup=ra2::test::scope_exit([&]{unit.Locomotor=nullptr;drive.LinkedTo=nullptr;unit.Target=nullptr;unit.Destination=nullptr;Unsorted::CurrentFrame=savedFrame;});
 std::ifstream input(RA2_UNIT_FIRING_FACING_FIXTURE);std::string format;int count=0;
 ASSERT_TRUE(bool(input>>format>>count));ASSERT_EQ(format,"UNIT_FIRING_FACING_V1");
 for(int row=0;row<count;++row){
  int turret,locked,destination,moving,y;std::array<unsigned,6> expected{};
  ASSERT_TRUE(bool(input>>turret>>locked>>destination>>moving>>y));for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
  for(bool voxel:{false,true}){
   SCOPED_TRACE(::testing::Message()<<"row="<<row<<" voxel="<<voxel);
   type.Turret=turret;type.HasTurret=locked;type.Voxel=voxel;drive.moving=moving;
   unit.Destination=destination?&target:nullptr;target.Location={512,y,0};Unsorted::CurrentFrame=100;
   unit.PrimaryFacing=FacingClass(5);unit.SecondaryFacing=FacingClass(5);
   unit.PrimaryFacing.SetCurrent(DirStruct(0));unit.SecondaryFacing.SetCurrent(DirStruct(0x8000));
   unit.UpdateFiring();
   const std::array<unsigned,6> actual={unit.PrimaryFacing.Desired().Raw,unit.PrimaryFacing.StartFacing.Raw,
    unsigned(unit.PrimaryFacing.RotationTimer.GetTimeLeft()),unit.SecondaryFacing.Desired().Raw,
    unit.SecondaryFacing.StartFacing.Raw,unsigned(unit.SecondaryFacing.RotationTimer.GetTimeLeft())};
   EXPECT_EQ(actual,expected);
  }
 }
 EXPECT_EQ(count,32);
}

TEST(MindControlDrawing, OriginalTacticalDispatch) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*p){auto&view=*static_cast<View*>(p);std::vector<int> calls;
  struct Probe:UnitClass{
   int id;std::vector<int>&calls;
   Probe(int id,std::vector<int>&calls):UnitClass(UnitTypeClass::Find("FV"),HouseClass::CurrentPlayer),id(id),calls(calls){}
   void DrawActionLines(bool force,DWORD unknown)override{if(id==0){EXPECT_FALSE(force);EXPECT_EQ(unknown,0u);calls.push_back(0);}}
   CoordStruct* GetFLH(CoordStruct*out,int,CoordStruct)const override{calls.push_back(id);*out={0,0,0};return out;}
  } actor(0,calls),firstOwner(1,calls),controller(2,calls),target(3,calls);
  // Separate query owners let the two NeedsToDrawLinks booleans vary
  // independently of the actor's selection flag, like the oracle's doubles.
  CaptureManagerClass first(&firstOwner,1,false),second(&controller,1,false);
  for(auto*manager:{&first,&second}){auto*node=GameCreate<ControlNode>();node->Unit=&target;node->OriginalOwner=HouseClass::CurrentPlayer;
   ::new(&node->LinkDrawTimer)CDTimerClass;ASSERT_TRUE(manager->ControlNodes.AddItem(node));}
  auto*player=HouseClass::CurrentPlayer;HouseClass*other=nullptr;for(auto*house:HouseClass::Array)if(house!=player){other=house;break;}ASSERT_NE(other,nullptr);
  firstOwner.Health=controller.Health=target.Health=0;actor.IsSelected=true;controller.CaptureManager=&second;
  const auto oldMode=view.view->loop.mode;const bool oldPlan=PlanningNodeClass::PlanningModeActive,oldLines=TechnoClass::ActionLines;
  auto restore=ra2::test::scope_exit([&]{actor.CaptureManager=controller.CaptureManager=nullptr;actor.MindControlledBy=nullptr;
   actor.IsSelected=firstOwner.IsSelected=controller.IsSelected=false;view.view->loop.mode=oldMode;
   PlanningNodeClass::PlanningModeActive=oldPlan;TechnoClass::ActionLines=oldLines;});
  view.view->loop.mode=GameMode::Skirmish;
  game::MapDrawingContext drawing;drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(&calls);
  drawing.shape_palette=[](void*,const BytePalette&pal,int,const game::DrawingPaletteHandle*&out)noexcept{out=reinterpret_cast<const game::DrawingPaletteHandle*>(&pal);return game::DrawingStatus::drawn;};
  drawing.color_scheme_palette=drawing.shape_palette;
  drawing.terrain_palette=[](void*,const BytePalette&pal,int,int,int,int,const game::DrawingPaletteHandle*&out)noexcept{out=reinterpret_cast<const game::DrawingPaletteHandle*>(&pal);return game::DrawingStatus::drawn;};
  drawing.types.backend.shape=[](void*,const game::ShapeDrawingRequest&){return game::DrawingStatus::drawn;};
  drawing.types.backend.indexed=[](void*,const game::IndexedDrawingRequest&){return game::DrawingStatus::drawn;};
  drawing.types.backend.raster=[](void*,const game::RasterDrawingRequest&){return game::DrawingStatus::drawn;};
  std::ifstream input(RA2_MIND_CONTROL_DISPATCH_FIXTURE);std::string magic;int count;
  ASSERT_TRUE(bool(input>>magic>>count));ASSERT_EQ(magic,"MIND_CONTROL_DISPATCH_V1");ASSERT_EQ(count,384);
  for(int row=0;row<count;++row){int control,planning,health,pointers,needed,on,n;
   ASSERT_TRUE(bool(input>>control>>planning>>health>>pointers>>needed>>on>>n));SCOPED_TRACE(row);
   std::vector<int> expected(n);for(auto&value:expected)ASSERT_TRUE(bool(input>>value));
   actor.Owner=control?player:other;actor.Health=health;actor.CaptureManager=(pointers&1)?&first:nullptr;actor.MindControlledBy=(pointers&2)?&controller:nullptr;
   firstOwner.IsSelected=needed&1;controller.IsSelected=needed&2;PlanningNodeClass::PlanningModeActive=planning;TechnoClass::ActionLines=on;
   calls.clear();game::MapDrawStatistics stats;
   EXPECT_TRUE(game::drawing_completed(game::draw_map_world(*view.view->world,drawing,TacticalClass::ViewBounds,stats)));
   EXPECT_EQ(calls,expected);
  }
 },&view));
}

TEST(MindControlDrawing, OriginalCurvePixels) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*p){auto&view=*static_cast<View*>(p);
  UnitClass actor(UnitTypeClass::Find("FV"),HouseClass::CurrentPlayer);
  const auto oldBounds=DSurface::ViewBounds,oldView=TacticalClass::ViewBounds;const auto oldCamera=view.view->tactical.TacticalPos;
  auto restore=ra2::test::scope_exit([&]{DSurface::ViewBounds=oldBounds;TacticalClass::ViewBounds=oldView;view.view->tactical.TacticalPos=oldCamera;});
  view.view->tactical.TacticalPos={-10,-80};
  struct State{UnitClass&actor;CoordStruct from{},to{};ColorStruct color{};std::uint32_t ms=0;std::array<WORD,128*128> pixels{};}s{actor};
  game::TypeDrawingContext drawing;drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(&s);drawing.backend_context=&s;
  drawing.backend.raster=[](void*p,const game::RasterDrawingRequest&r){auto&s=*static_cast<State*>(p);
   for(int y=r.position.Y;y<r.position.Y+r.height;++y)for(int x=r.position.X;x<r.position.X+r.width;++x)
    if(x>=0&&x<128&&y>=0&&y<128)s.pixels[y*128+x]=r.color;
   return game::DrawingStatus::drawn;
  };
  std::ifstream input(RA2_MIND_CONTROL_PIXELS_FIXTURE);std::string magic;int count;
  ASSERT_TRUE(bool(input>>magic>>count));ASSERT_EQ(magic,"MIND_CONTROL_PIXELS_V1");ASSERT_EQ(count,256);
  for(int row=0;row<count;++row){auto&c=DSurface::ViewBounds;unsigned color;int n;
   ASSERT_TRUE(bool(input>>s.from.X>>s.from.Y>>s.from.Z>>s.to.X>>s.to.Y>>s.to.Z>>c.X>>c.Y>>c.Width>>c.Height>>color>>s.ms>>n));
   TacticalClass::ViewBounds=c;s.color={byte(color),byte(color>>8),byte(color>>16)};
   SCOPED_TRACE(row);std::array<WORD,128*128> expected{};
   for(int i=0;i<n;++i){int index,value;ASSERT_TRUE(bool(input>>index>>value));ASSERT_GE(index,0);ASSERT_LT(index,128*128);expected[index]=WORD(value);}
   s.pixels.fill(0);
   game::ClockServices clock{&s,[](void*p)noexcept{return static_cast<State*>(p)->ms;}};
   struct Run{State&s;game::TypeDrawingContext&drawing;}run{s,drawing};
   ASSERT_TRUE(game::with_clock(clock,[](void*p){auto&r=*static_cast<Run*>(p);
    EXPECT_TRUE(game::drawing_completed(game::with_type_drawing(r.drawing,[](void*p){auto&s=*static_cast<State*>(p);s.actor.DrawMindControlLine(s.from,s.to,s.color);},&r.s)));
   },&run));
   EXPECT_EQ(s.pixels,expected);
  }
 },&view));
}

TEST(MindControlDrawing, OriginalNodeVisibilityAndEndpoints) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*p){auto&view=*static_cast<View*>(p);
  struct Probe:UnitClass{
   UnitTypeClass renderType;mutable std::vector<int> weapons;
   Probe():UnitClass(UnitTypeClass::Find("FV"),HouseClass::CurrentPlayer),renderType("LINK_FIXTURE"){}
   TechnoTypeClass* GetTechnoType()const override{return const_cast<UnitTypeClass*>(&renderType);}
   CoordStruct* GetFLH(CoordStruct*out,int weapon,CoordStruct base)const override{
    EXPECT_EQ(base,CoordStruct::Empty);weapons.push_back(weapon);*out={256-weapon*64,64+weapon*16,80-weapon*8};return out;
   }
  } owner,transport;
  std::array<std::unique_ptr<Probe>,6> units;
  CaptureManagerClass manager(&owner,6,false);
  for(int i=0;i<6;++i){units[i]=std::make_unique<Probe>();units[i]->Location={256+i*96,128+i*64,i*13-26};units[i]->renderType.LeptonMindControlOffset=20+i*17;
   auto*node=GameCreate<ControlNode>();node->Unit=units[i].get();node->OriginalOwner=HouseClass::CurrentPlayer;
   ::new(&node->LinkDrawTimer)CDTimerClass;ASSERT_TRUE(manager.ControlNodes.AddItem(node));
  }
  owner.Transporter=&transport;
  const auto oldBounds=DSurface::ViewBounds;const auto oldCamera=view.view->tactical.TacticalPos;const int oldFrame=Unsorted::CurrentFrame;
  const auto oldColor=owner.Owner->LaserColor;owner.Owner->LaserColor={249,179,101};
  auto restore=ra2::test::scope_exit([&]{manager.ControlNodes.Count=6;owner.Transporter=nullptr;owner.Owner->LaserColor=oldColor;
   DSurface::ViewBounds=oldBounds;view.view->tactical.TacticalPos=oldCamera;Unsorted::CurrentFrame=oldFrame;});
  DSurface::ViewBounds={0,0,128,128};view.view->tactical.TacticalPos={-10,-80};
  using Pixel=std::array<int,5>;
  struct Expected{CoordStruct from{},to{};ColorStruct color{};};
  struct State{CaptureManagerClass&manager;Probe&owner;std::vector<Pixel> pixels;std::vector<Expected> expected;game::TypeDrawingContext drawing;}s{manager,owner};
  s.drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(&s);s.drawing.backend_context=&s;
  s.drawing.backend.raster=[](void*p,const game::RasterDrawingRequest&r){static_cast<State*>(p)->pixels.push_back({r.position.X,r.position.Y,r.width,r.height,r.color});return game::DrawingStatus::drawn;};
  std::ifstream input(RA2_MIND_CONTROL_LINKS_FIXTURE);std::string magic;int count;
  ASSERT_TRUE(bool(input>>magic>>count));ASSERT_EQ(magic,"MIND_CONTROL_LINKS_V1");ASSERT_EQ(count,324);
  game::ClockServices clock{nullptr,[](void*)noexcept{return 128u;}};
  for(int row=0;row<count;++row){int nodes,selected,carSelected,mask,timers,elapsed,needed,calls;
   ASSERT_TRUE(bool(input>>nodes>>selected>>carSelected>>mask>>timers>>elapsed>>needed>>calls));SCOPED_TRACE(row);
   manager.ControlNodes.Count=nodes;owner.IsSelected=selected;transport.IsSelected=carSelected;Unsorted::CurrentFrame=100;
   for(int i=0;i<6;++i){units[i]->IsSelected=mask&(1<<i);manager.ControlNodes.Items[i]->LinkDrawTimer.Start((timers&(1<<i))?15:0);}
   Unsorted::CurrentFrame+=elapsed;s.expected.clear();std::vector<int> weapons;
   for(int i=0;i<calls;++i){Expected e;int weapon;unsigned color;
    ASSERT_TRUE(bool(input>>weapon>>e.from.X>>e.from.Y>>e.from.Z>>e.to.X>>e.to.Y>>e.to.Z>>color));
    e.color={byte(color),byte(color>>8),byte(color>>16)};s.expected.push_back(e);weapons.push_back(weapon);
   }
   EXPECT_EQ(manager.NeedsToDrawLinks(),bool(needed));s.pixels.clear();owner.weapons.clear();
   ASSERT_TRUE(game::with_clock(clock,[](void*p){auto&s=*static_cast<State*>(p);
    EXPECT_TRUE(game::drawing_completed(game::with_type_drawing(s.drawing,[](void*p){static_cast<State*>(p)->manager.DrawLinks();},&s)));
   },&s));
   EXPECT_EQ(owner.weapons,weapons);const auto actual=s.pixels;s.pixels.clear();
   ASSERT_TRUE(game::with_clock(clock,[](void*p){auto&s=*static_cast<State*>(p);
    EXPECT_TRUE(game::drawing_completed(game::with_type_drawing(s.drawing,[](void*p){auto&s=*static_cast<State*>(p);
     for(const auto&e:s.expected)s.owner.DrawMindControlLine(e.from,e.to,e.color);
    },&s)));
   },&s));
   EXPECT_EQ(actual,s.pixels);
  }
  for(auto&unit:units)unit->IsSelected=false;owner.IsSelected=transport.IsSelected=false;
 },&view));
}

TEST(MindControl, OriginalCaptureEligibilityAndQuota){
 struct QueryUnit final:UnitClass{
  using UnitClass::UnitClass;bool infantry=false;
  AbstractType WhatAmI() const override{return infantry?AbstractType::Infantry:AbstractType::Unit;}
 };
 UnitTypeClass type("CAPTURE_QUERY");QueryUnit owner(&type,nullptr),target(&type,nullptr);
 CaptureManagerClass manager(&owner,1,false);
 HouseTypeClass country("CAPTURE_COUNTRY");HouseClass house(&country);target.Owner=&house;
 const int savedFrame=Unsorted::CurrentFrame;Unsorted::CurrentFrame=100;
 auto cleanup=ra2::test::scope_exit([&]{target.Owner=nullptr;target.MindControlledBy=nullptr;target.MindControlledByHouse=nullptr;target.BunkerLinkedItem=nullptr;Unsorted::CurrentFrame=savedFrame;});
 std::ifstream input(RA2_CAPTURE_MANAGER_FIXTURE);std::string magic;int count=0;
 ASSERT_TRUE(bool(input>>magic>>count));ASSERT_EQ(magic,"CAPTURE_MANAGER_V1");
 for(int row=0;row<count;++row){
  int maximum,nodes,infinite,flags,capture,full;ASSERT_TRUE(bool(input>>maximum>>nodes>>infinite>>flags>>capture>>full));
  SCOPED_TRACE(row);manager.MaxControlNodes=maximum;manager.InfiniteMindControl=infinite;
  for(auto* node:manager.ControlNodes)GameDelete(node);manager.ControlNodes.Clear();
  for(int i=0;i<nodes;++i){auto* node=GameCreate<ControlNode>();node->Unit=nullptr;node->OriginalOwner=nullptr;ASSERT_TRUE(manager.ControlNodes.AddItem(node));}
  target.Owner=(flags&1)?nullptr:&house;type.ImmuneToPsionics=flags&2;
  target.BunkerLinkedItem=(flags&4)?&owner:nullptr;target.MindControlledBy=(flags&8)?&owner:nullptr;
  target.MindControlledByAUnit=flags&16;target.MindControlledByHouse=(flags&32)?&house:nullptr;
  target.IronCurtainTimer.Start((flags&64)?1:0);target.infantry=flags&1024;
  target.CurrentMission=(flags&128)?Mission::Selling:(flags&256)?Mission::Construction:Mission::Guard;
  EXPECT_EQ(manager.CanCapture((flags&512)?nullptr:&target),bool(capture));EXPECT_EQ(manager.CannotControlAnyMore(),bool(full));
 }
 EXPECT_EQ(count,144);
}

TEST(MindControl, ReplacementAndControlledVictimDeath){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 const int managers=CaptureManagerClass::Array.Count;
 {
  View view(data,true);view.load("ALL03UMD.MAP");
  ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
   InfantryClass* yuri=nullptr;std::vector<UnitClass*> tanks;
   for(auto* unit:InfantryClass::Array)if(!std::strcmp(unit->Type->ID,"YURI")){yuri=unit;break;}
   for(auto* unit:UnitClass::Array)if(unit->Owner==HouseClass::CurrentPlayer&&!std::strcmp(unit->Type->ID,"MTNK"))tanks.push_back(unit);
   ASSERT_NE(yuri,nullptr);ASSERT_GE(tanks.size(),2u);auto& manager=*yuri->CaptureManager;
   auto* first=tanks[0];auto* second=tanks[1];auto* house=first->Owner;
   ASSERT_TRUE(manager.CaptureUnit(first));EXPECT_EQ(first->MindControlledBy,yuri);
   EXPECT_TRUE(manager.CannotControlAnyMore());EXPECT_TRUE(manager.CanCapture(second));
   EXPECT_TRUE(manager.NeedsToDrawLinks());EXPECT_EQ(manager.GetOriginalOwner(first),house);
   ASSERT_TRUE(manager.CaptureUnit(second));EXPECT_EQ(manager.GetControlledCount(),1);
   EXPECT_EQ(first->Owner,house);EXPECT_EQ(first->MindControlledBy,nullptr);EXPECT_EQ(first->MindControlRingAnim,nullptr);
   EXPECT_EQ(second->MindControlledBy,yuri);EXPECT_FALSE(manager.CaptureUnit(second));
   bool applied=true;EXPECT_FALSE(manager.IsOverloading(&applied));EXPECT_TRUE(applied);
   const int delay=manager.OverloadDamageDelay;manager.HandleOverload();EXPECT_EQ(manager.OverloadDamageDelay,delay);
   int lethal=second->Health;
   EXPECT_EQ(second->ReceiveDamage(&lethal,0,WarheadTypeClass::Find("SA"),first,true,true,house),DamageState::NowDead);
   AbstractClass::RemoveAllInactive();EXPECT_EQ(manager.GetControlledCount(),0);EXPECT_FALSE(manager.IsControllingSomething());
   EXPECT_EQ(manager.GetOriginalOwner(second),nullptr);manager.FreeAll();
  },nullptr));
 }
 EXPECT_EQ(CaptureManagerClass::Array.Count,managers);
}

namespace {
void check_ordinary_retaliation(const char* map,bool vehicle){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 SCOPED_TRACE(::testing::Message()<<map<<" vehicle="<<vehicle);
 View view(data,true);view.load(map);ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapWorld* world;bool vehicle;bool firstMap;UnitClass* attacker=nullptr;FootClass* defender=nullptr;
  game::MapObjectId attackerId{},defenderId{};int attackerHealth=0,defenderHealth=0,lastShot=0;}s{view.view->world.get(),vehicle,!std::strcmp(map,"ALL01UMD.MAP")};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);InfantryClass* enemy=nullptr;
  for(auto* unit:InfantryClass::Array)if(!std::strcmp(unit->Type->ID,"INIT")&&unit->IsOnMap&&!unit->InLimbo&&!unit->Transporter
   &&!unit->Owner->IsAlliedWith(HouseClass::CurrentPlayer)&&(!s.firstMap||(unit->Location.X==33088&&unit->Location.Y==22720))){enemy=unit;break;}
  ASSERT_NE(enemy,nullptr);s.defender=enemy;
  for(auto* unit:UnitClass::Array)if(unit->Owner==HouseClass::CurrentPlayer&&!std::strcmp(unit->Type->ID,"MTNK")){
   if(!s.attacker)s.attacker=unit;
   else if(s.vehicle){s.defender=unit;ASSERT_TRUE(unit->SetOwningHouse(enemy->Owner,true));break;}
  }
  ASSERT_NE(s.attacker,nullptr);ASSERT_EQ(s.defender->WhatAmI(),s.vehicle?AbstractType::Unit:AbstractType::Infantry);
  ASSERT_EQ(s.defender->CaptureManager,nullptr);ASSERT_EQ(s.defender->Target,nullptr);
  ASSERT_GE(LogicClass::Instance.FindItemIndex(s.defender),0);
  bool placed=false;const auto center=s.defender->GetMapCoords();
  for(int radius=2;radius<=3&&!placed;++radius)for(int y=-radius;y<=radius&&!placed;++y)for(int x=-radius;x<=radius&&!placed;++x){
   if(std::abs(x)!=radius&&std::abs(y)!=radius)continue;
   auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(center.X+x),short(center.Y+y)});
   if(!cell||cell->SlopeIndex||cell->GetBuilding()||!MapClass::Instance.IsWithinUsableArea(cell->MapCoords,true)
    ||s.attacker->IsCellOccupied(cell,FacingType(-1),-1,nullptr,true)!=Move::OK)continue;
   s.attacker->SetLocation(cell->GetCoords());
   placed=s.attacker->IsCloseEnough(s.defender,0)&&s.defender->IsCloseEnough(s.attacker,0);
  }
  ASSERT_TRUE(placed);ASSERT_TRUE(s.attacker->Select());
  s.attackerId=game::object_id(*s.world,s.attacker);s.defenderId=game::object_id(*s.world,s.defender);
  s.attackerHealth=s.attacker->Health;s.defenderHealth=s.defender->Health;s.lastShot=s.defender->LastFireBulletFrame;
 },&s));
 ASSERT_NE(s.attacker,nullptr);ASSERT_NE(s.defender,nullptr);ASSERT_TRUE(click_world_object(*view.view,s.defender));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  bool assigned=false,admitted=false,fired=false,hurt=false;int hitFrame=-1,replyFrame=-1;
  for(int frame=0;frame<180&&!hurt;++frame){
   ++Unsorted::CurrentFrame;game::update_map_world(*s.world);
   ASSERT_EQ(game::resolve_map_object(*s.world,s.attackerId),s.attacker);
   ASSERT_EQ(game::resolve_map_object(*s.world,s.defenderId),s.defender);
   if(s.defender->Health<s.defenderHealth&&hitFrame<0)hitFrame=frame;
   assigned|=s.defender->Target==s.attacker&&s.defender->MissionIsOverriden();
   admitted|=LogicClass::Instance.FindItemIndex(s.defender)>=0;
   if(s.defender->LastFireBulletFrame!=s.lastShot){fired=true;if(replyFrame<0)replyFrame=frame;}
   hurt=s.attacker->Health<s.attackerHealth;
  }
  std::cout<<"ORDINARY_RETALIATION vehicle="<<s.vehicle<<" hit="<<hitFrame<<" reply="<<replyFrame
   <<" assigned="<<assigned<<" admitted="<<admitted<<" fired="<<fired<<" attacker_hp="<<s.attackerHealth<<"->"<<s.attacker->Health<<std::endl;
  EXPECT_GE(hitFrame,0);EXPECT_TRUE(assigned);EXPECT_TRUE(admitted);EXPECT_TRUE(fired);EXPECT_TRUE(hurt);
 },&s));
}
}

TEST(Retaliation, InitiateReturnsFireInAll01AndAll03){
 for(const char* map:{"ALL01UMD.MAP","ALL03UMD.MAP"})check_ordinary_retaliation(map,false);
}
TEST(Retaliation, GroundVehicleReturnsFire){check_ordinary_retaliation("ALL01UMD.MAP",true);}

TEST(InfantryCombat, YuriCloneTimedDeployment){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL03UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  InfantryClass* yuri=nullptr;
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"YURI")){yuri=actor;break;}
  ASSERT_NE(yuri,nullptr);ASSERT_FALSE(yuri->Owner->IsControlledByHuman());
  std::cout<<"YURI_DEPLOY delay="<<yuri->Type->UndeployDelay<<" sequence="<<int(yuri->SequenceAnim)<<" height="<<yuri->GetHeight()<<'\n';
  EXPECT_EQ(yuri->Type->UndeployDelay,150);
  for(auto* type:InfantryTypeClass::Array){
   if(!std::strcmp(type->ID,"YURIPR"))EXPECT_EQ(type->UndeployDelay,75);
   if(!std::strcmp(type->ID,"E1")||!std::strcmp(type->ID,"GGI"))EXPECT_EQ(type->UndeployDelay,-1);
  }
  yuri->SetTarget(nullptr);yuri->SetDestination(nullptr,true);yuri->ArchiveTarget=nullptr;
  yuri->ForceMission(Mission::Guard);yuri->CurrentMissionStartTime=Unsorted::CurrentFrame-10000;
  yuri->PlayAnim(Sequence::Ready,true);
  // An idle AI clone must not enter the persistent GI/Desolator deployment
  // branch merely because the automatic-deployment delay has elapsed.
  EXPECT_EQ(yuri->Guard_Deploy_AI(),-1);
  EXPECT_EQ(yuri->SequenceAnim,Sequence::Ready);
  EXPECT_EQ(yuri->GetHeight(),0);
  // Explicit deployment still performs the psychic-wave animation, then
  // returns to standing when the original mission scheduler wakes Guard.
  yuri->PlayAnim(Sequence::Ready,true);
  EXPECT_EQ(yuri->Mission_Unload(),150);
  EXPECT_EQ(yuri->SequenceAnim,Sequence::Deploy);
  yuri->Animation.Value=yuri->Type->Sequence->GetSequence(Sequence::Deploy).CountFrames;
  yuri->Doing_AI();ASSERT_EQ(yuri->SequenceAnim,Sequence::Deployed);
  EXPECT_EQ(yuri->Guard_Deploy_AI(),yuri->Type->Sequence->GetSequence(Sequence::Undeploy).CountFrames);
  ASSERT_EQ(yuri->SequenceAnim,Sequence::Undeploy);
  yuri->Animation.Value=yuri->Type->Sequence->GetSequence(Sequence::Undeploy).CountFrames;
  yuri->Doing_AI();EXPECT_EQ(yuri->SequenceAnim,Sequence::Ready);EXPECT_EQ(yuri->GetHeight(),0);
 },nullptr))<<game::map_view_error(*view.view);
}

TEST(MindControl, All03RangeBoundary){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL03UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  InfantryClass* yuri=nullptr;UnitClass* tank=nullptr;
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"YURI")){yuri=actor;break;}
  for(auto* actor:UnitClass::Array)if(actor->Owner==HouseClass::CurrentPlayer&&!std::strcmp(actor->Type->ID,"MTNK")){tank=actor;break;}
  ASSERT_NE(yuri,nullptr);ASSERT_NE(tank,nullptr);
  auto* weapon=yuri->GetWeapon(0)->WeaponType;ASSERT_NE(weapon,nullptr);
  ASSERT_STREQ(weapon->ID,"MindControl");EXPECT_EQ(weapon->Range,7*256);
  EXPECT_EQ(tank->GetWeapon(0)->WeaponType->Range,5*256);
  EXPECT_EQ(yuri->Type->Sight,12);EXPECT_FALSE(weapon->Projectile->SubjectToElevation);
  // Find an unobstructed, level strip in the real map so the one-lepton
  // boundary measures range, without a cliff/wall or a height difference.
  auto& map=MapClass::Instance;const auto anchor=yuri->GetMapCoords();
  CoordStruct origin{};bool found=false;
  for(int y=-25;y<=25&&!found;++y)for(int x=-25;x<=25&&!found;++x){
   auto* first=map.TryGetCellAt(CellStruct{short(anchor.X+x),short(anchor.Y+y)});if(!first)continue;
   bool clear=true;
   for(int i=0;i<=8;++i){auto* cell=map.TryGetCellAt(CellStruct{short(first->MapCoords.X+i),first->MapCoords.Y});
    if(!cell||!map.IsWithinUsableArea(cell,true)||cell->Level!=first->Level||cell->SlopeIndex||cell->OverlayTypeIndex!=-1||cell->FirstObject){clear=false;break;}}
   if(clear){origin=first->GetCoords();found=true;}
  }
  ASSERT_TRUE(found);
  const auto oldYuri=yuri->Location,oldTank=tank->Location;
  auto restore=ra2::test::scope_exit([&]{yuri->SetLocation(oldYuri);tank->SetLocation(oldTank);});
  yuri->SetLocation(origin);yuri->PlayAnim(Sequence::Ready,true);
  // Original sqrt table at 0x008650BC: distance 1793 becomes
  // 1792.9996337890625, then _ftol truncates it to 1792. Keep this tiny
  // original boundary tolerance; 1794 and eight cells must be rejected.
  struct Sample{int distance;bool allowed;};
  for(const auto [distance,allowed]:{Sample{1791,true},Sample{1792,true},Sample{1793,true},Sample{1794,false},Sample{1856,false},Sample{2048,false}}){
   tank->SetLocation(origin+CoordStruct{distance,0,0});
   const bool inRange=yuri->IsCloseEnough(tank,0);
   const auto error=yuri->GetFireError(tank,0,true);
   std::cout<<"YURI_RANGE leptons="<<distance<<" allowed="<<inRange<<" fire_error="<<int(error)<<'\n';
   EXPECT_EQ(inRange,allowed);
   EXPECT_EQ(error,allowed?FireError::OK:FireError::RANGE);
  }
 },nullptr))<<game::map_view_error(*view.view);
}

TEST(MindControl, MirageFirstHitKillsAll03Clone){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL03UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  UnitClass* tank=nullptr;InfantryClass* yuri=nullptr;
  for(auto* actor:UnitClass::Array)if(actor->Owner==HouseClass::CurrentPlayer&&!std::strcmp(actor->Type->ID,"MGTK")){tank=actor;break;}
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"YURI")){yuri=actor;break;}
  ASSERT_NE(tank,nullptr);ASSERT_NE(yuri,nullptr);
  const auto old=tank->Location;auto* originalOwner=tank->Owner;
  auto restore=ra2::test::scope_exit([&]{tank->SetLocation(old);});
  tank->SetLocation(yuri->Location+CoordStruct{3*256,0,0});
  auto* weapon=tank->GetWeapon(0)->WeaponType;ASSERT_STREQ(weapon->ID,"MirageGun");
  std::cout<<"MIRAGE_YURI hp="<<yuri->Health<<" damage="<<weapon->Damage<<" verse="<<weapon->Warhead->Verses[int(yuri->Type->Armor)]
   <<" owner_firepower="<<tank->Owner->FirepowerMultiplier<<" armor="<<yuri->ArmorMultiplier<<" house_armor="<<yuri->Owner->GetArmorMultiplier(yuri->Type)<<'\n';
  ASSERT_EQ(yuri->Health,100);
  auto* fatal=tank->Fire(yuri,0);ASSERT_NE(fatal,nullptr);
  // Also cover a control projectile already fired before the fatal impact.
  // Death invalidates its owner; it must not capture on behalf of a corpse.
  auto* control=yuri->Fire(tank,0);ASSERT_NE(control,nullptr);
  ASSERT_STREQ(control->WeaponType->ID,"MindControl");
  fatal->Update();
  std::cout<<"MIRAGE_YURI_AFTER hp="<<yuri->Health<<" alive="<<yuri->IsAlive<<" control_owner="<<control->Owner<<'\n';
  EXPECT_EQ(yuri->Health,0);EXPECT_FALSE(yuri->IsAlive);EXPECT_EQ(control->Owner,nullptr);
  ++Unsorted::CurrentFrame;control->Update();
  EXPECT_EQ(tank->Owner,originalOwner);EXPECT_EQ(tank->MindControlledBy,nullptr);
 },nullptr))<<game::map_view_error(*view.view);
}

TEST(MindControl, MirageClickKillsAll03Clone){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL03UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapWorld* world;UnitClass* tank=nullptr;InfantryClass* yuri=nullptr;game::MapObjectId target{};}s{view.view->world.get()};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* actor:UnitClass::Array)if(actor->Owner==HouseClass::CurrentPlayer&&!std::strcmp(actor->Type->ID,"MGTK")){s.tank=actor;break;}
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"YURI")){s.yuri=actor;break;}
  ASSERT_NE(s.tank,nullptr);ASSERT_NE(s.yuri,nullptr);s.target=game::object_id(*s.world,s.yuri);
  auto& map=MapClass::Instance;const auto anchor=s.yuri->GetMapCoords();bool placed=false;
  ASSERT_TRUE(s.tank->Limbo());
  for(int r=5;r>=3&&!placed;--r)for(int y=-r;y<=r&&!placed;++y)for(int x=-r;x<=r&&!placed;++x){
   if(std::abs(x)!=r&&std::abs(y)!=r)continue;
   auto* cell=map.TryGetCellAt(CellStruct{short(anchor.X+x),short(anchor.Y+y)});
   if(!cell||!map.IsWithinUsableArea(cell,true)||cell->SlopeIndex||cell->Level!=s.yuri->GetCell()->Level||cell->GetBuilding())continue;
   if(s.tank->IsCellOccupied(cell,FacingType(-1),-1,nullptr,true)!=Move::OK)continue;
   if(!s.tank->Unlimbo(cell->GetCoords(),DirType::North))continue;
   if(!s.tank->IsCloseEnough(s.yuri,0)){s.tank->Limbo();continue;}
   placed=true;
  }
  ASSERT_TRUE(placed);DirStruct direction;s.tank->GetDirectionTo(&direction,s.yuri);
  s.tank->PrimaryFacing.SetCurrent(direction);s.tank->SecondaryFacing.SetCurrent(direction);
  ASSERT_TRUE(s.tank->Select());
 },&s));
 ASSERT_TRUE(click_world_object(*view.view,s.yuri));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  const int oldShot=s.tank->LastFireBulletFrame;int frames=0;
  for(;frames<300;++frames){
   ++Unsorted::CurrentFrame;game::update_map_world(*s.world);
   auto* target=game::resolve_map_object(*s.world,s.target);
   if(!target||target->Health<=0||s.tank->MindControlledBy){
    std::cout<<"MIRAGE_CLICK frame="<<frames<<" hp="<<(target?target->Health:0)<<" first_shot="<<s.tank->LastFireBulletFrame
     <<" controller="<<s.tank->MindControlledBy<<" target="<<target<<'\n';
    EXPECT_TRUE(!target||target->Health==0);break;
   }
  }
  EXPECT_LT(frames,300);EXPECT_NE(s.tank->LastFireBulletFrame,oldShot);
 },&s))<<game::map_view_error(*view.view);
}

TEST(VehicleCombat, All03YuriCloneUnderFire){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL03UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapWorld* world;UnitClass* tank=nullptr;InfantryClass* target=nullptr;}s{view.view->world.get()};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* unit:UnitClass::Array)if(unit->Owner==HouseClass::CurrentPlayer&&!std::strcmp(unit->Type->ID,"MTNK")){s.tank=unit;break;}
  for(auto* unit:InfantryClass::Array)if(!std::strcmp(unit->Type->ID,"YURI")&&!unit->Owner->IsAlliedWith(HouseClass::CurrentPlayer)){s.target=unit;break;}
  ASSERT_NE(s.tank,nullptr);ASSERT_NE(s.target,nullptr);
  ASSERT_NE(s.target->CaptureManager,nullptr);
  std::cout<<"ALL03_YURI type="<<s.target->Type->ID<<" xyz="<<s.target->Location.X<<','<<s.target->Location.Y<<','<<s.target->Location.Z
   <<" capture="<<s.target->CaptureManager<<std::endl;
  bool placed=false;const auto anchor=s.target->GetMapCoords();
  for(int radius=3;radius<6&&!placed;++radius)for(int y=-radius;y<=radius&&!placed;++y)for(int x=-radius;x<=radius&&!placed;++x){
   if(std::abs(x)!=radius&&std::abs(y)!=radius)continue;
   auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(anchor.X+x),short(anchor.Y+y)});
   if(!cell||cell->SlopeIndex||cell->GetBuilding()||!MapClass::Instance.IsWithinUsableArea(cell->MapCoords,true)
    ||s.tank->IsCellOccupied(cell,FacingType(-1),-1,nullptr,true)!=Move::OK)continue;
   s.tank->SetLocation(cell->GetCoords());placed=true;
  }
  ASSERT_TRUE(placed);ASSERT_TRUE(s.tank->Select());
 },&s));
 ASSERT_NE(s.tank,nullptr);ASSERT_NE(s.target,nullptr);ASSERT_TRUE(click_world_object(*view.view,s.target));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  auto* original=s.tank->Owner;int frame=0;
  for(;frame<600&&s.tank->Owner==original;++frame){++Unsorted::CurrentFrame;game::update_map_world(*s.world);}
  ASSERT_LT(s.target->Health,s.target->Type->Strength);
  ASSERT_EQ(s.tank->Owner,s.target->Owner);ASSERT_EQ(s.tank->MindControlledBy,s.target);
  ASSERT_NE(s.tank->MindControlRingAnim,nullptr);ASSERT_EQ(s.tank->MindControlRingAnim->OwnerObject,s.tank);
  ASSERT_EQ(s.target->CaptureManager->GetOriginalOwner(s.tank),original);
  std::cout<<"ALL03_YURI_CAPTURE frame="<<frame<<" yuri_hp="<<s.target->Health<<std::endl;
  int lethal=s.target->Health;
  ASSERT_EQ(s.target->ReceiveDamage(&lethal,0,WarheadTypeClass::Find("SA"),s.tank,true,true,original),DamageState::NowDead);
  EXPECT_EQ(s.tank->Owner,original);EXPECT_EQ(s.tank->MindControlledBy,nullptr);EXPECT_EQ(s.tank->MindControlRingAnim,nullptr);
  EXPECT_EQ(s.target->CaptureManager->GetControlledCount(),0);
  for(int i=0;i<30;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.world);}
 },&s));
}

TEST(VehicleFiring, MirageOriginalBlinkAndDrawCorpus){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& world=*static_cast<game::MapWorld*>(p);
  auto* type=UnitTypeClass::Find("MGTK");ASSERT_NE(type,nullptr);
  EXPECT_TRUE(type->CanDisguise);EXPECT_TRUE(type->DisguiseWhenStill);EXPECT_FALSE(type->PermaDisguise);
  HouseClass* enemy=nullptr;for(auto* h:HouseClass::Array)if(!h->IsHumanPlayer&&!h->IsAlliedWith(HouseClass::CurrentPlayer)){enemy=h;break;}
  ASSERT_NE(enemy,nullptr);auto* player=HouseClass::CurrentPlayer;
  auto* tank=new UnitClass(type,enemy);ASSERT_TRUE(tank->InitializeLocomotor());
  BuildingClass* anchor=nullptr;for(auto* b:BuildingClass::Array)if(!std::strcmp(b->Type->ID,"GAWEAP")){anchor=b;break;}ASSERT_NE(anchor,nullptr);
  bool placed=false;const auto center=anchor->GetMapCoords();
  for(int radius=3;radius<8&&!placed;++radius)for(int y=-radius;y<=radius&&!placed;++y)for(int x=-radius;x<=radius&&!placed;++x){
   auto* c=MapClass::Instance.TryGetCellAt(CellStruct{short(center.X+x),short(center.Y+y)});
   if(!c||c->SlopeIndex||c->GetBuilding()||!MapClass::Instance.IsWithinUsableArea(c->MapCoords,true)||tank->IsCellOccupied(c,FacingType(-1),-1,nullptr,true)!=Move::OK)continue;
   placed=tank->Unlimbo(c->GetCoords(),DirType::North);
  }ASSERT_TRUE(placed);
  Unsorted::CurrentFrame=8;tank->Update();EXPECT_TRUE(tank->IsDisguised());
  auto* tree=RulesClass::Instance->DefaultMirageDisguises[0];ASSERT_NE(tree,nullptr);ASSERT_NE(tree->GetImage(),nullptr);
  auto* weapon=tank->GetWeapon(0)->WeaponType;const bool originalWhenStill=type->DisguiseWhenStill;
  const int oldBlink=weapon->DisguiseFakeBlinkTime;weapon->DisguiseFakeBlinkTime=15;
  const auto restore=ra2::test::scope_exit([&]{type->DisguiseWhenStill=originalWhenStill;weapon->DisguiseFakeBlinkTime=oldBlink;tank->Owner=enemy;});
  std::ifstream input(RA2_MIRAGE_DISGUISE_FIXTURE);ASSERT_TRUE(input.good());std::string line;int fires=0,draws=0,voxelFlags=0;
  while(std::getline(input,line)){
   std::istringstream row(line);char kind;row>>kind;if(kind!='F'&&kind!='D'&&kind!='V')continue;SCOPED_TRACE(line);
   Unsorted::CurrentFrame=1000;tank->Owner=enemy;tank->Disguise=tree;tank->DisguisedAsHouse=nullptr;
   tank->DisguiseBlinkTimer.StartTime=990;tank->DisguiseBlinkTimer.TimeLeft=0;
   if(kind=='F'){
    int disguised,whenstill,success;row>>disguised>>whenstill>>success;std::array<int,4> expected;for(auto& v:expected)ASSERT_TRUE(bool(row>>v));
    tank->Disguised=disguised;type->DisguiseWhenStill=whenstill;
    // Original fixture controls only base-Fire success. Here an ordinary
    // projectile exercises success; a null target exercises base-Fire failure.
    auto* bullet=tank->Fire(success?static_cast<AbstractClass*>(anchor):nullptr,0);
    const std::array<int,4> actual{bullet!=nullptr,tank->Disguised,tank->DisguiseBlinkTimer.StartTime,tank->DisguiseBlinkTimer.TimeLeft};
    EXPECT_EQ(actual,expected);if(bullet)bullet->Release();++fires;
   }else if(kind=='V'){
    int flags,age,expected;row>>flags>>age>>expected;
    tank->Disguised=flags&1;tank->Owner=flags&2?player:enemy;tank->DisguiseCreationFrame=1000-age;
    tank->DisguiseBlinkTimer.TimeLeft=flags&4?12:0;
    // Only rows dispatching to VXL are observable in the real Mirage draw.
    // Other rows are compared directly to the production flag method.
    const int computed=flags&1&&flags&2?tank->GetDisguiseFlags(0x2800):0x2800;
    EXPECT_EQ(computed,expected);
    tank->NeedsRedraw=true;game::map_object_changed();game::rebuild_world_sprites(world);
    for(const auto& sprite:world.impl->sprites)if(sprite.owner==tank&&sprite.voxel&&!sprite.shadow)EXPECT_EQ(int(sprite.flags),expected);
    ++voxelFlags;
   }else{
    int flags,age,expected;row>>flags>>age>>expected;
    tank->Disguised=flags&1;tank->Owner=flags&2?player:enemy;tank->DisguiseCreationFrame=1000-age;
    auto* cell=tank->GetCell();const unsigned index=player->ArrayIndex;
    const bool sensor=cell->DisguiseSensors_InclHouse(index);if(flags&4&&!sensor)cell->DisguiseSensors_AddOfHouse(index);
    tank->DisguiseBlinkTimer.TimeLeft=flags&8?12:0;
    tank->NeedsRedraw=true;game::map_object_changed();game::rebuild_world_sprites(world);
    int actual=0;for(const auto& sprite:world.impl->sprites)if(sprite.owner==tank&&!sprite.shadow){
     actual=sprite.voxel?1:2;
     if(actual==2){EXPECT_EQ(sprite.image,tree->GetImage());EXPECT_EQ(sprite.frame,0);EXPECT_EQ(sprite.palette,&FileSystem::ISOx_PAL);}
    }
    EXPECT_EQ(actual,expected);if(flags&4&&!sensor)cell->DisguiseSensors_RemOfHouse(index);++draws;
   }
  }
  EXPECT_EQ(fires,8);EXPECT_EQ(draws,80);EXPECT_EQ(voxelFlags,168);
 },view.view->world.get()));
}

TEST(VehicleFiring, MirageAutomaticallyContinuesThroughFourTargets){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& world=*static_cast<game::MapWorld*>(p);
  auto* type=UnitTypeClass::Find("MGTK");auto* targetType=UnitTypeClass::Find("MTNK");ASSERT_NE(type,nullptr);ASSERT_NE(targetType,nullptr);
  auto* player=HouseClass::CurrentPlayer;HouseClass* enemy=nullptr;
  for(auto* h:HouseClass::Array)if(!player->IsAlliedWith(h)&&!h->Type->MultiplayPassive){enemy=h;break;}ASSERT_NE(enemy,nullptr);
  auto* tank=new UnitClass(type,player);ASSERT_TRUE(tank->InitializeLocomotor());
  auto& map=MapClass::Instance;CellStruct center{};bool found=false;
  for(int y=map.MapCoordBounds.Top;y<map.MapCoordBounds.Bottom&&!found;++y)for(int x=map.MapCoordBounds.Left;x<map.MapCoordBounds.Right&&!found;++x){
   CellStruct at{short(x),short(y)};auto* cell=map.TryGetCellAt(at);
   if(!cell||!map.IsWithinUsableArea(at,true)||cell->SlopeIndex||cell->GetBuilding()||tank->IsCellOccupied(cell,FacingType(-1),-1,nullptr,true)!=Move::OK)continue;
   bool clear=true;
   for(auto* existing:TechnoClass::Array)if(existing!=tank&&!existing->InLimbo&&existing->IsAlive){auto c=existing->GetMapCoords();if(std::abs(c.X-x)<=4&&std::abs(c.Y-y)<=4){clear=false;break;}}
   for(const auto offset:{CellStruct{-2,-2},CellStruct{2,-2},CellStruct{-2,2},CellStruct{2,2}}){
    auto* c=map.TryGetCellAt(CellStruct{short(x+offset.X),short(y+offset.Y)});
    if(!c||!map.IsWithinUsableArea(c->MapCoords,true)||c->Level!=cell->Level||c->SlopeIndex||c->GetBuilding()||tank->IsCellOccupied(c,FacingType(-1),-1,nullptr,true)!=Move::OK)clear=false;
   }
   if(clear){center=at;found=true;}
  }ASSERT_TRUE(found);ASSERT_TRUE(tank->Unlimbo(map.GetCellAt(center)->GetCoords(),DirType::North));
  std::array<UnitClass*,4> victims{};std::array<game::MapObjectId,4> ids{};std::array<bool,4> targeted{};int index=0;
  for(const auto offset:{CellStruct{-2,-2},CellStruct{2,-2},CellStruct{-2,2},CellStruct{2,2}}){
   auto* victim=new UnitClass(targetType,enemy);ASSERT_TRUE(victim->InitializeLocomotor());
   ASSERT_TRUE(victim->Unlimbo(map.GetCellAt(CellStruct{short(center.X+offset.X),short(center.Y+offset.Y)})->GetCoords(),DirType::North));
   victim->Health=victim->EstimatedHealth=40;victims[index]=victim;ids[index]=game::object_id(world,victim);++index;
  }
  auto origin=tank->Location;
  EXPECT_EQ(tank->FootClass::GreatestThreat(ThreatType(1),&origin,false),nullptr)<<"the previously inherited entry supplies no target categories";
  ASSERT_NE(tank->GreatestThreat(ThreatType(1),&origin,false),nullptr);
  // One initial player attack order; all subsequent targets must come from
  // normal mission/expiration/scan updates, with no further test orders.
  tank->SetTarget(victims[0]);tank->QueueMission(Mission::Attack,true);
  if(LogicClass::Instance.FindItemIndex(tank)<0)ASSERT_TRUE(LogicClass::Instance.AddObject(tank,false));
  int alive=4,ticks=0;
  for(;ticks<1800&&alive;++ticks){
   for(int i=0;i<4;++i)if(auto* victim=game::resolve_map_object(world,ids[i]);victim&&tank->Target==victim)targeted[i]=true;
   ++Unsorted::CurrentFrame;game::update_map_world(world);
   alive=0;for(const auto id:ids)if(auto* victim=game::resolve_map_object(world,id);victim&&victim->IsAlive&&victim->Health>0)++alive;
  }
  EXPECT_EQ(alive,0)<<"mission="<<int(tank->CurrentMission)<<" queued="<<int(tank->QueuedMission)<<" target="<<tank->Target;
  EXPECT_TRUE(std::all_of(targeted.begin(),targeted.end(),[](bool v){return v;}));
  EXPECT_TRUE(tank->IsAlive);std::cout<<"MIRAGE_CHAIN ticks="<<ticks<<" remaining="<<alive<<'\n';
 },view.view->world.get()));
}

TEST(VehicleFiring, FirstMapMirageTurnsHullBeforeFiring){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapWorld* world;UnitClass* tank=nullptr;BuildingClass* target=nullptr;CoordStruct origin{};unsigned initial=0;}s{view.view->world.get()};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* b:BuildingClass::Array)if(!std::strcmp(b->Type->ID,"GAWEAP")){s.target=b;break;}
  ASSERT_NE(s.target,nullptr);auto* type=UnitTypeClass::Find("MGTK");ASSERT_NE(type,nullptr);
  ASSERT_TRUE(type->Voxel);ASSERT_FALSE(type->Turret);ASSERT_FALSE(type->HasTurret);
  s.tank=new UnitClass(type,HouseClass::CurrentPlayer);ASSERT_TRUE(s.tank->InitializeLocomotor());
  bool placed=false;const auto anchor=s.target->GetMapCoords();
  for(int radius=3;radius<6&&!placed;++radius)for(int y=-radius;y<=radius&&!placed;++y)for(int x=-radius;x<=radius&&!placed;++x){
   if(std::abs(x)!=radius&&std::abs(y)!=radius)continue;
   auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(anchor.X+x),short(anchor.Y+y)});
   if(!cell||cell->SlopeIndex||cell->GetBuilding()||!MapClass::Instance.IsWithinUsableArea(cell->MapCoords,true)
    ||s.tank->IsCellOccupied(cell,FacingType(-1),-1,nullptr,true)!=Move::OK)continue;
   placed=s.tank->Unlimbo(cell->GetCoords(),DirType::North);
  }
  ASSERT_TRUE(placed);s.origin=s.tank->Location;DirStruct direction;s.tank->GetDirectionTo(&direction,s.target);
  s.initial=static_cast<unsigned short>(direction.Raw+0x4000);
  s.tank->PrimaryFacing.SetCurrent(DirStruct(int(s.initial)));s.tank->SecondaryFacing.SetCurrent(DirStruct(int(s.initial)));
  ASSERT_TRUE(s.tank->Select());
 },&s));
 ASSERT_NE(s.tank,nullptr);ASSERT_NE(s.target,nullptr);
 ASSERT_TRUE(click_world_object(*view.view,s.target,2));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);const int lastShot=s.tank->LastFireBulletFrame;
  bool turned=false,fired=false;
  for(int frame=0;frame<180&&!fired;++frame){
   ++Unsorted::CurrentFrame;game::update_map_world(*s.world);
   const auto hull=s.tank->PrimaryFacing.Current();turned|=hull.Raw!=s.initial;
   EXPECT_EQ(s.tank->Location,s.origin);
   fired=s.tank->LastFireBulletFrame!=lastShot;
   if(fired){DirStruct direction;s.tank->GetDirectionTo(&direction,s.target);
    EXPECT_TRUE(turned);EXPECT_LE(std::abs(int(static_cast<short>(hull.Raw-direction.Raw))),0x800);
    std::cout<<"MIRAGE_FIRE frame="<<frame<<" hull="<<hull.Raw<<" initial="<<s.initial<<" target="<<direction.Raw<<'\n';
   }
  }
  EXPECT_TRUE(turned);EXPECT_TRUE(fired);
 },&s));
}

TEST(VehicleFiring, MirageKilledInitiateRetaliationStillDetonates) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& world=*static_cast<game::MapWorld*>(p);
  UnitClass* tank=nullptr;InfantryClass* victim=nullptr;
  for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"MGTK")&&unit->Owner==HouseClass::CurrentPlayer){tank=unit;break;}
  for(auto* unit:InfantryClass::Array)if(!std::strcmp(unit->Type->ID,"INIT")&&unit->Location.X==33344&&unit->Location.Y==22720){victim=unit;break;}
  ASSERT_NE(tank,nullptr);ASSERT_NE(victim,nullptr);
  // Reconstruct the recorded simultaneous shot order with actual map objects,
  // weapons, Fire, damage/death notifications and the production world loop.
  tank->SetLocation({32384,22656,832});ASSERT_EQ(tank->GetCell()->SlopeIndex,0);
  tank->RearmTimer.Start(0);victim->RearmTimer.Start(0);
  tank->SetTarget(victim);victim->SetTarget(tank);
  const int health=tank->Health;const auto victimId=game::object_id(world,victim);
  auto* fatal=tank->Fire(victim,0);ASSERT_NE(fatal,nullptr);
  auto* retaliation=victim->Fire(tank,0);ASSERT_NE(retaliation,nullptr);
  ASSERT_STREQ(retaliation->WeaponType->ID,"PsychicJab");
  ASSERT_STREQ(retaliation->Type->ID,"InvisibleLow");
  fatal->Update();ASSERT_EQ(retaliation->Owner,nullptr);
  ++Unsorted::CurrentFrame;retaliation->Update();
  EXPECT_FALSE(retaliation->IsAlive);EXPECT_LT(tank->Health,health);
  for(int frame=0;frame<60;++frame){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_EQ(game::resolve_map_object(world,victimId),nullptr);
 },view.view->world.get()));
}

TEST(VehicleFiring, FirstMapGrizzlyLaunchUsesOriginalTurretMuzzle){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapViewHandle* view;UnitClass* tank=nullptr;BuildingClass* target=nullptr;
  std::array<std::array<CoordStruct,32>,32> original{};}s{view.view};
 std::ifstream input(RA2_UNIT_FIRE_COORDINATES_FIXTURE);std::string format;int count=0;
 ASSERT_TRUE(bool(input>>format>>count));
 for(int i=0;i<count;++i){int turreted,hull,turret,offset,burst,facing;CoordStruct flh,base,at;
  ASSERT_TRUE(bool(input>>turreted>>hull>>turret>>offset>>burst>>flh.X>>flh.Y>>flh.Z>>base.X>>base.Y>>base.Z>>facing>>at.X>>at.Y>>at.Z));
  if(turreted&&offset==0)s.original[hull/2048][turret/2048]=at;
 }
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* b:BuildingClass::Array)if(!std::strcmp(b->Type->ID,"GAWEAP")){s.target=b;break;}ASSERT_NE(s.target,nullptr);
  for(auto* tank:UnitClass::Array)if(!std::strcmp(tank->Type->ID,"MTNK")&&tank->Owner==HouseClass::CurrentPlayer){
   s.tank=tank;break;
  }
  ASSERT_NE(s.tank,nullptr);ASSERT_TRUE(s.tank->Select());ASSERT_TRUE(s.tank->Type->Turret);
  ASSERT_EQ(s.tank->GetWeapon(0)->FLH,(CoordStruct{150,0,100}));ASSERT_EQ(s.tank->Type->TurretOffset,0);
 },&s));
 ASSERT_TRUE(click_world_object(*view.view,s.target,2));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);bool fired=false;
  for(int i=0;i<300&&!fired;++i){
   ++Unsorted::CurrentFrame;
   const auto hull=s.tank->PrimaryFacing.Current().GetValue<5>(),turret=s.tank->SecondaryFacing.Current().GetValue<5>();
   const auto origin=s.tank->GetRenderCoords();const auto reference=s.original[hull][turret];
   const CoordStruct expected{origin.X+reference.X-25216,origin.Y+reference.Y-28800,origin.Z+reference.Z-416};
   game::update_map_world(*s.view->world);
   for(auto* bullet:BulletClass::Array)if(bullet->Owner==s.tank){
    fired=true;EXPECT_NE(hull,turret);EXPECT_EQ(s.tank->GetRenderCoords(),origin);
    EXPECT_EQ(bullet->SourceCoords,expected);
    std::cout<<"GRIZZLY_LAUNCH hull="<<hull<<" turret="<<turret<<" origin="<<origin.X<<','<<origin.Y<<','<<origin.Z
     <<" source="<<bullet->SourceCoords.X<<','<<bullet->SourceCoords.Y<<','<<bullet->SourceCoords.Z
     <<" original="<<expected.X<<','<<expected.Y<<','<<expected.Z<<'\n';break;
   }
  }
  EXPECT_TRUE(fired);
 },&s));
}

TEST(VehicleFiring, OriginalBallisticPitchAcrossTargetHeights){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  UnitClass* tank=nullptr;InfantryClass* target=nullptr;
  for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"MTNK")){tank=unit;break;}
  for(auto* unit:InfantryClass::Array)if(!std::strcmp(unit->Type->ID,"INIT")){target=unit;break;}
  ASSERT_NE(tank,nullptr);ASSERT_NE(target,nullptr);
  auto* weapon=tank->GetWeapon(0)->WeaponType;auto* projectile=weapon->Projectile;
  ASSERT_TRUE(projectile->Arcing);ASSERT_FALSE(projectile->Inaccurate);ASSERT_EQ(projectile->ROT,0);
  const bool lobber=weapon->Lobber,floater=projectile->Floater;
  auto restore=ra2::test::scope_exit([&]{weapon->Lobber=lobber;projectile->Floater=floater;});
  ASSERT_EQ(RulesClass::Instance->Gravity,6);
  std::ifstream input(RA2_PROJECTILE_PITCH_FIXTURE);std::string magic;int count=0,differences=0;
  ASSERT_TRUE(bool(input>>magic>>count));ASSERT_EQ(magic,"PROJECTILE_PITCH_V2");
  for(int i=0;i<count;++i){
   int floating,high,distance,height,speed,valid,raw;BulletVelocity expected;
   ASSERT_TRUE(bool(input>>floating>>high>>distance>>height>>speed>>valid>>raw>>expected.X>>expected.Y>>expected.Z));
   weapon->Lobber=high;projectile->Floater=floating;
   CoordStruct muzzle;tank->GetFLH(&muzzle,0,CoordStruct::Empty);
   target->SetLocation({muzzle.X+distance,muzzle.Y,muzzle.Z+height});tank->SetTarget(target);
   EXPECT_EQ(weapon->GetSpeed(distance),speed);
   auto* bullet=tank->Fire(target,0);ASSERT_EQ(bool(bullet),bool(valid));
   if(!bullet)continue;
   auto release=ra2::test::scope_exit([&]{bullet->Release();});
   // The expected velocity comes from original x86 instructions. Compare
   // the emitted velocity through the shared native Fire entry, not a helper.
   const bool mismatch=std::abs(bullet->Velocity.X-expected.X)>1e-10||std::abs(bullet->Velocity.Y-expected.Y)>1e-10||std::abs(bullet->Velocity.Z-expected.Z)>1e-10;
   if(mismatch&&differences++<5)std::cout<<"BALLISTIC_DIFF row="<<i<<" height="<<height<<" pitch="<<raw<<" vx="<<bullet->Velocity.X<<"/"<<expected.X<<" vz="<<bullet->Velocity.Z<<"/"<<expected.Z<<'\n';
  }
  EXPECT_EQ(count,84);EXPECT_EQ(differences,0);
 },nullptr));
}

TEST(VehicleFiring, FirstMapGrizzlyHitsRaisedInitiateGroup){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapWorld* world;UnitClass* tank=nullptr;InfantryClass* target=nullptr;int health=0;}s{view.view->world.get()};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"MTNK")&&unit->Owner==HouseClass::CurrentPlayer){s.tank=unit;break;}
  for(auto* unit:InfantryClass::Array)if(!std::strcmp(unit->Type->ID,"INIT")&&unit->Location.X==33088&&unit->Location.Y==22720){s.target=unit;break;}
  ASSERT_NE(s.tank,nullptr);ASSERT_NE(s.target,nullptr);s.health=s.target->Health;
  // Place the existing tank on a real flat low cell beside this group so
  // this exercises uphill firing rather than the route from its map spawn.
  bool placed=false;const auto center=s.target->GetMapCoords();
  for(int radius=3;radius<=5&&!placed;++radius)for(int y=-radius;y<=radius&&!placed;++y)for(int x=-radius;x<=radius&&!placed;++x){
   if(std::abs(x)!=radius&&std::abs(y)!=radius)continue;
   auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(center.X+x),short(center.Y+y)});
   if(!cell||cell->SlopeIndex||cell->GetBuilding()||s.tank->IsCellOccupied(cell,FacingType(-1),-1,nullptr,true)!=Move::OK)continue;
   const auto at=cell->GetCoords();
   if(at.Z+100>=s.target->Location.Z||s.target->Location.Z-at.Z>416)continue;
   s.tank->SetLocation(at);placed=true;
  }
  ASSERT_TRUE(placed);
  ASSERT_LT(s.tank->Location.Z,s.target->Location.Z);ASSERT_TRUE(s.tank->Select());
 },&s));
 ASSERT_TRUE(click_world_object(*view.view,s.target));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);bool uphill=false,hit=false;
  for(int frame=0;frame<1800&&!hit;++frame){
   ++Unsorted::CurrentFrame;game::update_map_world(*s.world);
   for(auto* bullet:BulletClass::Array)if(bullet->Owner==s.tank&&bullet->SourceCoords.Z<s.target->Location.Z){
    if(!uphill)std::cout<<"GRIZZLY_UPHILL frame="<<frame<<" source_z="<<bullet->SourceCoords.Z<<" target_z="<<s.target->Location.Z
     <<" velocity="<<bullet->Velocity.X<<','<<bullet->Velocity.Y<<','<<bullet->Velocity.Z<<'\n';
    uphill=true;EXPECT_GT(std::hypot(bullet->Velocity.X,bullet->Velocity.Y),std::abs(bullet->Velocity.Z)*0.5);
   }
   hit=s.target->Health<s.health;
   if(hit)std::cout<<"GRIZZLY_UPHILL_HIT frame="<<frame<<" hp="<<s.health<<"->"<<s.target->Health<<'\n';
  }
  EXPECT_TRUE(uphill);EXPECT_TRUE(hit);
 },&s));
}

TEST(VehicleFiring, RaisedGroupRetainsExplicitTargetWhenFrontUnitIntercepts){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 for(const auto aim:{CoordStruct{33088,22720,832},CoordStruct{33344,22720,832},CoordStruct{33088,22976,832},CoordStruct{33344,22976,832}}){
  View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
  struct State{game::MapWorld* world;CoordStruct aim;UnitClass* tank=nullptr;InfantryClass* target=nullptr;std::vector<InfantryClass*> group;}s{view.view->world.get(),aim};
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"MTNK")&&unit->Owner==HouseClass::CurrentPlayer){s.tank=unit;break;}
   for(auto* unit:InfantryClass::Array)if(!std::strcmp(unit->Type->ID,"INIT")&&unit->Location.Z==832
    &&unit->Location.X>=33088&&unit->Location.X<=33344&&unit->Location.Y>=22720&&unit->Location.Y<=22976){
     s.group.push_back(unit);if(unit->Location==s.aim)s.target=unit;
   }
   ASSERT_NE(s.tank,nullptr);ASSERT_NE(s.target,nullptr);ASSERT_EQ(s.group.size(),4u);
   s.tank->SetLocation({31872,22656,728});ASSERT_TRUE(s.tank->Select());
  },&s));
  ASSERT_TRUE(click_world_object(*view.view,s.target));
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);bool hit=false,intercepted=false;int lastShot=-1;
   std::cout<<"FAR_AIM xyz="<<s.aim.X<<','<<s.aim.Y<<','<<s.aim.Z<<'\n';
   for(int frame=0;frame<500&&!hit;++frame){
    for(auto* bullet:BulletClass::Array)if(bullet->Owner==s.tank){
     const auto v=bullet->Velocity;const double gravity=RulesClass::Instance->Gravity*(bullet->Type->Floater?0.5:1.0);
     const CoordStruct next{int(bullet->Location.X+v.X),int(bullet->Location.Y+v.Y),int(bullet->Location.Z+v.Z-gravity)};
     auto* cell=MapClass::Instance.GetCellAt(next);auto* candidate=cell->FindTechnoNearestTo({0,0},false,nullptr);
     if(candidate&&candidate!=s.target&&candidate!=s.tank&&!s.tank->Owner->IsAlliedWith(candidate)){
      const auto delta=next-candidate->Location;
      if(int(Math::sqrt(double(delta.X)*delta.X+double(delta.Y)*delta.Y+double(delta.Z)*delta.Z))<128){
       intercepted=true;
       std::cout<<"INTERCEPT_INPUT "<<next.X<<' '<<next.Y<<' '<<next.Z<<' '<<candidate->Location.X<<' '<<candidate->Location.Y<<' '<<candidate->Location.Z<<'\n';
      }
     }
    }
    ++Unsorted::CurrentFrame;game::update_map_world(*s.world);
    ASSERT_EQ(s.tank->Target,s.target);
    for(auto* bullet:BulletClass::Array)if(bullet->Owner==s.tank&&bullet->Fetch_ID()!=lastShot){lastShot=bullet->Fetch_ID();auto at=bullet->Target->GetCoords();
     ASSERT_EQ(bullet->Target,s.target);
     std::cout<<"FAR_SHOT frame="<<frame<<" source="<<bullet->SourceCoords.X<<','<<bullet->SourceCoords.Y<<','<<bullet->SourceCoords.Z<<" target="<<at.X<<','<<at.Y<<','<<at.Z<<" requested="<<(bullet->Target==s.target)<<'\n';}
    for(auto* enemy:s.group)if(enemy->Health<enemy->Type->Strength){
     hit=true;EXPECT_EQ(enemy->Location,(CoordStruct{33088,s.aim.Y,832}));
     std::cout<<"FAR_HIT frame="<<frame<<" unit="<<enemy->Location.X<<','<<enemy->Location.Y<<" hp="<<enemy->Health<<" requested="<<(enemy==s.target)<<'\n';
    }
   }
   EXPECT_TRUE(hit);EXPECT_EQ(intercepted,s.aim.X==33344);
  },&s));
 }
}

TEST(InfantryTransport, EngineerClickChangesIFVWeaponAndTurret){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1024,768));
 struct State{game::MapViewHandle* view;UnitClass* carrier=nullptr;InfantryClass* engineer=nullptr;UnitClass* damaged=nullptr;int health=0;}s{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"FV")&&unit->Owner==HouseClass::CurrentPlayer){s.carrier=unit;break;}
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"ENGINEER")&&actor->Owner==HouseClass::CurrentPlayer){s.engineer=actor;break;}
  ASSERT_NE(s.carrier,nullptr);ASSERT_NE(s.engineer,nullptr);ASSERT_TRUE(s.engineer->Select());
 },&s));
 ASSERT_TRUE(click_world_object(*view.view,s.carrier));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& world=*s.view->world;
  ASSERT_EQ(EventClass::OutList.Count,1);EXPECT_EQ(EventClass::OutList.First().MegaMission.Mission,unsigned(Mission::Enter));
  int ticks=0;for(;ticks<1200&&!s.carrier->Passengers.NumPassengers;++ticks){++Unsorted::CurrentFrame;game::update_map_world(world);game::rebuild_world_sprites(world);}
  ASSERT_EQ(s.carrier->Passengers.NumPassengers,1);EXPECT_EQ(s.carrier->Passengers.FirstPassenger,s.engineer);EXPECT_TRUE(s.engineer->InLimbo);
  // Placement registers the carrier with Logic before it receives passengers.
  // Its flash must expire through the real scheduler.
  EXPECT_GE(LogicClass::Instance.FindItemIndex(s.carrier),0);
  EXPECT_EQ(s.carrier->Flashing.DurationRemaining,0);
  for(const auto& sprite:world.impl->sprites)if(sprite.owner==s.carrier&&!sprite.shadow)
   EXPECT_EQ(sprite.intensity,s.carrier->GetCell()->Intensity_Normal+RulesClass::Instance->ExtraUnitLight);
  EXPECT_EQ(s.engineer->Type->IFVMode,1);EXPECT_EQ(s.carrier->Type->TurretWeapon[1],2);
  EXPECT_EQ(s.carrier->CurrentWeaponNumber,1);EXPECT_EQ(s.carrier->CurrentTurretNumber,2);
  EXPECT_EQ(s.carrier->SelectWeapon(nullptr),1);ASSERT_NE(s.carrier->GetTurretWeapon()->WeaponType,nullptr);
  EXPECT_STREQ(s.carrier->GetTurretWeapon()->WeaponType->ID,"RepairBullet");EXPECT_LT(s.carrier->CombatDamage(-1),0);
  auto* effect=s.carrier->GetTurretWeapon()->WeaponType->AttachedParticleSystem;ASSERT_NE(effect,nullptr);
  EXPECT_STREQ(effect->ID,"WeldingSys");EXPECT_EQ(effect->BehavesLike,BehavesLike::Spark);EXPECT_EQ(effect->SparkSpawnFrames,20);
  auto* held=ParticleTypeClass::Array.GetItemOrDefault(effect->HoldsWhat);ASSERT_NE(held,nullptr);EXPECT_STREQ(held->ID,"WeldingSpark");
  ASSERT_EQ(held->ColorList.Count,5);EXPECT_EQ(held->MaxEC,500);
  EXPECT_EQ(held->ColorList[0],RGBClass(0,128,255));EXPECT_EQ(held->ColorList[1],RGBClass(255,255,255));
  game::BuildingVoxelPart parts[4];unsigned count=0;ASSERT_TRUE(game::unit_voxel_parts(*s.carrier,parts,4,count));
  bool repair_turret=false;for(unsigned i=0;i<count;++i)repair_turret|=parts[i].resource==&s.carrier->Type->ChargerTurrets[2];EXPECT_TRUE(repair_turret);
  std::cout<<"IFV_ENTER ticks="<<ticks<<" weapon="<<s.carrier->CurrentWeaponNumber<<" turret="<<s.carrier->CurrentTurretNumber<<'\n';
  for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"MTNK")&&unit->Owner==s.carrier->Owner){s.damaged=unit;break;}
  ASSERT_NE(s.damaged,nullptr);s.health=s.damaged->Type->Strength/2;s.damaged->Health=s.damaged->EstimatedHealth=s.health;
  ASSERT_TRUE(s.carrier->Select());
 },&s));
 ASSERT_TRUE(click_world_object(*view.view,s.damaged));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& world=*s.view->world;
  ASSERT_EQ(EventClass::OutList.Count,1);EXPECT_EQ(EventClass::OutList.First().MegaMission.Mission,unsigned(Mission::Attack));
  EXPECT_EQ(s.carrier->MouseOverObject(s.damaged),Action::GRepair);
  int ticks=0;for(;ticks<600&&s.damaged->Health<=s.health;++ticks){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_GT(s.damaged->Health,s.health);std::cout<<"IFV_REPAIR ticks="<<ticks<<" health="<<s.health<<"->"<<s.damaged->Health<<'\n';
  int peak=0,sparkPixels=0,lights=0;
  for(int i=0;i<600&&(s.damaged->Health<s.damaged->Type->Strength||s.carrier->Target||s.carrier->SparkParticleSystem);++i){
   ++Unsorted::CurrentFrame;game::update_map_world(world);peak=std::max(peak,ParticleClass::Array.Count);
   game::rebuild_world_sprites(world);
   struct Capture{int* pixels;int* lights;}capture{&sparkPixels,&lights};
   game::TypeDrawingContext context;context.target=reinterpret_cast<game::DrawingTargetHandle*>(&capture);context.backend_context=&capture;
   context.backend.raster=[](void* p,const game::RasterDrawingRequest&r){auto&c=*static_cast<Capture*>(p);if(r.blend_mode==game::RasterBlendMode::particle)++*c.pixels;if(r.blend_mode==game::RasterBlendMode::spotlight)++*c.lights;return game::DrawingStatus::drawn;};
   for(const auto& sprite:world.impl->sprites)if(sprite.draw_object){
    struct Draw{ObjectClass* object;Point2D point;RectangleStruct bounds;}draw{sprite.owner,sprite.position,TacticalClass::ViewBounds};
    EXPECT_TRUE(game::drawing_completed(game::with_type_drawing(context,[](void* p){auto&d=*static_cast<Draw*>(p);d.object->DrawIt(&d.point,&d.bounds);},&draw)));
   }
  }
  EXPECT_GT(peak,0);EXPECT_GT(sparkPixels,0);EXPECT_GT(lights,0);EXPECT_EQ(s.carrier->SparkParticleSystem,nullptr);
  EXPECT_EQ(ParticleClass::Array.Count,0);EXPECT_EQ(ParticleSystemClass::Array.Count,0);
  std::cout<<"IFV_SPARKS peak="<<peak<<" pixel_submissions="<<sparkPixels<<" light_submissions="<<lights<<'\n';
  EXPECT_EQ(s.damaged->Health,s.damaged->Type->Strength);EXPECT_EQ(s.carrier->Target,nullptr);
  EXPECT_NE(s.carrier->MouseOverObject(s.damaged),Action::GRepair);
  s.carrier->SetTarget(nullptr);s.carrier->SetDestination(nullptr,true);
 },&s));
 game::GameInputResult input;
 ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,'D',0,true},input));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& world=*s.view->world;
  ASSERT_EQ(EventClass::OutList.Count,1);EXPECT_EQ(EventClass::OutList.First().Type,EventType::Deploy);
  int ticks=0;for(;ticks<600&&s.engineer->InLimbo;++ticks){++Unsorted::CurrentFrame;game::update_map_world(world);}
  ASSERT_FALSE(s.engineer->InLimbo);EXPECT_TRUE(s.engineer->IsOnMap);EXPECT_EQ(s.engineer->Transporter,nullptr);
  EXPECT_EQ(s.carrier->Passengers.NumPassengers,0);EXPECT_EQ(s.carrier->Passengers.FirstPassenger,nullptr);
  EXPECT_EQ(s.carrier->CurrentWeaponNumber,0);EXPECT_EQ(s.carrier->CurrentTurretNumber,0);
  EXPECT_STREQ(s.carrier->GetTurretWeapon()->WeaponType->ID,"HoverMissile");
  EXPECT_GE(LogicClass::Instance.FindItemIndex(s.engineer),0);
  const auto passengerAt=s.engineer->Location,carrierAt=s.carrier->Location;
  std::cout<<"IFV_FIRST_VISIBLE passenger="<<passengerAt.X<<','<<passengerAt.Y<<','<<passengerAt.Z
   <<" carrier="<<carrierAt.X<<','<<carrierAt.Y<<','<<carrierAt.Z<<'\n';
  for(int i=0;i<120;++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_FALSE(s.engineer->Locomotor->Is_Moving());
  EXPECT_NE(s.engineer->GetCell()->OccupationFlags&0x1Fu,0u);
  std::cout<<"IFV_DEPLOY_UNLOAD ticks="<<ticks<<" passenger_cell="<<s.engineer->GetMapCoords().X<<','<<s.engineer->GetMapCoords().Y<<'\n';
 },&s));
 // The unloaded actor must be available to normal picking and movement again.
 ASSERT_TRUE(click_world_object(*view.view,s.engineer));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){EXPECT_TRUE(static_cast<State*>(p)->engineer->IsSelected);},&s));
 // Repeat through the HUD command; block the exits after maneuvering so the
 // failed-placement branch must restore the passenger AND its gunner mode.
 ASSERT_TRUE(click_world_object(*view.view,s.carrier));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(int i=0;i<1200&&!s.engineer->InLimbo;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  ASSERT_TRUE(s.engineer->InLimbo);ASSERT_EQ(s.carrier->Passengers.NumPassengers,1);ASSERT_TRUE(s.carrier->Select());
 },&s));
 ASSERT_TRUE(game::set_game_view_size(*view.view,1280,720));
 Point2D hud;
 struct Hud{game::MapViewHandle* view;Point2D* point;}hudState{view.view,&hud};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& h=*static_cast<Hud*>(p);
  h.view->ui_resources=std::make_unique<game::UiResources>();ASSERT_TRUE(h.view->ui_resources->load(h.view->scenario.PlayerSideIndex));
  game::MapDrawingContext drawing;game::MapDrawStatistics stats{};
  drawing.plain_palette=[](void*,const BytePalette& palette,const game::DrawingPaletteHandle*& output)noexcept{output=reinterpret_cast<const game::DrawingPaletteHandle*>(&palette);return game::DrawingStatus::drawn;};
  game::GameUiFrame frame{drawing,*h.view->ui_resources,stats};
  game::with_game_ui_frame(frame,[]{TabClass::Instance.InitializeCommandButtons();});
  const int slot=TabClass::CommandPositions[4];ASSERT_GE(slot,0);ASSERT_LT(slot,25);
  const auto& button=TabClass::CommandButtons[slot];ASSERT_FALSE(button.Disabled);EXPECT_EQ(button.ID,218);
  *h.point={button.X+button.Width/2,button.Y+button.Height/2};
 },&hudState));
 for(bool down:{true,false})ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,hud.X,hud.Y,1,0,down},input));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& world=*s.view->world;
  ASSERT_EQ(EventClass::OutList.Count,1);EXPECT_EQ(EventClass::OutList.First().Type,EventType::Deploy);
  for(int i=0;i<600&&!(s.carrier->CurrentMission==Mission::Unload&&s.carrier->MissionStatus==3);++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
  ASSERT_EQ(s.carrier->CurrentMission,Mission::Unload);ASSERT_EQ(s.carrier->MissionStatus,3);
  std::array<CellClass*,8> exits;std::array<unsigned,8> occupation;const auto center=s.carrier->GetMapCoords();
  for(int i=0;i<8;++i){const auto d=Unsorted::AdjacentCell[i];exits[i]=MapClass::Instance.GetCellAt(CellStruct{short(center.X+d.X),short(center.Y+d.Y)});
   occupation[i]=exits[i]->OccupationFlags;exits[i]->OccupationFlags|=0x20u;}
  for(int i=0;i<90;++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_TRUE(s.engineer->InLimbo);EXPECT_FALSE(s.engineer->IsOnMap);EXPECT_EQ(s.engineer->Transporter,s.carrier);
  EXPECT_EQ(s.carrier->Passengers.NumPassengers,1);EXPECT_EQ(s.carrier->Passengers.FirstPassenger,s.engineer);
  EXPECT_EQ(s.carrier->CurrentWeaponNumber,1);EXPECT_EQ(s.carrier->CurrentTurretNumber,2);
  EXPECT_STREQ(s.carrier->GetTurretWeapon()->WeaponType->ID,"RepairBullet");
  for(int i=0;i<8;++i)exits[i]->OccupationFlags=occupation[i];
  for(int i=0;i<600&&s.engineer->InLimbo;++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_FALSE(s.engineer->InLimbo);EXPECT_EQ(s.engineer->Transporter,nullptr);EXPECT_EQ(s.carrier->Passengers.NumPassengers,0);
  EXPECT_EQ(s.carrier->CurrentWeaponNumber,0);EXPECT_EQ(s.carrier->CurrentTurretNumber,0);
  std::cout<<"IFV_HUD_UNLOAD blocked_retry=True reentered=True\n";
 },&s));
}

TEST(UnitRendering, CommandFlashExpiresWithCachedSprites){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& world=*static_cast<game::MapViewHandle*>(p)->world;
  UnitClass* carrier=nullptr;for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"FV")){carrier=unit;break;}ASSERT_NE(carrier,nullptr);
  for(int i=0;i<20;++i){++Unsorted::CurrentFrame;carrier->Update();}
  ASSERT_TRUE(LogicClass::Instance.AddObject(carrier,false));
  game::rebuild_world_sprites(world);carrier->NeedsRedraw=true;carrier->Flash(7);
  bool bright=false;
  for(int i=0;i<10;++i){
   game::rebuild_world_sprites(world);
   const int expected=carrier->GetFlashingIntensity(carrier->GetCell()->Intensity_Normal+RulesClass::Instance->ExtraUnitLight);
   bool found=false;for(const auto& sprite:world.impl->sprites)if(sprite.owner==carrier&&!sprite.shadow){found=true;EXPECT_EQ(sprite.intensity,expected);}
   EXPECT_TRUE(found);bright|=(carrier->Flashing.DurationRemaining&2)!=0;
   ++Unsorted::CurrentFrame;game::update_map_world(world);
   EXPECT_EQ(carrier->Flashing.DurationRemaining,std::max(6-i,0));
  }
  EXPECT_TRUE(bright);EXPECT_EQ(carrier->Flashing.DurationRemaining,0);
 },view.view));
}

TEST(PlayerCommands, OriginalHotkeyMappingAndDeployEvent){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& view=*static_cast<game::MapViewHandle*>(p);
  CommandClass* command=nullptr;ASSERT_TRUE(CommandClass::Hotkeys.TryGet('D',command));ASSERT_NE(command,nullptr);
  EXPECT_STREQ(command->GetName(),"DeployObject");
  InfantryClass* actor=nullptr;for(auto* gi:InfantryClass::Array)if(!std::strcmp(gi->Type->ID,"E1")){actor=gi;break;}
  ASSERT_NE(actor,nullptr);ASSERT_TRUE(actor->Select());
  EXPECT_TRUE(game::dispatch_player_hotkey(WWKey('D'|int(WWKey::Release))));EXPECT_EQ(EventClass::OutList.Count,0);
  EXPECT_FALSE(game::dispatch_player_hotkey(WWKey('D'|int(WWKey::Ctrl))));EXPECT_EQ(EventClass::OutList.Count,0);
  EXPECT_TRUE(game::dispatch_player_hotkey(WWKey('D')));ASSERT_EQ(EventClass::OutList.Count,1);
  EXPECT_EQ(EventClass::OutList.First().Type,EventType::Deploy);EXPECT_EQ(EventClass::OutList.First().Deploy.Whom.As_Techno(),actor);
  for(int i=0;i<100;++i){++Unsorted::CurrentFrame;game::update_map_world(*view.world);}
  EXPECT_TRUE(actor->IsDeployed());
  // Runtime mapping comes from the INI, not a fixed D comparison.
  CCINIClass alternate;alternate.WriteInteger("Hotkey","DeployObject",'F'|int(WWKey::Ctrl));
  ASSERT_TRUE(game::load_player_hotkeys(alternate));
  EXPECT_FALSE(game::dispatch_player_hotkey(WWKey('D')));EXPECT_EQ(EventClass::OutList.Count,0);
  EXPECT_TRUE(game::dispatch_player_hotkey(WWKey('F'|int(WWKey::Ctrl))));ASSERT_EQ(EventClass::OutList.Count,1);
  EXPECT_EQ(EventClass::OutList.First().Type,EventType::Deploy);
  for(int i=0;i<100;++i){++Unsorted::CurrentFrame;game::update_map_world(*view.world);}
  EXPECT_FALSE(actor->IsDeployed());
 },view.view));
}

TEST(PlayerCommands, StopHotkeyStopsSelectedInfantryAndTank){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_game_view_size(*view.view,1280,720));
 struct State{game::MapViewHandle* view;std::array<FootClass*,2> actors{};InfantryClass* other=nullptr;BuildingClass* target=nullptr;
  std::array<CoordStruct,2> starts{},stops{};}s{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  CommandClass* command=nullptr;ASSERT_TRUE(CommandClass::Hotkeys.TryGet('S',command));ASSERT_NE(command,nullptr);
  EXPECT_STREQ(command->GetName(),"StopObject");
  for(auto* unit:UnitClass::Array){unit->ForceMission(Mission::Sleep);
   if(!s.actors[1]&&unit->Owner->IsControlledByCurrentPlayer()&&!std::strcmp(unit->Type->ID,"MTNK"))s.actors[1]=unit;}
  for(auto* actor:InfantryClass::Array){actor->ForceMission(Mission::Sleep);
   if(actor->Owner->IsControlledByCurrentPlayer()&&!std::strcmp(actor->Type->ID,"E1")){
    if(!s.actors[0])s.actors[0]=actor;else s.other=actor;}}
  for(auto* building:BuildingClass::Array)if(!std::strcmp(building->Type->ID,"GAWEAP"))s.target=building;
  ASSERT_NE(s.actors[0],nullptr);ASSERT_NE(s.actors[1],nullptr);ASSERT_NE(s.other,nullptr);ASSERT_NE(s.target,nullptr);
  for(unsigned i=0;i<s.actors.size();++i){auto* actor=s.actors[i];ASSERT_TRUE(actor->Select());s.starts[i]=actor->Location;
   const auto at=actor->GetMapCoords();CellClass* destination=nullptr;
   for(int y=-6;y<=6&&!destination;++y)for(int x=-6;x<=6&&!destination;++x){
    if(std::abs(x)+std::abs(y)<6)continue;
    auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(at.X+x),short(at.Y+y)});
    if(cell&&MapClass::Instance.IsWithinUsableArea(cell->MapCoords,true)&&cell->Level==actor->GetCell()->Level
       &&actor->IsCellOccupied(cell,FacingType::None,-1,nullptr,false)==Move::OK)destination=cell;
   }
   ASSERT_NE(destination,nullptr);ASSERT_TRUE(actor->ClickedMission(Mission::Move,nullptr,destination,nullptr));
  }
  for(int i=0;i<35;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  for(unsigned i=0;i<s.actors.size();++i){auto* actor=s.actors[i];EXPECT_NE(actor->Location,s.starts[i]);ASSERT_NE(actor->Destination,nullptr);
   actor->NavQueue.AddItem(s.target);actor->PlanningPathIdx=3;actor->WaypointIndex=2;
   actor->WaypointCell={12,13};actor->WaypointNearbyAccessibleCellDelta={1,2};
  }
  s.other->SetTarget(s.target);
 },&s));
 const auto key=[&](bool pressed,unsigned modifiers=0){game::GameInputResult result;
  // Global gameplay key: the cursor can be outside the world/over the sidebar.
  EXPECT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,1500,500,'S',modifiers,pressed},result));
 };
 key(false);key(true,2);
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){EXPECT_EQ(EventClass::OutList.Count,0);},nullptr));
 key(true);key(false);
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  ASSERT_EQ(EventClass::OutList.Count,2);EXPECT_EQ(EventClass::OutList.First().Type,EventType::Idle);
  EXPECT_EQ(EventClass::OutList.First().Idle.Whom.As_Techno(),s.actors[0]);
  ++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);
  for(auto* actor:s.actors){EXPECT_EQ(actor->Target,nullptr);EXPECT_EQ(actor->Destination,nullptr);EXPECT_EQ(actor->NavQueue.Count,0);
   EXPECT_EQ(actor->PlanningPathIdx,-1);EXPECT_EQ(actor->WaypointIndex,0);EXPECT_EQ(actor->WaypointCell,(CellStruct{0,0}));
   EXPECT_EQ(actor->WaypointNearbyAccessibleCellDelta,(CellStruct{0,0}));}
  EXPECT_EQ(s.other->Target,s.target);
  // Finish the current locomotor step; stopping must preserve valid cell occupancy.
  for(int i=0;i<120;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  for(unsigned i=0;i<s.actors.size();++i){s.stops[i]=s.actors[i]->Location;EXPECT_FALSE(s.actors[i]->Locomotor->Is_Moving());}
  for(int i=0;i<60;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  for(unsigned i=0;i<s.actors.size();++i){EXPECT_EQ(s.actors[i]->Location,s.stops[i]);
   // Cancel an explicit attack through the same event, without advancing AI
   // far enough to automatically reacquire another nearby enemy.
   s.actors[i]->ForceMission(Mission::Attack);s.actors[i]->SetTarget(s.target);
  }
 },&s));
 key(true);
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  ASSERT_EQ(EventClass::OutList.Count,2);
  while(EventClass::OutList.Count){EventClass event(EventClass::OutList.First());EventClass::OutList.Next();event.Execute();}
  for(auto* actor:s.actors){EXPECT_EQ(actor->Target,nullptr);EXPECT_EQ(actor->Destination,nullptr);EXPECT_TRUE(actor->IsSelected);}
  // Binding remains configurable and release never queues a second command.
  CCINIClass alternate;alternate.WriteInteger("Hotkey","StopObject",'X'|int(WWKey::Ctrl));ASSERT_TRUE(game::load_player_hotkeys(alternate));
  EXPECT_FALSE(game::dispatch_player_hotkey(WWKey('S')));
  EXPECT_TRUE(game::dispatch_player_hotkey(WWKey('X'|int(WWKey::Ctrl))));EXPECT_EQ(EventClass::OutList.Count,2);
 },&s));
}

TEST(PlayerCommands, EliteIFVStopAttackBurstCadence){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 std::array<int,3> shots{};
 for(int reset=0;reset<3;++reset){
  View view(data,true);view.load("ALL01UMD.MAP");
  struct State{game::MapViewHandle* view;UnitClass* actor=nullptr;CellClass* target=nullptr;int shots=0,last=0;};State s{view.view};
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   for(auto* unit:UnitClass::Array){unit->ForceMission(Mission::Sleep);if(!s.actor&&!std::strcmp(unit->Type->ID,"FV")&&unit->Owner->IsControlledByCurrentPlayer())s.actor=unit;}
   for(auto* actor:InfantryClass::Array)actor->ForceMission(Mission::Sleep);
   ASSERT_NE(s.actor,nullptr);s.actor->Veterancy.Veterancy=2.0f;ASSERT_TRUE(s.actor->Select());
   auto at=s.actor->GetMapCoords();s.target=MapClass::Instance.GetCellAt(CellStruct{short(at.X+2),at.Y});
   DirStruct facing;s.actor->GetDirectionTo(&facing,s.target);s.actor->PrimaryFacing.SetDesired(facing);s.actor->PrimaryFacing.SetCurrent(facing);
   s.actor->SecondaryFacing.SetDesired(facing);s.actor->SecondaryFacing.SetCurrent(facing);
   auto* weapon=s.actor->GetWeapon(s.actor->SelectWeapon(s.target))->WeaponType;
   std::cout<<"IFV_BURST weapon="<<weapon->ID<<" burst="<<weapon->Burst<<" rof="<<weapon->ROF<<"\n";
   ASSERT_GT(weapon->Burst,1);s.last=s.actor->LastFireBulletFrame;
   ASSERT_TRUE(s.actor->ClickedMission(Mission::Attack,s.target,nullptr,nullptr));
  },&s));
  for(int frame=0;frame<180;++frame){
   const int previous=s.shots;
   ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
    ++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);
    if(s.last!=s.actor->LastFireBulletFrame){s.last=s.actor->LastFireBulletFrame;++s.shots;
     std::cout<<"IFV_SHOT "<<s.shots<<" frame="<<Unsorted::CurrentFrame<<" burst="<<s.actor->CurrentBurstIndex<<" rearm="<<s.actor->RearmTimer.GetTimeLeft()<<"\n";}
   },&s));
   if(reset&&s.shots!=previous&&(reset==1||s.actor->CurrentBurstIndex==0)){
    // Stop resets the burst index, never the already-started cooldown. In
    // mode 2 the final missile has already installed the long ROF timer.
    const int remaining=s.actor->RearmTimer.GetTimeLeft();
    game::GameInputResult result;
    ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,'S',0,true},result));
    ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,'S',0,false},result));
    ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
     ASSERT_EQ(EventClass::OutList.Count,1);EventClass event(EventClass::OutList.First());EventClass::OutList.Next();event.Execute();
    },nullptr));
    EXPECT_EQ(s.actor->CurrentBurstIndex,0);EXPECT_EQ(s.actor->RearmTimer.GetTimeLeft(),remaining);
    ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);ASSERT_TRUE(s.actor->ClickedMission(Mission::Attack,s.target,nullptr,nullptr));},&s));
   }
  }
  shots[reset]=s.shots;
 }
 std::cout<<"IFV_CADENCE continuous="<<shots[0]<<" stop_attack="<<shots[1]<<" after_volley="<<shots[2]<<"\n";
 EXPECT_GT(shots[0],0);EXPECT_GT(shots[1],shots[0]*2);
 EXPECT_EQ(shots[2],shots[0]);
}

TEST(PlayerCommands, EliteIFVBlindStopClickCadence){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 int baseline=0;
 for(int period:{0,4,6,9,12}){
  SCOPED_TRACE(period);View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
  struct State{game::MapViewHandle* view;UnitClass* actor=nullptr;UnitClass* target=nullptr;Point2D point{};int shots=0,last=0;bool retaliation=false;}s{view.view};
  auto restore=ra2::test::scope_exit([&]{if(s.target)s.target->Type->CanRetaliate=s.retaliation;});
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   for(auto* unit:UnitClass::Array){unit->ForceMission(Mission::Sleep);
    if(!s.actor&&!std::strcmp(unit->Type->ID,"FV")&&unit->Owner->IsControlledByCurrentPlayer())s.actor=unit;
    if(!s.target&&!std::strcmp(unit->Type->ID,"LTNK")&&!unit->Owner->IsControlledByCurrentPlayer())s.target=unit;}
   for(auto* infantry:InfantryClass::Array)infantry->ForceMission(Mission::Sleep);
   ASSERT_NE(s.actor,nullptr);ASSERT_NE(s.target,nullptr);s.actor->Veterancy.Veterancy=2.0f;
   // An immobile, durable enemy isolates command timing from target loss.
   s.target->Health=s.target->EstimatedHealth=1000000;s.retaliation=s.target->Type->CanRetaliate;s.target->Type->CanRetaliate=false;
   ASSERT_TRUE(s.actor->Limbo());bool placed=false;auto at=s.target->GetMapCoords();
   for(int y=-3;y<=3&&!placed;++y)for(int x=-3;x<=3&&!placed;++x){
    if(std::abs(x)+std::abs(y)<2)continue;
    auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(at.X+x),short(at.Y+y)});
    if(cell&&cell->Level==s.target->GetCell()->Level&&!cell->SlopeIndex&&MapClass::Instance.IsWithinUsableArea(cell->MapCoords,true)
       &&s.actor->IsCellOccupied(cell,FacingType::None,-1,nullptr,false)==Move::OK)placed=s.actor->Unlimbo(cell->GetCoords(),DirType::North);
   }
   ASSERT_TRUE(placed);ASSERT_TRUE(s.actor->Select());
   DirStruct facing;s.actor->GetDirectionTo(&facing,s.target);
   s.actor->PrimaryFacing.SetDesired(facing);s.actor->PrimaryFacing.SetCurrent(facing);
   s.actor->SecondaryFacing.SetDesired(facing);s.actor->SecondaryFacing.SetCurrent(facing);
   s.last=s.actor->LastFireBulletFrame;
  },&s));
  ASSERT_TRUE(click_world_object(*view.view,s.target));
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   ASSERT_EQ(EventClass::OutList.Count,1);EXPECT_EQ(EventClass::OutList.First().MegaMission.Target.As_Object(),s.target);
   s.point=s.view->world->impl->press_point;
  },&s));
  for(int frame=0;frame<360;++frame){
   // No shot feedback, burst counter or cooldown inspection controls input.
   // S and attack are deliberately on different simulation frames.
   game::GameInputResult result;
   if(period&&frame%period==1)for(bool down:{true,false})
    ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,s.point.X,s.point.Y,'S',0,down},result));
   if(period&&frame%period==1+period/2)for(bool down:{true,false})
    ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,s.point.X,s.point.Y,1,0,down},result));
   ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
    ++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);
    ASSERT_TRUE(s.actor->IsSelected);ASSERT_TRUE(s.target->IsAlive);
    if(s.actor->LastFireBulletFrame!=s.last){++s.shots;s.last=s.actor->LastFireBulletFrame;}
   },&s));
  }
  std::cout<<"IFV_BLIND_INPUT period="<<period<<" shots="<<s.shots<<"\n";
  if(!period){baseline=s.shots;ASSERT_GT(baseline,0);}else EXPECT_GT(s.shots,baseline);
 }
}

TEST(PlayerCommands, StopEventPreservesTetherAndCancelsHarvester){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  UnitClass* miner=nullptr;for(auto* unit:UnitClass::Array)if(unit->Type->Harvester&&unit->Owner->IsControlledByCurrentPlayer()){miner=unit;break;}
  ASSERT_NE(miner,nullptr);auto* cell=miner->GetCell();
  for(auto mission:{Mission::Harvest,Mission::Enter}){
   miner->ForceMission(mission);miner->SetDestination(cell,true);miner->IsTether=true;
   miner->NavQueue.AddItem(cell);miner->PlanningPathIdx=2;
   ASSERT_TRUE(miner->ClickedEvent(EventType::Idle));EventClass event(EventClass::OutList.First());EventClass::OutList.Next();event.Execute();
   EXPECT_EQ(miner->CurrentMission,mission);EXPECT_EQ(miner->Destination,cell);EXPECT_EQ(miner->PlanningPathIdx,2);
   miner->IsTether=false;event.Execute();
   EXPECT_EQ(miner->CurrentMission,Mission::Guard);EXPECT_EQ(miner->Destination,nullptr);EXPECT_EQ(miner->NavQueue.Count,0);
  }
  // Open-topped stop also clears every limbo passenger's target through its
  // virtual SetTarget; it must not enter the original-only jump stub.
  struct Passenger:InfantryClass{using InfantryClass::InfantryClass;int calls=0;
   void SetTarget(AbstractClass* target)override{Target=target;++calls;}};
  auto* type=InfantryTypeClass::Find("E1");ASSERT_NE(type,nullptr);
  Passenger first(type,miner->Owner),second(type,miner->Owner);
  const bool open=miner->Type->OpenTopped;auto* previous=miner->Passengers.FirstPassenger;
  auto restore=ra2::test::scope_exit([&]{miner->Type->OpenTopped=open;miner->Passengers.FirstPassenger=previous;first.NextObject=nullptr;});
  miner->Type->OpenTopped=true;miner->Passengers.FirstPassenger=&first;first.NextObject=&second;
  first.Target=second.Target=cell;
  ASSERT_TRUE(miner->ClickedEvent(EventType::Idle));EventClass event(EventClass::OutList.First());EventClass::OutList.Next();event.Execute();
  EXPECT_EQ(first.Target,nullptr);EXPECT_EQ(second.Target,nullptr);EXPECT_EQ(first.calls,1);EXPECT_EQ(second.calls,1);
 },nullptr));
}

TEST(InfantryGarrison, PlainAndControlClickCivilianBuilding){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 for(int scenario=0;scenario<4;++scenario){
  const bool force=scenario==1||scenario==3,red=scenario>=2;
  SCOPED_TRACE(::testing::Message()<<"force="<<force<<" red="<<red);
  View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
  struct State{game::MapViewHandle* view;BuildingClass* building=nullptr;std::vector<InfantryClass*> actors;int health=0;bool attack;bool red;};
  State state{view.view,nullptr,{},0,force,red};
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   for(auto* building:BuildingClass::Array)
    if(building->Type->CanBeOccupied&&building->Type->CanOccupyFire&&building->Type->MaxNumberOccupants>=5
       &&building->Owner->Type->MultiplayPassive&&!building->IsRedHP()&&!building->GetOccupantCount()){
     s.building=building;break;
    }
   ASSERT_NE(s.building,nullptr);
   if(s.red){s.building->Health=int(s.building->Type->Strength*RulesClass::Instance->ConditionRed);s.building->EstimatedHealth=s.building->Health;}
   EXPECT_EQ(s.building->IsRedHP(),s.red);s.health=s.building->Health;
   for(int i=0;i<5;++i){auto* actor=infantry_near_building("E1",HouseClass::CurrentPlayer,s.building);
    ASSERT_NE(actor,nullptr);s.actors.push_back(actor);
   }
   EXPECT_TRUE(s.building->Owner->Type->MultiplayPassive);EXPECT_EQ(EventClass::OutList.Count,0);
  },&state));
  ASSERT_NE(state.building,nullptr);ASSERT_EQ(state.actors.size(),5u);
  for(std::size_t i=0;i<state.actors.size();++i)ASSERT_TRUE(click_world_object(*view.view,state.actors[i],i?1:0));
  for(auto* actor:state.actors)ASSERT_TRUE(actor->IsSelected);
  ASSERT_TRUE(click_world_object(*view.view,state.building,force?2:0));
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   // ALL01 declares Americans allied to Neutral. Red health prevents entry;
   // it does not turn a plain click on that ally into an attack order.
   if(s.red&&!s.attack){EXPECT_EQ(EventClass::OutList.Count,0);return;}
   ASSERT_EQ(EventClass::OutList.Count,5);
   for(int i=0;i<EventClass::OutList.Count;++i){auto& event=EventClass::OutList[i];
    EXPECT_EQ(event.MegaMission.Mission,unsigned(s.attack?Mission::Attack:Mission::Capture));
    if(s.attack)EXPECT_EQ(event.MegaMission.Target.As_Abstract(),s.building);
    else {EXPECT_EQ(event.MegaMission.Destination.As_Abstract(),s.building);EXPECT_EQ(event.MegaMission.Target.As_Abstract(),nullptr);}
   }
  },&state));
  if(red&&!force)continue;
  // Release Ctrl before consuming the queue: force attack must persist as an
  // order, while a plain click must reach the building without firing at it.
  game::GameInputResult result;
  ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,17,0,false},result));
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& world=*s.view->world;
   std::vector<int> lastFire;for(auto* actor:s.actors)lastFire.push_back(actor->LastFireBulletFrame);
   int frame=0;
   for(;frame<1800;++frame){++Unsorted::CurrentFrame;game::update_map_world(world);
    if(s.attack?s.building->Health<s.health:s.building->GetOccupantCount()==5)break;
   }
   if(s.attack){EXPECT_LT(s.building->Health,s.health);EXPECT_EQ(s.building->GetOccupantCount(),0);
    for(auto* actor:s.actors){EXPECT_FALSE(actor->InLimbo);EXPECT_TRUE(actor->IsOnMap);}
   }else{ASSERT_EQ(s.building->GetOccupantCount(),5);EXPECT_EQ(s.building->Health,s.health);
    for(std::size_t i=0;i<s.actors.size();++i){auto* actor=s.actors[i];EXPECT_GE(s.building->Occupants.FindItemIndex(actor),0);
     EXPECT_TRUE(actor->InLimbo);EXPECT_FALSE(actor->IsOnMap);EXPECT_EQ(actor->LastFireBulletFrame,lastFire[i]);
    }
   }
   std::cout<<"CIVILIAN_CLICK attack="<<s.attack<<" red="<<s.red<<" type="<<s.building->Type->ID<<" occupants="<<s.building->GetOccupantCount()
            <<" health="<<s.health<<"->"<<s.building->Health<<" frames="<<frame<<"\n";
  },&state));
 }
}

TEST(InfantryEngineer, ClickMoveRepairAndCapture){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* context){
  auto& world=*static_cast<game::MapViewHandle*>(context)->world;
  auto* owner=HouseClass::CurrentPlayer;ASSERT_NE(owner,nullptr);
  BuildingClass* repair=nullptr;BuildingClass* capture=nullptr;
  for(auto* building:BuildingClass::Array){
   if(!repair&&building->Owner==owner&&building->Type->Repairable&&!building->Type->CanBeOccupied)repair=building;
   if(!capture&&!owner->IsAlliedWith(building)&&building->Type->Capturable&&!building->Type->CanBeOccupied)capture=building;
  }
  ASSERT_NE(repair,nullptr);ASSERT_NE(capture,nullptr);
  repair->Health=repair->Type->Strength/2;repair->EstimatedHealth=repair->Health;
  auto* engineer=infantry_near_building("ENGINEER",owner,repair);ASSERT_NE(engineer,nullptr);
  EXPECT_EQ(engineer->MouseOverObject(repair,true),Action::GRepair);
  ASSERT_TRUE(engineer->ObjectClickedAction(Action::GRepair,repair,true));
  const auto repairId=engineer->UniqueID;
  for(int frame=0;frame<1200&&repair->Health<repair->Type->Strength;++frame){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_EQ(repair->Health,repair->Type->Strength);EXPECT_EQ(repair->EstimatedHealth,repair->Health);
  EXPECT_FALSE(std::any_of(InfantryClass::Array.begin(),InfantryClass::Array.end(),[&](auto* p){return p->UniqueID==repairId&&p->IsAlive&&!p->InLimbo;}));
  auto* previous=capture->Owner;const int previousCount=previous->OwnedBuildingTypes.GetItemCount(capture->Type->ArrayIndex);
  const int nextCount=owner->OwnedBuildingTypes.GetItemCount(capture->Type->ArrayIndex);
  engineer=infantry_near_building("ENGINEER",owner,capture);ASSERT_NE(engineer,nullptr);
  const auto action=engineer->MouseOverObject(capture,true);ASSERT_TRUE(action==Action::Capture||action==Action::Damage);
  ASSERT_TRUE(engineer->ObjectClickedAction(action,capture,true));const auto captureId=engineer->UniqueID;
  for(int frame=0;frame<1600&&capture->Owner!=owner;++frame){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_EQ(capture->Owner,owner);EXPECT_TRUE(capture->HasBeenCaptured);
  EXPECT_EQ(previous->Buildings.FindItemIndex(capture),-1);EXPECT_GE(owner->Buildings.FindItemIndex(capture),0);
  EXPECT_EQ(previous->OwnedBuildingTypes.GetItemCount(capture->Type->ArrayIndex),previousCount-1);
  EXPECT_EQ(owner->OwnedBuildingTypes.GetItemCount(capture->Type->ArrayIndex),nextCount+1);
  EXPECT_FALSE(std::any_of(InfantryClass::Array.begin(),InfantryClass::Array.end(),[&](auto* p){return p->UniqueID==captureId&&p->IsAlive&&!p->InLimbo;}));
 },view.view));
}

TEST(InfantryEngineer, HudCaptureEnemyBuildings){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 struct Case{const char* map;const char* type;};
 for(const auto& test:std::array<Case,6>{{{"ALL01UMD.MAP","NANRCT"},{"ALL01UMD.MAP","CAPOWR01"},{"ALL01UMD.MAP","CAPOWR02"},{"ALL01UMD.MAP","CAPOWR03"},
  {"ALL07SMD.MAP","NACNST"},{"ALL07SMD.MAP","NAPOWR"}}}){
  const char* id=test.type;SCOPED_TRACE(test.map);
  SCOPED_TRACE(id);
  View view(data,true);view.load(test.map);ASSERT_TRUE(game::set_game_view_size(*view.view,1280,720));
  struct State{game::MapViewHandle* view;const char* id;BuildingClass* building=nullptr;InfantryClass* engineer=nullptr;
   HouseClass* previous=nullptr;game::MapObjectId buildingID{};int engineerID=0,ownerIndex=-1;bool repairs=false;}s{view.view,id};
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   for(auto* b:BuildingClass::Array)if(!std::strcmp(b->Type->ID,s.id)&&b->Owner!=HouseClass::CurrentPlayer){s.building=b;break;}
   ASSERT_NE(s.building,nullptr);
   s.previous=s.building->Owner;s.buildingID=game::object_id(*s.view->world,s.building);s.ownerIndex=HouseClass::CurrentPlayer->ArrayIndex;
   s.repairs=HouseClass::CurrentPlayer->IsAlliedWith(s.building);
   s.engineer=infantry_near_building("ENGINEER",HouseClass::CurrentPlayer,s.building);ASSERT_NE(s.engineer,nullptr);
   // Keep the map's real orders, path and capture rules, but prevent nearby
   // defenders from killing the engineer before this ownership regression runs.
   s.engineer->Health=s.engineer->EstimatedHealth=100000;
   s.engineerID=s.engineer->UniqueID;ASSERT_TRUE(s.engineer->Select());
  },&s));
  ASSERT_NE(s.building,nullptr);ASSERT_NE(s.engineer,nullptr);
  ASSERT_TRUE(click_world_object(*view.view,s.building));
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   ASSERT_EQ(EventClass::OutList.Count,1);EXPECT_EQ(EventClass::OutList.First().MegaMission.Mission,unsigned(Mission::Capture));
   int ticks=0;for(;ticks<1600&&s.building->Owner!=HouseClass::CurrentPlayer;++ticks){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
   std::cout<<"ENGINEER_CAPTURE type="<<s.id<<" ticks="<<ticks<<" owner="<<s.building->Owner->ArrayIndex<<"\n";
   EXPECT_EQ(s.building->Owner,HouseClass::CurrentPlayer);EXPECT_TRUE(s.building->HasBeenCaptured);
   if(s.repairs)EXPECT_EQ(s.building->Health,s.building->Type->Strength);
   EXPECT_FALSE(std::any_of(InfantryClass::Array.begin(),InfantryClass::Array.end(),[&](auto* actor){return actor->UniqueID==s.engineerID&&actor->IsAlive&&!actor->InLimbo;}));
   // Do not stop at the first changed frame: garrison/animation updates must
   // not hand an engineer-captured building back to its previous house.
   for(int i=0;i<120;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);EXPECT_EQ(s.building->Owner,HouseClass::CurrentPlayer);}
   EXPECT_EQ(s.previous->Buildings.FindItemIndex(s.building),-1);
   EXPECT_GE(HouseClass::CurrentPlayer->Buildings.FindItemIndex(s.building),0);
  },&s));
  game::MapObjectSnapshot snapshot;ASSERT_TRUE(game::get_map_object(*view.view,s.buildingID,snapshot));EXPECT_EQ(snapshot.owner_index,s.ownerIndex);
 }
}

TEST(CampaignPresets, All02ZeroCapacityPassengerPips){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);
 ASSERT_TRUE(game::load_map_view(*view.view,"ALL02UMD.MAP",12))<<game::map_view_error(*view.view);
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* pointer){
  auto& view=*static_cast<game::MapViewHandle*>(pointer);
  auto& world=*view.world->impl;
  struct Sink{SHPStruct* mobile;int health=0,passengers=0;}sink{world.mobile_pips};
  game::TypeDrawingContext drawing;drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(&sink);
  drawing.backend_context=&sink;
  drawing.backend.shape=[](void* p,const game::ShapeDrawingRequest& r){
   auto& sink=*static_cast<Sink*>(p);if(r.image==sink.mobile)++sink.passengers;else ++sink.health;
   return game::DrawingStatus::drawn;
  };
  int pickups=0;
  for(auto* unit:UnitClass::Array){
   if(std::strcmp(unit->Type->ID,"PTRUCK"))continue;
   ++pickups;ASSERT_EQ(unit->Type->PipScale,PipScale::Passengers);ASSERT_EQ(unit->Type->Passengers,0);
   ASSERT_TRUE(unit->Owner->IsAlliedWith(HouseClass::CurrentPlayer));
   for(bool selected:{false,true}){
    unit->IsMouseHovering=true;unit->IsSelected=selected;sink.health=sink.passengers=0;
    game::BuildingHealthDrawing frame{drawing,world.health_pips};
    frame.palette=reinterpret_cast<const game::DrawingPaletteHandle*>(&sink);
    frame.mobile_pips=world.mobile_pips;frame.pip_border=world.pip_border;
    EXPECT_EQ(game::draw_techno_extras(*unit,frame,{200,200},{0,0,640,480}),game::DrawingStatus::drawn)
        <<"Zero-capacity PTRUCK must retain its health bar without a passenger pip error";
    EXPECT_GT(sink.health,0);EXPECT_EQ(sink.passengers,0);
   }
   unit->IsMouseHovering=false;unit->IsSelected=false;
  }
  EXPECT_GT(pickups,0);
 },view.view));
}

TEST(CampaignPresets, All02DrawingFailureContext){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL02UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1024,768));
 struct Sink{SHPStruct* pips=nullptr;UnitClass* truck=nullptr;bool fail=true;}sink;
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){
  auto& sink=*static_cast<Sink*>(p);
  for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"PTRUCK")){sink.truck=unit;unit->IsMouseHovering=true;break;}
 },&sink));
 ASSERT_NE(sink.truck,nullptr);sink.pips=view.view->world->impl->health_pips;
 game::MapDrawingContext drawing;drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(&sink);
 drawing.types.backend_context=&sink;
 drawing.shape_palette=[](void*,const BytePalette& p,int,const game::DrawingPaletteHandle*& out)noexcept{
  out=reinterpret_cast<const game::DrawingPaletteHandle*>(&p);return game::DrawingStatus::drawn;
 };
 drawing.color_scheme_palette=drawing.shape_palette;
 drawing.terrain_palette=[](void*,const BytePalette& p,int,int,int,int,const game::DrawingPaletteHandle*& out)noexcept{
  out=reinterpret_cast<const game::DrawingPaletteHandle*>(&p);return game::DrawingStatus::drawn;
 };
 drawing.types.backend.tile=[](void*,const game::TileDrawingRequest&){return game::DrawingStatus::drawn;};
 drawing.types.backend.indexed=[](void*,const game::IndexedDrawingRequest&){return game::DrawingStatus::drawn;};
 drawing.types.backend.raster=[](void*,const game::RasterDrawingRequest&){return game::DrawingStatus::drawn;};
 drawing.types.backend.shape=[](void* p,const game::ShapeDrawingRequest& r){
  auto& sink=*static_cast<Sink*>(p);
  return sink.fail&&r.image==sink.pips?game::DrawingStatus::unsupported:game::DrawingStatus::drawn;
 };
 game::MapDrawStatistics stats;
 EXPECT_EQ(game::draw_map_view(*view.view,drawing,stats),game::DrawingStatus::unsupported);
 const std::string error=game::map_view_error(*view.view);
 EXPECT_NE(error.find("Techno extras: unsupported"),std::string::npos);
 EXPECT_NE(error.find("object=PTRUCK"),std::string::npos);
 EXPECT_NE(error.find("id="+std::to_string(sink.truck->UniqueID)),std::string::npos);
 sink.fail=false;
 EXPECT_EQ(game::draw_map_view(*view.view,drawing,stats),game::DrawingStatus::drawn);
 EXPECT_STREQ(game::map_view_error(*view.view),"");
 EXPECT_STREQ(game::drawing_failure(),"");
}

TEST(CampaignPresets, EnteredByAndAttachedOwnership){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL07SMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  BuildingClass* yard=nullptr;BuildingClass* reactor=nullptr;
  for(auto* b:BuildingClass::Array){if(!std::strcmp(b->Type->ID,"NACNST"))yard=b;if(!std::strcmp(b->Type->ID,"NAPOWR"))reactor=b;}
  ASSERT_NE(yard,nullptr);ASSERT_NE(reactor,nullptr);ASSERT_NE(yard->AttachedTag,nullptr);
  auto* previous=yard->Owner;auto* player=HouseClass::CurrentPlayer;auto* trigger=yard->AttachedTag->FirstTrigger;
  ASSERT_NE(trigger,nullptr);auto* event=trigger->Type->FirstEvent;auto* action=trigger->Type->FirstAction;
  ASSERT_NE(event,nullptr);ASSERT_NE(action,nullptr);ASSERT_EQ(event->EventKind,TriggerEvent::EnteredBy);
  ASSERT_EQ(action->ActionKind,TriggerAction::ChangeHouse);
  auto* engineer=infantry_near_building("ENGINEER",player,yard);ASSERT_NE(engineer,nullptr);
  bool repeating=false;
  EXPECT_FALSE(event->HasOccured(int(TriggerEvent::EnteredBy),nullptr,yard,nullptr,&repeating)); // wrong house
  EXPECT_FALSE(event->HasOccured(int(TriggerEvent::EnteredBy),nullptr,nullptr,nullptr,&repeating));
  EXPECT_FALSE(event->HasOccured(int(TriggerEvent::DestroyedByAnything),nullptr,engineer,nullptr,&repeating));
  EXPECT_FALSE(repeating);EXPECT_EQ(event->House,nullptr);
  EXPECT_TRUE(trigger->RegisterEvent(TriggerEvent::EnteredBy,engineer,false,false,nullptr));
  EXPECT_EQ(event->House,player);EXPECT_EQ(trigger->GetHouse(),player);
  const auto entered=CellStruct::Empty;
  EXPECT_FALSE(action->Execute(nullptr,engineer,nullptr,entered)); // a source alone is not the recipient
  action->Value=-1;EXPECT_FALSE(action->Execute(nullptr,engineer,trigger,entered));
  action->Value=8997;EXPECT_TRUE(action->Execute(nullptr,engineer,trigger,entered));
  EXPECT_EQ(yard->Owner,player);EXPECT_EQ(reactor->Owner,previous);EXPECT_EQ(engineer->Owner,player);
  EXPECT_TRUE(action->Execute(nullptr,engineer,trigger,entered)); // eligible even when already owned
  const int slot=ScenarioClass::Instance->HouseIndices[0];ScenarioClass::Instance->HouseIndices[0]=previous->ArrayIndex;
  action->Value=HouseClass::PlayerAtA;EXPECT_TRUE(action->Execute(nullptr,engineer,trigger,entered));EXPECT_EQ(yard->Owner,previous);
  ScenarioClass::Instance->HouseIndices[0]=slot;
  action->Value=player->Type->ArrayIndex;
  yard->InLimbo=true;EXPECT_FALSE(action->Execute(nullptr,engineer,trigger,entered));yard->InLimbo=false;
  yard->IsOnMap=false;EXPECT_FALSE(action->Execute(nullptr,engineer,trigger,entered));yard->IsOnMap=true;
  EXPECT_TRUE(action->Execute(nullptr,engineer,trigger,entered));EXPECT_EQ(yard->Owner,player);
 },nullptr));
}

TEST(InfantryGarrison, ClickMoveEnterFireAndUnload){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* context){
  auto& world=*static_cast<game::MapViewHandle*>(context)->world;auto* owner=HouseClass::CurrentPlayer;
  BuildingClass* building=nullptr;
  for(auto* candidate:BuildingClass::Array)if(candidate->Type->CanBeOccupied&&candidate->Type->CanOccupyFire&&candidate->Owner->Type->MultiplayPassive&&!candidate->IsRedHP()){building=candidate;break;}
  ASSERT_NE(building,nullptr);auto* neutral=building->Owner;
  auto* actor=infantry_near_building("E1",owner,building);ASSERT_NE(actor,nullptr);
  ASSERT_EQ(actor->MouseOverObject(building,true),Action::Capture);
  ASSERT_TRUE(actor->ObjectClickedAction(Action::Capture,building,true));
  for(int frame=0;frame<1200&&!building->Occupants.Count;++frame){++Unsorted::CurrentFrame;game::update_map_world(world);}
  ASSERT_EQ(building->Occupants.Count,1);EXPECT_EQ(building->Occupants[0],actor);EXPECT_TRUE(actor->InLimbo);EXPECT_FALSE(actor->IsOnMap);
  ++Unsorted::CurrentFrame;game::update_map_world(world);EXPECT_EQ(building->Owner,owner);
  EXPECT_EQ(building->GetWeapon(0)->WeaponType,actor->Type->OccupyWeapon.WeaponType);
  CoordStruct muzzle;building->GetFLH(&muzzle,0,CoordStruct::Empty);
  const auto origin=building->GetRenderCoords();const auto offset=TacticalClass::Instance->ApplyMatrix_Pixel(building->Type->MuzzleFlash[0]);
  EXPECT_EQ(muzzle,(CoordStruct{origin.X+offset.X,origin.Y+offset.Y,origin.Z}));
  HouseClass* enemy=nullptr;for(auto* house:HouseClass::Array)if(!house->Type->MultiplayPassive&&!owner->IsAlliedWith(house)){enemy=house;break;}
  ASSERT_NE(enemy,nullptr);auto* victim=infantry_near_building("INIT",enemy,building);ASSERT_NE(victim,nullptr);
  victim->ForceMission(Mission::Sleep);const auto victimId=victim->UniqueID;
  // No attack order: original Guard/Techno targeting must acquire the enemy.
  for(int frame=0;frame<400&&actor->Veterancy.Veterancy==0;++frame){++Unsorted::CurrentFrame;game::update_map_world(world);}
  std::cout<<"GARRISON "<<building->Type->ID<<" mission="<<int(building->CurrentMission)<<" queued="<<int(building->QueuedMission)<<" target="<<bool(building->Target)<<" victim_hp="<<victim->Health<<" error="<<int(building->GetFireError(victim,0,true))<<" occupant_xp="<<actor->Veterancy.Veterancy<<" building_xp="<<building->Veterancy.Veterancy<<"\n";
  EXPECT_GT(actor->Veterancy.Veterancy,0);EXPECT_EQ(building->Veterancy.Veterancy,0);
  EXPECT_FALSE(std::any_of(InfantryClass::Array.begin(),InfantryClass::Array.end(),[&](auto* p){return p->UniqueID==victimId&&p->Health>0;}));
  ASSERT_TRUE(building->ClickedMission(Mission::Unload,nullptr,nullptr,nullptr));
  for(int frame=0;frame<120&&building->Occupants.Count;++frame){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_EQ(building->Occupants.Count,0);EXPECT_FALSE(actor->InLimbo);EXPECT_TRUE(actor->IsOnMap);EXPECT_EQ(building->Owner,neutral);
 },view.view));
}

TEST(InfantryGarrison, ForceAttackInputAndHudUnload){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_game_view_size(*view.view,1280,720));
 struct State{game::MapViewHandle* view;BuildingClass* building=nullptr;InfantryClass* occupant=nullptr;InfantryClass* victim=nullptr;Point2D hud{},ground{};CellClass* cell=nullptr;int hp=0;};
 State s{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* b:BuildingClass::Array)if(b->Type->CanOccupyFire&&b->Owner->Type->MultiplayPassive&&!b->IsRedHP()){s.building=b;break;}
  ASSERT_NE(s.building,nullptr);s.occupant=infantry_near_building("E1",HouseClass::CurrentPlayer,s.building);ASSERT_NE(s.occupant,nullptr);
  ASSERT_TRUE(s.occupant->ObjectClickedAction(Action::Capture,s.building,true));
  for(int i=0;i<1200&&!s.building->Occupants.Count;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  ASSERT_EQ(s.building->Occupants.Count,1);++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);
  s.victim=infantry_near_building("INIT",HouseClass::CurrentPlayer,s.building);ASSERT_NE(s.victim,nullptr);s.hp=s.victim->Health;
  s.victim->ForceMission(Mission::Sleep);ASSERT_TRUE(s.building->IsCloseEnough(s.victim,0));EXPECT_FALSE(s.building->IsControllable());
  ASSERT_FALSE(s.building->MouseOverObject(s.victim,true)==Action::Attack); // friendly, without Ctrl
 },&s));
 ASSERT_NE(s.building,nullptr);ASSERT_NE(s.victim,nullptr);
 ASSERT_TRUE(click_world_object(*view.view,s.building));ASSERT_TRUE(s.building->IsSelected);
 ASSERT_TRUE(click_world_object(*view.view,s.victim,2));ASSERT_TRUE(s.building->IsSelected);
 game::GameInputResult result;
 ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,17,0,false},result));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  ASSERT_EQ(EventClass::OutList.Count,1);auto& event=EventClass::OutList.First();EXPECT_EQ(event.Type,EventType::MegaMission);
  EXPECT_EQ(event.MegaMission.Mission,unsigned(Mission::Attack));EXPECT_EQ(event.MegaMission.Whom.As_Techno(),s.building);
  EXPECT_EQ(event.MegaMission.Target.As_Abstract(),s.victim);
  for(int i=0;i<250&&s.victim->Health==s.hp;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  EXPECT_LT(s.victim->Health,s.hp);EXPECT_EQ(s.building->Occupants.Count,1);
  std::cout<<"GARRISON_FORCE object_hp="<<s.hp<<"->"<<s.victim->Health<<"\n";
  game::rebuild_world_sprites(*s.view->world);const auto origin=s.building->GetMapCoords();
  for(int y=-4;y<=4&&!s.cell;++y)for(int x=-4;x<=4&&!s.cell;++x){
   auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(origin.X+x),short(origin.Y+y)});if(!cell||cell->FirstObject||!s.building->IsCloseEnough(cell,0))continue;
   Point2D point;CellStruct picked;
   if(TacticalClass::CoordsToClient(cell->GetCoords(),s.view->tactical.TacticalPos,TacticalClass::ViewBounds,point)
      &&point.X>30&&point.Y>30&&point.X<TacticalClass::ViewBounds.Width-30&&point.Y<TacticalClass::ViewBounds.Height-30
      &&!game::pick_world_object(*s.view->world,point)&&s.view->tactical.PickTerrainCell(point,TacticalClass::ViewBounds,picked)&&picked==cell->MapCoords){s.cell=cell;s.ground=point;}
  }
 },&s));ASSERT_NE(s.cell,nullptr);
 for(bool down:{true,false})ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,s.ground.X,s.ground.Y,1,2,down},result));
 ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,17,0,false},result));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  ASSERT_EQ(EventClass::OutList.Count,1);EXPECT_EQ(EventClass::OutList.First().MegaMission.Target.As_Cell(),s.cell);
  const int before=s.building->LastFireBulletFrame;int shots=0,previous=before;
  for(int i=0;i<150;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);if(s.building->LastFireBulletFrame!=previous){++shots;previous=s.building->LastFireBulletFrame;}}
  EXPECT_EQ(s.building->Target,s.cell);EXPECT_GE(shots,2);EXPECT_TRUE(s.building->IsSelected);
 },&s));
 // Build the real command bar from UIMD/SHP resources, then click its gadget.
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  s.view->ui_resources=std::make_unique<game::UiResources>();ASSERT_TRUE(s.view->ui_resources->load(s.view->scenario.PlayerSideIndex));
  game::MapDrawingContext drawing;game::MapDrawStatistics statistics;
  drawing.plain_palette=[](void*,const BytePalette& palette,const game::DrawingPaletteHandle*& output)noexcept{output=reinterpret_cast<const game::DrawingPaletteHandle*>(&palette);return game::DrawingStatus::drawn;};
  game::GameUiFrame frame{drawing,*s.view->ui_resources,statistics};
  ASSERT_EQ(game::with_game_ui_frame(frame,[]{TabClass::Instance.InitializeCommandButtons();}),game::DrawingStatus::skipped);
  const int slot=TabClass::CommandPositions[4];ASSERT_GE(slot,0);ASSERT_LT(slot,25);
  auto& button=TabClass::CommandButtons[slot];EXPECT_FALSE(button.Disabled);EXPECT_EQ(button.ID,218);
  s.hud={button.X+button.Width/2,button.Y+button.Height/2};
 },&s));
 for(bool down:{true,false})ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,s.hud.X,s.hud.Y,1,0,down},result));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  ASSERT_EQ(EventClass::OutList.Count,1);EXPECT_EQ(EventClass::OutList.First().Type,EventType::Deploy);
  EXPECT_EQ(EventClass::OutList.First().Deploy.Whom.As_Techno(),s.building);
  for(int i=0;i<120&&s.building->Occupants.Count;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  EXPECT_EQ(s.building->Occupants.Count,0);EXPECT_FALSE(s.occupant->InLimbo);EXPECT_TRUE(s.occupant->IsOnMap);
  std::cout<<"GARRISON_HUD deploy_button="<<s.hud.X<<","<<s.hud.Y<<" unloaded="<<!s.occupant->InLimbo<<"\n";
 },&s));
}

TEST(InfantryGarrison, ForceAttackOccupiedBuildingUpdatesFlash){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapViewHandle* view;BuildingClass* building=nullptr;InfantryClass* occupant=nullptr;InfantryClass* attacker=nullptr;int hp=0;}s{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* b:BuildingClass::Array)if(b->Type->CanOccupyFire&&b->Owner->Type->MultiplayPassive&&!b->IsRedHP()){s.building=b;break;}
  ASSERT_NE(s.building,nullptr);s.occupant=infantry_near_building("E1",HouseClass::CurrentPlayer,s.building);ASSERT_NE(s.occupant,nullptr);
  ASSERT_TRUE(s.occupant->ObjectClickedAction(Action::Capture,s.building,true));
  for(int i=0;i<1200&&!s.building->Occupants.Count;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  ASSERT_EQ(s.building->Occupants.Count,1);++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);
  s.attacker=infantry_near_building("E1",HouseClass::CurrentPlayer,s.building);ASSERT_NE(s.attacker,nullptr);s.hp=s.building->Health;
 },&s));
 ASSERT_NE(s.attacker,nullptr);ASSERT_TRUE(click_world_object(*view.view,s.attacker));
 ASSERT_TRUE(click_world_object(*view.view,s.building,2));
 game::GameInputResult result;
 ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,17,0,false},result));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& world=*s.view->world;
  ASSERT_EQ(EventClass::OutList.Count,1);EXPECT_EQ(EventClass::OutList.First().MegaMission.Target.As_Abstract(),s.building);
  RectangleStruct bounds;EXPECT_EQ(s.building->GetRenderDimensions(&bounds),&bounds);
  ASSERT_GT(bounds.Width,0);ASSERT_GT(bounds.Height,0);
  auto* tactical=TacticalClass::Instance;const auto camera=tactical->TacticalPos;
  tactical->TacticalPos.X+=13;tactical->TacticalPos.Y-=7;
  RectangleStruct moved;s.building->GetRenderDimensions(&moved);
  EXPECT_EQ(moved.X,bounds.X-13);EXPECT_EQ(moved.Y,bounds.Y+7);
  EXPECT_EQ(moved.Width,bounds.Width);EXPECT_EQ(moved.Height,bounds.Height);
  tactical->TacticalPos=camera;
  s.building->GetRenderDimensions(&moved);EXPECT_EQ(moved.X,bounds.X);EXPECT_EQ(moved.Y,bounds.Y);
  int transitions=0;bool acknowledged=false;
  for(int i=0;i<30;++i){
   const int before=s.building->Flashing.DurationRemaining;const auto revision=world.presentation_revision;
   ++Unsorted::CurrentFrame;game::update_map_world(world);
   const int after=s.building->Flashing.DurationRemaining;acknowledged|=after>0;
   if(before&&before!=after&&(before&2)!=(after&2)){
    ++transitions;EXPECT_GT(world.presentation_revision,revision);
    EXPECT_EQ(s.building->unknown_short_700,s.building->GetFlashingIntensity(static_cast<short>(s.building->GetCell()->Intensity_Normal)));
   }
  }
  EXPECT_TRUE(acknowledged);EXPECT_GE(transitions,2);EXPECT_EQ(s.building->Flashing.DurationRemaining,0);
  EXPECT_LT(s.building->Health,s.hp);EXPECT_EQ(s.building->Occupants.Count,1);EXPECT_EQ(s.building->Occupants[0],s.occupant);
  EXPECT_TRUE(s.occupant->InLimbo);EXPECT_EQ(s.attacker->Target,s.building);
  std::cout<<"GARRISON_TARGET_FLASH transitions="<<transitions<<" hp="<<s.hp<<"->"<<s.building->Health<<"\n";
 },&s));
}

TEST(Harvesting, FirstMapChronoMinersCollectAndCreditTwoLoads) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& world=*static_cast<game::MapWorld*>(p);
  UnitClass* miner=nullptr;BuildingClass* refinery=nullptr;int count=0;
  for(auto* building:BuildingClass::Array)if(building->Owner==HouseClass::CurrentPlayer&&!std::strcmp(building->Type->ID,"GAREFN"))refinery=building;
  for(auto* unit:UnitClass::Array)if(unit->Owner==HouseClass::CurrentPlayer&&!std::strcmp(unit->Type->ID,"CMIN")){miner=unit;++count;}
  ASSERT_NE(refinery,nullptr);ASSERT_NE(miner,nullptr);ASSERT_EQ(count,2);
  ASSERT_NE(refinery->Type->RefinerySmokeParticleSystem,nullptr);
  EXPECT_STREQ(refinery->Type->RefinerySmokeParticleSystem->ID,"SmallGreySSys");
  EXPECT_EQ(refinery->Type->RefinerySmokeFrames,50);
  // Both miners come from [Units]; opening preplaced refineries during map
  // initialization must not duplicate their FreeUnit. Isolate one income stream.
  for(auto* other:UnitClass::Array)if(other!=miner&&other->Type->Harvester)other->ForceMission(Mission::Sleep);
  EXPECT_EQ(refinery->Type->FreeUnit,miner->Type);EXPECT_EQ(miner->Type->Storage,20);
  EXPECT_EQ(miner->Type->Dock.Count,2);EXPECT_TRUE(miner->Type->Teleporter);
  const int units=UnitClass::Array.Count;refinery->Place(false);EXPECT_EQ(UnitClass::Array.Count,units);
  const int balance=miner->Owner->Available_Money();EXPECT_EQ(balance,10000);
  world.impl->view.scenario.TiberiumGrowthEnabled=false;
  bool collected=false,teleported=false;int deliveries=0;int lastMoney=balance,expectedIncome=0;
  for(int frame=0;frame<6000&&deliveries<2;++frame){
   const auto before=miner->Location;const float carried=miner->Tiberium.GetTotalAmount();
   auto* oreCell=miner->GetCell();const int oreBefore=oreCell->LandType==LandType::Tiberium?int(oreCell->OverlayData)+1:0;
   int storedValue=miner->Tiberium.GetTotalValue();
   ++Unsorted::CurrentFrame;game::update_map_world(world);
   collected|=miner->Tiberium.GetTotalAmount()>0;
   const auto delta=miner->Location-before;const bool jumped=std::abs(delta.X)>512||std::abs(delta.Y)>512;teleported|=jumped;
   if(jumped){
    int sourceEffects=0,destinationEffects=0;
    for(auto* effect:AnimClass::Array)if(effect->Type==RulesClass::Instance->WarpOut){
     sourceEffects+=effect->Location==before;destinationEffects+=effect->Location==miner->Location;
    }
    EXPECT_EQ(sourceEffects,1);EXPECT_EQ(destinationEffects,1);
    ASSERT_NE(RulesClass::Instance->WarpOut,RulesClass::Instance->WarpIn);
   }
   const int money=miner->Owner->Available_Money();
   if(money>lastMoney){++deliveries;expectedIncome+=storedValue;EXPECT_GT(carried,0);EXPECT_FLOAT_EQ(miner->Tiberium.GetTotalAmount(),0);
    EXPECT_EQ(money-lastMoney,storedValue);lastMoney=money;
    auto* dock=static_cast<BuildingClass*>(miner->GetNthLink());ASSERT_NE(dock,nullptr);
    int chimneys=0;
    for(auto* system:ParticleSystemClass::Array)if(system->Owner==dock&&system->Type==dock->Type->RefinerySmokeParticleSystem){
     EXPECT_TRUE(system->Location==dock->Location+dock->Type->RefinerySmokeOffsetOne||system->Location==dock->Location+dock->Type->RefinerySmokeOffsetTwo);
     EXPECT_GE(system->Lifetime,49);EXPECT_LE(system->Lifetime,50);++chimneys;
    }
    EXPECT_EQ(chimneys,2);
    std::cout<<"HARVEST_DELIVERY count="<<deliveries<<" frame="<<frame<<" amount="<<carried<<" money="<<money<<std::endl;
   }else if(miner->Tiberium.GetTotalAmount()>carried){
    EXPECT_EQ(money,lastMoney);
    const int oreAfter=oreCell->LandType==LandType::Tiberium?int(oreCell->OverlayData)+1:0;
    EXPECT_EQ(oreAfter,oreBefore-1);
   }
   if(frame%250==0)std::cout<<"HARVEST_TRACE frame="<<frame<<" mission="<<int(miner->CurrentMission)<<" status="<<miner->MissionStatus<<" pos="<<miner->Location.X<<","<<miner->Location.Y<<" cargo="<<miner->Tiberium.GetTotalAmount()<<" dest="<<bool(miner->Destination)<<" link="<<bool(miner->GetNthLink())<<std::endl;
  }
  EXPECT_TRUE(collected);EXPECT_TRUE(teleported);EXPECT_EQ(deliveries,2);
  EXPECT_EQ(miner->Owner->Available_Money(),balance+expectedIncome);
  // Destruction at the dock reaches Teleport.Limbo after Drive is unwrapped.
  // It must release the contact and remove the miner without entering a stub.
  auto* owner=miner->Owner;auto* dock=miner->GetNthLink();ASSERT_NE(dock,nullptr);
  int damage=miner->Health+1;
  EXPECT_EQ(miner->ReceiveDamage(&damage,0,WarheadTypeClass::Find("SA"),nullptr,true,true,nullptr),DamageState::NowDead);
  for(int i=0;i<60;++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_LT(UnitClass::Array.FindItemIndex(miner),0);EXPECT_FALSE(dock->ContainsLink(miner));
  EXPECT_EQ(owner->Available_Money(),balance+expectedIncome);
 },view.view->world.get()));
}

TEST(Harvesting, OrdinaryMinerDrivesHomeAndCreditsFullCapacity) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& world=*static_cast<game::MapWorld*>(p);
  for(auto* unit:UnitClass::Array)if(unit->Type->Harvester)unit->ForceMission(Mission::Sleep);
  BuildingClass* refinery=nullptr;for(auto* b:HouseClass::CurrentPlayer->Buildings)if(b->Type->Refinery)refinery=b;
  ASSERT_NE(refinery,nullptr);auto* type=UnitTypeClass::Find("HARV");ASSERT_NE(type,nullptr);
  EXPECT_EQ(type->Storage,40);EXPECT_FALSE(type->Teleporter);
  // A map-local Dock override lets the Soviet drive miner use an existing
  // ALL01 allied refinery; movement, harvesting and timing remain actual rules.
  type->Dock.AddItem(refinery->Type);
  auto* miner=new UnitClass(type,refinery->Owner);ASSERT_TRUE(miner->InitializeLocomotor());
  const auto anchor=refinery->GetMapCoords();bool placed=false;
  for(int radius=4;radius<12&&!placed;++radius)for(int y=-radius;y<=radius&&!placed;++y)for(int x=-radius;x<=radius&&!placed;++x){
   if(std::abs(x)!=radius&&std::abs(y)!=radius)continue;
   auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(anchor.X+x),short(anchor.Y+y)});
   if(cell&&MapClass::Instance.IsWithinUsableArea(cell->MapCoords,true)&&miner->IsCellOccupied(cell,FacingType(-1),-1,nullptr,true)==Move::OK)
    placed=miner->Unlimbo(cell->GetCoords(),DirType::North);
  }
  ASSERT_TRUE(placed);miner->ForceMission(Mission::Harvest);LogicClass::Instance.AddObject(miner,false);
  world.impl->view.scenario.TiberiumGrowthEnabled=false;
  const int cash=miner->Owner->Available_Money();bool full=false,jump=false;int frame=0;
  for(;frame<6000&&miner->Owner->Available_Money()==cash;++frame){
   const auto before=miner->Location;++Unsorted::CurrentFrame;game::update_map_world(world);
   const auto delta=miner->Location-before;jump|=std::abs(delta.X)>512||std::abs(delta.Y)>512;
   full|=miner->Tiberium.GetTotalAmount()==40;
  }
  EXPECT_TRUE(full);EXPECT_FALSE(jump);EXPECT_EQ(miner->Owner->Available_Money(),cash+1000);
  EXPECT_FLOAT_EQ(miner->Tiberium.GetTotalAmount(),0);
  std::cout<<"HARVEST_DRIVE frame="<<frame<<" cash="<<miner->Owner->Available_Money()<<" mission="<<int(miner->CurrentMission)<<" status="<<miner->MissionStatus<<" cargo="<<miner->Tiberium.GetTotalAmount()<<std::endl;
 },view.view->world.get()));
}

TEST(Harvesting, BothFirstMapMinersKeepTeleportingAcrossSixLoads) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& world=*static_cast<game::MapWorld*>(p);
  std::vector<UnitClass*> miners;for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"CMIN"))miners.push_back(unit);
  ASSERT_EQ(miners.size(),2u);std::array<int,2> deliveries{},jumps{};
  std::array<bool,2> jumped{};bool occupiedApproach=false;
  for(int frame=0;frame<16000&&(deliveries[0]<6||deliveries[1]<6);++frame){
   std::array<CoordStruct,2> before{miners[0]->Location,miners[1]->Location};
   std::array<float,2> cargo{miners[0]->Tiberium.GetTotalAmount(),miners[1]->Tiberium.GetTotalAmount()};
   for(auto* miner:miners)if(miner->CurrentMission==Mission::Enter&&miner->Destination&&miner->Destination->WhatAmI()==AbstractType::Cell){
    auto* occupant=static_cast<CellClass*>(miner->Destination)->FindObjectOfType(AbstractType::Unit,false);
    occupiedApproach|=occupant&&occupant!=miner&&miner->Locomotor->Is_Moving();
   }
   ++Unsorted::CurrentFrame;game::update_map_world(world);
   for(int i=0;i<2;++i){auto* miner=miners[i];auto delta=miner->Location-before[i];
    if(std::abs(delta.X)>512||std::abs(delta.Y)>512){
     ++jumps[i];jumped[i]=true;int sourceEffects=0,destinationEffects=0;
     for(auto* effect:AnimClass::Array)if(effect->Type==RulesClass::Instance->WarpOut){
      sourceEffects+=effect->Location==before[i];destinationEffects+=effect->Location==miner->Location;
     }
     EXPECT_EQ(sourceEffects,1);EXPECT_EQ(destinationEffects,1);
    }
    if(cargo[i]>0&&miner->Tiberium.GetTotalAmount()==0){
     EXPECT_TRUE(jumped[i])<<"Miner "<<i<<" drove home on load "<<deliveries[i]+1;
     ++deliveries[i];jumped[i]=false;
     std::cout<<"DUAL_MINER id="<<i<<" frame="<<frame<<" loads="<<deliveries[i]<<" teleports="<<jumps[i]<<" pos="<<miner->Location.X<<","<<miner->Location.Y<<std::endl;
    }
   }
  }
  EXPECT_TRUE(occupiedApproach); // Real first-map dock contention, not a forced teleport.
  for(int i=0;i<2;++i){EXPECT_GE(deliveries[i],6);EXPECT_GE(jumps[i],deliveries[i]);}
 },view.view->world.get()));
}

TEST(Harvesting, OriginalExecutableDepletionReturn) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  UnitClass* miner=nullptr;for(auto* unit:UnitClass::Array)if(unit->Type->Harvester)miner=unit;
  ASSERT_NE(miner,nullptr);CellStruct ore;miner->ScanForTiberium(&ore,32,0);ASSERT_NE(ore,CellStruct::Empty);
  auto* cell=MapClass::Instance.GetCellAt(ore);const int overlay=cell->OverlayTypeIndex;
  cell->OverlayTypeIndex=-1;ASSERT_TRUE(cell->RefreshTerrainGeometry());const auto backgroundLand=cell->LandType;
  std::ifstream input(RA2_HARVEST_DEPLETION_FIXTURE);std::string magic;input>>magic;ASSERT_EQ(magic,"HARVEST_DEPLETION_V1");
  int initialOverlay,initialFrame,requested,credited,expectedOverlay,expectedFrame,land,cases=0;
  while(input>>initialOverlay>>initialFrame>>requested>>credited>>expectedOverlay>>expectedFrame>>land){
   SCOPED_TRACE(::testing::Message()<<"overlay="<<initialOverlay<<" frame="<<initialFrame<<" request="<<requested);
   cell->OverlayTypeIndex=initialOverlay<0?-1:overlay;cell->OverlayData=BYTE(initialFrame);
   cell->LandType=initialOverlay<0?backgroundLand:LandType::Tiberium;
   EXPECT_EQ(cell->ReduceTiberium(requested),credited);
   EXPECT_EQ(cell->OverlayTypeIndex,expectedOverlay<0?-1:overlay);EXPECT_EQ(cell->OverlayData,expectedFrame);
   EXPECT_EQ(cell->LandType,land==7?backgroundLand:LandType::Tiberium);++cases;
  }
  EXPECT_TRUE(input.eof());EXPECT_EQ(cases,48);
 },nullptr));
}

TEST(Harvesting, OriginalExecutablePerBailTiming) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  UnitClass* miner=nullptr;for(auto* unit:UnitClass::Array)if(unit->Type->Harvester)miner=unit;
  ASSERT_NE(miner,nullptr);CellStruct ore;miner->ScanForTiberium(&ore,32,0);ASSERT_NE(ore,CellStruct::Empty);
  auto* cell=MapClass::Instance.GetCellAt(ore);const auto overlay=cell->OverlayTypeIndex;
  miner->Mark(MarkType::Up);miner->SetLocation(cell->GetCoords());miner->Mark(MarkType::Down);
  cell->OverlayTypeIndex=-1;ASSERT_TRUE(cell->RefreshTerrainGeometry());const auto backgroundLand=cell->LandType;
  // The EXE fixture isolates a finite cell with no next ore target. Keep the
  // native scan real, but give it the same empty search range.
  RulesClass::Instance->TiberiumShortScan=0;
  std::ifstream input(RA2_HARVEST_TIMING_FIXTURE);std::string magic;input>>magic;ASSERT_EQ(magic,"HARVEST_TIMING_V2");
  int rate,initialStage,initialRate,frame,amount,stage,animationRate,status,harvesting,expectedOverlay,remaining,land;
  int frames=0,samples=0;
  while(input>>rate>>initialStage>>initialRate>>frame>>amount>>stage>>animationRate>>status>>harvesting>>expectedOverlay>>remaining>>land){
   SCOPED_TRACE(::testing::Message()<<"sample="<<samples<<" frame="<<frame);
   Unsorted::CurrentFrame=frame;
   if(frame==0){
    ++samples;cell->OverlayTypeIndex=overlay;cell->OverlayData=11;cell->LandType=LandType::Tiberium;
    miner->Tiberium={};miner->SetArchiveTarget(nullptr);miner->SetDestination(nullptr,true);
    miner->ForceMission(Mission::Harvest);miner->MissionStatus=1;miner->IsHarvesting=true;miner->UpdateTimer.Start(0);
    miner->Animation.Value=initialStage;miner->Animation.Start(initialRate);miner->Animation.Step=1;
    RulesClass::Instance->HarvesterLoadRate=rate;
   }
   miner->Update();
   EXPECT_FLOAT_EQ(miner->Tiberium.GetTotalAmount(),amount);
   EXPECT_EQ(miner->Animation.Value,stage);EXPECT_EQ(miner->Animation.Rate,animationRate);
   EXPECT_EQ(miner->MissionStatus,status);EXPECT_EQ(miner->IsHarvesting,bool(harvesting));
   EXPECT_EQ(cell->OverlayTypeIndex,expectedOverlay<0?-1:overlay);EXPECT_EQ(cell->OverlayData,remaining);
   EXPECT_EQ(cell->LandType,land==7?backgroundLand:LandType::Tiberium);
   ++frames;
  }
  EXPECT_TRUE(input.eof());EXPECT_EQ(samples,9);EXPECT_EQ(frames,2061);
 },nullptr));
}

TEST(Harvesting, OriginalExecutableOreIncomeReference) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  std::ifstream input(RA2_HARVEST_FIXTURE);std::string magic;input>>magic;ASSERT_EQ(magic,"HARVEST_INCOME_V1");
  auto* house=HouseClass::CurrentPlayer;auto* ore=TiberiumClass::Array[0];
  int price,cash,score,afterCash,afterScore,cases=0;float income,amount;
  while(input>>price>>income>>amount>>cash>>score>>afterCash>>afterScore){
   SCOPED_TRACE(cases);ore->Value=price;house->Type->IncomeMult=income;house->Balance=cash;house->PointTotal=score;
   house->GiveTiberium(amount,0);EXPECT_EQ(house->Balance,afterCash);EXPECT_EQ(house->PointTotal,afterScore);++cases;
  }
  EXPECT_EQ(cases,120);
 },nullptr));
}

TEST(Harvesting, RefineryOpeningGrantsOneMinerAndCaptureDoesNot) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  BuildingClass* refinery=nullptr;for(auto* b:HouseClass::CurrentPlayer->Buildings)if(b->Type->Refinery)refinery=b;
  ASSERT_NE(refinery,nullptr);ASSERT_NE(refinery->Type->FreeUnit,nullptr);
  const int before=UnitClass::Array.Count,cash=refinery->Owner->Available_Money();
  // Exercise the construction-completion entry on a placed foundation.
  refinery->ActuallyPlacedOnMap=false;refinery->Value=0;refinery->Place(false);
  ASSERT_EQ(UnitClass::Array.Count,before+1);auto* miner=UnitClass::Array[before];
  EXPECT_EQ(miner->Type,refinery->Type->FreeUnit);EXPECT_EQ(miner->Owner,refinery->Owner);
  EXPECT_TRUE(miner->IsOnMap);EXPECT_FALSE(miner->InLimbo);EXPECT_EQ(miner->CurrentMission,Mission::Harvest);
  EXPECT_EQ(refinery->Owner->Available_Money(),cash);
  refinery->Place(false);refinery->Place(true);EXPECT_EQ(UnitClass::Array.Count,before+1);
 },nullptr));
}

TEST(Harvesting, ManualReturnAndHarvestOrdersUseNormalInput) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 for(bool moving:{false,true}){
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapWorld* world;bool moving;UnitClass* miner=nullptr;BuildingClass* refinery=nullptr;int cash=0;}s{view.view->world.get(),moving};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* unit:UnitClass::Array)if(unit->Type->Harvester){unit->ForceMission(Mission::Sleep);if(!s.miner)s.miner=unit;}
  ASSERT_NE(s.miner,nullptr);
  for(auto* b:s.miner->Owner->Buildings)if(b->Type->Refinery&&!s.refinery)s.refinery=b;
  ASSERT_NE(s.refinery,nullptr);s.cash=s.miner->Owner->Available_Money();
  if(s.moving){
   s.miner->ForceMission(Mission::Harvest);bool drivingAway=false;
   for(int i=0;i<500&&!drivingAway;++i){
    ++Unsorted::CurrentFrame;game::update_map_world(*s.world);
    const auto delta=s.miner->Location-s.refinery->Location;
    drivingAway=s.miner->Locomotor->Is_Moving_Now()&&(std::abs(delta.X)>1536||std::abs(delta.Y)>1536);
   }
   ASSERT_TRUE(drivingAway);
  }
  s.miner->Tiberium.AddAmount(3.0f,0); // A partial load should also be returnable.
  EXPECT_EQ(s.miner->MouseOverObject(s.refinery,false),Action::Enter);
 },&s));
 ASSERT_TRUE(click_world_object(*view.view,s.miner));ASSERT_TRUE(click_world_object(*view.view,s.refinery));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  bool jumped=false;
  for(int i=0;i<1000&&s.miner->Owner->Available_Money()==s.cash;++i){
   const auto before=s.miner->Location;++Unsorted::CurrentFrame;game::update_map_world(*s.world);
   const auto delta=s.miner->Location-before;jumped|=std::abs(delta.X)>512||std::abs(delta.Y)>512;
  }
  if(s.moving)EXPECT_TRUE(jumped);
  EXPECT_EQ(s.miner->Owner->Available_Money(),s.cash+75);EXPECT_FLOAT_EQ(s.miner->Tiberium.GetTotalAmount(),0);
  CellStruct ore;s.miner->ScanForTiberium(&ore,32,0);ASSERT_NE(ore,CellStruct::Empty);
  EXPECT_EQ(s.miner->MouseOverCell(&ore,false,false),Action::Harvest);
  auto follow=ore;ASSERT_TRUE(s.miner->CellClickedAction(Action::Harvest,&ore,&follow,false));
  EXPECT_EQ(EventClass::OutList.First().MegaMission.Mission,unsigned(Mission::Harvest));
 },&s));
 }
}

TEST(InfantryProgression, ActualKillsAwardVeterancyAndEliteWeapon) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  InfantryClass* actor=nullptr;std::vector<InfantryClass*> enemies;
  for(auto* infantry:InfantryClass::Array){
   if(!std::strcmp(infantry->Type->ID,"E1")&&infantry->Owner->IsControlledByCurrentPlayer()&&!actor)actor=infantry;
   if(!std::strcmp(infantry->Type->ID,"INIT"))enemies.push_back(infantry);
  }
  ASSERT_NE(actor,nullptr);ASSERT_GT(enemies.size(),6u);
  auto& rules=*RulesClass::Instance;ASSERT_GE(rules.VeteranCap,2.0);ASSERT_GT(rules.VeteranRatio,0.0);
  ASSERT_GT(actor->Type->GetActualCost(actor->Owner),0);EXPECT_TRUE(actor->Type->VeteranAbilities.FIREPOWER);
  EXPECT_TRUE(actor->Type->EliteAbilities.SELF_HEAL);auto* rookieWeapon=actor->GetWeapon(0)->WeaponType;
  // The kill changes Veterancy, but promotion notifications run on the next
  // Update. Establish the rookie rank first so the Invalid-rank branch cannot
  // suppress the notification and hide a native audio-entry crash.
  ++Unsorted::CurrentFrame;actor->Update();ASSERT_EQ(actor->CurrentRanking,Rank::Rookie);
  float expected=actor->Veterancy.Veterancy;int veteranKill=0,kills=0;
  for(auto* victim:enemies){
   ASSERT_FALSE(actor->Owner->IsAlliedWith(victim));
   const int value=victim->Type->GetActualCost(victim->Owner);
   const double gain=double(value)/(actor->Type->GetActualCost(victim->Owner)*rules.VeteranRatio);
   expected=static_cast<float>(std::min(double(expected)+gain,rules.VeteranCap));
   int damage=victim->Health;victim->ReceiveDamage(&damage,0,rookieWeapon->Warhead,actor,true,false,nullptr);++kills;
   EXPECT_FLOAT_EQ(actor->Veterancy.Veterancy,expected);EXPECT_EQ(victim->Health,0);
   std::cout<<"GI_PROMOTION kills="<<kills<<" experience="<<expected<<std::endl;
   ++Unsorted::CurrentFrame;actor->Update();
   EXPECT_EQ(actor->CurrentRanking,actor->Veterancy.GetRemainingLevel());
   if(actor->Veterancy.IsVeteran()&&!veteranKill)veteranKill=kills;
   if(actor->Veterancy.IsElite())break;
  }
  EXPECT_EQ(veteranKill,4);EXPECT_EQ(kills,7);
  ASSERT_TRUE(actor->Veterancy.IsElite());EXPECT_NE(actor->GetWeapon(0)->WeaponType,rookieWeapon);
  EXPECT_EQ(actor->GetWeapon(0)->WeaponType,actor->Type->EliteWeapon[0].WeaponType);
  // Audio availability must not suppress elite flashing or repeatedly restart
  // it on later updates. TechnoClass::Update consumes its first flash tick.
  ASSERT_GT(rules.EliteFlashTimer,1);
  EXPECT_EQ(actor->Flashing.DurationRemaining,rules.EliteFlashTimer-1);
  for(int frame=0;frame<rules.EliteFlashTimer+30;++frame){++Unsorted::CurrentFrame;actor->Update();}
  EXPECT_EQ(actor->Flashing.DurationRemaining,0);EXPECT_EQ(actor->CurrentRanking,Rank::Elite);
  EXPECT_TRUE(actor->IsAlive);EXPECT_GT(actor->Health,0);
  EXPECT_GE(actor->Owner->KilledUnitsOfHouses[enemies[0]->Owner->ArrayIndex],kills);
  // Friendly destruction contributes no experience, and post-mortem damage cannot award it twice.
  InfantryClass fresh(actor->Type,actor->Owner);fresh.Location=actor->Location;
  InfantryClass* friendly=nullptr;for(auto* p:InfantryClass::Array)if(p!=actor&&p!=&fresh&&p->Owner==actor->Owner&&p->Health>0){friendly=p;break;}
  ASSERT_NE(friendly,nullptr);const float before=fresh.Veterancy.Veterancy;
  int damage=friendly->Health;friendly->ReceiveDamage(&damage,0,rookieWeapon->Warhead,&fresh,true,false,nullptr);
  EXPECT_FLOAT_EQ(fresh.Veterancy.Veterancy,before);
  damage=100;enemies[0]->ReceiveDamage(&damage,0,rookieWeapon->Warhead,&fresh,true,false,nullptr);
  EXPECT_FLOAT_EQ(fresh.Veterancy.Veterancy,before);
 },nullptr));
}

TEST(InfantryDeath, MirageFlamingVictimRunsThenExpires) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  auto* mirage=UnitTypeClass::Find("MGTK");ASSERT_NE(mirage,nullptr);
  auto* weapon=mirage->Weapon[0].WeaponType;ASSERT_NE(weapon,nullptr);
  ASSERT_EQ(int(weapon->Warhead->InfDeath),4);
  auto* burning=RulesClass::Instance->FlamingInfantry;ASSERT_NE(burning,nullptr);
  EXPECT_STREQ(burning->ID,"FLAMEGUY");ASSERT_TRUE(burning->IsFlamingGuy);ASSERT_EQ(burning->RunningFrames,6);
  auto* image=burning->GetImage();ASSERT_NE(image,nullptr);image=image->GetData();ASSERT_NE(image,nullptr);
  const int last=image->Frames/2-1;ASSERT_GT(last,49);
  BuildingClass* factory=nullptr;for(auto* b:BuildingClass::Array)if(!std::strcmp(b->Type->ID,"GAWEAP")){factory=b;break;}
  ASSERT_NE(factory,nullptr);
  auto* victim=infantry_near_building("E1",factory->Owner,factory);ASSERT_NE(victim,nullptr);
  const auto origin=victim->Location;
  int damage=victim->Health;ASSERT_EQ(victim->ReceiveDamage(&damage,0,weapon->Warhead,nullptr,true,false,nullptr),DamageState::NowDead);
  AnimClass* flame=nullptr;for(auto* a:AnimClass::Array)if(a->Type==burning&&a->Location==origin){flame=a;break;}
  ASSERT_NE(flame,nullptr);EXPECT_EQ(flame->FlamingGuyCoords,CoordStruct::Empty);
  int running=0,dying=0,moved=0,previousStage=-1;bool expired=false;
  for(int i=0;i<400;++i){
   ++Unsorted::CurrentFrame;const auto before=flame->Location;flame->Update();
   if(flame->TimeToDie){expired=true;break;}
   if(flame->Location!=before)++moved;
   if(!flame->FlamingGuyExpire){
    ++running;EXPECT_EQ(flame->Animation.Rate,0);
    EXPECT_EQ(flame->Animation.Value%6,Unsorted::CurrentFrame/3%6);
    EXPECT_LT(flame->Animation.Value,48);
   }else{
    ++dying;EXPECT_EQ(flame->Animation.Rate,1);EXPECT_GE(flame->Animation.Value,49);EXPECT_LE(flame->Animation.Value,last);
    if(previousStage>=0)EXPECT_EQ(flame->Animation.Value,previousStage+1);
    previousStage=flame->Animation.Value;
   }
  }
  EXPECT_TRUE(expired);EXPECT_GT(running,10);EXPECT_GT(moved,10);EXPECT_GT(dying,5);
  // Exercise the normal Logic owner that deletes the finished animation.
  const auto id=flame->UniqueID;LogicClass::Instance.Update();
  for(auto* a:AnimClass::Array)EXPECT_NE(a->UniqueID,id);
 },nullptr));
}

TEST(BuildingCombat, PillboxForceAttackThroughPointerInput) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapViewHandle* view;BuildingClass* pillbox=nullptr;CellClass* ground=nullptr;InfantryClass* ally=nullptr;Point2D point{};}s{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* building:BuildingClass::Array)if(!std::strcmp(building->Type->ID,"GAPILL")&&building->Owner==HouseClass::CurrentPlayer
      &&building->GetMapCoords()==(CellStruct{89,115})){s.pillbox=building;break;}
  ASSERT_NE(s.pillbox,nullptr);ASSERT_TRUE(s.pillbox->IsArmed());EXPECT_FALSE(s.pillbox->CanOccupyFire());
 },&s));ASSERT_NE(s.pillbox,nullptr);
 ASSERT_TRUE(click_world_object(*view.view,s.pillbox));ASSERT_TRUE(s.pillbox->IsSelected);
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& map=MapClass::Instance;
  game::rebuild_world_sprites(*s.view->world);const auto origin=s.pillbox->GetMapCoords();
  for(int radius=2;radius<=4&&!s.ground;++radius)for(int y=-radius;y<=radius&&!s.ground;++y)for(int x=-radius;x<=radius&&!s.ground;++x){
   const CellStruct cell{short(origin.X+x),short(origin.Y+y)};auto* ground=map.TryGetCellAt(cell);Point2D point;CellStruct picked;
   if(!ground||ground->FirstObject||!map.IsWithinUsableArea(cell,true)||map.IsLocationShrouded(ground->GetCoords())
      ||!s.pillbox->IsCloseEnough(ground,0)||!s.pillbox->IsCloseEnough3D({int(cell.X)*256+128,int(cell.Y)*256+128,0},0))continue;
   if(TacticalClass::CoordsToClient(ground->GetCoords(),s.view->tactical.TacticalPos,TacticalClass::ViewBounds,point)
      &&point.X>20&&point.X<TacticalClass::ViewBounds.Width-20&&point.Y>20&&point.Y<TacticalClass::ViewBounds.Height-20
      &&!game::pick_world_object(*s.view->world,point)
      &&s.view->tactical.PickTerrainCell(point,TacticalClass::ViewBounds,picked)&&picked==cell){s.ground=ground;s.point=point;}
  }
 },&s));ASSERT_NE(s.ground,nullptr);
 game::GameInputResult result;
 for(bool down:{true,false})ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,s.point.X,s.point.Y,1,2,down},result));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  ASSERT_EQ(EventClass::OutList.Count,1);auto& event=EventClass::OutList.First();
  EXPECT_EQ(event.MegaMission.Whom.As_Techno(),s.pillbox);EXPECT_EQ(event.MegaMission.Mission,unsigned(Mission::Attack));
  EXPECT_EQ(event.MegaMission.Target.As_Cell(),s.ground);
 },&s));
 ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,17,0,false},result));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  const auto origin=s.pillbox->Location;int last=s.pillbox->LastFireBulletFrame,shots=0;
  for(int frame=0;frame<150;++frame){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);
   if(last!=s.pillbox->LastFireBulletFrame){last=s.pillbox->LastFireBulletFrame;++shots;}
  }
  EXPECT_GE(shots,2);EXPECT_EQ(s.pillbox->Target,s.ground);EXPECT_EQ(s.pillbox->Location,origin);EXPECT_TRUE(s.pillbox->IsSelected);
  s.ally=infantry_near_building("E1",s.pillbox->Owner,s.pillbox);ASSERT_NE(s.ally,nullptr);s.ally->SetHeight(0);s.ally->ForceMission(Mission::Sleep);
 },&s));ASSERT_NE(s.ally,nullptr);
 ASSERT_TRUE(click_world_object(*view.view,s.ally,2));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  ASSERT_EQ(EventClass::OutList.Count,1);EXPECT_EQ(EventClass::OutList.First().MegaMission.Target.As_Abstract(),s.ally);
 },&s));
 ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,17,0,false},result));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);const int health=s.ally->Health;
  for(int frame=0;frame<180&&s.ally->Health==health;++frame){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  EXPECT_LT(s.ally->Health,health);EXPECT_TRUE(s.pillbox->IsSelected);
 },&s));
}

TEST(CampaignPresets, FirstMapHarrierDefinitionsAndCleanup) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 const int teams=TeamTypeClass::Array.Count,triggers=TriggerTypeClass::Array.Count,tags=TagTypeClass::Array.Count;
 const int scripts=ScriptTypeClass::Array.Count,tasks=TaskForceClass::Array.Count;
 const int events=TEventClass::Array.Count,actions=TActionClass::Array.Count;
 const int ai=AITriggerTypeClass::Array.Count,announcements=SwizzleManagerClass::Instance.Swizzles_New.Count;
 {
  View view(data,true);view.load("ALL01UMD.MAP");
  ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
   // The actual AIMD.INI must be loaded, not just map-authored definitions.
   int globals=0,locals=0;
   for(auto* type:AITriggerTypeClass::Array){if(type->IsGlobal)++globals;else ++locals;}
   EXPECT_GT(globals,0);EXPECT_EQ(locals,0);
   auto* global=AITriggerTypeClass::Find("0CAD0DCC-G");ASSERT_NE(global,nullptr);EXPECT_TRUE(global->IsEnabled);
   auto* team=TeamTypeClass::Find("0780F21C");ASSERT_NE(team,nullptr);
   ASSERT_NE(team->Owner,nullptr);EXPECT_STREQ(team->Owner->PlainName,"Alliance");
   EXPECT_EQ(team->Waypoint,422);EXPECT_TRUE(team->Suicide);
   ASSERT_NE(team->TaskForce,nullptr);EXPECT_EQ(team->TaskForce->CountEntries,1);
   EXPECT_EQ(team->TaskForce->Entries[0].Amount,1);EXPECT_STREQ(team->TaskForce->Entries[0].Type->ID,"ORCA");
   ASSERT_NE(team->ScriptType,nullptr);ASSERT_EQ(team->ScriptType->ActionsCount,1);
   EXPECT_EQ(team->ScriptType->ScriptActions[0].Action,3);EXPECT_EQ(team->ScriptType->ScriptActions[0].Argument,214);
   EXPECT_EQ(ScenarioClass::Instance->GetWaypointCoords(422),(CellStruct{45,89}));
   EXPECT_EQ(ScenarioClass::Instance->GetWaypointCoords(214),(CellStruct{60,69}));
   auto* camera=TriggerTypeClass::Find("0C077BCC");ASSERT_NE(camera,nullptr);EXPECT_FALSE(camera->Enabled);
   ASSERT_NE(camera->FirstEvent,nullptr);EXPECT_EQ(camera->FirstEvent->EventKind,TriggerEvent::ElapsedTime);EXPECT_EQ(camera->FirstEvent->Value,9);
   auto* action=camera->FirstAction;ASSERT_NE(action,nullptr);EXPECT_EQ(int(action->ActionKind),7);EXPECT_EQ(action->TeamType,team);
   action=action->NextAction;ASSERT_NE(action,nullptr);EXPECT_EQ(int(action->ActionKind),7);EXPECT_EQ(action->TeamType,TeamTypeClass::Find("0780BC8C"));
   action=action->NextAction;ASSERT_NE(action,nullptr);EXPECT_EQ(int(action->ActionKind),48);EXPECT_EQ(action->Waypoint,364);
   action=action->NextAction;ASSERT_NE(action,nullptr);EXPECT_EQ(int(action->ActionKind),53);EXPECT_EQ(action->TriggerType,TriggerTypeClass::Find("0B0246CC"));
   action=action->NextAction;ASSERT_NE(action,nullptr);EXPECT_EQ(int(action->ActionKind),54);EXPECT_EQ(action->TriggerType,camera);EXPECT_EQ(action->NextAction,nullptr);
   EXPECT_NE(MapClass::Instance.GetCellAt(CellStruct{60,69})->Flags&CellFlags::IsWaypoint,CellFlags::Empty);
  },nullptr));
 }
 EXPECT_EQ(TeamTypeClass::Array.Count,teams);EXPECT_EQ(TriggerTypeClass::Array.Count,triggers);EXPECT_EQ(TagTypeClass::Array.Count,tags);
 EXPECT_EQ(ScriptTypeClass::Array.Count,scripts);EXPECT_EQ(TaskForceClass::Array.Count,tasks);
 EXPECT_EQ(TEventClass::Array.Count,events);EXPECT_EQ(TActionClass::Array.Count,actions);
 EXPECT_EQ(AITriggerTypeClass::Array.Count,ai);EXPECT_EQ(SwizzleManagerClass::Instance.Swizzles_New.Count,announcements);
}

TEST(CampaignPresets, FirstMapGattlingPowerAndAutonomousAttack) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& world=*static_cast<game::MapWorld*>(p);
  BuildingClass* tower=nullptr;BuildingClass* reactor=nullptr;
  for(auto* building:BuildingClass::Array){
   if(!std::strcmp(building->Type->ID,"YAGGUN")&&building->GetMapCoords()==(CellStruct{120,77}))tower=building;
   if(!std::strcmp(building->Type->ID,"NANRCT")&&!std::strcmp(building->Owner->PlainName,"Arabs"))reactor=building;
  }
  ASSERT_NE(tower,nullptr);ASSERT_NE(reactor,nullptr);ASSERT_EQ(tower->Owner,reactor->Owner);
  for(auto* unit:UnitClass::Array)unit->ForceMission(Mission::Sleep);
  for(auto* actor:InfantryClass::Array)actor->ForceMission(Mission::Sleep);
  tower->Owner->UpdatePower();EXPECT_GE(tower->Owner->PowerOutput,tower->Owner->PowerDrain);ASSERT_TRUE(tower->IsPowerOnline());
  HouseClass* yuri=nullptr;for(auto* house:HouseClass::Array)if(!std::strcmp(house->PlainName,"YuriCountry"))yuri=house;
  ASSERT_NE(yuri,nullptr);EXPECT_TRUE(tower->Owner->IsAlliedWith(yuri));EXPECT_FALSE(tower->Owner->IsAlliedWith(HouseClass::CurrentPlayer));
  auto* victim=infantry_near_building("E1",HouseClass::CurrentPlayer,tower);ASSERT_NE(victim,nullptr);
  victim->SetHeight(0);victim->ForceMission(Mission::Sleep);
  const int before=victim->Health,last=tower->LastFireBulletFrame;
  for(int i=0;i<300&&victim->Health==before;++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_NE(tower->LastFireBulletFrame,last);EXPECT_LT(victim->Health,before);
  std::cout<<"CAMPAIGN_GATT power="<<tower->Owner->PowerOutput<<"/"<<tower->Owner->PowerDrain<<" health="<<before<<"->"<<victim->Health<<" mission="<<int(tower->CurrentMission)<<"\n";
  reactor->Health=1;reactor->Owner->UpdatePower();ASSERT_FALSE(tower->IsPowerOnline());
  EXPECT_EQ(tower->GetFireError(victim,tower->SelectWeapon(victim),true),FireError::CANT);
  const int stopped=tower->LastFireBulletFrame;
  for(int i=0;i<90;++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_EQ(tower->LastFireBulletFrame,stopped);
  reactor->Health=reactor->Type->Strength;reactor->Owner->UpdatePower();ASSERT_TRUE(tower->IsPowerOnline());
  for(int i=0;i<300&&tower->LastFireBulletFrame==stopped;++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_NE(tower->LastFireBulletFrame,stopped);
 },view.view->world.get()));
}

TEST(BuildingDestruction, OriginalResourcesExplosionsAndDebris) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapViewHandle* view;const char* name;};
 for(const char* name:{"CASANF02","GAWEAP"}){
  State state{view.view,name};
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& state=*static_cast<State*>(p);auto& world=*state.view->world;
   BuildingClass* building=nullptr;for(auto* candidate:BuildingClass::Array)if(!std::strcmp(candidate->Type->ID,state.name)){building=candidate;break;}
   ASSERT_NE(building,nullptr);auto* type=building->Type;const auto origin=building->Location;
   EXPECT_GT(type->MaxDebris,0);EXPECT_GT(type->DebrisAnims.Count,0);
   if(!std::strcmp(state.name,"CASANF02")){EXPECT_EQ(type->MinDebris,5);EXPECT_EQ(type->MaxDebris,15);EXPECT_EQ(type->DebrisAnims.Count,6);}
   int footprint=0;for(auto* offset=building->GetFoundationData(false);*offset!=CellStruct{0x7FFF,0x7FFF};++offset)++footprint;
   // Damage deletes attached animations before creating explosions; their addresses may be reused.
   std::set<int> previous;for(auto* anim:AnimClass::Array)previous.insert(anim->UniqueID);
   auto* warhead=WarheadTypeClass::Find("SA");ASSERT_NE(warhead,nullptr);
   int damage=building->Health+1;EXPECT_EQ(building->ReceiveDamage(&damage,0,warhead,nullptr,true,true,nullptr),DamageState::NowDead);
   if(!type->Explodes){EXPECT_FALSE(building->IsOnMap);EXPECT_FALSE(building->IsAlive);EXPECT_TRUE(building->InLimbo);}
   std::vector<AnimClass*> debris;int explosions=0;
   for(auto* anim:AnimClass::Array)if(!previous.count(anim->UniqueID)){
    if(anim->HasExtras){debris.push_back(anim);EXPECT_TRUE(anim->Type->Bouncer);EXPECT_EQ(anim->RemainingIterations,255);
     EXPECT_GE(anim->Animation.Rate,1);EXPECT_LE(anim->Animation.Rate,4);
     EXPECT_EQ(anim->Bounce.Coords.Z,float(origin.Z+30));EXPECT_GT(anim->Bounce.Velocity.Z,0);
     EXPECT_DOUBLE_EQ(anim->Bounce.Gravity,double(1.4f));
    }
    for(auto* explosion:type->Explosion)if(anim->Type==explosion){++explosions;EXPECT_GE(anim->LoopDelay,0);EXPECT_LE(anim->LoopDelay,3);break;}
   }
   EXPECT_EQ(explosions,footprint);EXPECT_GE(int(debris.size()),type->MinDebris);EXPECT_LT(int(debris.size()),type->MaxDebris);
   ASSERT_FALSE(debris.empty());auto* sample=debris.front();const auto before=sample->Bounce.Coords;const auto velocity=sample->Bounce.Velocity;
   sample->Update();EXPECT_FLOAT_EQ(sample->Bounce.Coords.X,before.X+velocity.X);
   EXPECT_FLOAT_EQ(sample->Bounce.Coords.Y,before.Y+velocity.Y);
   EXPECT_FLOAT_EQ(sample->Bounce.Coords.Z,before.Z+float(velocity.Z-double(1.4f)));
   EXPECT_FALSE(sample->TimeToDie);EXPECT_FALSE(sample->SkipProcessOnce);
   const auto center=TacticalClass::CoordsToScreen(origin);state.view->tactical.TacticalPos={center.X-640,center.Y-360};
   game::rebuild_world_sprites(world);int bodies=0,shadows=0;
   for(const auto& sprite:world.impl->sprites)if(sprite.image==sample->Type->Image){if(sprite.shadow)++shadows;else ++bodies;}
   EXPECT_GT(bodies,0);EXPECT_GT(shadows,0);
   std::set<int> debrisIds;for(auto* anim:debris)debrisIds.insert(anim->UniqueID);
   int frame=0;bool alive=true;
   for(;frame<240&&alive;++frame){++Unsorted::CurrentFrame;game::update_map_world(world);alive=false;
    for(auto* anim:AnimClass::Array)if(debrisIds.count(anim->UniqueID))alive=true;
   }
   EXPECT_FALSE(alive);EXPECT_GT(frame,2);
   std::cout<<"DESTRUCTION "<<state.name<<" explosions="<<explosions<<" debris="<<debris.size()<<" cleared_frame="<<frame<<"\n";
  },&state));
 }
}

TEST(BuildingDestruction, DebrisImpactOnWall) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  auto& map=MapClass::Instance;CellClass* wall=nullptr;
  for(int i=0;i<map.Cells.Capacity;++i)if(auto* cell=map.Cells[i]){auto* type=OverlayTypeClass::Array.GetItemOrDefault(cell->OverlayTypeIndex);
   if(type&&type->Wall&&map.IsWithinUsableArea(cell->MapCoords,true)){wall=cell;break;}}
  ASSERT_NE(wall,nullptr);auto* debrisType=AnimTypeClass::Find("DBRIS1LG");ASSERT_NE(debrisType,nullptr);
  ASSERT_TRUE(debrisType->Bouncer);ASSERT_NE(debrisType->ExpireAnim,nullptr);ASSERT_NE(debrisType->Warhead,nullptr);ASSERT_TRUE(debrisType->Warhead->Wall);
  auto at=wall->GetCoords();at.Z=map.GetCellFloorHeight(at);
  auto* debris=new AnimClass(debrisType,at);
  debris->Bounce.Coords={float(at.X),float(at.Y),float(at.Z+1)};
  debris->Bounce.Velocity={0,0,-4};
  // This enters the same Anim.Update -> DamageArea -> DamageWall chain.
  debris->Update();EXPECT_TRUE(debris->TimeToDie);
  bool expired=false;for(auto* anim:AnimClass::Array)if(anim->Type==debrisType->ExpireAnim&&anim->Location==at)expired=true;
  EXPECT_TRUE(expired);delete debris;
 },nullptr));
}

TEST(BuildingDestruction, WallDamageStagesRemovalAndNavigation) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  auto& map=MapClass::Instance;CellClass* wall=nullptr;
  for(int i=0;i<map.Cells.Capacity;++i)if(auto* cell=map.Cells[i]){
   auto* type=OverlayTypeClass::Array.GetItemOrDefault(cell->OverlayTypeIndex);
   if(type&&type->Wall&&type->DamageLevels==3&&cell->OverlayData>0&&cell->OverlayData<0x10
     &&map.IsWithinUsableArea(cell->MapCoords,true)){wall=cell;break;}
  }
  ASSERT_NE(wall,nullptr);auto* type=OverlayTypeClass::Array[wall->OverlayTypeIndex];const int initialFrame=wall->OverlayData;
  auto& random=ScenarioClass::Instance->Random;const bool oldDisabled=random.unknown_00;random.unknown_00=true;
  const auto restore=ra2::test::scope_exit([&]{random.unknown_00=oldDisabled;});
  wall->DamageWall(0);EXPECT_EQ(wall->OverlayData,initialFrame);
  wall->DamageWall(type->Strength);EXPECT_EQ(wall->OverlayData,initialFrame+0x10);
  struct CellState{CellClass* cell;bool wall;int blocked;};std::vector<CellState> before;
  for(int y=wall->MapCoords.Y-3;y<=wall->MapCoords.Y+3;++y)for(int x=wall->MapCoords.X-3;x<=wall->MapCoords.X+3;++x){
   auto* cell=map.TryGetCellAt(CellStruct{short(x),short(y)});if(!cell)continue;
   auto* overlay=OverlayTypeClass::Array.GetItemOrDefault(cell->OverlayTypeIndex);before.push_back({cell,overlay&&overlay->Wall,cell->BlockedNeighbours});
  }
  InfantryClass* attacker=nullptr;for(auto* actor:InfantryClass::Array)if(actor->IsAlive){attacker=actor;break;}
  ASSERT_NE(attacker,nullptr);attacker->SetTarget(wall);
  wall->DamageWall(-1);
  EXPECT_EQ(wall->OverlayTypeIndex,-1);EXPECT_EQ(wall->OverlayData,0);EXPECT_EQ(wall->WallOwnerIndex,-1);
  EXPECT_NE(attacker->Target,wall);EXPECT_NE(wall->Passability,PassabilityType(2));
  for(const auto& entry:before){int removed=0;
   for(const auto& neighbour:before)if(neighbour.wall&&neighbour.cell->OverlayTypeIndex==-1){
    const int dx=std::abs(entry.cell->MapCoords.X-neighbour.cell->MapCoords.X),dy=std::abs(entry.cell->MapCoords.Y-neighbour.cell->MapCoords.Y);
    if((dx||dy)&&dx<=1&&dy<=1)++removed;
   }
   EXPECT_EQ(entry.cell->BlockedNeighbours,BYTE(entry.blocked-removed));
  }
  const int index=map.GetCellZoneIndex(wall->MapCoords);const auto& sub=map.LevelAndPassabilityStruct2pointer_70[index];
  EXPECT_EQ(map.LevelAndPassability[index].CellPassability,char(wall->Passability));
  for(int level=0;level<3;++level){ASSERT_GT(sub.SubzoneIDs[level],0);
   EXPECT_EQ(map.SubzoneTracking[level][sub.SubzoneIDs[level]].Passability,unsigned(wall->Passability));
   if(level<2)EXPECT_EQ(map.SubzoneTracking[level][sub.SubzoneIDs[level]].ParentSubzoneID,sub.SubzoneIDs[level+1]);
   for(const auto& tracking:map.SubzoneTracking[level])for(const auto& link:tracking.SubzoneConnections){
    ASSERT_LT(link.SubzoneID,unsigned(map.SubzoneTracking[level].Count));
    const auto id=unsigned(&tracking-map.SubzoneTracking[level].Items);const auto& reverse=map.SubzoneTracking[level][link.SubzoneID].SubzoneConnections;
    EXPECT_TRUE(std::any_of(reverse.begin(),reverse.end(),[&](const auto& entry){return entry.SubzoneID==id;}));
   }
  }
  const auto counts=std::array{map.SubzoneTrackingCounts[0],map.SubzoneTrackingCounts[1],map.SubzoneTrackingCounts[2]};
  wall->DamageWall(-1);for(int i=0;i<3;++i)EXPECT_EQ(map.SubzoneTrackingCounts[i],counts[i]);
 },nullptr));
}

TEST(InfantryCombat, OriginalBuildingAttackRangeAndApproachCorpus) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-building-range-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);
 std::ofstream(root/"world.map")<<"[Map]\nSize=0,0,32,32\nLocalSize=0,0,32,32\nLevel=0\nTheater=TEMPERATE\n[Basic]\nNewINIFormat=4\n";
 View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Actor:InfantryClass {
   using InfantryClass::InfantryClass;
   bool IsOnFloor()const override{return true;}
   bool IsInAir()const override{return false;}
   int SelectWeapon(AbstractClass*)const override{return 0;}
   void SetDestination(AbstractClass* destination,bool immediate)override{EXPECT_TRUE(immediate);Destination=destination;}
  };
  struct Target:BuildingClass {
   using BuildingClass::BuildingClass;
   bool IsOnFloor()const override{return true;}
   bool IsInAir()const override{return false;}
  };
  HouseTypeClass country("RANGE_COUNTRY");HouseClass owner(&country);
  InfantryTypeClass actorType("RANGE_GI");Actor actor(&actorType,&owner);
  BuildingTypeClass type("RANGE_BUILDING",BuildingTypeClass::ConstructionDefaults{});Target target(&type,&owner);
  BulletTypeClass bullet("RANGE_BULLET");WeaponTypeClass weapon("RANGE_M60");
  bullet.SubjectToWalls=bullet.SubjectToCliffs=bullet.SubjectToElevation=bullet.Arcing=false;
  weapon.Projectile=&bullet;weapon.Range=1024;weapon.MinimumRange=0;actorType.Weapon[0].WeaponType=&weapon;
  target.Location={8320,8320,0};
  auto& map=MapClass::Instance;
  OverlayTypeClass wall("RANGE_WALL");wall.Wall=true;
  RulesClass::Instance->ElevationIncrement=2;RulesClass::Instance->ElevationIncrementBonus=1.0;
  RulesClass::Instance->ElevationBonusCap=2.0;RulesClass::Instance->AlliedWallTransparency=false;
  const auto terrainPattern=[&](int terrain){
   for(int i=0;i<map.Cells.Capacity;++i)if(auto* cell=map.Cells[i]){
    const int x=cell->MapCoords.X;
    cell->Level=terrain==1&&x>=29&&x<=30?4:0;
    cell->OverlayTypeIndex=terrain==2&&x==30?wall.ArrayIndex:-1;
   }
  };
  auto restoreTerrain=ra2::test::scope_exit([&]{terrainPattern(0);});
  int previousTerrain=-1;
  std::ifstream input(RA2_BUILDING_ATTACK_RANGE_FIXTURE);ASSERT_TRUE(input.good());int foundation,cases=0;
  while(input>>foundation){
   int x,y,z,cellRange,terrain,flags,expectedCoordinate,expectedVirtual;ASSERT_TRUE(bool(input>>x>>y>>z>>cellRange>>terrain>>flags>>expectedCoordinate>>expectedVirtual));
   if(terrain!=previousTerrain){terrainPattern(terrain);previousTerrain=terrain;}
   bullet.SubjectToCliffs=bullet.SubjectToWalls=bullet.SubjectToElevation=flags;
   type.Foundation=static_cast<Foundation>(foundation);weapon.CellRangefinding=cellRange;actor.Location={8320+x,8320+y,z};
   ASSERT_EQ(actor.IsCloseEnough(actor.Location,&target,&weapon),bool(expectedCoordinate))<<"coordinate case "<<cases;
   const TechnoClass* combat=&actor;
   ASSERT_EQ(combat->IsCloseEnough(&target,0),bool(expectedVirtual))<<"virtual case "<<cases;
   ++cases;
  }
  EXPECT_EQ(cases,10080);
  terrainPattern(0);bullet.SubjectToCliffs=bullet.SubjectToWalls=bullet.SubjectToElevation=true;
  actorType.Locomotor=LocomotionClass::CLSIDs::Walk;ASSERT_TRUE(actor.InitializeLocomotor());
  actorType.SpeedType=SpeedType::Foot;actorType.MovementZone=MovementZone::Normal;
  actorType.CloseRange=false;actorType.CanApproachTarget=true;actorType.CanRecalcApproachTarget=true;
  actor.CurrentMission=Mission::Attack;actor.IsInPlayfield=true;weapon.CellRangefinding=false;
  const float oldFoot=GroundType::Array[0].Cost[0],oldWheel=GroundType::Array[0].Cost[2];
  auto restore=ra2::test::scope_exit([&]{actor.Target=nullptr;actor.Destination=nullptr;GroundType::Array[0].Cost[0]=oldFoot;GroundType::Array[0].Cost[2]=oldWheel;});
  GroundType::Array[0].Cost[0]=GroundType::Array[0].Cost[2]=1.0f;
  // Match the original fixture diamond; map loading reserves a smaller playable area.
  map.VisibleRect={0,0,32,31};
  for(int i=0;i<map.Cells.Capacity;++i)if(auto* cell=map.Cells[i]){
   cell->RecalcPassability();map.LevelAndPassability[map.GetCellZoneIndex(cell->MapCoords)].CellPassability=char(cell->Passability);
  }
  map.ResetAllZones();map.ResetAllSubzones();
  std::ifstream approach(RA2_BUILDING_ATTACK_APPROACH_FIXTURE);ASSERT_TRUE(approach.good());int previousMode=-1;cases=0;
  while(approach>>foundation){
   int x,y,mode,query,px,py,multiplier,rx,ry,dx,dy;ASSERT_TRUE(bool(approach>>x>>y>>mode>>query>>px>>py>>multiplier>>rx>>ry>>dx>>dy));
   if(mode!=previousMode){auto& map=MapClass::Instance;
    for(int i=0;i<map.Cells.Capacity;++i)if(auto* cell=map.Cells[i]){
     const int cx=cell->MapCoords.X,cy=cell->MapCoords.Y;
     const bool blocked=(mode==1&&std::max(std::abs(cx-32),std::abs(cy-32))>=3)
       ||(mode==2&&cx<31)||(mode==3&&(cx*17+cy*31)%5<3);
     cell->OccupationFlags=blocked?4:0;
    }
    previousMode=mode;
   }
   type.Foundation=static_cast<Foundation>(foundation);actor.Location={8320+x,8320+y,0};actor.Target=&target;actor.Destination=px==-1?nullptr:map.GetCellAt(CellStruct{short(px),short(py)});
   RulesClass::Instance->ApproachTargetResetMultiplier=multiplier;
   auto* position=actor.FootClass::ApproachTarget(query);
   const CellStruct result=position?static_cast<CellClass*>(position)->MapCoords:CellStruct{-1,-1};
   const CellStruct destination=actor.Destination?static_cast<CellClass*>(actor.Destination)->MapCoords:CellStruct{-1,-1};
   ASSERT_EQ(result,(CellStruct{short(rx),short(ry)}))<<"approach case "<<cases<<" foundation "<<foundation<<" from "<<x<<","<<y<<" mode "<<mode;
   ASSERT_EQ(destination,(CellStruct{short(dx),short(dy)}))<<"assigned approach case "<<cases;
   ++cases;
  }
  EXPECT_EQ(cases,7168);
 },nullptr));
}

TEST(InfantryCombat, OriginalBuildingFirePermissionChain) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-building-fire-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);
 std::ofstream(root/"world.map")<<"[Map]\nSize=0,0,32,32\nLocalSize=0,0,32,32\nLevel=0\nTheater=TEMPERATE\n[Basic]\nNewINIFormat=4\n";
 View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Actor:InfantryClass {
   using InfantryClass::InfantryClass;
   bool IsOnFloor()const override{return true;}
   bool IsInAir()const override{return false;}
   WeaponStruct* GetWeapon(int)const override{return &Type->Weapon[0];}
  };
  struct Target:BuildingClass {
   using BuildingClass::BuildingClass;
   bool IsOnFloor()const override{return true;}
   bool IsInAir()const override{return false;}
   VisualType VisualCharacter(VARIANT_BOOL,HouseClass*)const override{return VisualType::Normal;}
  };
  HouseTypeClass country("FIRE_COUNTRY");HouseClass owner(&country);owner.IsHumanPlayer=true;
  InfantryTypeClass actorType("FIRE_GI");Actor actor(&actorType,&owner);
  BuildingTypeClass type("FIRE_BUILDING",BuildingTypeClass::ConstructionDefaults{});Target target(&type,&owner);
  BulletTypeClass bullet("FIRE_BULLET");WeaponTypeClass weapon("FIRE_M60");WarheadTypeClass warhead("FIRE_WARHEAD");
  actorType.Locomotor=LocomotionClass::CLSIDs::Walk;ASSERT_TRUE(actor.InitializeLocomotor());
  auto* driver=static_cast<WalkLocomotionClass*>(static_cast<LocomotionClass*>(actor.Locomotor));
  actorType.Weapon[0].WeaponType=&weapon;actorType.Strength=actor.Health=125;
  actorType.JumpJet=actorType.Pushy=actorType.DeployFire=false;
  actor.InLimbo=target.InLimbo=false;actor.IsAlive=target.IsAlive=true;
  type.Foundation=Foundation(6);type.Armor=Armor(0);type.Strength=target.Health=1000;target.Location={8320,8320,0};
  bullet.SubjectToWalls=bullet.SubjectToCliffs=bullet.SubjectToElevation=bullet.AG=true;bullet.Arcing=false;
  weapon.Projectile=&bullet;weapon.Warhead=&warhead;weapon.Damage=15;weapon.Range=1024;weapon.MinimumRange=0;weapon.CellRangefinding=false;
  warhead.Verses[0]=1.0;
  actor.CurrentMission=Mission::Attack;actor.QueuedMission=Mission::None;actor.Target=&target;
  RulesClass::Instance->ElevationIncrement=2;RulesClass::Instance->ElevationIncrementBonus=1.0;RulesClass::Instance->ElevationBonusCap=2.0;
  for(int i=0;i<MapClass::Instance.Cells.Capacity;++i)if(auto* c=MapClass::Instance.Cells[i]){c->Level=0;c->OverlayTypeIndex=-1;}
  const int savedFrame=Unsorted::CurrentFrame;Unsorted::CurrentFrame=500;
  auto restore=ra2::test::scope_exit([&]{actor.Target=actor.Destination=nullptr;Unsorted::CurrentFrame=savedFrame;});
  const double speeds[]{0.0,0.05,0.1,std::nextafter(0.1,1.0),0.5,1.0};
  std::ifstream input(RA2_INFANTRY_BUILDING_FIRE_FIXTURE);ASSERT_TRUE(input.good());int seq,cases=0;
  while(input>>seq){
   int dest,speed,dist,ammo,rearm,moving,base,infantry;ASSERT_TRUE(bool(input>>dest>>speed>>dist>>ammo>>rearm>>moving>>base>>infantry));
   actor.SequenceAnim=Sequence(seq);actor.Destination=dest?MapClass::Instance.GetCellAt(CellStruct{28,33}):nullptr;
   actor.SpeedPercentage=speeds[speed];actor.Location={8576-dist,8576,0};actor.Ammo=ammo;actor.RearmTimer.Start(rearm?15:0);driver->IsMoving=moving;
   ASSERT_EQ(int(actor.TechnoClass::GetFireError(&target,0,true)),base)<<"base case "<<cases;
   ASSERT_EQ(int(actor.GetFireError(&target,0,true)),infantry)<<"infantry case "<<cases<<" sequence "<<seq<<" speed "<<speed;
   ++cases;
  }
  EXPECT_EQ(cases,30960);
 },nullptr));
}

TEST(InfantryCombat, FirstMapGroupBuildingAttack) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 for(const char* name:{"GAWEAP","CASANF02","CANEWY12","CASANF01"}){
  View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
  struct Actor {InfantryClass* actor;int lastFire,rangeFrame=-1,fireFrame=-1;CoordStruct fireAt{};bool fireInRange=false;};
  struct State {game::MapViewHandle* view;const char* name;BuildingClass* target=nullptr;std::vector<Actor> actors;Point2D click{};bool found=false;}s{view.view,name};
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   for(auto* building:BuildingClass::Array)if(!std::strcmp(building->Type->ID,s.name)){s.target=building;break;}
   ASSERT_NE(s.target,nullptr);
   // This test measures approach/fire, with unrelated combat and harvest orders asleep.
   for(auto* unit:UnitClass::Array){unit->SetTarget(nullptr);unit->ForceMission(Mission::Sleep);}
   for(auto* actor:InfantryClass::Array)if(std::strcmp(actor->Type->ID,"E1")||!actor->Owner->IsControlledByCurrentPlayer()){
    actor->SetTarget(nullptr);actor->ForceMission(Mission::Sleep);
   }
   std::cout<<"GROUP_TARGET "<<s.name<<" foundation="<<int(s.target->Type->Foundation)<<" center="<<s.target->GetCoords().X<<","<<s.target->GetCoords().Y<<" reset="<<RulesClass::Instance->ApproachTargetResetMultiplier<<"\n";
   for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"E1")&&actor->Owner->IsControlledByCurrentPlayer()){
    ASSERT_TRUE(actor->Select());s.actors.push_back({actor,actor->LastFireBulletFrame});
   }
   if(!s.actors.empty()){auto* w=s.actors[0].actor->GetWeapon(0)->WeaponType;auto* b=w->Projectile;
    std::cout<<"GROUP_WEAPON "<<w->ID<<" range="<<w->Range<<" cell="<<w->CellRangefinding<<" projectile="<<b->ID<<" walls="<<b->SubjectToWalls<<" cliffs="<<b->SubjectToCliffs<<" elevation="<<b->SubjectToElevation<<"\n";}
  },&s));ASSERT_NE(s.target,nullptr);ASSERT_EQ(s.actors.size(),5u);
  const auto center=TacticalClass::CoordsToScreen(s.target->Location);ASSERT_TRUE(game::center_map_view(*view.view,center.X,center.Y));
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);game::rebuild_world_sprites(*s.view->world);
   for(const auto& sprite:s.view->world->impl->sprites)if(sprite.owner==s.target&&!sprite.shadow)
    for(int y=-120;y<=10&&!s.found;++y)for(int x=-70;x<=70&&!s.found;++x){
     Point2D point{sprite.position.X+x,sprite.position.Y+y};if(game::pick_world_object(*s.view->world,point)==s.target){s.click=point;s.found=true;}
    }
  },&s));ASSERT_TRUE(s.found);game::GameInputResult result;
  for(bool down:{true,false})ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,s.click.X,s.click.Y,1,2,down},result));
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   for(int frame=0;frame<1800;++frame){
    // Keep the target alive until every member has reached its firing position.
    s.target->Health=s.target->Type->Strength;
    game::update_map_world(*s.view->world);++Unsorted::CurrentFrame;
    bool complete=true;
    for(auto& entry:s.actors){auto* actor=entry.actor;
     if(entry.rangeFrame<0&&actor->Target==s.target&&actor->IsCloseEnough(s.target,actor->SelectWeapon(s.target))){
      entry.rangeFrame=frame;
      std::cout<<"GROUP_RANGE "<<s.name<<" id="<<actor->UniqueID<<" frame="<<frame<<" at="<<actor->Location.X<<","<<actor->Location.Y
       <<" dest="<<(actor->Destination?actor->Destination->GetCoords().X:0)<<","<<(actor->Destination?actor->Destination->GetCoords().Y:0)<<"\n";
     }
     if(entry.fireFrame<0&&actor->Target==s.target&&actor->LastFireBulletFrame!=entry.lastFire){
      entry.fireFrame=frame;entry.fireAt=actor->Location;
      entry.fireInRange=actor->IsCloseEnough(s.target,actor->SelectWeapon(s.target));
     }
     complete=complete&&entry.fireFrame>=0;
    }
    if(complete)break;
   }
   for(const auto& entry:s.actors){auto* actor=entry.actor;
    std::cout<<"GROUP_FIRE "<<s.name<<" id="<<actor->UniqueID<<" range="<<entry.rangeFrame<<" fire="<<entry.fireFrame
     <<" at="<<entry.fireAt.X<<","<<entry.fireAt.Y<<" mission="<<int(actor->CurrentMission)<<" target="<<(actor->Target==s.target)<<"\n";
    EXPECT_GE(entry.rangeFrame,0);EXPECT_GE(entry.fireFrame,entry.rangeFrame);EXPECT_GE(entry.fireFrame,0);
    EXPECT_TRUE(entry.fireInRange);EXPECT_TRUE(actor->IsSelected);
   }
  },&s));
 }
}

TEST(InfantryCombat, ForceAttackGroundPersistsAfterControlRelease) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapViewHandle* view;InfantryClass* actor=nullptr;CellClass* target=nullptr;Point2D point{};int shots=0,lastFire=-1;}s{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"E1")&&actor->Owner->IsControlledByCurrentPlayer()){s.actor=actor;break;}
  ASSERT_NE(s.actor,nullptr);ASSERT_TRUE(s.actor->Select());
 },&s));ASSERT_NE(s.actor,nullptr);
 const auto center=TacticalClass::CoordsToScreen(s.actor->Location);ASSERT_TRUE(game::center_map_view(*view.view,center.X,center.Y));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& map=MapClass::Instance;
  game::rebuild_world_sprites(*s.view->world);const auto origin=s.actor->GetMapCoords();
  for(int radius=2;radius<=4&&!s.target;++radius)for(int y=-radius;y<=radius&&!s.target;++y)for(int x=-radius;x<=radius&&!s.target;++x){
   const CellStruct cell{short(origin.X+x),short(origin.Y+y)};auto* ground=map.TryGetCellAt(cell);Point2D point;CellStruct picked;
   if(!ground||ground->FirstObject||!map.IsWithinUsableArea(cell,true)||map.IsLocationShrouded(ground->GetCoords())
      ||!s.actor->IsCloseEnough(ground,0))continue;
   if(TacticalClass::CoordsToClient(ground->GetCoords(),s.view->tactical.TacticalPos,TacticalClass::ViewBounds,point)
      &&point.X>20&&point.X<TacticalClass::ViewBounds.Width-20&&point.Y>20&&point.Y<TacticalClass::ViewBounds.Height-20
      &&!game::pick_world_object(*s.view->world,point)
      &&s.view->tactical.PickTerrainCell(point,TacticalClass::ViewBounds,picked)&&picked==cell){s.target=ground;s.point=point;}
  }
 },&s));ASSERT_NE(s.target,nullptr);
 game::GameInputResult result;
 for(bool down:{true,false})ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,s.point.X,s.point.Y,1,2,down},result));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  EXPECT_EQ(TechnoClass::ActionLineTimer.GetTimeLeft(),25);
  ASSERT_EQ(EventClass::OutList.Count,1);auto& event=EventClass::OutList.First();
  EXPECT_EQ(event.MegaMission.Mission,unsigned(Mission::Attack));EXPECT_EQ(event.MegaMission.Target.As_Cell(),s.target);
 },&s));
 ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,s.point.X,s.point.Y,17,0,false},result));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  s.lastFire=s.actor->LastFireBulletFrame;
  for(int i=0;i<240;++i){game::update_map_world(*s.view->world);++Unsorted::CurrentFrame;
   if(s.actor->LastFireBulletFrame!=s.lastFire){++s.shots;s.lastFire=s.actor->LastFireBulletFrame;}
  }
  EXPECT_EQ(s.actor->Target,s.target);EXPECT_TRUE(s.actor->IsSelected);EXPECT_GE(s.shots,2);
 },&s));
}

TEST(UnitActionLines, OriginalSegmentPixels) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& view=*static_cast<View*>(p);
  UnitClass actor(UnitTypeClass::Find("FV"),HouseClass::CurrentPlayer);
  const auto oldBounds=DSurface::ViewBounds;const auto oldCamera=view.view->tactical.TacticalPos;
  const int oldFrame=Unsorted::CurrentFrame;auto& palette=view.view->world->impl->selection_palette;
  const auto oldColor=palette.Entries[0];palette.Entries[0]={8,8,8};
  auto restore=ra2::test::scope_exit([&]{DSurface::ViewBounds=oldBounds;view.view->tactical.TacticalPos=oldCamera;Unsorted::CurrentFrame=oldFrame;palette.Entries[0]=oldColor;});
  view.view->tactical.TacticalPos={0,0};
  struct State{UnitClass& actor;CoordStruct from{},to{};bool dashed=false,shadow=false;std::array<WORD,128*128> pixels{};}state{actor};
  game::TypeDrawingContext drawing;drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(&state);drawing.backend_context=&state;
  drawing.backend.raster=[](void* p,const game::RasterDrawingRequest& r){auto&s=*static_cast<State*>(p);
   for(int y=r.position.Y;y<r.position.Y+r.height;++y)for(int x=r.position.X;x<r.position.X+r.width;++x)
    if(x>=0&&x<128&&y>=0&&y<128)s.pixels[y*128+x]=r.color;
   return game::DrawingStatus::drawn;
  };
  std::ifstream input(RA2_ACTION_LINE_PIXELS_FIXTURE);std::string magic;int count;
  ASSERT_TRUE(bool(input>>magic>>count));ASSERT_EQ(magic,"ACTION_LINE_PIXELS_V1");ASSERT_EQ(count,240);
  for(int row=0;row<count;++row){auto&c=DSurface::ViewBounds;int n;
   ASSERT_TRUE(bool(input>>state.from.X>>state.from.Y>>state.from.Z>>state.to.X>>state.to.Y>>state.to.Z
    >>c.X>>c.Y>>c.Width>>c.Height>>state.dashed>>state.shadow>>Unsorted::CurrentFrame>>n));
   SCOPED_TRACE(row);std::array<WORD,128*128> expected{};
   for(int i=0;i<n;++i){int index,color;ASSERT_TRUE(bool(input>>index>>color));ASSERT_GE(index,0);ASSERT_LT(index,128*128);expected[index]=WORD(color);}
   state.pixels.fill(0);
   EXPECT_TRUE(game::drawing_completed(game::with_type_drawing(drawing,[](void*p){auto&s=*static_cast<State*>(p);
    s.actor.DrawActionLine(s.from,s.to,{249,179,101},s.dashed,s.shadow);
   },&state)));
   EXPECT_EQ(state.pixels,expected);
  }
 },&view));
}

TEST(UnitActionLines, OriginalFootDecisions) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*p){auto&view=*static_cast<View*>(p);
  struct Probe:UnitClass{
   Probe():UnitClass(UnitTypeClass::Find("FV"),HouseClass::CurrentPlayer){}
   AbstractType WhatAmI()const override{return AbstractType::Cell;}
   CoordStruct* GetCoords(CoordStruct*out)const override{*out=Location;return out;}
   CoordStruct* GetCenterCoords(CoordStruct*out)const override{*out=Location;return out;}
   CoordStruct* vt_entry_300(CoordStruct*out,DWORD)const override{*out=Location;return out;}
  } actor,attack,dest,route;
  actor.Location={25728,25728,0};attack.Location={26496,25728,0};dest.Location={25728,26496,0};route.Location={26496,26496,0};
  auto* a=MapClass::Instance.GetCellAt(dest.Location);auto* b=MapClass::Instance.GetCellAt(route.Location);
  const auto af=a->Flags,bf=b->Flags;const auto al=a->Level,bl=b->Level;const auto as=a->SlopeIndex,bs=b->SlopeIndex;
  a->Level=b->Level=0;a->SlopeIndex=b->SlopeIndex=0;
  const auto oldBounds=DSurface::ViewBounds;const auto oldCamera=view.view->tactical.TacticalPos;
  const int oldFrame=Unsorted::CurrentFrame;const auto oldTimer=TechnoClass::ActionLineTimer;
  auto&palette=view.view->world->impl->selection_palette;const auto c3=palette.Entries[3],c8=palette.Entries[8];
  palette.Entries[3]={20,230,180};palette.Entries[8]={249,179,101};
  auto restore=ra2::test::scope_exit([&]{a->Flags=af;b->Flags=bf;a->Level=al;b->Level=bl;a->SlopeIndex=as;b->SlopeIndex=bs;
   DSurface::ViewBounds=oldBounds;view.view->tactical.TacticalPos=oldCamera;Unsorted::CurrentFrame=oldFrame;
   TechnoClass::ActionLineTimer=oldTimer;palette.Entries[3]=c3;palette.Entries[8]=c8;
   actor.Target=actor.Destination=nullptr;actor.unknown_abstract_array_588.Clear();});
  DSurface::ViewBounds={0,0,512,512};view.view->tactical.TacticalPos={-180,2900};
  struct Pixel{int x,y,w,h;WORD color;bool operator==(const Pixel&)const=default;};
  struct State{Probe&actor;bool force=false;DWORD dashed=0;CoordStruct from{},to{};ColorStruct color{};bool ed=false,es=false;std::vector<Pixel> pixels;}s{actor};
  game::TypeDrawingContext drawing;drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(&s);drawing.backend_context=&s;
  drawing.backend.raster=[](void*p,const game::RasterDrawingRequest&r){static_cast<State*>(p)->pixels.push_back({r.position.X,r.position.Y,r.width,r.height,r.color});return game::DrawingStatus::drawn;};
  std::ifstream input(RA2_ACTION_LINE_BODY_FIXTURE);std::string magic;int count;
  ASSERT_TRUE(bool(input>>magic>>count));ASSERT_EQ(magic,"ACTION_LINE_BODY_V1");ASSERT_EQ(count,384);
  for(int row=0;row<count;++row){int target,destination,queue,bridge,elapsed,calls;unsigned color;
   ASSERT_TRUE(bool(input>>target>>destination>>queue>>s.force>>s.dashed>>bridge>>elapsed>>calls
    >>s.from.X>>s.from.Y>>s.from.Z>>s.to.X>>s.to.Y>>s.to.Z>>color>>s.ed>>s.es));SCOPED_TRACE(row);
   s.color={byte(color),byte(color>>8),byte(color>>16)};
   actor.Target=target?&attack:nullptr;actor.Destination=destination?&dest:nullptr;
   actor.unknown_abstract_array_588.Clear();if(queue)actor.unknown_abstract_array_588.AddItem(&route);
   a->Flags=b->Flags=static_cast<CellFlags>(bridge?0x100u:0u);
   Unsorted::CurrentFrame=100;TacticalClass::StartDrawActionLineTimer();Unsorted::CurrentFrame+=elapsed;
   s.pixels.clear();EXPECT_TRUE(game::drawing_completed(game::with_type_drawing(drawing,[](void*p){auto&s=*static_cast<State*>(p);s.actor.DrawActionLines(s.force,s.dashed);},&s)));
   const auto actual=s.pixels;s.pixels.clear();
   if(calls)EXPECT_TRUE(game::drawing_completed(game::with_type_drawing(drawing,[](void*p){auto&s=*static_cast<State*>(p);s.actor.DrawActionLine(s.from,s.to,s.color,s.ed,s.es);},&s)));
   EXPECT_EQ(actual,s.pixels);
  }
 },&view));
}

TEST(UnitActionLines, OriginalDispatchGatesAndOrder) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& view=*static_cast<View*>(p);auto& world=*view.view->world;
  struct Capture{std::vector<int> calls;bool band=false;bool rasterAfterCall=false;}capture;
  struct Probe final:UnitClass{
   Capture& capture;int id;
   Probe(UnitTypeClass* type,HouseClass* owner,Capture& c,int i):UnitClass(type,owner),capture(c),id(i){}
   void DrawActionLines(bool force,DWORD unknown)override{
    EXPECT_FALSE(force);EXPECT_EQ(unknown,0u);EXPECT_TRUE(capture.band);
    ASSERT_NE(game::active_type_drawing(),nullptr);
    EXPECT_EQ(game::active_type_drawing()->backend_context,&capture);capture.calls.push_back(id);
   }
  };
  auto* player=HouseClass::CurrentPlayer;HouseClass* other=nullptr;
  for(auto* house:HouseClass::Array)if(house!=player){other=house;break;}
  ASSERT_NE(player,nullptr);ASSERT_NE(other,nullptr);auto* type=UnitTypeClass::Find("FV");ASSERT_NE(type,nullptr);
  const bool playerHuman=player->IsHumanPlayer,playerControl=player->IsInPlayerControl;
  const bool otherHuman=other->IsHumanPlayer,otherControl=other->IsInPlayerControl;
  const bool enabled=TechnoClass::ActionLines,planning=PlanningNodeClass::PlanningModeActive;
  const auto mode=view.view->loop.mode;
  auto restore=ra2::test::scope_exit([&]{player->IsHumanPlayer=playerHuman;player->IsInPlayerControl=playerControl;
   other->IsHumanPlayer=otherHuman;other->IsInPlayerControl=otherControl;TechnoClass::ActionLines=enabled;
   PlanningNodeClass::PlanningModeActive=planning;view.view->loop.mode=mode;world.impl->dragging=false;});
  Probe first(type,player,capture,1),second(type,player,capture,2);
  // They deliberately are not members of CurrentObjects: the original call
  // walks TechnoClass::Array, and the virtual body owns target/timer checks.
  EXPECT_LT(ObjectClass::CurrentObjects.FindItemIndex(&first),0);
  world.impl->dragging=true;view.view->tactical.Band={20,20,50,50};
  game::MapDrawingContext drawing;drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(&capture);
  drawing.types.backend_context=&capture;
  drawing.shape_palette=[](void*,const BytePalette& pal,int,const game::DrawingPaletteHandle*& out)noexcept{out=reinterpret_cast<const game::DrawingPaletteHandle*>(&pal);return game::DrawingStatus::drawn;};
  drawing.color_scheme_palette=drawing.shape_palette;
  drawing.terrain_palette=[](void*,const BytePalette& pal,int,int,int,int,const game::DrawingPaletteHandle*& out)noexcept{out=reinterpret_cast<const game::DrawingPaletteHandle*>(&pal);return game::DrawingStatus::drawn;};
  drawing.types.backend.shape=[](void*,const game::ShapeDrawingRequest&){return game::DrawingStatus::drawn;};
  drawing.types.backend.indexed=[](void*,const game::IndexedDrawingRequest&){return game::DrawingStatus::drawn;};
  drawing.types.backend.raster=[](void* p,const game::RasterDrawingRequest&r){auto&c=*static_cast<Capture*>(p);
   if(!c.calls.empty())c.rasterAfterCall=true;
   if(r.position==Point2D{20,20}&&r.width==31&&r.height==1)c.band=true;
   return game::DrawingStatus::drawn;
  };
  std::ifstream input(RA2_ACTION_LINE_CALL_FIXTURE);std::string magic;int count;
  ASSERT_TRUE(bool(input>>magic>>count));ASSERT_EQ(magic,"ACTION_LINE_CALL_V1");ASSERT_EQ(count,128);
  for(int row=0;row<count;++row){int mp,current,human,control,plan,selected,on,calls,force,unknown;
   ASSERT_TRUE(bool(input>>mp>>current>>human>>control>>plan>>selected>>on>>calls>>force>>unknown));SCOPED_TRACE(row);
   view.view->loop.mode=mp?GameMode::Skirmish:GameMode::Campaign;
   auto* owner=current?player:other;owner->IsHumanPlayer=human;owner->IsInPlayerControl=control;
   first.Owner=second.Owner=owner;first.IsSelected=second.IsSelected=selected;
   PlanningNodeClass::PlanningModeActive=plan;TechnoClass::ActionLines=on;
   capture={};game::MapDrawStatistics stats;
   EXPECT_TRUE(game::drawing_completed(game::draw_map_world(world,drawing,TacticalClass::ViewBounds,stats)));
   EXPECT_EQ(capture.calls.size(),std::size_t(calls*2));EXPECT_FALSE(capture.rasterAfterCall);
   if(calls){EXPECT_EQ(force,0);EXPECT_EQ(unknown,0);EXPECT_EQ(capture.calls,(std::vector<int>{1,2}));}
  }
  first.IsSelected=second.IsSelected=false;
 },&view))<<game::map_view_error(*view.view);
}

TEST(InfantryCombat, IFVMissileOriginalDrawing) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 Point2D center{};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){
  for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"FV")){
   *static_cast<Point2D*>(p)=TacticalClass::CoordsToScreen(unit->Location);return;
  }
  FAIL()<<"Map needs an IFV";
 },&center));
 ASSERT_TRUE(game::center_map_view(*view.view,center.X,center.Y));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& view=*static_cast<View*>(p);auto& world=*view.view->world;
  UnitClass* actor=nullptr;for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"FV")){actor=unit;break;}
  ASSERT_NE(actor,nullptr);
  auto* weapon=WeaponTypeClass::Find("HoverMissile");ASSERT_NE(weapon,nullptr);
  auto* type=weapon->Projectile;ASSERT_NE(type,nullptr);
  EXPECT_STREQ(type->ID,"AAHeatSeeker2");EXPECT_STREQ(type->ImageFile,"DRAGON");
  ASSERT_FALSE(type->AnimPalette);ASSERT_FALSE(type->FirersPalette);ASSERT_FALSE(type->NoRotate);
  ASSERT_FALSE(type->Shadow);ASSERT_FALSE(type->Voxel);
  auto* bullet=type->CreateBullet(actor->GetCell(),actor,weapon->Damage,weapon->Warhead,weapon->Speed,false);
  ASSERT_NE(bullet,nullptr);
  const bool originalShadow=type->Shadow,originalFirers=type->FirersPalette;
  auto cleanup=ra2::test::scope_exit([&]{type->AnimPalette=false;type->Shadow=originalShadow;type->FirersPalette=originalFirers;bullet->Release();});
  bullet->SetWeaponType(weapon);ASSERT_TRUE(bullet->MoveTo(actor->Location,{1.0,0.0,0.0}));
  ASSERT_NE(bullet->GetImage(),nullptr);EXPECT_EQ(bullet->GetImage()->Frames,32);
  std::ifstream input(RA2_BULLET_DRAWING_FIXTURE);std::string magic;int count=0;
  ASSERT_TRUE(bool(input>>magic>>count));ASSERT_EQ(magic,"BULLET_DRAWING_V1");ASSERT_EQ(count,64);
  for(int row=0;row<count;++row){
   int anim,height,frame,palette,flags,depth,gradient,intensity;double vx,vy,vz;
   ASSERT_TRUE(bool(input>>anim>>vx>>vy>>vz>>height>>frame>>palette>>flags>>depth>>gradient>>intensity));
   SCOPED_TRACE(row);type->AnimPalette=anim!=0;bullet->Velocity={vx,vy,vz};bullet->Location.Z=height;
   ++world.presentation_revision;game::rebuild_world_sprites(world);
   int draws=0;for(const auto& sprite:world.impl->sprites)if(sprite.owner==bullet){
    ++draws;EXPECT_EQ(sprite.image,bullet->GetImage());EXPECT_EQ(sprite.frame,frame);
    EXPECT_EQ(sprite.palette,palette==0?&world.impl->selection_palette:&world.impl->anim_palette);
    EXPECT_EQ(sprite.flags,flags);EXPECT_EQ(sprite.depth_adjustment,depth);
    EXPECT_EQ(sprite.gradient,gradient);EXPECT_EQ(sprite.intensity,intensity);
    EXPECT_FALSE(sprite.shadow);EXPECT_FALSE(sprite.cell_tint);EXPECT_FALSE(sprite.color_scheme);
   }
   EXPECT_EQ(draws,1);
  }
  // YR 0x00468374 / 0x0046837F..0x004683D1: the shadow uses NormalDrawer;
  // body AnimPalette wins over FirersPalette, and only index -1 uses the player.
  struct PaletteCalls {
   std::vector<std::pair<game::DrawingPaletteKind,int>> choices;
   std::vector<game::ShapeDrawingRequest> shapes;
   int normal=0,animation=0,scheme=0;
  } calls;
  game::TypeDrawingContext drawing;drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(&calls);drawing.backend_context=&calls;
  drawing.backend.shape=[](void* p,const game::ShapeDrawingRequest& r){static_cast<PaletteCalls*>(p)->shapes.push_back(r);return game::DrawingStatus::drawn;};
  game::DrawingResources resources;resources.context=&calls;
  resources.palette=[](void* p,game::DrawingPaletteKind kind,int index,const game::DrawingPaletteHandle*& output) noexcept {
   auto& c=*static_cast<PaletteCalls*>(p);
   try{c.choices.emplace_back(kind,index);}catch(...){return game::DrawingStatus::backend_failure;}
   output=reinterpret_cast<const game::DrawingPaletteHandle*>(kind==game::DrawingPaletteKind::normal?&c.normal:kind==game::DrawingPaletteKind::animation?&c.animation:&c.scheme);
   return game::DrawingStatus::drawn;
  };
  type->Shadow=true;bullet->Location.Z=MapClass::Instance.GetCellFloorHeight(bullet->Location)+256;
  for(int flags=0;flags<4;++flags)for(int inherited:{-1,7}){
   type->AnimPalette=flags&1;type->FirersPalette=flags&2;bullet->InheritedColor=inherited;
   calls.choices.clear();calls.shapes.clear();
   EXPECT_EQ(game::with_drawing_resources(resources,drawing,[](void* p){Point2D at{100,100};RectangleStruct clip{0,0,512,512};static_cast<BulletClass*>(p)->DrawIt(&at,&clip);},bullet),game::DrawingStatus::drawn);
   ASSERT_EQ(calls.choices.size(),2u);ASSERT_EQ(calls.shapes.size(),2u);
   EXPECT_EQ(calls.choices[0],(std::pair{game::DrawingPaletteKind::normal,-1}));
   const auto expected=flags&1?game::DrawingPaletteKind::animation:flags&2?game::DrawingPaletteKind::color_scheme:game::DrawingPaletteKind::normal;
   const int index=expected==game::DrawingPaletteKind::color_scheme?(inherited==-1?HouseClass::CurrentPlayer->ColorSchemeIndex:inherited):-1;
   EXPECT_EQ(calls.choices[1],(std::pair{expected,index}));
   EXPECT_EQ(calls.shapes[0].palette,reinterpret_cast<const game::DrawingPaletteHandle*>(&calls.normal));
   EXPECT_EQ(calls.shapes[1].palette,reinterpret_cast<const game::DrawingPaletteHandle*>(expected==game::DrawingPaletteKind::normal?&calls.normal:expected==game::DrawingPaletteKind::animation?&calls.animation:&calls.scheme));
  }
 },&view))<<game::map_view_error(*view.view);
}

TEST(TacticalDrawing, FrameScopeRestoresOnFailureAndRejectsSplitModes) {
 game::TacticalDrawingFrame outer;
 EXPECT_EQ(game::tactical_drawing(),nullptr);
 EXPECT_EQ(game::with_tactical_drawing(outer,[](void* p){
  EXPECT_EQ(game::tactical_drawing(),p);
  game::TacticalDrawingFrame inner;
  EXPECT_EQ(game::with_tactical_drawing(inner,[](void*){throw 1;},nullptr),game::DrawingStatus::backend_failure);
  EXPECT_EQ(game::tactical_drawing(),p);
  TacticalClass tactical({0,0},{0,0,100,100},0,1,0,1,1);
  for(int mode:{0x0,0x1,0x2}){
   std::pair<TacticalClass*,int> call{&tactical,mode};
   EXPECT_EQ(game::with_tactical_drawing(inner,[](void* p){
    auto& call=*static_cast<std::pair<TacticalClass*,int>*>(p);call.first->Render(nullptr,true,call.second);
   },&call),game::DrawingStatus::unsupported);
  }
  EXPECT_EQ(game::tactical_drawing(),p);
 },&outer),game::DrawingStatus::skipped);
 EXPECT_EQ(game::tactical_drawing(),nullptr);
}

TEST(TacticalDrawing, WholeFrameReportsTilesWithoutObjectPass) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::set_map_viewport(*view.view,640,480));
 auto& world=*view.view->world;
 const bool loaded=world.impl->loaded;world.impl->loaded=false;
 auto restore=ra2::test::scope_exit([&]{world.impl->loaded=loaded;});
 struct Sink{int tiles=0;game::DrawingStatus result=game::DrawingStatus::drawn;}sink;
 game::MapDrawingContext drawing;drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(&sink);
 drawing.types.backend_context=&sink;
 drawing.terrain_palette=[](void*,const BytePalette& p,int,int,int,int,const game::DrawingPaletteHandle*& out)noexcept{
  out=reinterpret_cast<const game::DrawingPaletteHandle*>(&p);return game::DrawingStatus::drawn;
 };
 drawing.types.backend.tile=[](void* p,const game::TileDrawingRequest&){
  auto& sink=*static_cast<Sink*>(p);++sink.tiles;
  EXPECT_NE(game::tactical_drawing(),nullptr);
  return sink.result;
 };
 drawing.types.backend.shape=[](void*,const game::ShapeDrawingRequest&){return game::DrawingStatus::skipped;};
 drawing.types.backend.raster=[](void*,const game::RasterDrawingRequest&){return game::DrawingStatus::drawn;};
 game::MapDrawStatistics stats;
 EXPECT_EQ(game::draw_map_view(*view.view,drawing,stats),game::DrawingStatus::drawn);
 EXPECT_GT(sink.tiles,0);EXPECT_GT(stats.drawn,0u);EXPECT_EQ(game::tactical_drawing(),nullptr);
 sink.result=game::DrawingStatus::skipped;
 EXPECT_EQ(game::draw_map_view(*view.view,drawing,stats),game::DrawingStatus::skipped);
 EXPECT_EQ(stats.drawn,0u);
 sink.result=game::DrawingStatus::backend_failure;
 EXPECT_EQ(game::draw_map_view(*view.view,drawing,stats),game::DrawingStatus::backend_failure);
 EXPECT_EQ(game::tactical_drawing(),nullptr);
}

TEST(TacticalDrawing, OriginalRubberBandAndResourceScope) {
 auto* previous=TacticalClass::Instance;auto restore=ra2::test::scope_exit([&]{TacticalClass::Instance=previous;});
 TacticalClass tactical({0,0},{0,0,100,100},0,1,0,1,1);
 BytePalette palette{};palette.Entries[15]={0xF8,0,0xF8};
 std::vector<game::RasterDrawingRequest> output;
 game::TypeDrawingContext drawing;drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(&output);drawing.backend_context=&output;
 drawing.backend.raster=[](void* p,const game::RasterDrawingRequest& r){static_cast<std::vector<game::RasterDrawingRequest>*>(p)->push_back(r);return game::DrawingStatus::drawn;};
 game::DrawingResources resources;resources.normal_palette=&palette;resources.clip={11,13,100,100};
 const auto draw=[](void* p){static_cast<TacticalClass*>(p)->DrawRubberBand();};
 tactical.Band={5,7,2,3};
 EXPECT_EQ(game::with_drawing_resources(resources,drawing,draw,&tactical),game::DrawingStatus::drawn);
 const RectangleStruct expected[]={{13,16,4,1},{13,20,4,1},{13,16,1,5},{16,16,1,5}};
 ASSERT_EQ(output.size(),4u);
 for(unsigned i=0;i<4;++i){EXPECT_EQ(output[i].position,(Point2D{expected[i].X,expected[i].Y}));EXPECT_EQ(output[i].width,expected[i].Width);EXPECT_EQ(output[i].height,expected[i].Height);EXPECT_EQ(output[i].color,0xF81F);}
 output.clear();tactical.Band={0,0,50,60};
 EXPECT_EQ(game::with_drawing_resources(resources,drawing,draw,&tactical),game::DrawingStatus::skipped);EXPECT_TRUE(output.empty());
 struct Nested{game::DrawingResources& resources;game::TypeDrawingContext& drawing;}nested{resources,drawing};
 EXPECT_EQ(game::with_drawing_resources(resources,drawing,[](void* p){auto& n=*static_cast<Nested*>(p);game::DrawingResources inner;
  EXPECT_EQ(game::active_drawing_resources(),&n.resources);
  EXPECT_EQ(game::with_drawing_resources(inner,n.drawing,[](void*){throw 1;},nullptr),game::DrawingStatus::backend_failure);
  EXPECT_EQ(game::active_drawing_resources(),&n.resources);
 },&nested),game::DrawingStatus::skipped);
 EXPECT_EQ(game::active_drawing_resources(),nullptr);EXPECT_EQ(game::active_type_drawing(),nullptr);
}

TEST(InfantryCombat, IFVMissileTrailViewport) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::set_game_view_size(*view.view,1280,720));
 Point2D focus{};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){
  for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"FV")){
   *static_cast<Point2D*>(p)=TacticalClass::CoordsToScreen(unit->Location);return;
  }
  FAIL()<<"Map needs an IFV";
 },&focus));
 ASSERT_TRUE(game::center_map_view(*view.view,focus.X,focus.Y));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  UnitClass* owner=nullptr;for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"FV")){owner=unit;break;}
  ASSERT_NE(owner,nullptr);auto* weapon=WeaponTypeClass::Find("HoverMissile");ASSERT_NE(weapon,nullptr);
  ASSERT_TRUE(weapon->Projectile->UseLineTrail);
  auto* bullet=weapon->Projectile->CreateBullet(owner->GetCell(),owner,weapon->Damage,weapon->Warhead,weapon->Speed,false);
  ASSERT_NE(bullet,nullptr);auto cleanup=ra2::test::scope_exit([&]{bullet->Release();});
  bullet->SetWeaponType(weapon);ASSERT_TRUE(bullet->MoveTo(owner->Location,{1.0,0.0,0.0}));
  ASSERT_NE(bullet->LineTrailer,nullptr);
  Point2D first{},last{};
  // Constant height is intentional: broken projection used to produce (0,0)
  // endpoints while a varying Z still generated bogus raster requests there.
  for(int i=0;i<12;++i){
   bullet->Location.X+=40;bullet->Location.Z=256;LineTrail::UpdateAll();
   auto expected=TacticalClass::CoordsToScreen(bullet->Location);expected-=TacticalClass::Instance->TacticalPos;
   Point2D actual{};ASSERT_TRUE(TacticalClass::Instance->CoordsToClient(&bullet->Location,&actual));
   EXPECT_EQ(actual,expected);if(!i)first=expected;last=expected;
  }
  struct Capture{Point2D first,last;int pixels=0;}capture{first,last};
  game::TypeDrawingContext drawing;drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(&capture);drawing.backend_context=&capture;
  drawing.backend.raster=[](void* p,const game::RasterDrawingRequest& r){auto& c=*static_cast<Capture*>(p);
   EXPECT_EQ(r.blend_mode,game::RasterBlendMode::depth_alpha);EXPECT_EQ(r.line_rgb,0xFFD8D8u);
   EXPECT_GE(r.position.X-r.clip.X,c.first.X);EXPECT_LE(r.position.X-r.clip.X,c.last.X);
   EXPECT_GE(r.position.Y-r.clip.Y,c.first.Y);EXPECT_LE(r.position.Y-r.clip.Y,c.last.Y);
   ++c.pixels;return game::DrawingStatus::drawn;
  };
  EXPECT_EQ(game::with_type_drawing(drawing,[](void* p){static_cast<LineTrail*>(p)->Draw();},bullet->LineTrailer),game::DrawingStatus::drawn);
  EXPECT_GT(capture.pixels,20);
 },nullptr))<<game::map_view_error(*view.view);
}

TEST(InfantryCombat, GGIDeployedMissileAttack) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 const int beforeTrails=LineTrail::Array.Count;
 {
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapViewHandle* view;InfantryClass* actor=nullptr;BuildingClass* target=nullptr;ObjectClass* picking=nullptr;Point2D point{};bool found=false,sawMissile=false,sawTrail=false,sawDetached=false,sawSprite=false,sawTailPixels=false;int lastBullet=-1,missiles=0;}s{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"GGI")&&actor->Owner->IsControlledByCurrentPlayer()){s.actor=actor;break;}
  ASSERT_NE(s.actor,nullptr);double nearest=1e30;
  for(auto* target:BuildingClass::Array)if(target->CanBeSelected()&&s.actor->MouseOverObject(target,true)==Action::Attack){
   const auto d=target->Location-s.actor->Location;const double distance=double(d.X)*d.X+double(d.Y)*d.Y;
   if(distance<nearest){s.target=target;nearest=distance;}
  }
  ASSERT_NE(s.target,nullptr);std::cout<<"GGI target "<<s.target->Type->ID<<" distance="<<std::sqrt(nearest)/256.0<<"\n";
 },&s));
 const auto click=[&](ObjectClass* target,unsigned modifiers){
  const auto center=TacticalClass::CoordsToScreen(target->Location);ASSERT_TRUE(game::center_map_view(*view.view,center.X,center.Y));s.picking=target;s.found=false;
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& world=*s.view->world;game::rebuild_world_sprites(world);
   for(const auto& sprite:world.impl->sprites)if(sprite.owner==s.picking&&!sprite.shadow)
    for(int y=-120;y<=10&&!s.found;++y)for(int x=-70;x<=70&&!s.found;++x){Point2D point{sprite.position.X+x,sprite.position.Y+y};
     if(game::pick_world_object(world,point)==s.picking){s.point=point;s.found=true;}}
  },&s));ASSERT_TRUE(s.found);game::GameInputResult result;
  for(bool down:{true,false})ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,s.point.X,s.point.Y,1,modifiers,down},result));
 };
 const auto tick=[&]{ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);game::update_map_world(*s.view->world);++Unsorted::CurrentFrame;
  for(auto* bullet:BulletClass::Array)if(bullet->Owner==s.actor&&bullet->WeaponType==s.actor->GetWeapon(1)->WeaponType){
   if(s.lastBullet!=bullet->Fetch_ID()){s.lastBullet=bullet->Fetch_ID();++s.missiles;}
   s.sawMissile=true;if(bullet->LineTrailer){s.sawTrail=true;EXPECT_EQ(bullet->LineTrailer->Owner,bullet);EXPECT_GT(LineTrail::Array.Count,0);}
   game::rebuild_world_sprites(*s.view->world);
   for(const auto& sprite:s.view->world->impl->sprites)if(sprite.owner==bullet&&!sprite.shadow){
    s.sawSprite=true;EXPECT_EQ(sprite.frame,bullet->GetAnimFrame());EXPECT_EQ(sprite.layer,Layer::Air);
    EXPECT_EQ(sprite.flags,0x2E00u);EXPECT_EQ(sprite.gradient,0);EXPECT_EQ(sprite.depth_adjustment,-30-TacticalClass::AdjustForZ(bullet->Location.Z));
   }
  }
  for(auto* trail:LineTrail::Array)if(!trail->Owner)s.sawDetached=true;
  game::TypeDrawingContext drawing;drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(&s);drawing.backend_context=&s;
  drawing.backend.raster=[](void* p,const game::RasterDrawingRequest& r){auto& s=*static_cast<State*>(p);
   EXPECT_EQ(r.blend_mode,game::RasterBlendMode::depth_alpha);EXPECT_GE(r.line_opacity,8);EXPECT_LE(r.line_opacity,255);s.sawTailPixels=true;return game::DrawingStatus::drawn;};
  EXPECT_TRUE(game::drawing_completed(game::with_type_drawing(drawing,[](void*){LineTrail::DrawAll();},nullptr)));
 },&s));};
 click(s.actor,0);ASSERT_TRUE(s.actor->IsSelected);click(s.target,2);
 // Let the public attack order finish approaching and stop to fire. Deploy
 // from that stationary posture, matching the reported deployed attack.
 const int approachHealth=s.target->Health;
 for(int i=0;i<3600&&s.target->Health==approachHealth;++i)tick();
 game::GameInputResult result;
 ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,'D',0,true},result));
 for(int i=0;i<180&&s.actor->SequenceAnim!=Sequence::Deployed;++i)tick();
 ASSERT_TRUE(s.actor->IsDeployed());ASSERT_STREQ(s.actor->GetWeapon(1)->WeaponType->ID,"MissileLauncher");
 ASSERT_TRUE(s.actor->GetWeapon(1)->WeaponType->Projectile->UseLineTrail);
 click(s.target,2);
 const int before=s.target->Health;
 for(int i=0;i<1200&&s.target->Health==before;++i)tick();
 EXPECT_TRUE(s.sawMissile);EXPECT_TRUE(s.sawTrail);EXPECT_LT(s.target->Health,before);
 // Keep simulating through projectile destruction and detached-tail decay.
 for(int i=0;i<60;++i)tick();
 EXPECT_GE(s.missiles,2);EXPECT_TRUE(s.sawDetached);EXPECT_TRUE(s.sawSprite);EXPECT_TRUE(s.sawTailPixels);
 std::cout<<"GGI missiles="<<s.missiles<<" health="<<before<<"->"<<s.target->Health<<" detached_tail="<<s.sawDetached<<"\n";
 }
 EXPECT_EQ(LineTrail::Array.Count,beforeTrails);
}

TEST(InfantryCombat, DogLeftClickPounceAndReturn) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapViewHandle* view;InfantryClass* dog=nullptr;InfantryClass* victim=nullptr;game::MapObjectId victimId{},dogId{};ObjectClass* picking=nullptr;Point2D point{};bool found=false;}s{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"ADOG")&&actor->Owner->IsControlledByCurrentPlayer()){s.dog=actor;break;}
  ASSERT_NE(s.dog,nullptr);ASSERT_NE(s.dog->ParasiteImUsing,nullptr);double nearest=1e30;
  for(auto* candidate:InfantryClass::Array)if(!s.dog->Owner->IsAlliedWith(candidate)&&s.dog->MouseOverObject(candidate,false)==Action::Attack){
   const auto delta=candidate->Location-s.dog->Location;const double distance=double(delta.X)*delta.X+double(delta.Y)*delta.Y;
   if(distance<nearest){s.victim=candidate;nearest=distance;}
  }
  ASSERT_NE(s.victim,nullptr);s.victimId=game::object_id(*s.view->world,s.victim);
  s.dogId=game::object_id(*s.view->world,s.dog);
  // A controlled pounce target; distant armed units must not kill the dog
  // before it reaches the target. Retaliation is verified in its own suite.
  for(auto* unit:UnitClass::Array){unit->SetTarget(nullptr);unit->ForceMission(Mission::Sleep);}
  for(auto* actor:InfantryClass::Array)if(actor!=s.dog){actor->SetTarget(nullptr);actor->ForceMission(Mission::Sleep);}

 },&s));ASSERT_NE(s.dog,nullptr);ASSERT_NE(s.victim,nullptr);
 const auto click=[&](ObjectClass* target){
  const auto center=TacticalClass::CoordsToScreen(target->Location);ASSERT_TRUE(game::center_map_view(*view.view,center.X,center.Y));s.picking=target;s.found=false;
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto& world=*s.view->world;game::rebuild_world_sprites(world);
   for(const auto& sprite:world.impl->sprites)if(sprite.owner==s.picking&&!sprite.shadow)
    for(int y=-25;y<=10&&!s.found;++y)for(int x=-16;x<=16&&!s.found;++x){Point2D point{sprite.position.X+x,sprite.position.Y+y};
     if(game::pick_world_object(world,point)==s.picking){s.point=point;s.found=true;}}
  },&s));ASSERT_TRUE(s.found);game::GameInputResult result;
  for(bool down:{true,false})ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,s.point.X,s.point.Y,1,0,down},result));
 };
 click(s.dog);ASSERT_TRUE(s.dog->IsSelected);click(s.victim);
 const int lastFire=s.dog->LastFireBulletFrame;bool removed=false;
 for(int i=0;i<3600;++i){
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);game::update_map_world(*s.view->world);++Unsorted::CurrentFrame;},&s));
  game::MapObjectSnapshot dog{};ASSERT_TRUE(game::get_map_object(*view.view,s.dogId,dog));
  game::MapObjectSnapshot victim{};removed=!game::get_map_object(*view.view,s.victimId,victim);
  if(removed)break;
 }
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  std::cout<<"DOG state health="<<s.dog->Health<<" mission="<<int(s.dog->CurrentMission)<<" sequence="<<int(s.dog->SequenceAnim)
   <<" target="<<s.dog->Target<<" selected="<<s.dog->IsSelected<<" pos="<<s.dog->Location.X<<","<<s.dog->Location.Y
   <<" destination="<<s.dog->Destination<<" moving="<<s.dog->Locomotor->Is_Moving()<<"\n";
  if(auto* target=game::resolve_map_object(*s.view->world,s.victimId))std::cout<<"victim="<<target->Health<<" fire_error="<<int(s.dog->GetFireError(target,0,true))<<" at="<<target->Location.X<<","<<target->Location.Y<<"\n";
 },&s));
 // Infection and lethal damage may both occur within one host tick. Assert
 // the observable return contract, not a transient sampled between ticks.
 EXPECT_NE(s.dog->LastFireBulletFrame,lastFire);EXPECT_TRUE(removed);
 EXPECT_TRUE(s.dog->IsAlive);EXPECT_TRUE(s.dog->IsOnMap);EXPECT_FALSE(s.dog->InLimbo);
 EXPECT_TRUE(s.dog->IsSelected);EXPECT_EQ(s.dog->ParasiteImUsing->Victim,nullptr);
}

TEST(InfantryCombat, LeftClickContinuousAttackAndDeploy) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapViewHandle* view;InfantryClass* actor=nullptr;InfantryClass* victim=nullptr;game::MapObjectId victimId{};Point2D center{},pick{};bool found=false;}s{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* gi:InfantryClass::Array)if(!std::strcmp(gi->Type->ID,"E1")&&gi->Owner->IsControlledByCurrentPlayer()){s.actor=gi;break;}
  ASSERT_NE(s.actor,nullptr);double distance=1e30;
  for(auto* candidate:InfantryClass::Array)if(candidate!=s.actor&&!s.actor->Owner->IsAlliedWith(candidate)){
   const auto delta=candidate->Location-s.actor->Location;const double d=double(delta.X)*delta.X+double(delta.Y)*delta.Y;
   if(d<distance&&s.actor->MouseOverObject(candidate,false)==Action::Attack){s.victim=candidate;distance=d;}
  }
  ASSERT_NE(s.victim,nullptr);s.victimId=game::object_id(*s.view->world,s.victim);
  std::cout<<"GI input target "<<s.victim->Type->ID<<" distance "<<std::sqrt(distance)/256.0<<" cells\n";
  // Keep the click/deploy target stationary throughout the preceding building
  // attack. Autonomous combat and retaliation have separate coverage.
  for(auto* unit:UnitClass::Array){unit->SetTarget(nullptr);unit->ForceMission(Mission::Sleep);}
  for(auto* actor:InfantryClass::Array)if(actor!=s.actor){actor->SetTarget(nullptr);actor->ForceMission(Mission::Sleep);}
 },&s));
 ASSERT_NE(s.actor,nullptr);ASSERT_NE(s.victim,nullptr);
 const auto click=[&](ObjectClass* target,unsigned modifiers=0){
  s.center=TacticalClass::CoordsToScreen(target->Location);ASSERT_TRUE(game::center_map_view(*view.view,s.center.X,s.center.Y));
  struct Pick{State* state;ObjectClass* target;}pick{&s,target};s.found=false;
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& c=*static_cast<Pick*>(p);auto& s=*c.state;auto& world=*s.view->world;
   game::rebuild_world_sprites(world);
   for(const auto& sprite:world.impl->sprites)if(sprite.owner==c.target&&!sprite.shadow)
    for(int y=-120;y<=10&&!s.found;++y)for(int x=-70;x<=70&&!s.found;++x){
     Point2D pt{sprite.position.X+x,sprite.position.Y+y};if(game::pick_world_object(world,pt)==c.target){s.pick=pt;s.found=true;}
    }
  },&pick));ASSERT_TRUE(s.found);
  game::GameInputResult result;
  for(bool down:{true,false})ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,s.pick.X,s.pick.Y,1,modifiers,down},result));
 };
 click(s.actor);ASSERT_TRUE(s.actor->IsSelected);
 const auto tick=[&](int count){struct Tick{State* state;int count;}t{&s,count};
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& t=*static_cast<Tick*>(p);for(int i=0;i<t.count;++i){game::update_map_world(*t.state->view->world);++Unsorted::CurrentFrame;}},&t));
 };
 game::GameInputResult result;
 BuildingClass* factory=nullptr;
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto** found=static_cast<BuildingClass**>(p);
  for(auto* b:BuildingClass::Array)if(!std::strcmp(b->Type->ID,"GAWEAP")){*found=b;break;}
 },&factory));ASSERT_NE(factory,nullptr);
 // A real click must execute the acknowledgement Flash before any shot.
 // Direct Fire/ReceiveDamage tests skipped this EventClass dependency.
 click(factory,2);tick(1);EXPECT_EQ(s.actor->Target,factory);EXPECT_GT(factory->Flashing.DurationRemaining,0);
 const int factoryHealth=factory->Health;
 for(int i=0;i<1200&&factory->Health==factoryHealth;++i)tick(1);
 EXPECT_LT(factory->Health,factoryHealth);
 struct Civilian{InfantryClass* actor;BuildingClass* building=nullptr;game::MapObjectId id{};game::MapWorld* world;}civilian{s.actor,nullptr,{},view.view->world.get()};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& c=*static_cast<Civilian*>(p);double nearest=1e30;
  for(auto* b:BuildingClass::Array)if(b->Type->CanBeOccupied&&!b->Type->Immune&&b->Health>0&&!b->GetOccupantCount()){
   const auto delta=b->Location-c.actor->Location;const double distance=double(delta.X)*delta.X+double(delta.Y)*delta.Y;
   if(distance<nearest){c.building=b;nearest=distance;}
  }
  ASSERT_NE(c.building,nullptr);c.id=game::object_id(*c.world,c.building);
  std::cout<<"GI click-to-destroy "<<c.building->Type->ID<<" full health "<<c.building->Health<<"\n";
 },&civilian));ASSERT_NE(civilian.building,nullptr);
 click(civilian.building,2);bool buildingGone=false;
 for(int i=0;i<12000;++i){tick(1);game::MapObjectSnapshot snapshot{};
  if(!game::get_map_object(*view.view,civilian.id,snapshot)){buildingGone=true;break;}
 }
 EXPECT_TRUE(buildingGone);EXPECT_NE(s.actor->Target,civilian.building);
 tick(30); // cleanup and target reuse after the destroyed foundation is gone
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);s.actor->SetTarget(nullptr);s.actor->SetDestination(nullptr,true);s.actor->StopMoving();s.actor->ForceMission(Mission::Guard);},&s));
 tick(90);
 ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,'D',0,true},result));tick(90);
 EXPECT_EQ(s.actor->SequenceAnim,Sequence::Deployed);EXPECT_STREQ(s.actor->GetWeapon(s.actor->SelectWeapon(nullptr))->WeaponType->ID,"Para");
 click(s.actor);tick(90);EXPECT_EQ(s.actor->SelectWeapon(nullptr),0);EXPECT_STREQ(s.actor->GetWeapon(0)->WeaponType->ID,"M60");
 click(s.victim);ASSERT_TRUE(s.actor->IsSelected);
 const int initialHealth=s.victim->Health;int hits=0,lastHealth=initialHealth;bool gone=false,deployedFire=false;
 for(int i=0;i<2400;++i){tick(1);game::MapObjectSnapshot snapshot{};
  if(!game::get_map_object(*view.view,s.victimId,snapshot)){gone=true;break;}
  if(snapshot.health<lastHealth){++hits;lastHealth=snapshot.health;
   if(hits==1)ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::key,0,0,'D',0,true},result));
  }
  deployedFire=deployedFire||s.actor->SequenceAnim==Sequence::DeployedFire;
 }
 EXPECT_GE(hits,2);EXPECT_TRUE(gone)<<"mission="<<int(s.actor->CurrentMission)<<" target="<<s.actor->Target<<" destination="<<s.actor->Destination;
 EXPECT_TRUE(deployedFire);EXPECT_STREQ(s.actor->GetWeapon(s.actor->SelectWeapon(nullptr))->WeaponType->ID,"Para");
 // Deployed Guard can acquire the next nearby enemy in the same tick; the
 // expired victim must never remain the target.
 EXPECT_NE(s.actor->Target,s.victim);
}

TEST(InfantryDisplay, OriginalHoverExtrasDispatch) {
 struct Actor:InfantryClass {
  mutable int calls=0;mutable bool hoverArgument=false;
  bool disguised=false;HouseClass* disguiseHouse=nullptr;
  Actor(InfantryTypeClass* type,HouseClass* house):InfantryClass(type,house){}
  void DrawHealthBar(Point2D*,RectangleStruct*,bool hover)const override{++calls;hoverArgument=hover;}
  bool IsDisguisedAs(HouseClass*)const override{return disguised;}
  HouseClass* GetDisguiseHouse(bool allies)const override{EXPECT_TRUE(allies);return disguiseHouse;}
 };
 HouseTypeClass country("HOVER_COUNTRY");HouseClass owner(&country);InfantryTypeClass type("HOVER_GI");
 Actor actor(&type,&owner);game::TypeDrawingContext drawing{};SHPStruct pips;
 const auto draw=[&]{
  actor.calls=0;game::BuildingHealthDrawing frame{drawing,&pips};
  EXPECT_EQ(game::draw_techno_extras(actor,frame,{50,50},{0,0,100,100}),game::DrawingStatus::skipped);
 };
 draw();EXPECT_EQ(actor.calls,0);
 actor.IsMouseHovering=true;draw();EXPECT_EQ(actor.calls,1);EXPECT_TRUE(actor.hoverArgument);
 actor.IsSelected=true;draw();EXPECT_EQ(actor.calls,1);EXPECT_FALSE(actor.hoverArgument);
 actor.IsSelected=false;actor.IsMouseHovering=false;draw();EXPECT_EQ(actor.calls,0);
 actor.IsMouseHovering=true;actor.IsSinking=true;draw();EXPECT_EQ(actor.calls,0);
 actor.IsSinking=false;actor.disguised=true;draw();EXPECT_EQ(actor.calls,0);
 actor.disguiseHouse=&owner;draw();EXPECT_EQ(actor.calls,1);EXPECT_TRUE(actor.hoverArgument);
 actor.IsMouseHovering=false;
}

TEST(InfantryCombat, OriginalGIWeaponsAndDeployMission) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 const int bulletCount=BulletTypeClass::Array.Count,warheadCount=WarheadTypeClass::Array.Count;
 auto restoreTypes=ra2::test::scope_exit([&]{
  while(BulletTypeClass::Array.Count>bulletCount)delete BulletTypeClass::Array[BulletTypeClass::Array.Count-1];
  while(WarheadTypeClass::Array.Count>warheadCount)delete WarheadTypeClass::Array[WarheadTypeClass::Array.Count-1];
 });
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& view=*static_cast<game::MapViewHandle*>(p);auto& world=*view.world;
  struct Data{game::MapWorld* world;InfantryTypeClass* type=nullptr;}input{&world};
  auto services=world.type_services;services.combat_unavailable=false;
  EXPECT_EQ(game::with_type_resources(services,[](void* p){auto& c=*static_cast<Data*>(p);
   c.type=InfantryTypeClass::Find("E1");ASSERT_NE(c.type,nullptr);
   ASSERT_TRUE(c.type->LoadFromINI(&c.world->rules_ini));
   for(int elite=0;elite<2;++elite)for(int index=0;index<2;++index){
    auto* weapon=(elite?c.type->EliteWeapon[index]:c.type->Weapon[index]).WeaponType;
    ASSERT_NE(weapon,nullptr);ASSERT_TRUE(weapon->LoadFromINI(&c.world->rules_ini));
    ASSERT_NE(weapon->Projectile,nullptr);ASSERT_NE(weapon->Warhead,nullptr);
    ASSERT_TRUE(weapon->Projectile->LoadFromINI(&c.world->rules_ini));
    ASSERT_TRUE(weapon->Warhead->LoadFromINI(&c.world->rules_ini));
    weapon->CalculateSpeed();
   }
  },&input),game::TypeResourceStatus::complete);
  auto* type=input.type;ASSERT_NE(type,nullptr);ASSERT_TRUE(type->DeployFire);ASSERT_TRUE(type->Deployer);
  EXPECT_STREQ(type->Weapon[0].WeaponType->ID,"M60");EXPECT_STREQ(type->Weapon[1].WeaponType->ID,"Para");
  EXPECT_EQ(type->Weapon[0].WeaponType->Damage,15);EXPECT_EQ(type->Weapon[1].WeaponType->Damage,25);
  EXPECT_EQ(type->Weapon[0].WeaponType->Range,4*256);EXPECT_EQ(type->Weapon[1].WeaponType->Range,5*256);
  EXPECT_EQ(type->Weapon[0].WeaponType->ROF,20);EXPECT_EQ(type->Weapon[1].WeaponType->ROF,15);
  InfantryClass* actor=nullptr;for(auto* item:InfantryClass::Array)if(item->Type==type&&item->Owner->IsControlledByCurrentPlayer()){actor=item;break;}
  ASSERT_NE(actor,nullptr);
  // Real original-class gates, using actual GI weapons and map rules.
  ASSERT_GT(RulesClass::Instance->ElevationIncrement,0);
  EXPECT_EQ(actor->VisualCharacter(true,actor->Owner),VisualType::Normal);
  actor->RearmTimer.Start(0);
  EXPECT_EQ(actor->GetFireError(actor,0,true),FireError::OK);
  actor->RearmTimer.Start(3);EXPECT_EQ(actor->GetFireError(actor,0,true),FireError::REARM);
  actor->RearmTimer.Start(0);
  const int ammo=actor->Ammo;actor->Ammo=0;EXPECT_EQ(actor->GetFireError(actor,0,true),FireError::AMMO);actor->Ammo=ammo;
  actor->SpeedPercentage=0.5;EXPECT_EQ(actor->GetFireError(actor,0,true),FireError::MOVING);actor->SpeedPercentage=0.0;
  actor->EMPLockRemaining=1;EXPECT_EQ(actor->GetFireError(actor,0,true),FireError::CANT);actor->EMPLockRemaining=0;
  actor->CloakState=CloakState::Cloaked;
  EXPECT_EQ(actor->VisualCharacter(true,nullptr),VisualType::Hidden);
  actor->CloakState=CloakState::Uncloaked;
  EXPECT_EQ(actor->GetFireError(nullptr,0,true),FireError::ILLEGAL);
  // At 4.5 cells the original M60 cannot reach, while Para can. This calls
  // the coordinate overload so the fixture need not relocate a map object.
  bool openRay=false;
  for(const auto offset:{CoordStruct{1152,0,0},CoordStruct{-1152,0,0},CoordStruct{0,1152,0},CoordStruct{0,-1152,0}}) {
   auto from=actor->GetCoords();from.X+=offset.X;from.Y+=offset.Y;
   // The actual map contains cliffs; only compare the weapon ranges along
   // a ray which the original Para trajectory query confirms is open.
   if(!actor->IsCloseEnough(from,actor,type->Weapon[1].WeaponType))continue;
   EXPECT_FALSE(actor->IsCloseEnough(from,actor,type->Weapon[0].WeaponType));openRay=true;break;
  }
  EXPECT_TRUE(openRay);
  auto* distant=InfantryClass::Array[InfantryClass::Array.Count-1];
  ASSERT_NE(distant,actor);
  EXPECT_EQ(actor->GetFireError(distant,0,false),FireError::OK);
  EXPECT_EQ(actor->GetFireError(distant,0,true),FireError::RANGE);
  const int beforeBullets=BulletClass::Array.Count;
  const LONG beforeReferences=Game::COMReferenceCount;
  auto* weapon=type->Weapon[0].WeaponType;
  auto* bullet=weapon->Projectile->CreateBullet(distant,actor,weapon->Damage,weapon->Warhead,weapon->Speed,false);
  ASSERT_NE(bullet,nullptr);
  auto releaseBullet=ra2::test::scope_exit([&]{if(bullet)bullet->Release();});
  EXPECT_EQ(BulletClass::Array.Count,beforeBullets+1);EXPECT_EQ(Game::COMReferenceCount,beforeReferences+1);
  EXPECT_EQ(bullet->WhatAmI(),AbstractType::Bullet);EXPECT_EQ(bullet->Owner,actor);EXPECT_EQ(bullet->Target,distant);
  EXPECT_EQ(bullet->Health,15);EXPECT_EQ(bullet->DamageMultiplier,256);EXPECT_TRUE(bullet->InLimbo);
  bullet->PointerExpired(distant,false);EXPECT_EQ(bullet->Target,MapClass::Instance.GetCellAt(distant->GetCoords()));
  // Exercise the actual invisible GI projectile placement, not a host hit.
  const int beforeLogic=LogicClass::Instance.Count;
  const auto layer=bullet->InWhichLayer();
  const int beforeLayer=DisplayClass::ObjectsInLayers[int(layer)].Count;
  bullet->SetWeaponType(weapon);
  const auto shotFrom=actor->Location,shotAt=bullet->Target->GetCenterCoords();
  EXPECT_EQ(bullet->GetType(),weapon->Projectile);
  ASSERT_TRUE(bullet->MoveTo(shotFrom,{1.0,0.0,0.0}));
  EXPECT_EQ(bullet->SourceCoords,shotFrom);EXPECT_EQ(bullet->TargetCoords,shotAt);
  EXPECT_EQ(bullet->Location,shotAt);EXPECT_EQ(bullet->Speed,0);
  EXPECT_EQ(bullet->Velocity.X,0.0);EXPECT_EQ(bullet->Velocity.Y,0.0);EXPECT_EQ(bullet->Velocity.Z,0.0);
  EXPECT_TRUE(bullet->IsOnMap);EXPECT_FALSE(bullet->InLimbo);EXPECT_TRUE(bullet->IsInLogic);
  EXPECT_EQ(bullet->Data.Location,shotAt);EXPECT_EQ(bullet->Data.Distance,0);
  EXPECT_EQ(LogicClass::Instance.Count,beforeLogic+1);
  EXPECT_EQ(DisplayClass::ObjectsInLayers[int(layer)].Count,beforeLayer+1);
  EXPECT_EQ(bullet->Release(),0u);bullet=nullptr;
  EXPECT_EQ(LogicClass::Instance.Count,beforeLogic);
  EXPECT_EQ(DisplayClass::ObjectsInLayers[int(layer)].Count,beforeLayer);
  actor->SetTarget(distant);
  auto* fired=actor->Fire(distant,0);
  ASSERT_NE(fired,nullptr);EXPECT_EQ(fired->Owner,actor);EXPECT_EQ(fired->WeaponType,weapon);
  EXPECT_EQ(fired->Location,distant->GetCenterCoords());EXPECT_TRUE(fired->IsInLogic);
  EXPECT_GT(actor->RearmTimer.GetTimeLeft(),0);
  const int victimHealth=distant->Health;
  fired->Update();
  EXPECT_LT(distant->Health,victimHealth);
  EXPECT_FALSE(fired->IsAlive);EXPECT_TRUE(fired->InLimbo);
  AbstractClass::RemoveAllInactive();
  actor->SetTarget(nullptr);actor->RearmTimer.Start(0);
  EXPECT_EQ(BulletClass::Array.Count,beforeBullets);EXPECT_EQ(Game::COMReferenceCount,beforeReferences);
  const auto originalRandom=ScenarioClass::Instance->Random;
  auto expectedRandom=originalRandom;
  const int expectedROF=int(20.0*actor->Owner->ROFMultiplier+expectedRandom.RandomRanged(0,2));
  actor->CurrentBurstIndex=type->Weapon[0].WeaponType->Burst;
  EXPECT_EQ(actor->GetROF(0),expectedROF);
  actor->CurrentBurstIndex=0;ScenarioClass::Instance->Random=originalRandom;
  // The already-verified factory ground is an explicit attack destination.
  // Query-only Foot search must not issue movement; its infantry override
  // intentionally has the original different query semantics.
  auto* ground=MapClass::Instance.GetCellAt(CellStruct{93,108});
  actor->SetTarget(ground);actor->ForceMission(Mission::Attack);
  const auto before=actor->Location;auto* oldDestination=actor->Destination;
  auto* position=actor->FootClass::ApproachTarget(1);
  ASSERT_NE(position,nullptr);EXPECT_EQ(actor->Destination,oldDestination);EXPECT_EQ(actor->Location,before);
  EXPECT_NE(position,ground);
  actor->SetTarget(nullptr);actor->ForceMission(Mission::Guard);
  ASSERT_TRUE(actor->ClickedMission(Mission::Attack,ground,nullptr,nullptr));
  const int savedFrame=Unsorted::CurrentFrame;
  auto restoreFrame=ra2::test::scope_exit([&]{Unsorted::CurrentFrame=savedFrame;});
  for(int step=0;step<20;++step){
   ASSERT_FALSE(actor->IsCloseEnough(ground,0)); // this block isolates attack approach
   game::update_map_world(world);++Unsorted::CurrentFrame;
  }
  EXPECT_NE(actor->Location,before);EXPECT_EQ(actor->Target,ground);EXPECT_EQ(actor->CurrentMission,Mission::Attack);
  actor->SetTarget(nullptr);actor->SetDestination(nullptr,true);actor->StopMoving();actor->ForceMission(Mission::Sleep);
  // Stop_Moving retains the current reserved subcell until the walk step
  // finishes. Let the original driver finish it before testing deployment.
  for(int step=0;step<120 && actor->Locomotor->Is_Moving();++step){game::update_map_world(world);++Unsorted::CurrentFrame;}
  ASSERT_FALSE(actor->Locomotor->Is_Moving());actor->ForceMission(Mission::Guard);
  const auto finish=[&]{actor->Animation.Value=type->Sequence->GetSequence(actor->SequenceAnim).CountFrames;actor->Doing_AI();};
  actor->PlayAnim(Sequence::Ready,true,false);EXPECT_EQ(actor->SelectWeapon(nullptr),0);
  // Use the original object-click command branch and Event wire, not a host
  // sequence assignment. Continuous firing is checked by the input case above.
  ASSERT_TRUE(actor->FootClass::ObjectClickedAction(Action::Self_Deploy,actor,false));
  ASSERT_EQ(EventClass::OutList.Count,1);EventClass order(EventClass::OutList.First());EventClass::OutList.Next();
  EXPECT_EQ(int(order.MegaMission.Mission),int(Mission::Unload));order.Execute();ASSERT_TRUE(actor->NextMission());
  EXPECT_EQ(actor->Mission_Unload(),450);EXPECT_EQ(actor->SequenceAnim,Sequence::Deploy);EXPECT_EQ(actor->SelectWeapon(nullptr),1);
  finish();EXPECT_EQ(actor->SequenceAnim,Sequence::Deployed);EXPECT_EQ(actor->Uncrushable,!type->DeployedCrushable);
  EXPECT_STREQ(actor->GetWeapon(actor->SelectWeapon(nullptr))->WeaponType->ID,"Para");
  InfantryClass* nearby=nullptr;
  for(auto* candidate:InfantryClass::Array)if(candidate!=actor && actor->IsCloseEnough(candidate,1)){nearby=candidate;break;}
  ASSERT_NE(nearby,nullptr);actor->SetTarget(nearby);actor->ForceMission(Mission::Attack);
  EXPECT_GT(actor->Mission_Attack(),0);EXPECT_EQ(actor->Target,nearby);EXPECT_EQ(actor->Destination,nullptr);
  // Enter the actual deployed firing action, stopping before the launch
  // frame selected by the available secondary sequence: this checks the
  // actual action/gate, not a bullet/damage simulation.
  ASSERT_GT(type->Sequence->GetSequence(Sequence::SecondaryFire).CountFrames?type->SecondaryFire:type->FireUp,0);
  EXPECT_EQ(actor->GetFireError(nearby,1,true),FireError::OK);
  actor->Firing_AI();EXPECT_TRUE(actor->IsFiring);EXPECT_EQ(actor->SequenceAnim,Sequence::DeployedFire);
  actor->SetTarget(nullptr);
  actor->Veterancy.Veterancy=2.0f;EXPECT_STREQ(actor->GetWeapon(actor->SelectWeapon(nullptr))->WeaponType->ID,"ParaE");
  EXPECT_EQ(actor->Mission_Unload(),450);EXPECT_EQ(actor->SequenceAnim,Sequence::Undeploy);EXPECT_EQ(actor->SelectWeapon(nullptr),0);
  finish();EXPECT_EQ(actor->SequenceAnim,Sequence::Ready);EXPECT_FALSE(actor->Uncrushable);
  EXPECT_STREQ(actor->GetWeapon(actor->SelectWeapon(nullptr))->WeaponType->ID,"M60E");
  // Check the actual event's target and mission before the fatal-hit check.
  auto* victim=InfantryClass::Array[InfantryClass::Array.Count-1];
  ASSERT_TRUE(actor->FootClass::ObjectClickedAction(Action::Attack,victim,false));
  ASSERT_EQ(EventClass::OutList.Count,1);auto& attack=EventClass::OutList.First();
  EXPECT_EQ(int(attack.MegaMission.Mission),int(Mission::Attack));EXPECT_EQ(attack.MegaMission.Target.As_Abstract(),victim);
  EventClass::OutList.Next();
  // Complete the native hit -> death sequence -> Limbo -> deferred-delete
  // chain, using the map's actual object and logic scheduler.
  const auto victimId=game::object_id(world,victim);
  auto* victimCell=victim->GetCell();auto* victimOwner=victim->Owner;
  const int ownedBefore=victimOwner->OwnedInfantryTypes.GetItemCount(victim->Type->ArrayIndex);
  const int victimType=victim->Type->ArrayIndex;
  actor->Veterancy.Veterancy=0.0f; // restore the weapon-query fixture's forced elite rank
  victim->Health=1;actor->RearmTimer.Start(0);actor->SetTarget(victim);
  auto* fatal=actor->Fire(victim,actor->SelectWeapon(victim));ASSERT_NE(fatal,nullptr);
  fatal->Update();EXPECT_EQ(victim->Health,0);EXPECT_TRUE(victim->IsPlayingDeathSequence());
  for(int i=0;i<180&&game::resolve_map_object(world,victimId);++i){game::update_map_world(world);++Unsorted::CurrentFrame;}
  EXPECT_EQ(game::resolve_map_object(world,victimId),nullptr);
  EXPECT_EQ(victimOwner->OwnedInfantryTypes.GetItemCount(victimType),ownedBefore-1);
  for(auto* link=victimCell->FirstObject;link;link=link->NextObject)EXPECT_NE(link,victim);
  // The same GI projectile must enter Building.ReceiveDamage rather than an
  // EXE trampoline. Check a normal factory hit and its fatal removal too.
  BuildingClass* factory=nullptr;for(auto* building:BuildingClass::Array)if(!std::strcmp(building->Type->ID,"GAWEAP")){factory=building;break;}
  ASSERT_NE(factory,nullptr);const auto factoryId=game::object_id(world,factory);
  const int factoryHealth=factory->Health;actor->SetTarget(factory);actor->RearmTimer.Start(0);
  auto* factoryShot=actor->Fire(factory,0);ASSERT_NE(factoryShot,nullptr);factoryShot->Update();
  EXPECT_LT(factory->Health,factoryHealth);AbstractClass::RemoveAllInactive();
  factory->Health=1;actor->RearmTimer.Start(0);
  factoryShot=actor->Fire(factory,0);ASSERT_NE(factoryShot,nullptr);factoryShot->Update();
  for(int i=0;i<90&&game::resolve_map_object(world,factoryId);++i){game::update_map_world(world);++Unsorted::CurrentFrame;}
  EXPECT_EQ(game::resolve_map_object(world,factoryId),nullptr);
 },view.view));
}

TEST(InfantryDisplay, FirstMapHoverStateLifetime) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct Check{game::MapViewHandle* view;InfantryClass* actor=nullptr;Point2D center{},pick{};bool picked=false;}check{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& c=*static_cast<Check*>(p);
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"E1")&&actor->Owner->IsControlledByCurrentPlayer()){c.actor=actor;break;}
  ASSERT_NE(c.actor,nullptr);c.center=TacticalClass::CoordsToScreen(c.actor->Location);
 },&check));
 ASSERT_TRUE(game::center_map_view(*view.view,check.center.X,check.center.Y));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& c=*static_cast<Check*>(p);auto& world=*c.view->world;
  game::rebuild_world_sprites(world);
  for(const auto& sprite:world.impl->sprites)if(sprite.owner==c.actor&&!sprite.shadow)
   for(int y=-20;y<=10&&!c.picked;++y)for(int x=-12;x<=12&&!c.picked;++x){
    Point2D point{sprite.position.X+x,sprite.position.Y+y};
    if(game::pick_world_object(world,point)==c.actor){c.pick=point;c.picked=true;}
   }
 },&check));
 ASSERT_TRUE(check.picked);
 const auto input=[&](game::GameInputKind kind,Point2D at){game::GameInputResult result;EXPECT_TRUE(game::submit_game_input(*view.view,{kind,at.X,at.Y,0,0,false},result));};
 input(game::GameInputKind::pointer_move,check.pick);EXPECT_TRUE(check.actor->IsMouseHovering);
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& c=*static_cast<Check*>(p);
  c.actor->IsMouseHovering=false;game::refresh_map_world_hover(*c.view->world);EXPECT_TRUE(c.actor->IsMouseHovering);
 },&check));
 input(game::GameInputKind::pointer_move,{1279,719});EXPECT_FALSE(check.actor->IsMouseHovering);
 for(auto kind:{game::GameInputKind::pointer_leave,game::GameInputKind::focus_lost}){
  input(game::GameInputKind::pointer_move,check.pick);EXPECT_TRUE(check.actor->IsMouseHovering);
  input(kind,check.pick);EXPECT_FALSE(check.actor->IsMouseHovering);EXPECT_EQ(view.view->world->impl->hover.value,0u);
 }
 input(game::GameInputKind::pointer_move,check.pick);EXPECT_TRUE(check.actor->IsMouseHovering);
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& c=*static_cast<Check*>(p);
  game::detach_map_object(*c.actor,false);EXPECT_FALSE(c.actor->IsMouseHovering);EXPECT_EQ(c.view->world->impl->hover.value,0u);
  game::refresh_map_world_hover(*c.view->world);EXPECT_FALSE(c.actor->IsMouseHovering);
 },&check));
}

TEST(InfantryMovement, OriginalIdleAndMissionReadinessCorpus) {
 auto runtime=game::default_scenario_runtime();runtime.session_mode=[](void*)noexcept{return 0;};
 ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void*){
  struct Actor:InfantryClass {
   std::array<int,6> calls{0,-1,0,0,0,0};bool armed=false,recordIdle=false;
   Actor(InfantryTypeClass* t,HouseClass* h):InfantryClass(t,h){}
   bool QueueMission(Mission m,bool start)override{++calls[0];calls[1]=int(m);calls[2]=start;return false;}
   void SetDestination(AbstractClass* to,bool immediate)override{++calls[3];calls[4]=immediate;Destination=to;}
   void SetTarget(AbstractClass* to)override{Target=to;}
   void Scatter(const CoordStruct& coord,bool forced,bool noKidding)override{
    ++calls[5];EXPECT_EQ(coord,CoordStruct::Empty);EXPECT_TRUE(forced);EXPECT_FALSE(noKidding);
   }
   bool IsArmed()const override{return armed;}
   bool EnterIdleMode(bool initial,bool resume)override{
    if(!recordIdle)return InfantryClass::EnterIdleMode(initial,resume);
    ++calls[0];EXPECT_FALSE(initial);EXPECT_TRUE(resume);return false;
   }
   bool PlayAnim(Sequence action,bool force,bool random)override{
    calls[1]=int(action);EXPECT_FALSE(force);EXPECT_FALSE(random);return true;
   }
  };
  struct Driver:WalkLocomotionClass {bool moving=false;bool YRPP_STDCALL Is_Moving_Now()override{return moving;}} driver;
  RulesClass rules;auto* previous=RulesClass::Instance;RulesClass::Instance=&rules;rules.GuardArea=3;
  std::array<MissionControlClass,32> controls;std::copy_n(MissionControlClass::Array,32,controls.begin());
  auto restore=ra2::test::scope_exit([&]{RulesClass::Instance=previous;std::copy(controls.begin(),controls.end(),MissionControlClass::Array);});
  HouseTypeClass country("IDLE_COUNTRY");HouseClass owner(&country);InfantryTypeClass type("IDLE_GI");
  ScenarioClass scenario;auto* previousScenario=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
  auto restoreScenario=ra2::test::scope_exit([&]{ScenarioClass::Instance=previousScenario;});
  Actor gi(&type,&owner);InfantryClass other(&type,&owner);gi.Location={512,768,0};other.Location={1024,1280,0};
  std::ifstream input(RA2_INFANTRY_IDLE_FIXTURE);ASSERT_TRUE(input.good());int mode;unsigned cases=0;
  while(input>>mode){
   int mission,flags;ASSERT_TRUE(bool(input>>mission>>flags));
   gi.CurrentMission=static_cast<Mission>(mission);gi.QueuedMission=Mission::None;
   if(mode==0){
    std::array<int,16> expected;for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
    gi.calls={0,-1,0,0,0,0};gi.Locomotor=nullptr;gi.PlanningPathIdx=-1;
    gi.MegaMission=flags&32768?Mission::AttackMove:Mission::None;
    gi.MegaDestination=(flags&32768)&&(flags&2)?&other:nullptr;gi.MegaTarget=(flags&32768)&&(flags&1)?&other:nullptr;
    gi.HaveAttackMoveTarget=(flags&32768)&&(flags&16);
    gi.SpeedPercentage=0.75;gi.Target=flags&1?&other:nullptr;gi.Destination=flags&2?&other:nullptr;
    gi.NavQueue.Clear();gi.unknown_abstract_array_588.Clear();
    if(flags&4){ASSERT_TRUE(gi.NavQueue.AddItem(&other));ASSERT_TRUE(gi.NavQueue.AddItem(&gi));}
    if(flags&8){ASSERT_TRUE(gi.unknown_abstract_array_588.AddItem(&other));ASSERT_TRUE(gi.unknown_abstract_array_588.AddItem(&gi));}
    gi.ArchiveTarget=flags&16?&other:nullptr;owner.IsHumanPlayer=flags&32;
    gi.Team=flags&64?reinterpret_cast<TeamClass*>(&other):nullptr;type.DefaultToGuardArea=flags&128;
    gi.armed=flags&256;owner.IQLevel2=flags&256?4:1;
    for(auto& control:MissionControlClass::Array){control.Zombie=false;control.Paralyzed=false;}
    if(mission>=0){MissionControlClass::Array[mission].Zombie=flags&512;MissionControlClass::Array[mission].Paralyzed=flags&1024;}
    gi.SlaveOwner=flags&2048?reinterpret_cast<TechnoClass*>(&other):nullptr;
    gi.ShouldScatterInNextIdle=flags&4096;gi.unknown_bool_6B3=flags&8192;gi.unknown_bool_6B1=flags&16384;
    const bool result=gi.EnterIdleMode(false,true);
    const std::array<int,16> actual{result,gi.calls[0],gi.calls[1],gi.calls[2],gi.calls[3],gi.calls[4],gi.calls[5],
     int(gi.Destination!=nullptr),int(gi.ArchiveTarget!=nullptr),gi.unknown_abstract_array_588.Count,gi.NavQueue.Count,
     int(gi.NavQueue.Count>0 && gi.NavQueue[0]==&gi),gi.unknown_bool_6B3,gi.ShouldScatterInNextIdle,int(gi.MegaMission),int(gi.SpeedPercentage*100)};
    ASSERT_EQ(actual,expected)<<"mission "<<mission<<" flags "<<flags;
   }else if(mode==1){
    int sequence,expected;ASSERT_TRUE(bool(input>>sequence>>expected));
    gi.Locomotor=&driver;driver.moving=flags&1;gi.IsFiring=flags&2;gi.IsFallingDown=flags&4;
    gi.Target=flags&8?&other:nullptr;gi.SequenceAnim=static_cast<Sequence>(sequence);
    EXPECT_EQ(gi.ReadyToNextMission(),bool(expected))<<mission<<" "<<sequence<<" "<<flags;
   }else{
    std::array<int,7> expected;for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
    gi.calls={0,-1,0,0,0,0};gi.recordIdle=true;gi.Locomotor=&driver;driver.IsMoving=flags&2;
    gi.CurrentMission=Mission::Move;gi.QueuedMission=flags&4?Mission::Guard:Mission::None;
    gi.SequenceAnim=static_cast<Sequence>(mission);gi.Destination=flags&1?&other:nullptr;
    owner.IsHumanPlayer=flags&8;type.UndeployDelay=flags&16?0:-1;
    SequenceStruct sequence{};sequence.Sequences[int(Sequence::Undeploy)].CountFrames=17;type.Sequence=&sequence;
    MissionControlClass::Array[int(Mission::Move)].Rate=0.016;scenario.Random=Randomizer(99);
    const int result=gi.Mission_Move();type.Sequence=nullptr;
    const std::array<int,7> actual{result,gi.calls[0],gi.calls[1],gi.calls[3],gi.calls[4],int(gi.Destination!=nullptr),scenario.Random.Next1};
    EXPECT_EQ(actual,expected)<<"sequence "<<mission<<" flags "<<flags;
   }
   // These two pointers are identity-only fixture values, not owned objects.
   gi.Locomotor=nullptr;gi.Team=nullptr;gi.SlaveOwner=nullptr;++cases;
  }
  EXPECT_EQ(cases,24464u);
 },nullptr));
}

TEST(InfantryMovement, IdlePiggybackOwnershipAndNavigationMutation) {
 InfantryTypeClass type("IDLE_COM_GI");
 struct Actor:InfantryClass {
  bool clearQueue=false;int destinationCalls=0;
  Actor(InfantryTypeClass* t):InfantryClass(t,nullptr){}
  void SetDestination(AbstractClass* next,bool)override {
   ++destinationCalls;Destination=next;
   if(clearQueue){NavQueue.Clear();unknown_abstract_array_588.Clear();}
  }
 } gi(&type);
 InfantryClass first(&type,nullptr),second(&type,nullptr);
 ASSERT_TRUE(gi.NavQueue.AddItem(&first));ASSERT_TRUE(gi.NavQueue.AddItem(&second));
 gi.clearQueue=true;gi.unknown_bool_6B1=true;gi.HandleNavigationList();
 EXPECT_EQ(gi.Destination,&first);ASSERT_EQ(gi.NavQueue.Count,1);EXPECT_EQ(gi.NavQueue[0],&first);
 gi.Destination=nullptr;gi.NavQueue.Clear();ASSERT_TRUE(gi.NavQueue.AddItem(nullptr));gi.HandleNavigationList();
 EXPECT_EQ(gi.NavQueue.Count,1);EXPECT_EQ(gi.destinationCalls,1);
 ASSERT_TRUE(gi.unknown_abstract_array_588.AddItem(&first));
 EXPECT_TRUE(gi.FootClass::EnterIdleMode(false,false));EXPECT_EQ(gi.unknown_abstract_array_588.Count,0);
 gi.unknown_bool_6B3=false;gi.clearQueue=false;
 auto* current=GameCreate<WalkLocomotionClass>();auto* restored=GameCreate<WalkLocomotionClass>();
 ASSERT_NE(current,nullptr);ASSERT_NE(restored,nullptr);
 current->AddRef();restored->AddRef();current->Link_To_Object(&gi);restored->Link_To_Object(&gi);
 ASSERT_EQ(current->Begin_Piggyback(restored),0);current->AddRef();gi.Locomotor=current;
 gi.SpeedPercentage=0.75;
 EXPECT_TRUE(gi.FootClass::EnterIdleMode(false,false));EXPECT_EQ(gi.Locomotor,static_cast<ILocomotion*>(restored));
 EXPECT_EQ(current->RefCount,1);EXPECT_EQ(restored->RefCount,2);EXPECT_EQ(current->Piggybackee,nullptr);
 EXPECT_DOUBLE_EQ(gi.SpeedPercentage,0.75); // piggyback exit skips the stop-speed branch
 EXPECT_FALSE(gi.FootClass::EnterIdleMode(false,false)); // frame-local idle reentry guard
 gi.Locomotor->Release();gi.Locomotor=nullptr;current->Release();restored->Release();
}

TEST(InfantryMovement, OriginalSpeedHouseVeterancyAndPostureCorpus) {
 const int rounding=std::fegetround();auto restore_rounding=ra2::test::scope_exit([&]{std::fesetround(rounding);});
 std::fesetround(FE_TOWARDZERO);
 RulesClass rules;auto* previous=RulesClass::Instance;RulesClass::Instance=&rules;
 auto restore_rules=ra2::test::scope_exit([&]{RulesClass::Instance=previous;});rules.VeteranSpeed=1.3;
 HouseTypeClass country("SPEED_COUNTRY");HouseClass owner(&country);
 InfantryTypeClass type("SPEED_GI");InfantryClass gi(&type,&owner);
 std::ifstream input(RA2_INFANTRY_SPEED_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0;int speed;
 while(input>>speed){
  int bias,mult,percent,rank,abilities,posture,expected;
  ASSERT_TRUE(bool(input>>bias>>mult>>percent>>rank>>abilities>>posture>>expected));
  constexpr double factors[]{0.5,1.0,1.5};constexpr float biases[]{0.5f,1.0f,1.25f};
  type.Speed=speed;country.SpeedInfantryMult=biases[bias];country.SpeedAircraftMult=country.SpeedUnitsMult=9.0f;
  gi.SpeedMultiplier=factors[mult];gi.SpeedPercentage=percent*0.5;gi.Veterancy.Veterancy=float(rank);
  type.VeteranAbilities[Ability::Faster]=abilities&1;type.EliteAbilities[Ability::Faster]=abilities&2;
  gi.Crawling=posture&1;type.Crawls=posture&2;
  EXPECT_EQ(gi.GetCurrentSpeed(),expected)<<cases;++cases;
 }
 EXPECT_EQ(cases,12960u);
}

TEST(InfantryMovement, MarkUsesOriginalBaseDispatchAndChangeFastPath) {
 struct Actor:InfantryClass {
  mutable std::vector<int> calls;
  Actor(InfantryTypeClass* type):InfantryClass(type,nullptr){}
  int GetThreatValue()const override{calls.push_back(1);return 9;}
  int GetOwningHouseIndex()const override{calls.push_back(2);return 0;}
  CellStruct* GetMapCoords(CellStruct* out)const override{calls.push_back(3);*out={8,6};return out;}
  void MarkForRedraw()override{calls.push_back(4);NeedsRedraw=true;}
  RadioCommand SendToFirstLink(RadioCommand command)override{calls.push_back(5);EXPECT_EQ(command,RadioCommand::RequestRedraw);return RadioCommand::AnswerPositive;}
  Layer InWhichLayer()const override{calls.push_back(6);return Layer::Air;}
 };
 InfantryTypeClass type("MARK_CONTRACT");Actor actor(&type);
 for(int entry=0;entry<3;++entry)for(int mark=-1;mark<=5;++mark)for(int flags=0;flags<16;++flags){
  actor.InLimbo=flags&1;actor.IsOnMap=flags&2;actor.NeedsRedraw=flags&4;actor.IsTether=flags&8;actor.calls.clear();
  const bool change=mark==2,fast=entry==2 && change;
  const bool success=fast || (!actor.InLimbo && (change?(!actor.NeedsRedraw && actor.IsOnMap)
      :((mark==1 || mark==3)?!actor.IsOnMap:mark==0 && actor.IsOnMap)));
  std::vector<int> expected;
  if(!fast && !actor.InLimbo){
   if(!change)expected={1,2,3};
   if(success && mark!=0)expected.push_back(4);
   if(success && entry>=1 && actor.IsTether)expected.push_back(5);
   if(success && entry==2)expected.push_back(6);
  }
  const bool actual=entry==0?actor.ObjectClass::Mark(static_cast<MarkType>(mark)):
      entry==1?actor.TechnoClass::Mark(static_cast<MarkType>(mark)):actor.FootClass::Mark(static_cast<MarkType>(mark));
  EXPECT_EQ(actual,success)<<entry<<" "<<mark<<" "<<flags;
  EXPECT_EQ(actor.calls,expected)<<entry<<" "<<mark<<" "<<flags;
  EXPECT_EQ(actor.InLimbo,bool(flags&1));
  EXPECT_EQ(actor.IsOnMap,success && !change?mark!=0:bool(flags&2));
 }
 actor.IsOnMap=false;actor.InLimbo=true;
}


TEST(InfantryDisplay, TypeSequenceOverlayAndRealLifecycle) {
 const int feet=FootClass::Array.Count,infantry=InfantryClass::Array.Count,techno=TechnoClass::Array.Count;
 InfantryTypeClass type("GI_TEST");CCINIClass rules,art;
 rules.WriteString(type.ID,"Image","GI");rules.WriteString(type.ID,"Strength","125");
 rules.WriteString(type.ID,"Cyborg","yes");rules.WriteString(type.ID,"C4","yes");
 rules.WriteString(type.ID,"DetectionDistance","7");rules.WriteString(type.ID,"Deployer","yes");
 art.WriteString("GI","Sequence","GISequence");art.WriteString("GI","Crawls","no");
 art.WriteString("GI","FireUp","3");art.WriteString("GISequence","Ready","10,3,5");
 art.WriteString("GISequence","SecondaryProne","200,4,6,NE");
 game::TypeResourceServices resources;resources.art=&art;resources.audio_unavailable=true;
 struct Read {InfantryTypeClass& type;CCINIClass& rules;}read{type,rules};
 ASSERT_EQ(game::with_type_resources(resources,[](void* p){auto&r=*static_cast<Read*>(p);EXPECT_TRUE(r.type.LoadFromINI(&r.rules));},&read),game::TypeResourceStatus::complete);
 EXPECT_TRUE(type.DamageSparks);EXPECT_TRUE(type.Infiltrate);EXPECT_FALSE(type.Crawls);EXPECT_EQ(type.FireUp,3);EXPECT_EQ(type.DirectionDistance,7);
 EXPECT_EQ(type.Sequence->Sequences[41].StartFrame,200);EXPECT_EQ(type.Sequence->Sequences[41].Facing,SequenceFacing::NE);
 // Original partial sscanf assignments and missing keys preserve prior fields.
 art.WriteString("GISequence","Ready","12");
 ASSERT_EQ(game::with_type_resources(resources,[](void* p){static_cast<InfantryTypeClass*>(p)->ReadSequence();},&type),game::TypeResourceStatus::complete);
 EXPECT_EQ(type.Sequence->Sequences[0].StartFrame,12);EXPECT_EQ(type.Sequence->Sequences[0].CountFrames,3);
 {
  std::unique_ptr<ObjectClass> object(type.CreateObject(nullptr));auto*gi=dynamic_cast<InfantryClass*>(object.get());ASSERT_NE(gi,nullptr);
  EXPECT_EQ(gi->WhatAmI(),AbstractType::Infantry);EXPECT_EQ(gi->GetType(),&type);EXPECT_EQ(gi->Health,125);
  EXPECT_EQ(FootClass::Array.Count,feet+1);EXPECT_EQ(InfantryClass::Array.Count,infantry+1);EXPECT_EQ(TechnoClass::Array.Count,techno+1);
  EXPECT_EQ(TargetClass(gi).As_Abstract(),gi);EXPECT_TRUE(gi->InLimbo);EXPECT_FALSE(gi->IsInLogic);
  gi->SequenceAnim=Sequence::Ready;gi->Animation.Value=2;
  constexpr int facing_frames[]{7,6,5,4,3,2,1,0};
  for(int i=0;i<8;++i){gi->PrimaryFacing.SetCurrent(DirStruct(static_cast<unsigned short>(i*0x2000)));EXPECT_EQ(gi->GetCurrentFrame(),14+5*facing_frames[i]);}
  gi->SequenceAnim=Sequence::Deploy;EXPECT_TRUE(gi->IsDeployed());gi->SequenceAnim=Sequence::Undeploy;EXPECT_FALSE(gi->IsDeployed());
 }
 EXPECT_EQ(FootClass::Array.Count,feet);EXPECT_EQ(InfantryClass::Array.Count,infantry);EXPECT_EQ(TechnoClass::Array.Count,techno);
}

TEST(InfantryDisplay, OriginalFrameCorpus) {
 InfantryTypeClass type("FRAME_GI");InfantryClass gi(&type,nullptr);
 std::ifstream input(RA2_INFANTRY_FRAME_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0;int action;
 while(input>>action){int start,count,stride,raw,phase,expected;ASSERT_TRUE(bool(input>>start>>count>>stride>>raw>>phase>>expected));
  auto&s=type.Sequence->Sequences[action];s.StartFrame=start;s.CountFrames=count;s.FacingMultiplier=stride;
  gi.SequenceAnim=static_cast<Sequence>(action);gi.Animation.Value=phase;gi.PrimaryFacing.SetCurrent(DirStruct(raw));
  EXPECT_EQ(gi.GetCurrentFrame(),expected)<<cases;++cases;
 }EXPECT_EQ(cases,1008u);
}

TEST(InfantryActions, OriginalActionTransitionsAndRandomConsumption) {
 struct Clock {
  int frame=Unsorted::CurrentFrame, speed=GameOptionsClass::Instance.GameSpeed;
  ScenarioClass* previous=ScenarioClass::Instance;ScenarioClass scenario;
  Clock(){Unsorted::CurrentFrame=500;ScenarioClass::Instance=&scenario;}
  ~Clock(){Unsorted::CurrentFrame=frame;GameOptionsClass::Instance.GameSpeed=speed;ScenarioClass::Instance=previous;}
 } clock;
 struct Actor : InfantryClass {
  CellClass* cell=nullptr;bool flying=false;int stops=0;
  explicit Actor(InfantryTypeClass* type):InfantryClass(type,nullptr){}
  bool IsInAir()const override{return flying;}
  double GetStoragePercentage()const override{return 1.0;}
  CellClass* GetCell()const override{return cell;}
  bool StopMoving()override{++stops;return false;}
 };
 InfantryTypeClass type("ACTION_GI");Actor gi(&type);
 auto* cell=CellClass::Create();ASSERT_NE(cell,nullptr);
 struct CellOwner{CellClass* cell;~CellOwner(){GameDelete(cell);}}cell_owner{cell};
 gi.cell=cell;cell->LandType=LandType::Water;
 type.DeploySound=type.UndeploySound=type.EnterWaterSound=type.LeaveWaterSound=-1;
 for(int action=0;action<42;++action){auto&s=type.Sequence->Sequences[action];s.StartFrame=100+10*action;s.CountFrames=action%7+1;}
 std::ifstream input(RA2_INFANTRY_ACTION_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0;int previous;
 while(input>>previous){
  int requested,force,variant,random,speed,expected[10];
  ASSERT_TRUE(bool(input>>requested>>force>>variant>>random>>speed));for(auto&v:expected)ASSERT_TRUE(bool(input>>v));
  gi.SequenceAnim=static_cast<Sequence>(previous);gi.Animation.Value=19;gi.Animation.Timer.StartTime=31;
  gi.Animation.Timer.TimeLeft=gi.Animation.Rate=7;gi.IsFallingDown=variant&1;type.Crawls=!(variant&2);
  gi.OnBridge=variant&8;type.MovementZone=variant&4?MovementZone::AmphibiousDestroyer:MovementZone::Normal;
  gi.unknown_int_6E8=2;gi.flying=variant&16;gi.PanicDurationLeft=variant&32?200:0;
  gi.SlaveOwner=variant&64?&gi:nullptr;type.Storage=variant&64?10:0;
  gi.Health=variant&128?0:100;gi.Crawling=true;gi.stops=0;
  type.Sequence->Sequences[requested].CountFrames=variant&256?0:requested%7+1;
  GameOptionsClass::Instance.GameSpeed=speed;clock.scenario.Random=Randomizer(99);
  const bool result=gi.PlayAnim(static_cast<Sequence>(requested),force!=0,random!=0);
  const int actual[]{int(result),int(gi.SequenceAnim),gi.Animation.Value,gi.Animation.Timer.StartTime,
      gi.Animation.Timer.TimeLeft,gi.Animation.Rate,int(gi.Crawling),gi.unknown_int_6E8,gi.stops,clock.scenario.Random.Next1};
  for(unsigned i=0;i<10;++i)EXPECT_EQ(actual[i],expected[i])<<"case "<<cases<<" field "<<i<<" previous "<<previous<<" requested "<<requested<<" variant "<<variant;
  type.Sequence->Sequences[requested].CountFrames=requested%7+1;++cases;
 }
 EXPECT_EQ(cases,4368u);
}

TEST(InfantryActions, TargetSwitchRejectsDyingAndRestoresPosture) {
 InfantryTypeClass type("TARGET_ACTION");type.DeploySound=type.UndeploySound=-1;
 for(auto&s:type.Sequence->Sequences)s.CountFrames=3;
 InfantryClass gi(&type,nullptr),other(&type,nullptr),last(&type,nullptr);
 gi.Health=other.Health=last.Health=100;
 gi.SequenceAnim=Sequence::FireUp;gi.IsFiring=true;gi.Crawling=true;
 gi.Target=&last;gi.DirectRockerLinkedUnit=&last;last.DirectRockerLinkedUnit=&gi;
 gi.ShouldLoseTargetNow=0x12345678u;gi.PathDirections[0]=3;
 gi.SetTarget(&other);
 EXPECT_EQ(gi.Target,&other);EXPECT_EQ(gi.SequenceAnim,Sequence::Prone);EXPECT_FALSE(gi.IsFiring);
 EXPECT_EQ(gi.ShouldLoseTargetNow,0x12345600u);EXPECT_EQ(gi.PathDirections[0],-1);
 EXPECT_EQ(gi.DirectRockerLinkedUnit,nullptr);EXPECT_EQ(last.DirectRockerLinkedUnit,nullptr);
 for(auto death:{Sequence::Die1,Sequence::Die5,Sequence::WetDie1,Sequence::AirDeathStart,Sequence::AirDeathFinish}){
  other.SequenceAnim=death;gi.Target=&last;gi.SetTarget(&other);EXPECT_EQ(gi.Target,nullptr);
 }
 other.SequenceAnim=Sequence::Ready;gi.Target=nullptr;gi.SequenceAnim=Sequence::Deployed;type.DeployFire=false;
 gi.SetTarget(&other);EXPECT_EQ(gi.Target,nullptr);
 type.DeployFire=true;gi.SetTarget(&other);EXPECT_EQ(gi.Target,&other);
 gi.CurrentBurstIndex=7;gi.SetTarget(nullptr);EXPECT_EQ(gi.CurrentBurstIndex,0);
 // Elite slots fall back to normal only if their weapon is absent.
 WeaponTypeClass ordinary("TARGET_WEAPON"),elite("TARGET_ELITE");
 type.Weapon[0].WeaponType=&ordinary;type.EliteWeapon[0].WeaponType=&elite;
 EXPECT_EQ(gi.GetWeapon(0)->WeaponType,&ordinary);gi.Veterancy.Veterancy=2.0f;
 EXPECT_EQ(gi.GetWeapon(0)->WeaponType,&elite);type.EliteWeapon[0].WeaponType=nullptr;
 EXPECT_EQ(gi.GetWeapon(0)->WeaponType,&ordinary);EXPECT_EQ(gi.GetWeapon(-1),nullptr);
 type.Storage=10;gi.Tiberium.Tiberium1=0.8f;gi.Tiberium.Tiberium2=0.8f;
 EXPECT_DOUBLE_EQ(gi.GetStoragePercentage(),0.0);gi.Tiberium.Tiberium3=9.9f;
 EXPECT_DOUBLE_EQ(gi.GetStoragePercentage(),0.9);
}

TEST(InfantryActions, OriginalFrameDecisionCorpus) {
 const int rounding=std::fegetround();auto restore_rounding=ra2::test::scope_exit([&]{std::fesetround(rounding);});
 const auto root=std::filesystem::temp_directory_path()/("ra2-frame-decision-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  const int rounding=std::fegetround(),frame=Unsorted::CurrentFrame,init=Unsorted::ScenarioInit;
  auto* previous=ScenarioClass::Instance;ScenarioClass scenario;ScenarioClass::Instance=&scenario;
  auto restore=ra2::test::scope_exit([&]{std::fesetround(rounding);Unsorted::CurrentFrame=frame;Unsorted::ScenarioInit=init;ScenarioClass::Instance=previous;});
  std::fesetround(FE_TOWARDZERO);Unsorted::CurrentFrame=500;Unsorted::ScenarioInit=0;
  struct Actor:InfantryClass {
   int flags=0;mutable std::array<int,8> calls{};const Actor* subject=nullptr;
   explicit Actor(InfantryTypeClass* type):InfantryClass(type,nullptr){}
   bool PlayAnim(Sequence sequence,bool force,bool random)override{
    ++calls[0];calls[1]=int(sequence);calls[2]=force;calls[3]=random;SequenceAnim=sequence;Animation.Value=0;return true;
   }
   void UnInit()override{++calls[6];}
   Mission GetCurrentMission()const override{return CurrentMission;}
   CoordStruct* GetDestination(CoordStruct* out,TechnoClass* docker)const override{
    if(subject)subject->calls[7]=docker==subject;*out={8*256+192,6*256+64,0};return out;
   }
   bool EnterIdleMode(bool initial,bool unused)override{EXPECT_FALSE(initial);EXPECT_TRUE(unused);++calls[4];return false;}
   void SetDestination(AbstractClass* value,bool)override{Destination=value;++calls[5];}
   Move IsCellOccupied(CellClass*,FacingType,int,CellClass*,bool)const override{return flags&256?Move::No:Move::OK;}
   bool IsOnBridge(TechnoClass*)const override{return false;}
   bool vt_entry_320()const override{return false;}
  };
  struct Driver:WalkLocomotionClass {
   bool jumpjet=false;
   HRESULT YRPP_STDCALL GetClassID(CLSID* out)override{*out=jumpjet?CLSIDs::Jumpjet:CLSIDs::Walk;return 0;}
  };
  InfantryTypeClass type("FRAME_DECISION");Actor gi(&type),other(&type);other.subject=&gi;
  auto* driver=GameCreate<Driver>();ASSERT_NE(driver,nullptr);driver->Link_To_Object(&gi);
#if !defined(_MSC_VER)
  driver->AddRef();
#endif
  gi.Locomotor=driver;
  auto clear_references=ra2::test::scope_exit([&]{gi.Airstrike=nullptr;gi.Target=nullptr;gi.Destination=nullptr;});
  for(auto& sequence:type.Sequence->Sequences){sequence.CountFrames=5;sequence.Facing=SequenceFacing::E;sequence.SoundCount=0;}
  type.NotHuman=true;type.MovementZone=MovementZone::None;
  std::ifstream input(RA2_INFANTRY_FRAME_UPDATE_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0;int mode;
  while(input>>mode){
   int sequence,phase,flags;std::array<int,18> expected;
   ASSERT_TRUE(bool(input>>sequence>>phase>>flags));for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
   gi.flags=flags;gi.calls={0,-1,0,0,0,0,0,0};gi.SequenceAnim=static_cast<Sequence>(sequence);gi.Animation.Value=phase;
   gi.Animation.HasChanged=false;gi.Uncrushable=true;type.DeployedCrushable=flags&16;
   gi.TargetingTimer.StartTime=-1;gi.TargetingTimer.TimeLeft=50;scenario.Random=Randomizer(99);
   gi.PrimaryFacing.SetCurrent(DirStruct(0x1234));gi.PrimaryFacing.ROT=DirStruct(0x7F00);
   gi.Location={8*256+192,6*256+64,0};driver->IsMoving=flags&1;driver->jumpjet=flags&16;
   if(!mode){
    gi.Crawling=flags&2;gi.IsFallingDown=flags&8;gi.IsFiring=true;
    // Opaque identity only: this fixture never dereferences Airstrike.
    gi.Airstrike=flags&16?reinterpret_cast<AirstrikeClass*>(&other):nullptr;
    gi.Target=flags&32?&other:nullptr;gi.CurrentMission=flags&32?Mission::Harvest:Mission::Guard;
    gi.Destination=nullptr;gi.SpeedPercentage=flags&4?0.2:0.1;gi.Doing_AI();
   }else{
    driver->IsReallyMoving=flags&2;gi.Crawling=flags&4;type.JumpJet=flags&8;
    gi.IsFiring=flags&32;gi.Destination=flags&64?&other:nullptr;gi.CurrentMission=flags&128?Mission::Move:Mission::Guard;
    gi.unknown_bool_6DC=flags&256;gi.IsInPlayfield=true;gi.SpeedPercentage=flags&256?0.9:0.8;gi.Movement_AI();
   }
   const std::array<int,18> actual{int(gi.SequenceAnim),gi.Animation.Value,int(gi.Uncrushable),int(gi.IsFiring),
    gi.PrimaryFacing.Desired().Raw,gi.TargetingTimer.StartTime,gi.TargetingTimer.TimeLeft,scenario.Random.Next1,
    int(std::lround(gi.SpeedPercentage*10)),int(gi.Destination!=nullptr),
    gi.calls[0],gi.calls[1],gi.calls[2],gi.calls[3],gi.calls[4],gi.calls[5],gi.calls[6],gi.calls[7]};
   EXPECT_EQ(actual,expected)<<"case "<<cases<<" mode "<<mode<<" sequence "<<sequence<<" phase "<<phase<<" flags "<<flags;
   EXPECT_EQ(driver->RefCount,1);++cases;
  }
  EXPECT_EQ(cases,12672u);
  // Rendezvous queries must not accidentally become current-position queries.
  gi.Location={-25600,-25600,0};CellStruct at;
  EXPECT_EQ(gi.GetMapCoords(&at),&at);EXPECT_EQ(at,(CellStruct{-100,-100}));
  EXPECT_EQ(gi.GetMapCoordsAgain(&at),&at);EXPECT_EQ(at,(CellStruct{8,6}));
  EXPECT_EQ(gi.GetCell(),&MapClass::InvalidCell);
  EXPECT_EQ(gi.GetCellAgain(),MapClass::Instance.GetCellAt(CellStruct{8,6}));
 },nullptr));
}
TEST(InfantryMovement, OriginalDiscoveryObserverAndFailureCorpus) {
 int mode=-1;auto runtime=game::default_scenario_runtime();runtime.context=&mode;
 runtime.session_mode=[](void* context)noexcept{return *static_cast<int*>(context);};
 ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void* context){
  auto& mode=*static_cast<int*>(context);
  struct Actor:InfantryClass {
   int queued=-1;
   Actor(InfantryTypeClass* type,HouseClass* owner):InfantryClass(type,owner){}
   bool QueueMission(Mission mission,bool start)override{EXPECT_FALSE(start);queued=int(mission);return false;}
  };
  HouseTypeClass country("DISCOVERY_COUNTRY");HouseClass owner(&country),player(&country),observer(&country);
  auto* previous=HouseClass::CurrentPlayer;const int init=Unsorted::ScenarioInit;
  auto restore=ra2::test::scope_exit([&]{HouseClass::CurrentPlayer=previous;Unsorted::ScenarioInit=init;});
  HouseClass::CurrentPlayer=&player;InfantryTypeClass type("DISCOVERY_GI");Actor gi(&type,&owner);
  std::ifstream input(RA2_TECHNO_DISCOVERY_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0;int who;
  while(input>>who){
   int flags;std::array<int,7> expected;ASSERT_TRUE(bool(input>>flags));for(auto& v:expected)ASSERT_TRUE(bool(input>>v));
   gi.DiscoveredByCurrentPlayer=flags&1;gi.DiscoveredByComputer=flags&2;gi.IsOwnedByCurrentPlayer=flags&4;
   for(auto* house:{&player,&observer}){house->IsHumanPlayer=flags&8;house->IsInPlayerControl=flags&16;}
   mode=flags&32?0:3;gi.CurrentMission=flags&64?Mission::Ambush:Mission::Guard;Unsorted::ScenarioInit=bool(flags&128);
   owner.RecheckPower=owner.RecheckRadar=owner.DiscoveredByPlayer=false;gi.queued=-1;
   HouseClass* houses[]{nullptr,&player,&observer};const bool result=gi.DiscoveredBy(houses[who]);
   const std::array<int,7> actual{result,gi.DiscoveredByCurrentPlayer,gi.DiscoveredByComputer,
    owner.RecheckPower,owner.RecheckRadar,owner.DiscoveredByPlayer,gi.queued};
   EXPECT_EQ(actual,expected)<<"case "<<cases<<" observer "<<who<<" flags "<<flags;++cases;
  }
  EXPECT_EQ(cases,768u);
 },&mode));
}

TEST(InfantryMovement, OriginalFootArrivalThreatAndAdjacencyCorpus) {
 const int rounding=std::fegetround();auto restore_rounding=ra2::test::scope_exit([&]{std::fesetround(rounding);});
 const auto root=std::filesystem::temp_directory_path()/("ra2-foot-arrival-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Actor:InfantryClass {
   mutable std::array<int,8> calls{};Actor* subject=nullptr;
   Actor(InfantryTypeClass* type,HouseClass* owner):InfantryClass(type,owner){}
   void RemoveSensorsAt(CellStruct)override{++calls[0];}
   void AddSensorsAt(CellStruct)override{++calls[1];}
   int SelectWeapon(AbstractClass*)const override{++calls[2];return 1;}
   CoordStruct* vt_entry_4F0(CoordStruct* out)override{++subject->calls[3];*out={9*256+128,6*256+128,0};return out;}
   bool IsCloseEnough3D(const CoordStruct&,int weapon)const override{EXPECT_EQ(weapon,1);++calls[4];return true;}
   int GetWeaponRange(int weapon)const override{EXPECT_EQ(weapon,1);++calls[5];return 300;}
   void SetDestination(AbstractClass* dest,bool)override{EXPECT_EQ(dest,nullptr);++calls[6];Destination=dest;}
   void Sensed()override{++calls[7];}
   int GetThreatValue()const override{return 24;}
  };
  HouseTypeClass country("ARRIVAL_THREAT");HouseClass owner(&country),enemy(&country);
  InfantryTypeClass type("ARRIVAL_CORPUS_GI");Actor gi(&type,&owner),other(&type,&owner);other.subject=&gi;
  gi.DiscoveredByCurrentPlayer=true;other.Location={9*256+128,6*256+128,0};
  auto clear=ra2::test::scope_exit([&]{gi.Target=gi.Destination=nullptr;gi.unknown_abstract_array_588.Count=0;});
  ASSERT_TRUE(gi.unknown_abstract_array_588.AddItem(&other));ASSERT_TRUE(gi.NavQueue.AddItem(&other));
  std::ifstream input(RA2_FOOT_ARRIVAL_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0;int reason;
  while(input>>reason){
   int flags;std::array<int,49> expected;ASSERT_TRUE(bool(input>>flags));for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
   gi.calls={};type.SensorsSight=flags&4?2:0;type.OpenTopped=flags&128;gi.Location={8*256+128,6*256+128,0};
   const CellStruct old{short(flags&2?6:8),6},current{8,6};gi.LastMapCoords=flags&1?old:CellStruct::Empty;gi.ThreatPosed=9;
   gi.Target=flags&8?&other:nullptr;gi.CurrentMission=flags&16?Mission::Attack:Mission::Move;
   gi.unknown_abstract_array_588.Count=flags&32?1:0;gi.Destination=&other;gi.PathDirections[0]=3;
   gi.IsAlive=!(flags&64);gi.IsInPlayfield=false;gi.unknown_bool_6B0=gi.unknown_bool_6B2=true;
   for(auto at:{old,current})for(int face=0;face<8;++face){const auto d=Unsorted::AdjacentCell[face];
    auto* cell=MapClass::Instance.GetCellAt(CellStruct{short(at.X+d.X),short(at.Y+d.Y)});ASSERT_NE(cell,&MapClass::InvalidCell);cell->BlockedNeighbours=7;}
   for(auto& row:enemy.ThreatPosedEstimates)for(auto& value:row)value=200;
   gi.FootClass::UpdatePosition(static_cast<PCPType>(reason));
   std::vector<int> actual{int(static_cast<unsigned short>(gi.LastMapCoords.X))|(int(static_cast<unsigned short>(gi.LastMapCoords.Y))<<16),
    int(gi.ThreatPosed),gi.PathDirections[0],gi.Destination!=nullptr,gi.IsInPlayfield,gi.unknown_bool_6B0,gi.unknown_bool_6B2};
   actual.insert(actual.end(),gi.calls.begin(),gi.calls.end());
   for(auto at:{old,current})for(int face=0;face<8;++face){const auto d=Unsorted::AdjacentCell[face];
    actual.push_back(MapClass::Instance.GetCellAt(CellStruct{short(at.X+d.X),short(at.Y+d.Y)})->BlockedNeighbours);}
   for(auto at:{old,current})for(int d:{-131,-130,-129,-1,0,1,129,130,131}){
    const int index=MapClass::CellRegion(at)+d;actual.push_back(int(enemy.ThreatPosedEstimates[index/130][index%130]));}
   ASSERT_EQ(actual.size(),expected.size());for(unsigned i=0;i<actual.size();++i)EXPECT_EQ(actual[i],expected[i])<<"case "<<cases<<" flags "<<flags<<" field "<<i;
   ++cases;
  }
  EXPECT_EQ(cases,1024u);
 },nullptr));
}

TEST(InfantryMovement, OriginalInfantryArrivalMissionAndWaterCorpus) {
 const int rounding=std::fegetround();auto restore_rounding=ra2::test::scope_exit([&]{std::fesetround(rounding);});
 const auto root=std::filesystem::temp_directory_path()/("ra2-infantry-arrival-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  auto runtime=game::map_runtime();
  ASSERT_TRUE(game::with_map_runtime(runtime,[](void*){
  struct Actor:InfantryClass {
   mutable std::array<int,9> calls{};
   Actor(InfantryTypeClass* t,HouseClass* h):InfantryClass(t,h){}
   bool EnterIdleMode(bool initial,bool resume)override{EXPECT_FALSE(initial);EXPECT_TRUE(resume);++calls[0];return true;}
   bool NextMission()override{++calls[1];return true;}
   void vt_entry_48C(bool a,int b,bool c,HouseClass* d)override{EXPECT_FALSE(a);EXPECT_EQ(b,0);EXPECT_FALSE(c);EXPECT_EQ(d,nullptr);++calls[2];}
   void UpdateSight(bool a,int b,bool c,HouseClass* d,int e)override{EXPECT_FALSE(a);EXPECT_EQ(b,0);EXPECT_FALSE(c);EXPECT_EQ(d,nullptr);EXPECT_EQ(e,0);++calls[3];}
   DamageState ReceiveDamage(int* damage,int distance,WarheadTypeClass*,ObjectClass* source,bool ignore,bool escape,HouseClass* house)override{
    EXPECT_EQ(*damage,125);EXPECT_EQ(distance,0);EXPECT_EQ(source,nullptr);EXPECT_TRUE(ignore);EXPECT_FALSE(escape);EXPECT_EQ(house,nullptr);++calls[4];return DamageState{};
   }
   void Sensed()override{++calls[5];}
   int SelectWeapon(AbstractClass*)const override{++calls[6];return 0;}
   RadioCommand SendToFirstLink(RadioCommand command)override{EXPECT_EQ(command,RadioCommand::NotifyUnloaded);++calls[7];return RadioCommand::AnswerPositive;}
   void DropAsBomb()override{++calls[8];}
   bool IsCloseEnough3D(const CoordStruct&,int)const override{return false;}
   CoordStruct* vt_entry_4F0(CoordStruct* out)override{*out={9*256+128,6*256+128,0};return out;}
  };
  HouseTypeClass country("INF_ARRIVAL");HouseClass owner(&country);InfantryTypeClass type("INF_ARRIVAL_GI");
  type.Locomotor=LocomotionClass::CLSIDs::Walk;Actor gi(&type,&owner),other(&type,&owner);
  ASSERT_TRUE(gi.InitializeLocomotor());auto* walk=dynamic_cast<WalkLocomotionClass*>(gi.Locomotor);ASSERT_NE(walk,nullptr);
  gi.Location={8*256+128,6*256+128,0};other.Location={9*256+128,6*256+128,0};gi.DiscoveredByCurrentPlayer=true;gi.Health=125;
  auto* cell=MapClass::Instance.GetCellAt(CellStruct{8,6});auto* dest=MapClass::Instance.GetCellAt(CellStruct{9,6});
  const auto land=cell->LandType;const auto flags=cell->Flags;const int bitfield=MapClass::Instance.Bitfield;
  auto restore=ra2::test::scope_exit([&]{gi.Destination=gi.Target=nullptr;cell->LandType=land;cell->Flags=flags;MapClass::Instance.Bitfield=bitfield;});
  MapClass::Instance.Bitfield=2;
  cell->AltFlags=AltCellFlags{};
  std::ifstream input(RA2_INFANTRY_ARRIVAL_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0;int reason;
  while(input>>reason){
   int mission,variant,bits;std::array<int,13> expected;ASSERT_TRUE(bool(input>>mission>>variant>>bits));for(auto& v:expected)ASSERT_TRUE(bool(input>>v));
   gi.calls={};gi.CurrentMission=static_cast<Mission>(mission);gi.QueuedMission=bits&4?Mission::Guard:Mission::None;
   gi.Destination=bits&1?dest:nullptr;gi.Target=bits&2?&other:nullptr;gi.IsAlive=!(bits&8);type.C4=bits&16;walk->IsMoving=bits&32;
   gi.OnBridge=bits&64;gi.IsTether=bits&128;gi.IsInPlayfield=false;gi.unknown_bool_6B0=gi.unknown_bool_6B2=true;
   constexpr LandType lands[]{LandType::Clear,LandType::Water,LandType::Rock,LandType::Water};cell->LandType=lands[variant];
   cell->Flags=static_cast<CellFlags>(variant==3?0x100:0);cell->Visibility=char(-1);
   try {gi.UpdatePosition(static_cast<PCPType>(reason));}
   catch(const std::exception& error){FAIL()<<"case "<<cases<<": "<<error.what();}
   std::array<int,13> actual;std::copy(gi.calls.begin(),gi.calls.end(),actual.begin());
   actual[9]=gi.IsInPlayfield;actual[10]=gi.unknown_bool_6B0;actual[11]=gi.unknown_bool_6B2;actual[12]=cell->Visibility;
   ASSERT_EQ(actual,expected)<<"case "<<cases<<" reason "<<reason<<" mission "<<mission<<" variant "<<variant<<" flags "<<bits;++cases;
  }
  EXPECT_EQ(cases,32768u);
  },nullptr));
 },nullptr));
}

TEST(InfantrySpace, OriginalOcclusionAndCellSpreadCorpus) {
 const int rounding=std::fegetround();auto restore_rounding=ra2::test::scope_exit([&]{std::fesetround(rounding);});
 const auto root=std::filesystem::temp_directory_path()/("ra2-visibility-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  auto& map=MapClass::Instance;auto* tactical=TacticalClass::Instance;ASSERT_NE(tactical,nullptr);
  auto* center=map.GetCellAt(CellStruct{8,6});std::ifstream input(RA2_VISIBILITY_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0;int mode;
  while(input>>mode){
   if(mode==0){int fog,state,mask,expected;ASSERT_TRUE(bool(input>>fog>>state>>mask>>expected));
    for(int y=5;y<=7;++y)for(int x=7;x<=9;++x){auto* cell=map.GetCellAt(CellStruct{short(x),short(y)});ASSERT_NE(cell,&MapClass::InvalidCell);cell->Flags=CellFlags{};cell->AltFlags=AltCellFlags{};}
    if(fog)center->Flags=static_cast<CellFlags>(state);else center->AltFlags=static_cast<AltCellFlags>((state&1)*8+(state/2)*16);
    constexpr CellStruct neighbors[]{{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1}};
    for(int i=0;i<8;++i){auto d=neighbors[i];auto* cell=map.GetCellAt(CellStruct{short(8+d.X),short(6+d.Y)});
     if(fog)cell->Flags=static_cast<CellFlags>(mask&(1<<i)?0:2);else cell->AltFlags=static_cast<AltCellFlags>(mask&(1<<i)?0:8);}
    EXPECT_EQ(int(tactical->GetOcclusion({8,6},fog)),expected)<<cases;
   }else if(mode==1){int radius,count;ASSERT_TRUE(bool(input>>radius>>count));EXPECT_EQ(CellSpread::NumCells(radius),size_t(count));}
   else {int index,x,y;ASSERT_TRUE(bool(input>>index>>x>>y));EXPECT_EQ(CellSpread::GetCell(index),(CellStruct{short(x),short(y)}));}
   ++cases;
  }
  EXPECT_EQ(cases,2429u);
  for(int y=5;y<=7;++y)for(int x=7;x<=9;++x)map.GetCellAt(CellStruct{short(x),short(y)})->AltFlags=AltCellFlags{};
  const int field=map.Bitfield;auto restore=ra2::test::scope_exit([&]{map.Bitfield=field;});map.Bitfield=2;
  CoordStruct at{8*256+128,6*256+128,0};center->Visibility=char(-1);
  map.RevealArea3(&at,0,-9,true);EXPECT_EQ(center->Visibility,char(-1));
  map.RevealArea3(&at,4,3,false);EXPECT_EQ(center->Visibility,char(-1)); // empty annulus, not an extra center visit
  map.RevealArea3(&at,-9,-9,false);EXPECT_EQ(center->Visibility,char(-2));EXPECT_TRUE(center->VisibilityChanged);
  auto runtime=game::map_runtime();const RectangleStruct bounds{0,0,640,480};runtime.view_bounds=&bounds;
  ASSERT_TRUE(game::with_map_runtime(runtime,[](void*){
   auto* tactical=TacticalClass::Instance;auto* cell=MapClass::Instance.GetCellAt(CellStruct{8,6});MapClass::Instance.Bitfield=0;
   const auto pixel=TacticalClass::AdjustForZShapeMove(cell->GetCoords().X,cell->GetCoords().Y);
   tactical->TacticalPos={pixel.X-100,pixel.Y-100};tactical->VisibleCellCount=798;cell->unknown_5C=0xFFFFFFFFu;cell->VisibilityChanged=true;
   tactical->RegisterCellAsVisible(cell);EXPECT_EQ(tactical->VisibleCellCount,799);EXPECT_EQ(tactical->VisibleCells[798],cell);EXPECT_FALSE(cell->VisibilityChanged);
   cell->VisibilityChanged=true;tactical->RegisterCellAsVisible(cell);EXPECT_EQ(tactical->VisibleCellCount,799);EXPECT_TRUE(cell->VisibilityChanged);
   cell->unknown_5C=0xFFFFFFFFu;tactical->RegisterCellAsVisible(cell);EXPECT_EQ(tactical->VisibleCellCount,799);EXPECT_FALSE(cell->VisibilityChanged);
  },nullptr));
 },nullptr));
}

TEST(InfantrySpace, SensorsRespectStrictRadiusGroundChainAndReferenceCount) {
 const int rounding=std::fegetround();auto restore_rounding=ra2::test::scope_exit([&]{std::fesetround(rounding);});
 const auto root=std::filesystem::temp_directory_path()/("ra2-sensors-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Actor:InfantryClass{int sensed=0;Actor(InfantryTypeClass* t,HouseClass* h):InfantryClass(t,h){}void Sensed()override{++sensed;}};
  HouseTypeClass country("SENSOR_OWNER");HouseClass owner(&country);InfantryTypeClass type("SENSOR_GI");type.SensorsSight=2;
  Actor source(&type,&owner),ground(&type,&owner),bridge(&type,&owner);source.Location={8*256+128,6*256+128,0};
  auto* cell=MapClass::Instance.GetCellAt(CellStruct{8,6});auto* first=cell->FirstObject;auto* alt=cell->AltObject;
  cell->FirstObject=&ground;cell->AltObject=&bridge;
  auto restore=ra2::test::scope_exit([&]{cell->FirstObject=first;cell->AltObject=alt;});
  source.AddSensorsAt(CellStruct::Empty);source.AddSensorsAt({8,6});
  for(int y=-2;y<=2;++y)for(int x=-2;x<=2;++x)EXPECT_EQ(bool(MapClass::Instance.GetCellAt(CellStruct{short(8+x),short(6+y)})->Sensors_InclHouse(owner.ArrayIndex)),x*x+y*y<4);
  EXPECT_EQ(ground.sensed,2);EXPECT_EQ(bridge.sensed,0);
  source.RemoveSensorsAt({8,6});EXPECT_TRUE(cell->Sensors_InclHouse(owner.ArrayIndex));
  source.RemoveSensorsAt({8,6});EXPECT_FALSE(cell->Sensors_InclHouse(owner.ArrayIndex));
  source.RemoveSensorsAt({8,6});EXPECT_EQ(ground.sensed,4);EXPECT_EQ(bridge.sensed,0);
  EXPECT_EQ(cell->GetInfantry(false),&ground);EXPECT_EQ(cell->GetInfantry(true),&bridge);
  const bool active=Game::IsActive;Game::IsActive=false;EXPECT_EQ(cell->GetInfantry(false),nullptr);Game::IsActive=active;
 },nullptr));
}

TEST(InfantryActions, WeaponRangeThreatAndBuildingTargetBranches) {
 RulesClass rules;auto* previous=RulesClass::Instance;RulesClass::Instance=&rules;
 auto restore_rules=ra2::test::scope_exit([&]{RulesClass::Instance=previous;});rules.ThreatPerOccupant=12;
 HouseTypeClass country("TARGET_COUNTRY");HouseClass owner(&country),enemy(&country);
 InfantryTypeClass type("TARGET_GI"),passenger_type("RANGE_GI");InfantryClass gi(&type,&owner),passenger(&passenger_type,&owner);
 BuildingTypeClass building_type("TARGET_BUILDING",BuildingTypeClass::ConstructionDefaults{});BuildingClass building(&building_type,&enemy);
 WeaponTypeClass primary("SELECT_PRIMARY"),secondary("SELECT_SECONDARY"),passenger_weapon("PASSENGER_RANGE");
 WarheadTypeClass primary_wh("SELECT_PRIMARY_WH"),secondary_wh("SELECT_SECONDARY_WH");BulletTypeClass bullet("SELECT_BULLET");
 primary.Warhead=&primary_wh;secondary.Warhead=&secondary_wh;primary.Projectile=secondary.Projectile=&bullet;
 type.Weapon[0].WeaponType=&primary;type.Weapon[1].WeaponType=&secondary;
 secondary_wh.Airstrike=true;building_type.CanC4=false;EXPECT_EQ(gi.SelectWeapon(&building),0);
 building_type.CanC4=true;EXPECT_EQ(gi.SelectWeapon(&building),1);
 building_type.ResourceDestination=true;building_type.ResourceGatherer=true;EXPECT_EQ(gi.SelectWeapon(&building),0);
 building_type.ResourceGatherer=false;EXPECT_EQ(gi.SelectWeapon(&building),1);
 secondary_wh.Airstrike=false;primary_wh.IsLocomotor=true;EXPECT_EQ(gi.SelectWeapon(&building),1);primary_wh.IsLocomotor=false;
 secondary_wh.ElectricAssault=true;building.Owner=&owner;building_type.Overpowerable=true;EXPECT_EQ(gi.SelectWeapon(&building),1);
 building_type.ThreatPosed=27;EXPECT_EQ(building.GetThreatValue(),27);
 ASSERT_TRUE(building.Occupants.AddItem(&gi));ASSERT_TRUE(building.Occupants.AddItem(&passenger));EXPECT_EQ(building.GetThreatValue(),24);building.Occupants.Clear();
 building.BunkerLinkedItem=&gi;type.ThreatPosed=45;EXPECT_EQ(building.GetThreatValue(),45);building.BunkerLinkedItem=nullptr;
 primary.Range=500;passenger_weapon.Range=250;passenger_type.Weapon[0].WeaponType=&passenger_weapon;
 EXPECT_EQ(gi.GetWeaponRange(0),500);type.OpenTopped=true;EXPECT_EQ(gi.GetWeaponRange(0),500);
 gi.Passengers.FirstPassenger=&passenger;EXPECT_EQ(gi.GetWeaponRange(0),250);
 passenger_weapon.Range=700;EXPECT_EQ(gi.GetWeaponRange(0),500);passenger_type.Weapon[0].WeaponType=nullptr;EXPECT_EQ(gi.GetWeaponRange(0),500);gi.Passengers.FirstPassenger=nullptr;
 const int region=MapClass::CellRegion({8,6});owner.AdjustThreat(region,17);
 EXPECT_EQ(owner.ThreatPosedEstimates[region/130][region%130],17u);
 EXPECT_EQ(owner.ThreatPosedEstimates[(region-1)/130][(region-1)%130],8u);
 EXPECT_EQ(owner.ThreatPosedEstimates[(region-131)/130][(region-131)%130],4u);
 owner.AdjustThreat(region,-3);EXPECT_EQ(owner.ThreatPosedEstimates[region/130][region%130],14u);
 owner.AdjustThreat(region,-100);EXPECT_EQ(owner.ThreatPosedEstimates[region/130][region%130],0u);
 building_type.UndeploysInto=reinterpret_cast<UnitTypeClass*>(&type);building_type.Foundation=static_cast<Foundation>(0);
 EXPECT_TRUE(building.IsStrange());building_type.Foundation=static_cast<Foundation>(1);EXPECT_FALSE(building.IsStrange());building_type.UndeploysInto=nullptr;
 WaypointClass a{{256,512,0}},b{{256,512,1}};EXPECT_FALSE(a==b);
 gi.unknown_5A0=&building;gi.Destination=&building;gi.LastDestination=&passenger;gi.PathDirections[0]=4;
 gi.AbortMotion();EXPECT_EQ(gi.unknown_5A0,nullptr);EXPECT_EQ(gi.Destination,nullptr);EXPECT_EQ(gi.LastDestination,&passenger);EXPECT_EQ(gi.PathDirections[0],4);gi.LastDestination=nullptr;
}

TEST(InfantryActions, OriginalWeaponSelectionGuardsNavalAndDeploymentCorpus) {
 struct Actor:InfantryClass {
  bool air=false,occupy=false;CellClass* cell=nullptr;
  Actor(InfantryTypeClass* type,HouseClass* owner):InfantryClass(type,owner){}
  bool IsInAir()const override{return air;}
  bool CanOccupyFire()const override{return occupy;}
  CellClass* GetCell()const override{return cell;}
 };
 HouseTypeClass country("WEAPON_CHOICE");HouseClass owner(&country),enemy(&country);
 InfantryTypeClass type("CHOICE_GI"),other_type("CHOICE_TARGET");Actor gi(&type,&owner),other(&other_type,&enemy);
 auto* cell=CellClass::Create();ASSERT_NE(cell,nullptr);auto cleanup=ra2::test::scope_exit([&]{GameDelete(cell);});other.cell=cell;
 WeaponTypeClass primary("CHOICE_1"),secondary("CHOICE_2");WarheadTypeClass wh1("CHOICE_WH1"),wh2("CHOICE_WH2");BulletTypeClass bullet("CHOICE_BULLET");
 primary.Warhead=&wh1;secondary.Warhead=&wh2;primary.Projectile=secondary.Projectile=&bullet;
 std::ifstream input(RA2_WEAPON_SELECTION_FIXTURE);ASSERT_TRUE(input.good());int mode;unsigned cases=0;
 while(input>>mode){
  int variant,flags,expected;ASSERT_TRUE(bool(input>>variant>>flags>>expected));
  type.TurretCount=0;type.IsGattling=type.DeployFire=type.Naval=false;type.OpenTransportWeapon=-1;
  type.LandTargeting=LandTargetingType::Land_OK;type.NavalTargeting=NavalTargetingType::Underwater_Never;
  type.Weapon[0].WeaponType=&primary;type.Weapon[1].WeaponType=&secondary;
  secondary.NeverUse=secondary.DrainWeapon=secondary.AreaFire=false;
  wh1.IsLocomotor=wh2.Airstrike=wh2.ElectricAssault=false;wh1.Verses[0]=wh2.Verses[0]=1.0;
  gi.occupy=gi.InOpenToppedTransport=gi.Deactivated=false;gi.CurrentWeaponNumber=0;gi.CurrentGattlingStage=2;gi.CurrentMission=Mission::Guard;
  other_type.Underwater=other_type.Organic=other_type.Unnatural=other_type.Drainable=false;other_type.SpeedType=static_cast<SpeedType>(0);
  other_type.Armor=static_cast<Armor>(0);other.CloakState=::CloakState::Uncloaked;other.OnBridge=false;other.air=false;
  other.Owner=flags&16?&owner:&enemy;cell->LandType=LandType::Clear;cell->Flags=CellFlags{};bullet.AA=false;
  AbstractClass* target=&other;int actual=0;
  if(mode==0){
   type.IsGattling=flags&1;type.TurretCount=flags&2?2:0;gi.occupy=flags&4;
   type.Weapon[0].WeaponType=flags&16?&primary:nullptr;type.Weapon[1].WeaponType=flags&8?&secondary:nullptr;
   secondary.NeverUse=flags&32;target=flags&64?&other:nullptr;gi.InOpenToppedTransport=flags&128;type.OpenTransportWeapon=2;
   gi.CurrentWeaponNumber=flags&256?-1:3;bullet.AA=other.air=flags&512;actual=gi.TechnoClass::SelectWeapon(target);
  }else if(mode==1){
   type.NavalTargeting=static_cast<NavalTargetingType>(variant);other_type.Underwater=flags&1;other_type.Organic=flags&2;
   other_type.SpeedType=static_cast<SpeedType>(flags&4?3:0);other_type.Unnatural=flags&8;
   other.CloakState=static_cast<::CloakState>(flags&16?1:0);if(flags&32)target=cell;if(flags&64)target=nullptr;
   actual=gi.SelectNavalTargeting(target);
  }else if(mode==2){
   gi.SequenceAnim=static_cast<Sequence>(variant);type.DeployFire=flags&1;type.DeployFireWeapon=3;
   gi.InOpenToppedTransport=flags&2;type.OpenTransportWeapon=flags&4?2:-1;actual=gi.SelectWeapon(nullptr);
  }else{
   bullet.AA=variant!=9;other.air=flags&1;other.OnBridge=flags&2;gi.Deactivated=flags&32;
   cell->LandType=flags&8?LandType::Beach:flags&4?LandType::Water:LandType::Clear;type.NavalTargeting=static_cast<NavalTargetingType>(variant%8);
   if(variant==1)wh2.Airstrike=true;
   if(variant==2)wh1.IsLocomotor=true;
   if(variant==3){secondary.DrainWeapon=true;other_type.Drainable=true;}
   if(variant==4){secondary.AreaFire=true;gi.CurrentMission=Mission::Unload;}
   if(variant==5)wh2.Verses[0]=0.0;
   if(variant==6)wh1.Verses[0]=0.0;
   if(variant==7){
    target=cell;cell->Flags=static_cast<CellFlags>(flags&2?0x100:0);type.Naval=true;
    // This corpus explicitly fixtures IsOnFloor=false. Match that input now
    // that CellClass overrides the AbstractClass default with its tile test.
    cell->IsoTileTypeIndex=IsometricTileTypeClass::WaterSet;
   }
   if(variant==7 || variant==8)type.LandTargeting=LandTargetingType::Land_Secondary;
   if(variant==10){gi.InOpenToppedTransport=true;type.OpenTransportWeapon=2;}
   if(variant==11)type.IsGattling=true;
   actual=gi.TechnoClass::SelectWeapon(target);
  }
  EXPECT_EQ(actual,expected)<<"case "<<cases<<" mode "<<mode<<" variant "<<variant<<" flags "<<flags;++cases;
 }
 EXPECT_EQ(cases,3160u);
}

TEST(InfantryMovement, TechnoArrivalAndCloakReacquisitionOrder) {
 const int rounding=std::fegetround();auto restore_rounding=ra2::test::scope_exit([&]{std::fesetround(rounding);});
 const auto root=std::filesystem::temp_directory_path()/("ra2-arrival-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Actor:InfantryClass {
   int id=0;bool ready=true,relocate=false;std::vector<int>* log=nullptr;
   Actor(InfantryTypeClass* type,HouseClass* owner):InfantryClass(type,owner){}
   bool IsReadyToCloak()const override{log->push_back(-2);return ready;}
   void Deselect()override{log->push_back(-1);}
   void Cloak(bool sound)override{EXPECT_FALSE(sound);log->push_back(-3);}
   void SetTarget(AbstractClass* target)override{log->push_back(id);Target=target;}
   void Sensed()override{log->push_back(-4);if(relocate)Location={-25600,-25600,0};}
   bool DiscoveredBy(HouseClass* house)override{EXPECT_EQ(house,HouseClass::CurrentPlayer);log->push_back(-5);return false;}
  };
  HouseTypeClass country("ARRIVAL_COUNTRY");HouseClass owner(&country),player(&country),unsensed(&country);
  auto* previous=HouseClass::CurrentPlayer;auto restore=ra2::test::scope_exit([&]{HouseClass::CurrentPlayer=previous;});HouseClass::CurrentPlayer=&player;
  InfantryTypeClass type("ARRIVAL_GI");Actor gi(&type,&owner),friendly(&type,&owner),detector(&type,&player),blind(&type,&unsensed);
  std::vector<int> log;for(auto* actor:{&gi,&friendly,&detector,&blind})actor->log=&log;
  friendly.id=1;detector.id=2;blind.id=3;friendly.Target=detector.Target=blind.Target=&gi;
  const CoordStruct location{8*256+128,6*256+128,0};auto* cell=MapClass::Instance.GetCellAt(location);
  ASSERT_TRUE(MapClass::Instance.IsWithinUsableArea(CellStruct{8,6},true));
  cell->CloakedByHouses=0;cell->Sensors_AddOfHouse(player.ArrayIndex);
  for(int flags=0;flags<8;++flags){
   gi.Location=location;gi.CloakState=::CloakState::Cloaked;gi.ready=flags&2;
   cell->CloakedByHouses=flags&1?1u<<owner.ArrayIndex:0;
   if(!(flags&4))cell->Sensors_RemOfHouse(player.ArrayIndex);
   log.clear();gi.TechnoClass::Sensed();std::vector<int> expected;
   if(!(flags&4))expected.push_back(-1);
   if(flags&1){expected.push_back(-2);if(flags&2){expected.push_back(-3);expected.push_back(1);if(flags&4)expected.push_back(2);}}
   EXPECT_EQ(log,expected)<<flags;
   if(!(flags&4))cell->Sensors_AddOfHouse(player.ArrayIndex);
  }
  gi.relocate=true;cell->CloakedByHouses=0;cell->OverlayTypeIndex=-1;
  for(int reason=0;reason<4;++reason)for(int flags=0;flags<8;++flags){
   gi.Location=location;gi.IsInPlayfield=flags&1;gi.DiscoveredByCurrentPlayer=flags&2;
   cell->AltFlags=flags&4?AltCellFlags::NoFog:AltCellFlags{};log.clear();gi.TechnoClass::UpdatePosition(static_cast<PCPType>(reason));
   std::vector<int> expected;if(reason==int(PCPType::End)){expected.push_back(-4);if(!(flags&2)&&(flags&4))expected.push_back(-5);}
   EXPECT_EQ(log,expected);EXPECT_EQ(gi.IsInPlayfield,reason==int(PCPType::End)||bool(flags&1));
  }
  friendly.Target=detector.Target=blind.Target=nullptr;cell->Sensors_RemOfHouse(player.ArrayIndex);
 },nullptr));
}
TEST(InfantryDisplay, OriginalFootDrawingDepthCorpus) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-gi-depth-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);auto cleanup=ra2::test::scope_exit([&]{std::error_code e;std::filesystem::remove_all(root,e);});
 map_fixture::fixtures(root);
 {std::ofstream f(root/"world.map",std::ios::app);f<<"\n[Map]\nSize=0,0,40,40\nLocalSize=0,0,40,40\n";}
 View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Driver:WalkLocomotionClass{int adjust=0;int YRPP_STDCALL Z_Adjust()override{return adjust;}}driver;
  InfantryTypeClass type("DEPTH_GI");InfantryClass actor(&type,nullptr);actor.Locomotor=&driver;actor.Location={40*256+128,40*256+128,0};
  auto release=ra2::test::scope_exit([&]{actor.Locomotor=nullptr;});type.ZFudgeBridge=7;
  OverlayTypeClass rock("DEPTH_ROCK"),plain("DEPTH_PLAIN");rock.IsARock=true;plain.IsARock=false;
  const int old_bridge=IsometricTileTypeClass::BridgeSet,old_tubes=TubeClass::Array.Count;
  IsometricTileTypeClass::BridgeSet=0;TubeClass::Array.Count=1;
  auto restore=ra2::test::scope_exit([&]{IsometricTileTypeClass::BridgeSet=old_bridge;TubeClass::Array.Count=old_tubes;});
  auto* tile=reinterpret_cast<TMPStruct*>(IsometricTileTypeClass::Array[0]->GetImage());ASSERT_NE(tile,nullptr);
  const int old_height=tile->Height;auto restore_height=ra2::test::scope_exit([&]{tile->Height=old_height;});
  const auto cell=[](int x,int y){return MapClass::Instance.GetCellAt(CellStruct{short(x),short(y)});};
  std::ifstream input(RA2_FOOT_DRAWING_DEPTH_FIXTURE);std::string magic;int count=0,different=0;input>>magic>>count;ASSERT_EQ(magic,"FOOT_DRAWING_DEPTH_V1");
  for(int i=0;i<count;++i){int variant,bridge,facing,z,base,foot;ASSERT_TRUE(bool(input>>variant>>bridge>>facing>>z>>base>>foot));
   for(int y=37;y<=43;++y)for(int x=37;x<=43;++x){auto*c=cell(x,y);ASSERT_NE(c,nullptr);
    c->Flags=CellFlags{};c->SlopeIndex=0;c->Level=0;c->Height=0;c->IsoTileTypeIndex=0;c->OverlayTypeIndex=-1;c->TubeIndex=-1;c->LandType=LandType::Clear;
   }
   actor.OnBridge=bridge;actor.Location.Z=z;actor.PrimaryFacing.SetCurrent(DirStruct(facing*0x2000));
   tile->Height=variant==1?40:30;driver.adjust=variant==15?-3:0;
   if(variant==2)cell(40,40)->Flags=static_cast<CellFlags>(0x10000);
   if(variant==3)cell(40,39)->SlopeIndex=1;
   if(variant>=4&&variant<=6)cell(40,40)->SlopeIndex=1;
   if(variant==5)cell(40,41)->OverlayTypeIndex=rock.ArrayIndex;
   if(variant==6){cell(40,41)->OverlayTypeIndex=plain.ArrayIndex;cell(41,40)->OverlayTypeIndex=rock.ArrayIndex;}
   if(variant==7||variant==9||variant==14)cell(40,40)->Flags=static_cast<CellFlags>(0x100);
   if(variant==7)cell(40,41)->IsoTileTypeIndex=6;
   if(variant==8){cell(40,40)->Flags=static_cast<CellFlags>(0x400);cell(41,41)->IsoTileTypeIndex=7;}
   if(variant==9)for(auto at:{CellStruct{40,41},CellStruct{41,40},CellStruct{41,41}})cell(at.X,at.Y)->IsoTileTypeIndex=6;
   if(variant==10||variant==11)for(int y:{variant==10?40:39,variant==10?38:37}){cell(40,y)->TubeIndex=0;cell(40,y)->LandType=LandType::Tunnel;}
   if(variant==12||variant==13)cell(41,41)->Level=4;
   if(variant==13)cell(42,42)->Level=4;
   const int actual_base=actor.TechnoClass::GetZAdjustment(),actual_foot=actor.GetZAdjustment();
   if(actual_base!=base||actual_foot!=foot){if(different++<8)std::cout<<"DEPTH_DIFF "<<variant<<' '<<bridge<<' '<<facing<<' '<<z<<" actual "<<actual_base<<' '<<actual_foot<<" expected "<<base<<' '<<foot<<'\n';}
  }
  EXPECT_EQ(count,1024);EXPECT_EQ(different,0);
 },nullptr))<<game::map_view_error(*view.view);
}

TEST(InfantryDisplay, MapSubcellsDrawPacketsPickingAndReload) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-gi-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);map_fixture::shp(root/"GI.SHP",16,24,24);
 {std::ofstream f(root/"RULESMD.INI",std::ios::app);f<<"\n[AudioVisual]\nExtraInfantryLight=0.2\n[InfantryTypes]\n0=E1\n[E1]\nName=GI\nImage=GI\nStrength=125\nSelectable=yes\nSpeed=4\nMovementZone=Infantry\nLocomotor={4A582744-9839-11D1-B709-00A024DDAFD1}\n";}
 {std::ofstream f(root/"ARTMD.INI",std::ios::app);f<<"\n[GI]\nSequence=GISequence\n[GISequence]\nReady=0,1,1\nWalk=0,1,1\n";}
 {std::ofstream f(root/"world.map",std::ios::app);f<<"\n[Infantry]\n";
  for(int i=0;i<5;++i)f<<i<<"=Neutral,E1,128,8,6,"<<i<<",Guard,"<<32*i<<",None,0,-1,0,1,1\n";
  f<<"5=Neutral,E1,256,8,6,9,Guard,0,None,0,-1,0,1,1\n";}
 const int baseline=InfantryClass::Array.Count;View view(root.string());view.load("world.map");
 game::MapWorldSnapshot status;ASSERT_TRUE(game::get_map_world_snapshot(*view.view,status));EXPECT_EQ(status.infantry,5u);EXPECT_EQ(status.unknown_records,1u);
 // Original Unlimbo 0x51DFF0 uses Cell 0x481180: the northwest subslot
 // resolves to spot zero even during ScenarioInit. Compare final placement.
 constexpr Point2D offsets[]{{128,128},{128,128},{192,64},{64,192},{192,192}};
 auto objects=view.objects();game::MapObjectId old{};unsigned index=0;
 for(auto&o:objects)if(o.kind==game::MapObjectKind::infantry){
  EXPECT_EQ(o.world_x,8*256+offsets[index].X);EXPECT_EQ(o.world_y,6*256+offsets[index].Y);
  EXPECT_EQ(o.health,62);EXPECT_STREQ(o.type_id,"E1");EXPECT_STREQ(o.owner_name,"Neutral");EXPECT_EQ(o.frame,7-int(index));old=o.id;++index;
 }EXPECT_EQ(index,5u);
 ASSERT_TRUE(game::set_map_viewport(*view.view,640,480));
 ASSERT_TRUE(game::center_map_view(*view.view,60,225));
 struct Query{game::MapViewHandle&v;unsigned bodies=0,shadows=0;bool picked=false;} q{*view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*p){auto&q=*static_cast<Query*>(p);auto&w=*q.v.world;
  game::rebuild_world_sprites(w);
  for(auto&s:w.impl->sprites)if(s.owner&&s.owner->WhatAmI()==AbstractType::Infantry){
   EXPECT_TRUE(s.original_depth);if(s.shadow){++q.shadows;EXPECT_EQ(s.flags,0x2E01u);EXPECT_EQ(s.gradient,0);EXPECT_EQ(s.depth_adjustment,-4);}
   else {++q.bodies;EXPECT_EQ(s.flags,0x2E00u);EXPECT_EQ(s.gradient,2);EXPECT_EQ(s.depth_adjustment,-13);
    ASSERT_NE(s.owner->GetCell(),nullptr);EXPECT_EQ(s.intensity,s.owner->GetCell()->Intensity_Normal+200);}
  }
  // Use the foremost actual packet to avoid selecting a rear overlapping GI.
  for(auto i=w.impl->sprites.rbegin();i!=w.impl->sprites.rend();++i)if(i->owner&&i->owner->WhatAmI()==AbstractType::Infantry&&!i->shadow){
   auto*b=game::pick_world_object(w,i->position);q.picked=b&&b->WhatAmI()==AbstractType::Infantry;if(q.picked)EXPECT_TRUE(b->Select());break;}
 },&q));EXPECT_EQ(q.bodies,5u);EXPECT_EQ(q.shadows,5u);EXPECT_TRUE(q.picked);
 game::MapObjectSnapshot snap;ASSERT_TRUE(game::get_map_object(*view.view,old,snap));
 view.load("world.map");EXPECT_EQ(InfantryClass::Array.Count,baseline+5);EXPECT_FALSE(game::get_map_object(*view.view,old,snap));
 game::destroy_map_view(view.view);EXPECT_EQ(InfantryClass::Array.Count,baseline);
}
TEST(InfantryDisplay, OriginalFirstMapHas56InfantryAnd5GI) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA to validate original MIX resources";
 const int baseline=InfantryClass::Array.Count;View view(data,true);view.load("ALL01UMD.MAP");
 unsigned count=0,gi=0;for(auto&o:view.objects())if(o.kind==game::MapObjectKind::infantry){++count;EXPECT_TRUE(o.has_image);EXPECT_GE(o.frame,0);if(!std::strcmp(o.type_id,"E1"))++gi;}
 EXPECT_EQ(count,56u);EXPECT_EQ(gi,5u);view.load("ALL01UMD.MAP");EXPECT_EQ(InfantryClass::Array.Count,baseline+56);
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  auto& map=MapClass::Instance;EXPECT_GT(map.somecount_4C,1u);for(auto* zones:map.MovementZones)EXPECT_NE(zones,nullptr);
  for(int level=0;level<3;++level){EXPECT_GT(map.SubzoneTrackingCounts[level],1);EXPECT_EQ(map.SubzoneTrackingCounts[level],map.SubzoneTracking[level].Count);}
  // Use the production INI values and held Walk drivers created by map loading.
  for(auto* actor:InfantryClass::Array)ASSERT_NE(actor->Locomotor,nullptr);
  unsigned routed=0;
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"E1")){
   const bool playfield=actor->IsInPlayfield;
   auto restore=ra2::test::scope_exit([&]{actor->IsInPlayfield=playfield;});
   // Full Unlimbo/UpdatePosition has not been ported. The fixture supplies
   // that one lifecycle flag explicitly, not movement/type/driver substitutes.
   actor->IsInPlayfield=true;
   EXPECT_EQ(actor->Type->MovementZone,MovementZone::Infantry);EXPECT_EQ(actor->Type->SpeedType,SpeedType::Foot);
   EXPECT_EQ(actor->Type->Speed,10);EXPECT_EQ(std::memcmp(&actor->Type->Locomotor,&LocomotionClass::CLSIDs::Walk,sizeof(GUID)),0);
   EXPECT_EQ(actor->FindPath(nullptr,nullptr,0,0,0,0),nullptr);
   const CellStruct start{short(actor->Location.X/256),short(actor->Location.Y/256)};std::array<int,2002> directions{};bool found=false;
   for(int y=-4;y<=4 && !found;++y)for(int x=-4;x<=4 && !found;++x){if(std::abs(x)+std::abs(y)<3)continue;
    CellStruct end{short(start.X+x),short(start.Y+y)};if(!map.IsWithinUsableArea(end,true))continue;
    if(auto* path=actor->FindPath(&end,directions.data(),0,0,0,0)){CellStruct reached;AStarClass::FollowPath(&reached,&start,path->PathLength-1,directions.data());
     if(reached==end){EXPECT_GT(AStarClass::Instance.PassabilityCounts[0],0);
      ASSERT_TRUE(actor->UpdatePathfinding(end,0,0));EXPECT_NE(actor->PathDirections[0],-1);
      EXPECT_TRUE(actor->IsOnMap);EXPECT_FALSE(actor->InLimbo);found=true;++routed;}}
   }
  }
  EXPECT_EQ(routed,5u);
 },nullptr));
 game::destroy_map_view(view.view);EXPECT_EQ(InfantryClass::Array.Count,baseline);
}

TEST(InfantryMovement, FirstMapBuildingFoundationBlocksAndReleasesMovement) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& view=*static_cast<game::MapViewHandle*>(p);auto& map=MapClass::Instance;
  const auto contains=[](CellClass* cell,const ObjectClass* wanted){
   int count=0;for(auto* item=cell->FirstObject;item && count++<=AbstractClass::Array.Count;item=item->NextObject)if(item==wanted)return true;
   return false;
  };
  int footprintCells=0;
  for(auto* building:BuildingClass::Array)if(building->IsOnMap){
   const auto anchor=building->GetMapCoords();
   ASSERT_EQ(building->GetFoundationData(false),building->Type->FoundationData);
   for(auto* offset=building->GetFoundationData(false);*offset!=CellStruct{0x7FFF,0x7FFF};++offset){
    auto* cell=map.GetCellAt(CellStruct{short(anchor.X+offset->X),short(anchor.Y+offset->Y)});
    if(cell==&MapClass::InvalidCell)continue;
    ASSERT_TRUE(contains(cell,building))<<building->Type->ID<<" at "<<cell->MapCoords.X<<","<<cell->MapCoords.Y;
    ASSERT_NE(cell->OccupationFlags&0x80u,0u);++footprintCells;
   }
  }
  ASSERT_GT(footprintCells,BuildingClass::Array.Count);
  auto* type=InfantryTypeClass::Find("E1");ASSERT_NE(type,nullptr);
  auto actor=std::make_unique<InfantryClass>(type,HouseClass::CurrentPlayer);ASSERT_TRUE(actor->InitializeLocomotor());
  BuildingClass* obstacle=nullptr;CellStruct start=CellStruct::Empty,destination=CellStruct::Empty;
  for(auto* building:BuildingClass::Array){
   const auto* kind=building->Type;const auto anchor=building->GetMapCoords();
   if(!building->IsOnMap || kind->GetFoundationWidth()<2 || kind->GetFoundationHeight(false)<2
       || kind->InvisibleInGame || kind->Gate || kind->FirestormWall || kind->LaserFence)continue;
   const CellStruct from{short(anchor.X-1),short(anchor.Y+1)},to{short(anchor.X+kind->GetFoundationWidth()),short(anchor.Y+1)};
   auto* source=map.GetCellAt(from);auto* target=map.GetCellAt(to);
   if(source==&MapClass::InvalidCell || target==&MapClass::InvalidCell || source->FirstObject || target->FirstObject
       || source->Level!=target->Level || !map.IsWithinUsableArea(from,true) || !map.IsWithinUsableArea(to,true))continue;
   actor->Location=source->GetCoords();
   if(actor->IsCellOccupied(source,FacingType::None,-1,nullptr,false)!=Move::OK
       || actor->IsCellOccupied(target,FacingType::None,-1,nullptr,false)!=Move::OK)continue;
   // Deliberately cross a non-anchor foundation row: anchor-only placement
   // allowed the old implementation to walk straight through this row.
   if(!game::foundation_contains(*building,{anchor.X,short(anchor.Y+1)}))continue;
   obstacle=building;start=from;destination=to;break;
  }
  ASSERT_NE(obstacle,nullptr);
  actor->CurrentMission=Mission::Guard;game::attach_map_object(*actor);ASSERT_TRUE(actor->IsOnMap);
  const auto anchor=obstacle->GetMapCoords();
  std::vector<CellClass*> occupied;
  for(auto* offset=obstacle->GetFoundationData(false);*offset!=CellStruct{0x7FFF,0x7FFF};++offset){
   auto* cell=map.GetCellAt(CellStruct{short(anchor.X+offset->X),short(anchor.Y+offset->Y)});
   occupied.push_back(cell);
   EXPECT_EQ(actor->IsCellOccupied(cell,FacingType::None,-1,nullptr,false),Move::No);
  }
  CellStruct follow=CellStruct::Empty;
  ASSERT_TRUE(actor->CellClickedAction(Action::Move,&destination,&follow,false));
  ASSERT_EQ(EventClass::OutList.Count,1);
  const int frame=Unsorted::CurrentFrame;auto restore=ra2::test::scope_exit([&]{Unsorted::CurrentFrame=frame;});
  bool arrived=false,detoured=false;
  for(int step=0;step<900;++step){
   game::update_map_world(*view.world);++Unsorted::CurrentFrame;
   const auto head=static_cast<WalkLocomotionClass*>(actor->Locomotor)->HeadToCoord;
   if(head!=CoordStruct::Empty){
    const CellStruct next{short(head.X/256),short(head.Y/256)};
    // Original Walk checks/reserves destination subcells. Do not invent an
    // extra cell-center collision test on the segment between diagonal spots.
    ASSERT_FALSE(game::foundation_contains(*obstacle,next))<<"step="<<step;
    if(next.Y!=start.Y)detoured=true;
   }
   if(!actor->Destination && actor->GetMapCoords()==destination){arrived=true;break;}
  }
  ASSERT_TRUE(arrived)<<obstacle->Type->ID<<" from "<<start.X<<","<<start.Y<<" to "<<destination.X<<","<<destination.Y;
  ASSERT_TRUE(detoured);
  // Continuous orders while walking, including a non-anchor building cell.
  // This used to abort in Foot.UpdatePathfinding -> Map.NearByLocation.
  CellStruct blocked{anchor.X,short(anchor.Y+1)};
  ASSERT_TRUE(actor->CellClickedAction(Action::Move,&blocked,&follow,false));
  bool repaired=false;
  const auto advance=[&]{
   game::update_map_world(*view.world);++Unsorted::CurrentFrame;
   const auto head=static_cast<WalkLocomotionClass*>(actor->Locomotor)->HeadToCoord;
   if(head!=CoordStruct::Empty)EXPECT_FALSE(game::foundation_contains(*obstacle,{short(head.X/256),short(head.Y/256)}));
  };
  for(int step=0;step<120;++step){
   advance();
   if(actor->Destination && actor->Destination!=map.GetCellAt(blocked))repaired=true;
  }
  ASSERT_TRUE(repaired);
  int issued=1;
  for(int command=0;command<80;++command){
   CellStruct target=command%3==0?blocked:command%3==1?start:destination;
   ASSERT_TRUE(actor->CellClickedAction(Action::Move,&target,&follow,false));++issued;
   for(int step=0;step<3+command%5;++step)advance();
  }
  ASSERT_TRUE(actor->CellClickedAction(Action::Move,&destination,&follow,false));++issued;
  arrived=false;
  for(int step=0;step<900;++step){advance();if(!actor->Destination && actor->GetMapCoords()==destination){arrived=true;break;}}
  ASSERT_TRUE(arrived);EXPECT_EQ(EventClass::OutList.Count,0);
  std::fprintf(stderr,"CONTINUOUS_MOVE commands=%d blocked_repaired=%d final_arrived=%d\n",issued,repaired,arrived);
  game::detach_map_object(*obstacle);
  for(auto* cell:occupied){EXPECT_FALSE(contains(cell,obstacle));EXPECT_EQ(cell->OccupationFlags&0x80u,0u);}
  game::attach_map_object(*obstacle);
  for(auto* cell:occupied){EXPECT_TRUE(contains(cell,obstacle));EXPECT_NE(cell->OccupationFlags&0x80u,0u);}
  std::fprintf(stderr,"BUILDING_BLOCK type=%s foundation_cells=%d crossing=%d,%d -> %d,%d detoured=%d arrived=%d removal_restored=1\n",obstacle->Type->ID,footprintCells,start.X,start.Y,destination.X,destination.Y,detoured,arrived);
 },view.view));
}

TEST(InfantryMovement, FirstMapCellClickRunsRealInfantryFrame) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct Check {game::MapViewHandle* view;InfantryClass* actor=nullptr;CoordStruct start;CellStruct destination=CellStruct::Empty;Point2D pick,click,center;bool picked=false,found=false;}check{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& check=*static_cast<Check*>(p);
  InfantryClass* actor=nullptr;
  for(auto* item:InfantryClass::Array)if(!std::strcmp(item->Type->ID,"E1") && item->Owner->IsControlledByCurrentPlayer()){actor=item;break;}
  ASSERT_NE(actor,nullptr);ASSERT_TRUE(actor->IsInPlayfield);
  check.actor=actor;check.start=actor->Location;check.center=TacticalClass::CoordsToScreen(actor->Location);
 },&check));
 ASSERT_TRUE(game::center_map_view(*view.view,check.center.X,check.center.Y));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& check=*static_cast<Check*>(p);auto* actor=check.actor;auto& world=*check.view->world;
  game::rebuild_world_sprites(world);
  for(const auto& sprite:world.impl->sprites)if(sprite.owner==actor&&!sprite.shadow){
   for(int y=-20;y<=10&&!check.picked;++y)for(int x=-12;x<=12&&!check.picked;++x){
    const Point2D point{sprite.position.X+x,sprite.position.Y+y};
    if(game::pick_world_object(world,point)==actor){check.pick=point;check.picked=true;}
   }
  }
  ASSERT_TRUE(check.picked);
  const auto origin=actor->GetMapCoords();
  for(int y=-2;y<=2 && !check.found;++y)for(int x=-2;x<=2 && !check.found;++x) {
   if(std::abs(x)+std::abs(y)!=2)continue;
   CellStruct candidate{short(origin.X+x),short(origin.Y+y)};
   CoordStruct at{int(candidate.X)*256+128,int(candidate.Y)*256+128,0};at.Z=MapClass::Instance.GetCellFloorHeight(at);
   Point2D point;CellStruct resolved;
   if(actor->MouseOverCell(&candidate)==Action::Move && TacticalClass::CoordsToClient(at,check.view->tactical.TacticalPos,TacticalClass::ViewBounds,point)
       && !game::pick_world_object(world,point) && check.view->tactical.PickTerrainCell(point,TacticalClass::ViewBounds,resolved) && resolved==candidate){
    check.destination=candidate;check.click=point;check.found=true;
   }
  }
  ASSERT_TRUE(check.found);
 },&check));
 ASSERT_TRUE(check.picked);ASSERT_TRUE(check.found);
 for(const auto point:{check.pick,check.click})for(bool down:{true,false}){
  game::GameInputResult result;game::GameInputEvent event{game::GameInputKind::pointer_button,point.X,point.Y,1,0,down};
  ASSERT_TRUE(game::submit_game_input(*view.view,event,result));ASSERT_TRUE(result.consumed);
 }
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& check=*static_cast<Check*>(p);auto* actor=check.actor;
  ASSERT_TRUE(actor->IsSelected);ASSERT_EQ(EventClass::OutList.Count,1);
  const int frame=Unsorted::CurrentFrame;
  auto restore=ra2::test::scope_exit([&]{Unsorted::CurrentFrame=frame;});
  for(int i=0;i<600;++i){game::update_map_world(*check.view->world);++Unsorted::CurrentFrame;}
  EXPECT_GE(LogicClass::Instance.FindItemIndex(actor),0);EXPECT_EQ(EventClass::OutList.Count,0);
  EXPECT_TRUE(actor->IsSelected);EXPECT_NE(actor->Location,check.start);
  EXPECT_EQ(actor->Destination,nullptr);
  EXPECT_EQ(actor->GetMapCoords(),check.destination);
 },&check));
}

// Validate the input to all AStar hierarchy levels, independently of whether
// the final route happens to succeed. Counts come from original placement,
// not from terrain passability or building footprint occupation flags.
static void expect_original_adjacency_counts(const std::vector<CellStruct>& airborneOrigins={}) {
 auto& map=MapClass::Instance;std::vector<unsigned> expected(MapClass::MaxCells,0);
 const auto add=[&](CellStruct at){const int index=MapClass::GetCellIndex(at);
  if(index>=0 && index<map.Cells.Capacity && map.Cells[index])++expected[index];
 };
 const auto adjacent=[&](CellStruct at){for(int face=0;face<8;++face){const auto d=Unsorted::AdjacentCell[face];add({short(at.X+d.X),short(at.Y+d.Y)});}};
 for(auto* object:BuildingClass::Array)if(object->IsOnMap && !object->InLimbo){const auto at=object->GetMapCoords();
  for(int y=-1;y<=object->Type->GetFoundationHeight(false);++y)for(int x=-1;x<=object->Type->GetFoundationWidth();++x)add({short(at.X+x),short(at.Y+y)});
 }
 for(auto* object:TerrainClass::Array)if(object->IsOnMap && !object->InLimbo)adjacent(object->GetMapCoords());
 for(auto* object:InfantryClass::Array)if(object->IsOnMap && !object->InLimbo)adjacent(object->LastMapCoords);
 for(auto* object:UnitClass::Array)if(object->IsOnMap && !object->InLimbo)adjacent(object->LastMapCoords);
 for(auto* object:AircraftClass::Array)if(object->IsOnMap && !object->InLimbo&&object->LastMapCoords!=CellStruct{})adjacent(object->LastMapCoords);
 // YR 0x4D7170 increments around every spawn but leaves LastMapCoords zero
 // for airborne placement. Landing 0x4CE840 adds its new contribution without
 // removing that original spawn contribution. Preserve this observed quirk.
 for(const auto origin:airborneOrigins)adjacent(origin);
 for(int i=0;i<map.Cells.Capacity;++i)if(auto* cell=map.Cells[i]){
  const auto* type=OverlayTypeClass::Array.GetItemOrDefault(cell->OverlayTypeIndex);
  if(type && type->Wall)adjacent(cell->MapCoords);
 }
 for(int i=0;i<map.Cells.Capacity;++i)if(auto* cell=map.Cells[i])
  ASSERT_EQ(unsigned(cell->BlockedNeighbours),expected[i]&0xFFu)<<"cell "<<cell->MapCoords.X<<","<<cell->MapCoords.Y;
}

TEST(InfantryMovement, FirstMapBandboxReachesWarFactory) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct Check {game::MapViewHandle* view;std::vector<InfantryClass*> actors;std::vector<CoordStruct> starts;Point2D center{},first{},last{},single{},click{};CellStruct target{};}check{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& c=*static_cast<Check*>(p);
  expect_original_adjacency_counts();ASSERT_FALSE(::testing::Test::HasFatalFailure());
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"E1") && actor->Owner->IsControlledByCurrentPlayer()){
   c.actors.push_back(actor);c.starts.push_back(actor->Location);const auto point=TacticalClass::CoordsToScreen(actor->Location);c.center+=point;
  }
  ASSERT_GE(c.actors.size(),3u);c.center.X/=int(c.actors.size());c.center.Y/=int(c.actors.size());
 },&check));
 ASSERT_GE(check.actors.size(),3u);ASSERT_TRUE(game::center_map_view(*view.view,check.center.X,check.center.Y));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& c=*static_cast<Check*>(p);
  c.first={1280,720};c.last={0,0};
  for(auto* actor:c.actors){Point2D point;ASSERT_TRUE(TacticalClass::CoordsToClient(actor->Location,c.view->tactical.TacticalPos,TacticalClass::ViewBounds,point));
   if(actor==c.actors.front())c.single=point;
   c.first.X=std::min(c.first.X,point.X-15);c.first.Y=std::min(c.first.Y,point.Y-22);
   c.last.X=std::max(c.last.X,point.X+15);c.last.Y=std::max(c.last.Y,point.Y+10);
  }
  ASSERT_GT(c.first.X,0);ASSERT_GT(c.first.Y,0);ASSERT_LT(c.last.X,TacticalClass::ViewBounds.Width);ASSERT_LT(c.last.Y,TacticalClass::ViewBounds.Height);
  auto* actor=c.actors.front();BuildingClass* factory=nullptr;
  for(auto* building:BuildingClass::Array)if(!std::strcmp(building->Type->ID,"GAWEAP") && building->Owner->IsControlledByCurrentPlayer()){factory=building;break;}
  ASSERT_NE(factory,nullptr);const auto origin=factory->GetMapCoords();bool found=false;
  for(int radius=1;radius<=3 && !found;++radius)for(int y=-radius;y<=factory->Type->GetFoundationHeight(false)+radius && !found;++y)for(int x=-radius;x<=factory->Type->GetFoundationWidth()+radius && !found;++x){
   if(x>=0 && y>=0 && x<factory->Type->GetFoundationWidth() && y<factory->Type->GetFoundationHeight(false))continue;
   const CellStruct cell{short(origin.X+x),short(origin.Y+y)};auto* ground=MapClass::Instance.GetCellAt(cell);
   if(!MapClass::Instance.IsWithinUsableArea(cell,true) || ground->FirstObject || ground->Level!=actor->GetCell()->Level
       || actor->IsCellOccupied(ground,FacingType::None,-1,nullptr,false)!=Move::OK)continue;
   Point2D point;CellStruct picked;
   if(TacticalClass::CoordsToClient(ground->GetCoords(),c.view->tactical.TacticalPos,TacticalClass::ViewBounds,point)
       && point.X>20 && point.X<TacticalClass::ViewBounds.Width-20 && point.Y>20 && point.Y<TacticalClass::ViewBounds.Height-20
       && !game::pick_world_object(*c.view->world,point)
       && c.view->tactical.PickTerrainCell(point,TacticalClass::ViewBounds,picked) && picked==cell){c.target=cell;c.click=point;found=true;}
  }
  ASSERT_TRUE(found);
 },&check));
 const auto input=[&](game::GameInputKind kind,Point2D point,unsigned code,bool pressed,unsigned modifiers=0){
  game::GameInputResult result;EXPECT_TRUE(game::submit_game_input(*view.view,{kind,point.X,point.Y,code,modifiers,pressed},result));
 };
 // Reverse drag across actual displayed GI; selected buildings/enemies are excluded.
 input(game::GameInputKind::pointer_button,check.last,1,true);
 input(game::GameInputKind::pointer_move,check.first,0,false);
 EXPECT_TRUE(view.view->world->impl->dragging);
 input(game::GameInputKind::pointer_button,check.first,1,false);
 EXPECT_FALSE(view.view->world->impl->dragging);
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& c=*static_cast<Check*>(p);
  EXPECT_EQ(TechnoClass::ActionLineTimer.GetTimeLeft(),25);
  EXPECT_EQ(ObjectClass::CurrentObjects.Count,int(c.actors.size()));for(auto* actor:c.actors)EXPECT_TRUE(actor->IsSelected);
  EXPECT_EQ(EventClass::OutList.Count,0);
 },&check));
 // Replacement band, Shift-union and focus-loss cancellation use the same
 // public input path; none is allowed to leak a terrain movement command.
 const Point2D smallStart{check.single.X-2,check.single.Y-2},smallEnd{check.single.X+3,check.single.Y+3};
 input(game::GameInputKind::pointer_button,smallStart,1,true);
 input(game::GameInputKind::pointer_move,smallEnd,0,false);
 input(game::GameInputKind::pointer_button,smallEnd,1,false);
 EXPECT_EQ(ObjectClass::CurrentObjects.Count,1);
 input(game::GameInputKind::pointer_button,check.first,1,true,1);
 input(game::GameInputKind::pointer_move,check.last,0,false,1);
 input(game::GameInputKind::pointer_button,check.last,1,false,1);
 EXPECT_EQ(ObjectClass::CurrentObjects.Count,int(check.actors.size()));
 input(game::GameInputKind::pointer_button,smallStart,1,true);
 input(game::GameInputKind::pointer_move,smallEnd,0,false);
 input(game::GameInputKind::focus_lost,smallEnd,0,false);
 EXPECT_FALSE(view.view->world->impl->dragging);
 input(game::GameInputKind::focus_gained,smallEnd,0,false);
 input(game::GameInputKind::pointer_button,smallEnd,1,false);
 EXPECT_EQ(ObjectClass::CurrentObjects.Count,int(check.actors.size()));
 input(game::GameInputKind::pointer_button,check.click,1,true);input(game::GameInputKind::pointer_button,check.click,1,false);
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& c=*static_cast<Check*>(p);
  EXPECT_EQ(TechnoClass::ActionLineTimer.GetTimeLeft(),25);
  ASSERT_EQ(EventClass::OutList.Count,int(c.actors.size()));
  const int frame=Unsorted::CurrentFrame;auto restore=ra2::test::scope_exit([&]{Unsorted::CurrentFrame=frame;});
  int settled=0;std::vector<CellStruct> airborneOrigins;
  for(int step=0;step<1800;++step){
   const int planes=AircraftClass::Array.Count;
   game::update_map_world(*c.view->world);++Unsorted::CurrentFrame;
   for(int i=planes;i<AircraftClass::Array.Count;++i)if(AircraftClass::Array[i]->LastMapCoords==CellStruct{})airborneOrigins.push_back(AircraftClass::Array[i]->GetMapCoords());
   settled=0;for(auto* actor:c.actors)if(!actor->Destination && !actor->Locomotor->Is_Moving())++settled;
   if(step>0 && settled==int(c.actors.size()))break;
  }
  EXPECT_EQ(settled,int(c.actors.size()));int moved=0;
  for(std::size_t i=0;i<c.actors.size();++i){auto* actor=c.actors[i];if(actor->Location!=c.starts[i])++moved;
   EXPECT_TRUE(actor->IsSelected);const auto at=actor->GetMapCoords();
   EXPECT_LE(std::max(std::abs(at.X-c.target.X),std::abs(at.Y-c.target.Y)),2);
   for(std::size_t j=0;j<i;++j)EXPECT_NE(actor->Location,c.actors[j]->Location);
  }
  EXPECT_EQ(moved,int(c.actors.size()));
  expect_original_adjacency_counts(airborneOrigins);ASSERT_FALSE(::testing::Test::HasFatalFailure());
  game::detach_map_object(*c.actors.back());
  expect_original_adjacency_counts(airborneOrigins);ASSERT_FALSE(::testing::Test::HasFatalFailure());
  game::attach_map_object(*c.actors.back());
  expect_original_adjacency_counts(airborneOrigins);ASSERT_FALSE(::testing::Test::HasFatalFailure());
  std::fprintf(stderr,"GI_FACTORY_ROUTE selected=%zu moved=%d arrived=%d target=%d,%d adjacency_balanced=1\n",c.actors.size(),moved,settled,c.target.X,c.target.Y);
 },&check));
}

TEST(InfantryMovement, CoordinatesAndFootprintDoNotUnlimboOrExpireReferences) {
 const int rounding=std::fegetround();auto restore_rounding=ra2::test::scope_exit([&]{std::fesetround(rounding);});
 const auto root=std::filesystem::temp_directory_path()/("ra2-mark-contract-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Actor:InfantryClass {
   CellStruct footprint[3]{{0,0},{1,0},{0x7FFF,0x7FFF}};
   Actor(InfantryTypeClass* t,HouseClass* h):InfantryClass(t,h){}
   const CellStruct* GetFoundationData(bool)const override{return footprint;}
  };
  auto& map=MapClass::Instance;InfantryTypeClass type("MARK_GI");type.Locomotor=LocomotionClass::CLSIDs::Walk;
  Actor gi(&type,HouseClass::Array[0]);ASSERT_TRUE(gi.InitializeLocomotor());
  auto* driver=static_cast<WalkLocomotionClass*>(static_cast<ILocomotion*>(gi.Locomotor));
  auto* from=map.GetCellAt(CellStruct{8,5});auto* tail=map.GetCellAt(CellStruct{9,5});
  auto* to=map.GetCellAt(CellStruct{10,5});auto* to_tail=map.GetCellAt(CellStruct{11,5});
  for(auto* c:{from,tail,to,to_tail}){ASSERT_EQ(c->FirstObject,nullptr);c->IsoTileTypeIndex=0xFFFF;c->OverlayTypeIndex=-1;c->Level=0;c->Flags=static_cast<CellFlags>(0);}
  gi.Location={8*256+192,5*256+64,0};gi.InLimbo=false;gi.NeedsRedraw=false;
  ASSERT_TRUE(LogicClass::Instance.AddObject(&gi,false));ASSERT_TRUE(gi.Mark(MarkType::Down));
  ASSERT_EQ(from->FirstObject,&gi);ASSERT_EQ(tail->FirstObject,&gi);const int logic_count=LogicClass::Instance.Count;
  gi.MarkAllOccupationBits(gi.Location);
  BuildingLightClass light(&gi);light.FollowingObject=&gi;
  const auto old=gi.Location;const bool redraw=gi.NeedsRedraw;
  gi.ObjectClass::SetLocation({10*256+192,5*256+64,700});
  EXPECT_EQ(gi.Location.Z,700);EXPECT_TRUE(gi.IsOnMap);EXPECT_FALSE(gi.InLimbo);EXPECT_TRUE(gi.IsInLogic);
  EXPECT_EQ(gi.NeedsRedraw,redraw);EXPECT_EQ(from->FirstObject,&gi);EXPECT_EQ(tail->FirstObject,&gi);EXPECT_EQ(to->FirstObject,nullptr);
  EXPECT_EQ(light.FollowingObject,&gi);EXPECT_EQ(light.OwnerObject,&gi);EXPECT_EQ(from->OccupationFlags&0x1Cu,4u);
  gi.ObjectClass::SetLocation(old);
  // Reserve before lifting: content membership and occupation are independent.
  ASSERT_TRUE(driver->Mark_Head_To({10*256+192,5*256+64,0}));
  EXPECT_EQ(from->OccupationFlags&0x1Cu,0u);EXPECT_EQ(to->OccupationFlags&0x1Cu,4u);
  gi.SetLocation(driver->HeadToCoord);
  EXPECT_EQ(from->FirstObject,nullptr);EXPECT_EQ(tail->FirstObject,nullptr);
  EXPECT_EQ(to->FirstObject,&gi);EXPECT_EQ(to_tail->FirstObject,&gi);EXPECT_EQ(to->OccupationFlags&0x1Cu,4u);
  EXPECT_TRUE(gi.IsInLogic);EXPECT_EQ(LogicClass::Instance.Count,logic_count+1); // light is also in Logic
  EXPECT_EQ(light.FollowingObject,&gi);EXPECT_FALSE(gi.InLimbo);
  for(auto* c:{from,tail,to,to_tail}){
   const int index=map.GetCellZoneIndex(c->MapCoords);
   EXPECT_EQ(map.LevelAndPassability[index].CellPassability,static_cast<char>(c->Passability));
   EXPECT_EQ(map.LevelAndPassabilityStruct2pointer_70[index].CellLevel,c->Level);
  }
  ASSERT_TRUE(gi.Mark(MarkType::Up));EXPECT_TRUE(gi.IsInLogic);EXPECT_FALSE(gi.InLimbo);
  gi.UnmarkAllOccupationBits(driver->HeadToCoord);LogicClass::Instance.RemoveObject(&gi);
 },nullptr));
}

TEST(InfantryMovement, VirtualOrdersPreservePostureDockerAndOneShotSkip) {
 const int rounding=std::fegetround();auto restore_rounding=ra2::test::scope_exit([&]{std::fesetround(rounding);});
 const auto root=std::filesystem::temp_directory_path()/("ra2-order-contract-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Driver:WalkLocomotionClass {
   int moves=0,stops=0;CoordStruct received{};
   void YRPP_STDCALL Move_To(CoordStruct value)override{++moves;received=value;}
   void YRPP_STDCALL Stop_Moving()override{++stops;}
  };
  struct Actor:InfantryClass {
   Sequence requested=Sequence::Nothing;int animations=0;
   mutable FacingType checked=FacingType::None;mutable int level=-1;
   Actor(InfantryTypeClass* type,HouseClass* owner):InfantryClass(type,owner){}
   bool PlayAnim(Sequence sequence,bool,bool)override{requested=sequence;++animations;return true;}
   Move IsCellOccupied(CellClass*,FacingType facing,int height,CellClass*,bool)const override{checked=facing;level=height;return Move::Temp;}
  };
  struct Destination:InfantryClass {
   mutable TechnoClass* docker=nullptr;
   explicit Destination(InfantryTypeClass* type):InfantryClass(type,nullptr){}
   CoordStruct* GetDestination(CoordStruct* out,TechnoClass* visitor)const override{docker=visitor;*out={1234,2345,345};return out;}
  };
  auto* owner=HouseClass::Array[0];const bool human=owner->IsHumanPlayer;owner->IsHumanPlayer=true;
  auto restore_owner=ra2::test::scope_exit([&]{owner->IsHumanPlayer=human;});
  InfantryTypeClass type("ORDER_GI");type.Fraidycat=type.Cyborg=type.JumpJet=false;
  Actor gi(&type,owner);Destination destination(&type),link(&type);
  auto* driver=GameCreate<Driver>();ASSERT_NE(driver,nullptr);driver->Link_To_Object(&gi);
#if !defined(_MSC_VER)
  driver->AddRef();
#endif
  gi.Locomotor=driver;gi.PathDirections[0]=3;gi.SequenceAnim=Sequence::Deployed;
  gi.DirectRockerLinkedUnit=&link;link.DirectRockerLinkedUnit=&gi;
  TechnoClass* command=&gi;command->SetDestination(&destination,true);
  EXPECT_EQ(gi.Destination,nullptr);EXPECT_EQ(driver->moves,0);EXPECT_EQ(gi.PathDirections[0],3);EXPECT_EQ(gi.DirectRockerLinkedUnit,&link);
  gi.SequenceAnim=Sequence::Ready;gi.unknown_bool_6AC=true;
  command->SetDestination(&destination,true);
  EXPECT_EQ(gi.Destination,&destination);EXPECT_EQ(gi.PathDirections[0],-1);EXPECT_EQ(driver->moves,0);EXPECT_FALSE(gi.unknown_bool_6AC);
  EXPECT_EQ(gi.DirectRockerLinkedUnit,nullptr);EXPECT_EQ(link.DirectRockerLinkedUnit,nullptr);EXPECT_EQ(driver->RefCount,1);
  gi.Crawling=true;command->SetDestination(&destination,true);
  EXPECT_EQ(gi.requested,Sequence::Up);EXPECT_EQ(driver->moves,1);EXPECT_EQ(destination.docker,&gi);EXPECT_EQ(driver->received,(CoordStruct{1234,2345,345}));
  gi.Location={8*256+192,6*256+64,0};gi.PrimaryFacing.SetCurrent(DirStruct(0x4000));
  gi.SequenceAnim=Sequence::Walk;FootClass* foot=&gi;
  EXPECT_FALSE(foot->StopMoving());EXPECT_EQ(gi.requested,Sequence::Prone);EXPECT_EQ(driver->stops,1);
  EXPECT_EQ(gi.checked,FacingType::East);EXPECT_EQ(gi.level,gi.GetCellLevel());EXPECT_TRUE(gi.unknown_bool_6DC);
  gi.SequenceAnim=Sequence::Deploy;EXPECT_FALSE(foot->StopMoving());EXPECT_EQ(gi.requested,Sequence::Deployed);EXPECT_EQ(driver->stops,2);
  gi.SequenceAnim=Sequence::Ready;command->SetDestination(nullptr,true);EXPECT_EQ(gi.Destination,nullptr);EXPECT_EQ(driver->stops,3);
 },nullptr));
}

TEST(InfantryLocomotion, OriginalWalkStateCorpus) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-walk-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Actor:InfantryClass {
   int stopped=0,cleared=0;CoordStruct cleared_at{};
   Actor():InfantryClass(nullptr,nullptr){}
   void vt_entry_54C()override{++stopped;}
   void UnmarkAllOccupationBits(const CoordStruct& at)override{++cleared;cleared_at=at;}
  } gi;
  WalkLocomotionClass walk;walk.Link_To_Object(&gi);
  auto*cell=MapClass::Instance.GetCellAt(CellStruct{8,6});
  const auto saved_flags=cell->Flags;
  std::ifstream input(RA2_WALK_LOCOMOTION_FIXTURE);ASSERT_TRUE(input.good());int operation;unsigned cases=0;
  while(input>>operation){
   int flags,expected[17];ASSERT_TRUE(bool(input>>flags));for(auto&v:expected)ASSERT_TRUE(bool(input>>v));
   gi.Location={8*256+100,6*256+100,17};walk.DestinationCoord={9*256+180,6*256+100,7};
   walk.HeadToCoord=flags&4?CoordStruct{8*256+48,6*256+200,128+int(bool(flags&2))}:CoordStruct{};
   walk.IsMoving=flags&1;walk.IsReallyMoving=flags&2;walk.InProcessing=flags&256;
   gi.EMPLockRemaining=bool(flags&8);gi.BeingWarpedOut=flags&16;gi.WarpingOut=flags&32;
   gi.SpeedPercentage=flags&64?0.75:0.0;cell->Flags=static_cast<CellFlags>(flags&128?0x100:0);
   gi.PrimaryFacing.SetCurrent(DirStruct(0x8000));gi.stopped=gi.cleared=0;gi.cleared_at={};
   switch(operation){
    case 0:walk.Move_To({});break;case 1:walk.Move_To({8*256+200,6*256+200,24});break;
    case 2:walk.Stop_Moving();break;case 3:walk.Limbo();break;case 4:walk.Stop_Movement_Animation();break;
    case 5:walk.Do_Turn(DirStruct(flags*127));break;case 6:walk.Mark_All_Occupation_Bits(MarkType::Up);break;
    case 7:walk.Mark_All_Occupation_Bits(MarkType::Down);break;
   }
   const auto d=walk.DestinationCoord,h=walk.HeadToCoord,c=gi.cleared_at;
   const int actual[]{d.X,d.Y,d.Z,h.X,h.Y,h.Z,int(walk.IsMoving),int(walk.InProcessing),int(walk.IsReallyMoving),
     int(walk.Is_Moving_Now()),int(walk.Is_Moving_Here({8*256+200,6*256+200,24})),gi.stopped,gi.cleared,c.X,c.Y,c.Z,gi.PrimaryFacing.Current().Raw};
   for(unsigned i=0;i<17;++i)ASSERT_EQ(actual[i],expected[i])<<"case="<<cases<<" field="<<i;
   EXPECT_EQ(walk.Destination(),walk.IsMoving?d:CoordStruct{});EXPECT_EQ(walk.Head_To_Coord(),h!=CoordStruct{}?h:gi.Location);
   ++cases;
  }
  cell->Flags=saved_flags;EXPECT_EQ(cases,4608u);
 },nullptr));
}

TEST(InfantryLocomotion, OriginalWalkProcessStepAndArrivalCorpus) {
 const int rounding=std::fegetround();auto restore_rounding=ra2::test::scope_exit([&]{std::fesetround(rounding);});
 const auto root=std::filesystem::temp_directory_path()/("ra2-walk-process-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  // Boundary doubles match the EXE corpus. Actual Foot/Map occupation is
  // covered separately; these callbacks do NOT replace production methods.
  struct Actor:InfantryClass {
   WalkLocomotionClass* driver=nullptr;int speed=0;std::array<int,7> calls{};
   Actor(InfantryTypeClass* type):InfantryClass(type,nullptr){}
   bool Mark(MarkType mark)override{++calls[static_cast<int>(mark)];IsOnMap=mark==MarkType::Down;return true;}
   void SetLocation(const CoordStruct& at)override{++calls[2];Location=at;}
   void SetHeight(DWORD height)override{Location.Z=int(height)+(MapClass::Instance.GetCellAt(Location)->Level+(OnBridge?4:0))*104;}
   int GetCurrentSpeed()const override{return speed;}
   void UpdatePosition(PCPType value)override{EXPECT_EQ(value,PCPType::End);EXPECT_TRUE(driver->InProcessing);++calls[3];}
   void UnmarkAllOccupationBits(const CoordStruct&)override{++calls[4];}
   void MarkAllOccupationBits(const CoordStruct&)override{}
   void vt_entry_54C()override{++calls[5];}
   void SetDestination(AbstractClass* destination,bool)override{EXPECT_EQ(destination,nullptr);++calls[6];driver->DestinationCoord=CoordStruct::Empty;}
  };
  InfantryTypeClass type("WALK_PROCESS");type.Locomotor=LocomotionClass::CLSIDs::Walk;Actor gi(&type);
  ASSERT_TRUE(gi.InitializeLocomotor());
  auto& walk=*static_cast<WalkLocomotionClass*>(static_cast<LocomotionClass*>(gi.Locomotor));gi.driver=&walk;
  std::fesetround(FE_TOWARDZERO);
  std::ifstream input(RA2_WALK_PROCESS_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0;int variant;
  while(input>>variant){
   int dx,dy,speed,flags;std::array<int,31> expected{};
   ASSERT_TRUE(bool(input>>dx>>dy>>speed>>flags));for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
   SCOPED_TRACE(cases);
   for(int y=4;y<=8;++y)for(int x=6;x<=10;++x){auto* cell=MapClass::Instance.GetCellAt(CellStruct{short(x),short(y)});
    const bool old=x==8 && y==6;cell->Level=old || !(flags&8)?4:0;
    cell->Flags=static_cast<CellFlags>((old?bool(flags&2):bool(flags&4))?0x100:0);cell->OverlayTypeIndex=-1;
   }
   gi.Location={8*256+(variant?250:128),6*256+(variant?250:128),17};
   walk.HeadToCoord=gi.Location+CoordStruct{dx,dy,0};walk.DestinationCoord=flags&16?CoordStruct::Empty:walk.HeadToCoord;
   walk.IsMoving=true;walk.InProcessing=walk.IsReallyMoving=false;
   gi.IsOnMap=gi.IsAlive=true;gi.InLimbo=gi.IsFallingDown=false;gi.OnBridge=flags&2;
   gi.EMPLockRemaining=flags&1?1:0;gi.ShouldScanForTarget=gi.IsWaitingBlockagePath=true;
   gi.SpeedPercentage=0.5;gi.speed=speed;gi.calls.fill(0);gi.CurrentMapCoords={8,6};
   gi.PrimaryFacing.SetCurrent(DirStruct(0x1234));
   for(int i=0;i<24;++i)gi.PathDirections[i]=i<3?2:-1;
   const bool moving=walk.Process();
   const std::array<int,31> actual{gi.Location.X,gi.Location.Y,gi.Location.Z,
    walk.HeadToCoord.X,walk.HeadToCoord.Y,walk.HeadToCoord.Z,
    walk.DestinationCoord.X,walk.DestinationCoord.Y,walk.DestinationCoord.Z,
    walk.IsMoving,walk.InProcessing,walk.IsReallyMoving,moving,gi.IsOnMap,gi.OnBridge,
    gi.PrimaryFacing.Current().Raw,int(gi.SpeedPercentage*2),
    gi.PathDirections[0],gi.PathDirections[1],gi.PathDirections[2],gi.PathDirections[23],
    int(static_cast<unsigned short>(gi.CurrentMapCoords.X))+(int(static_cast<unsigned short>(gi.CurrentMapCoords.Y))<<16),
    gi.ShouldScanForTarget,gi.IsWaitingBlockagePath,gi.calls[0],gi.calls[1],gi.calls[2],gi.calls[3],gi.calls[4],gi.calls[5],gi.calls[6]};
   EXPECT_EQ(actual,expected);
   ++cases;
  }
  EXPECT_EQ(cases,3840u);gi.IsOnMap=false;gi.InLimbo=true;
 },nullptr));
}

TEST(InfantryLocomotion, OriginalCOMIdentityOwnershipAndPiggybackTransfer) {
 auto*walk=GameCreate<WalkLocomotionClass>();ASSERT_NE(walk,nullptr);EXPECT_EQ(walk->RefCount,0);EXPECT_EQ(walk->AddRef(),1u);
 struct Owner{WalkLocomotionClass* p;~Owner(){p->Release();}}owner{walk};
 InfantryClass gi(nullptr,nullptr);EXPECT_EQ(walk->Link_To_Object(&gi),0);EXPECT_EQ(walk->Owner,&gi);EXPECT_EQ(walk->LinkedTo,&gi);
 constexpr GUID unknown{0,0,0,{0xC0,0,0,0,0,0,0,0x46}},persist{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}},
  stream{0x10C,0,0,{0xC0,0,0,0,0,0,0,0x46}},loco{0x070F3290,0x9841,0x11D1,{0xB7,0x09,0,0xA0,0x24,0xDD,0xAF,0xD1}},
  piggy{0x92FEA800,0xA184,0x11D1,{0xB7,0x0A,0,0xA0,0x24,0xDD,0xAF,0xD1}},bad{};
 const GUID identities[]{unknown,persist,stream,loco,piggy};
 void* addresses[]{static_cast<ILocomotion*>(walk),static_cast<IPersistStream*>(walk),static_cast<IPersistStream*>(walk),static_cast<ILocomotion*>(walk),static_cast<IPiggyback*>(walk)};
 for(unsigned i=0;i<5;++i){void* output=nullptr;EXPECT_EQ(walk->QueryInterface(identities[i],&output),0);EXPECT_EQ(output,addresses[i]);EXPECT_EQ(walk->RefCount,2);EXPECT_EQ(walk->Release(),1u);}
 void* rejected=walk;EXPECT_EQ(walk->QueryInterface(bad,&rejected),static_cast<HRESULT>(0x80004002u));EXPECT_EQ(rejected,nullptr);
 EXPECT_EQ(walk->QueryInterface(unknown,nullptr),static_cast<HRESULT>(0x80004003u));
 CLSID id{};EXPECT_EQ(walk->GetClassID(&id),0);EXPECT_EQ(std::memcmp(&id,&LocomotionClass::CLSIDs::Walk,sizeof(id)),0);
 EXPECT_EQ(walk->GetClassID(nullptr),static_cast<HRESULT>(0x80004003u));
 EXPECT_EQ(walk->Piggyback_CLSID(&id),0);EXPECT_EQ(walk->RefCount,1);
 ULARGE_INTEGER size{};EXPECT_EQ(walk->GetSizeMax(&size),0);EXPECT_EQ(size.QuadPart,0x40u);
 EXPECT_EQ(walk->IsDirty(),0);walk->Dirty=false;EXPECT_EQ(walk->IsDirty(),1);
 EXPECT_FALSE(walk->Power_Off());EXPECT_FALSE(walk->Is_Powered());EXPECT_TRUE(walk->Power_On());
 EXPECT_TRUE(walk->LocomotionClass::Process());EXPECT_EQ(walk->Get_Track_Number(),-1);EXPECT_EQ(walk->Get_Track_Index(),-1);EXPECT_EQ(walk->Get_Speed_Accum(),-1);
 auto*child=GameCreate<WalkLocomotionClass>();ASSERT_NE(child,nullptr);child->AddRef();
 EXPECT_EQ(walk->Begin_Piggyback(child),0);EXPECT_EQ(child->RefCount,2);EXPECT_TRUE(walk->Is_Piggybacking());
 EXPECT_EQ(walk->Begin_Piggyback(child),static_cast<HRESULT>(0x80004005u));EXPECT_TRUE(walk->Is_Ok_To_End());
 gi.IsAttackedByLocomotor=true;EXPECT_FALSE(walk->Is_Ok_To_End());gi.IsAttackedByLocomotor=false;
 walk->InProcessing=true;EXPECT_FALSE(walk->Is_Ok_To_End());walk->InProcessing=false;
 walk->IsMoving=true;EXPECT_FALSE(walk->Is_Ok_To_End());walk->IsMoving=false;
 EXPECT_EQ(walk->Piggyback_CLSID(&id),0);EXPECT_EQ(child->RefCount,2);
 ILocomotion* returned=nullptr;EXPECT_EQ(walk->End_Piggyback(&returned),0);EXPECT_EQ(returned,static_cast<ILocomotion*>(child));
 EXPECT_EQ(child->RefCount,2);EXPECT_FALSE(walk->Is_Piggybacking());EXPECT_EQ(walk->End_Piggyback(&returned),1);EXPECT_EQ(returned,static_cast<ILocomotion*>(child));
 EXPECT_EQ(returned->Release(),1u);EXPECT_EQ(child->Release(),0u);
 // The Foot owns one reference, and releases it at destruction.
 auto*foot=GameCreate<InfantryClass>(nullptr,nullptr);
#if !defined(_MSC_VER)
 walk->AddRef();
#endif
 foot->Locomotor=walk;GameDelete(foot);EXPECT_EQ(walk->RefCount,1);
}

TEST(InfantryLocomotion, OriginalFootDestinationVirtualDispatch) {
 InfantryTypeClass type("DESTINATION_GI");type.Locomotor=LocomotionClass::CLSIDs::Walk;
 InfantryClass actor(&type,nullptr);ASSERT_TRUE(actor.InitializeLocomotor());
 auto* walk=dynamic_cast<WalkLocomotionClass*>(actor.Locomotor);ASSERT_NE(walk,nullptr);
 CellStruct entrance{1,2};TubeClass tube(&entrance,0);tube.ExitCell={17,29};
 int tubeIndex=-1;for(int i=0;i<TubeClass::Array.Count;++i)if(TubeClass::Array[i]==&tube)tubeIndex=i;
 ASSERT_GE(tubeIndex,0);ASSERT_LT(tubeIndex,128);
 auto restore=ra2::test::scope_exit([&]{actor.TubeIndex=-1;});
 const AbstractClass* moving=&actor;
 std::ifstream input(RA2_FOOT_DESTINATION_FIXTURE);ASSERT_TRUE(input.good());int tunnel,cases=0;
 while(input>>tunnel){
  int isMoving,docker;CoordStruct location,head,expected,actual;
  ASSERT_TRUE(bool(input>>isMoving>>location.X>>location.Y>>location.Z>>head.X>>head.Y>>head.Z>>docker>>expected.X>>expected.Y>>expected.Z));
  actor.Location=location;actor.TubeIndex=tunnel?tubeIndex:-1;walk->HeadToCoord=head;walk->IsMoving=isMoving;
  ASSERT_EQ(moving->GetDestination(&actual,docker?&actor:nullptr),&actual);
  ASSERT_EQ(actual,expected)<<"original virtual destination case "<<cases;
  ++cases;
 }
 EXPECT_EQ(cases,120);
}

TEST(InfantryLocomotion, OriginalSharedReservationTransitions) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-shared-reservation-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);std::ofstream(root/"world.map")<<"[Map]\nSize=0,0,32,32\nLocalSize=0,0,32,32\nLevel=0\nTheater=TEMPERATE\n[Basic]\nNewINIFormat=4\n";
 View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Actor:InfantryClass {Actor():InfantryClass(nullptr,nullptr){}int GetOwningHouseIndex()const override{return 0;}};
  std::array<Actor,5> actors;std::array<WalkLocomotionClass,5> drivers;
  const CellStruct cells[]{{24,26},{25,26},{26,26},{27,26},{28,26},{32,32},{33,32}};
  const Point2D spots[]{{64,64},{128,128},{192,64},{64,192},{192,192}};
  const int steps[][2]{{0,0},{1,0},{2,0},{3,0},{4,0},{1,1},{4,0},{0,2},{2,3},{0,1},{1,1},{2,1},{3,1},{4,1}};
  auto& map=MapClass::Instance;
  auto restore=ra2::test::scope_exit([&]{for(const auto at:cells){auto* c=map.GetCellAt(at);c->OccupationFlags=0;c->InfantryOwnerIndex=-1;}});
  std::ifstream input(RA2_WALK_RESERVATION_FIXTURE);ASSERT_TRUE(input.good());int seed,cases=0;
  while(input>>seed){
   int spot,mask,step;ASSERT_TRUE(bool(input>>spot>>mask>>step));std::array<int,32> expected;for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
   if(step==0){
    ScenarioClass::Instance->Random=Randomizer(seed);
    for(const auto at:cells){auto* cell=map.GetCellAt(at);cell->OccupationFlags=0;cell->AltOccupationFlags=0;cell->InfantryOwnerIndex=-1;cell->OverlayTypeIndex=-1;cell->Level=0;cell->SlopeIndex=0;cell->Flags=CellFlags{};}
    for(int i=0;i<5;++i){auto& actor=actors[i];actor.Location={(24+i)*256+spots[spot].X,26*256+spots[spot].Y,0};actor.CurrentMission=Mission::Attack;
     drivers[i].Link_To_Object(&actor);drivers[i].HeadToCoord={};actor.MarkAllOccupationBits(actor.Location);}
    map.GetCellAt(CellStruct{32,32})->OccupationFlags=mask;
   }
   const int i=steps[step][0],operation=steps[step][1];
   if(operation==2&&drivers[i].HeadToCoord!=CoordStruct::Empty)actors[i].Location=drivers[i].HeadToCoord;
   const CoordStruct request=operation==1||operation==2?CoordStruct::Empty:CoordStruct{(operation==3?33:32)*256+spots[spot].X,32*256+spots[spot].Y,0};
   std::array<int,32> actual{};int index=0;actual[index++]=drivers[i].Mark_Head_To(request);
   for(auto& driver:drivers){actual[index++]=driver.HeadToCoord.X;actual[index++]=driver.HeadToCoord.Y;actual[index++]=driver.HeadToCoord.Z;}
   for(const auto at:cells){auto* c=map.GetCellAt(at);actual[index++]=c->OccupationFlags;actual[index++]=c->InfantryOwnerIndex;}
   actual[index++]=ScenarioClass::Instance->Random.Next1;actual[index++]=ScenarioClass::Instance->Random.Next2;
   ASSERT_EQ(actual,expected)<<"seed "<<seed<<" spot "<<spot<<" mask "<<mask<<" step "<<step;++cases;
  }
  EXPECT_EQ(cases,2240);
 },nullptr));
}

TEST(InfantryLocomotion, ReservationMovesBitsAndRestoresOnFailure) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-walk-reserve-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Actor:InfantryClass {int stops=0;Actor():InfantryClass(nullptr,nullptr){}void vt_entry_54C()override{++stops;}}gi;
  WalkLocomotionClass walk;walk.Link_To_Object(&gi);gi.Location={8*256+192,6*256+64,0};
  gi.CurrentMission=Mission::Move;
  auto*from=MapClass::Instance.GetCellAt(CellStruct{8,6});auto*to=MapClass::Instance.GetCellAt(CellStruct{9,6});
  from->Level=to->Level=0;from->SlopeIndex=to->SlopeIndex=0;from->Flags=to->Flags=static_cast<CellFlags>(0);
  from->OccupationFlags=to->OccupationFlags=from->AltOccupationFlags=to->AltOccupationFlags=0;
  gi.MarkAllOccupationBits(gi.Location);EXPECT_EQ(from->OccupationFlags,4u);
  const CoordStruct requested{9*256+192,6*256+64,0};
  ASSERT_TRUE(walk.Mark_Head_To(requested));EXPECT_EQ(walk.HeadToCoord,requested);
  EXPECT_EQ(from->OccupationFlags,0u);EXPECT_EQ(to->OccupationFlags,4u);
  walk.IsMoving=walk.IsReallyMoving=true;walk.DestinationCoord={};
  walk.Force_Immediate_Destination({});EXPECT_EQ(walk.HeadToCoord,CoordStruct{});EXPECT_FALSE(walk.IsMoving);
  EXPECT_TRUE(walk.IsReallyMoving);EXPECT_EQ(gi.stops,1);EXPECT_EQ(from->OccupationFlags,4u);EXPECT_EQ(to->OccupationFlags,0u);
  // Failure restores the current subspot; no phantom reservation in the full cell.
  to->OccupationFlags=0x1C;EXPECT_FALSE(walk.Mark_Head_To(requested));EXPECT_EQ(walk.HeadToCoord,CoordStruct{});
  EXPECT_EQ(from->OccupationFlags,4u);EXPECT_EQ(to->OccupationFlags,0x1Cu);
  // At > 3 levels, reserve on the bridge, not in the full ground cell below it.
  to->Flags=static_cast<CellFlags>(0x100);gi.Location.Z=4*Unsorted::LevelHeight;
  from->OccupationFlags=0;from->AltOccupationFlags=4;
  ASSERT_TRUE(walk.Mark_Head_To(requested));EXPECT_EQ(walk.HeadToCoord.Z,CellClass::BridgeHeight);
  EXPECT_EQ(from->AltOccupationFlags,0u);EXPECT_EQ(to->AltOccupationFlags,4u);EXPECT_EQ(to->OccupationFlags,0x1Cu);
  walk.Mark_All_Occupation_Bits(MarkType::Up);EXPECT_EQ(to->AltOccupationFlags,0u);
  // Original nearest-Techno query preserves first ties, excludes a given object,
  // and uses the requested content chain even before the objects are on-map.
  InfantryClass a(nullptr,nullptr),b(nullptr,nullptr),bridge(nullptr,nullptr);
  a.Location=b.Location={100,100,0};bridge.Location={30,30,0};a.NextObject=&b;
  from->FirstObject=&a;from->AltObject=&bridge;
  EXPECT_EQ(from->FindTechnoNearestTo({0,0},false),&a);EXPECT_EQ(from->FindTechnoNearestTo({0,0},false,&a),&b);
  EXPECT_EQ(from->FindTechnoNearestTo({0,0},true),&bridge);
  from->FirstObject=from->AltObject=nullptr;a.NextObject=nullptr;
 },nullptr));
}

TEST(InfantryLocomotion, OriginalMovementAnimationCallbacks) {
 struct Actor:InfantryClass {
  int actions=0;Sequence action=Sequence::Nothing;Actor():InfantryClass(nullptr,nullptr){}
  bool PlayAnim(Sequence s,bool,bool)override{++actions;action=s;return true;}
 } gi;
 for(int i=-1;i<42;++i){gi.SequenceAnim=static_cast<Sequence>(i);gi.vt_entry_548();EXPECT_EQ(int(gi.SequenceAnim),i==3||i==6||i==17?-1:i);}
 gi.ShouldDeploy=false;gi.vt_entry_54C();EXPECT_EQ(gi.actions,0);
 gi.ShouldDeploy=true;gi.vt_entry_54C();EXPECT_FALSE(gi.ShouldDeploy);EXPECT_EQ(gi.actions,1);EXPECT_EQ(gi.action,Sequence::Deploy);
 gi.EMPLockRemaining=0xFFFFFFFFu;EXPECT_FALSE(gi.IsUnderEMP());gi.EMPLockRemaining=1;EXPECT_TRUE(gi.IsUnderEMP());
}

TEST(InfantryLocomotion, OriginalElevationRampAndBridgeReachCorpus) {
 struct Cells{CellClass* destination=CellClass::Create();CellClass* source=CellClass::Create();~Cells(){GameDelete(destination);GameDelete(source);}}cells;
 ASSERT_NE(cells.destination,nullptr);ASSERT_NE(cells.source,nullptr);InfantryClass gi(nullptr,nullptr);
 ObjectClass* dispatch=&gi;
 std::ifstream input(RA2_FOOT_REACH_FIXTURE);ASSERT_TRUE(input.good());int dh;unsigned cases=0;
 while(input>>dh){
  int sh,df,sf,ds,ss,level,facing,bridge,result,expected_level,expected_bridge;
  ASSERT_TRUE(bool(input>>sh>>df>>sf>>ds>>ss>>level>>facing>>bridge>>result>>expected_level>>expected_bridge));
  cells.destination->Level=dh;cells.source->Level=sh;cells.destination->Flags=static_cast<CellFlags>(df*0x100);cells.source->Flags=static_cast<CellFlags>(sf*0x100);
  cells.destination->SlopeIndex=ds;cells.source->SlopeIndex=ss;bool alt=bridge!=0;
  ASSERT_EQ(int(dispatch->CanReachCell(cells.destination,static_cast<FacingType>(facing),level,alt,cells.source)),result)<<cases;
  ASSERT_EQ(level,expected_level)<<cases;ASSERT_EQ(int(alt),expected_bridge)<<cases;++cases;
 }
 EXPECT_EQ(cases,13824u);
}

TEST(InfantrySpace, OriginalSubpositionAndOccupationCorpus) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-occupation-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Infantry:InfantryClass{Infantry():InfantryClass(nullptr,nullptr){}int GetOwningHouseIndex()const override{return 12;}}gi;
  auto*c=MapClass::Instance.TryGetCellAt(CellStruct{8,6});ASSERT_NE(c,nullptr);
  c->Level=1;c->SlopeIndex=0;
  std::ifstream input(RA2_INFANTRY_OCCUPATION_FIXTURE);ASSERT_TRUE(input.good());char kind;unsigned cases=0;
  while(input>>kind){
   if(kind=='S'){int x,y;unsigned expected;ASSERT_TRUE(bool(input>>x>>y>>expected));
    EXPECT_EQ(CellClass::InfantrySubpositionIndex({x,y,0}),expected)<<x<<","<<y;
   }else{unsigned flags;int x,y,z;std::uint32_t values[8];ASSERT_TRUE(bool(input>>flags>>x>>y>>z));for(auto&v:values)ASSERT_TRUE(bool(input>>v));
    c->Flags=static_cast<CellFlags>(flags);c->OccupationFlags=0xA000009Cu;c->AltOccupationFlags=0xB000009Cu;c->InfantryOwnerIndex=7;c->AltInfantryOwnerIndex=9;
    const CoordStruct coord{8*256+x,6*256+y,z};gi.MarkAllOccupationBits(coord);
    EXPECT_EQ(c->OccupationFlags,values[0]);EXPECT_EQ(c->AltOccupationFlags,values[1]);
    EXPECT_EQ(std::uint32_t(c->InfantryOwnerIndex),values[2]);EXPECT_EQ(std::uint32_t(c->AltInfantryOwnerIndex),values[3]);
    gi.UnmarkAllOccupationBits(coord);
    EXPECT_EQ(c->OccupationFlags,values[4]);EXPECT_EQ(c->AltOccupationFlags,values[5]);
    EXPECT_EQ(std::uint32_t(c->InfantryOwnerIndex),values[6]);EXPECT_EQ(std::uint32_t(c->AltInfantryOwnerIndex),values[7]);
   }++cases;
  }EXPECT_EQ(cases,65596u);
 },nullptr));
}

TEST(InfantrySpace, OriginalTargetNavigationAndDuplicateExpirationCorpus) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-expire-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Actor:InfantryClass {
   int assignments=0;Actor(InfantryTypeClass*t,HouseClass*h):InfantryClass(t,h){}
   void SetTarget(AbstractClass* object)override{Target=object;++assignments;}
  };
  InfantryTypeClass type("EXPIRE_GI");HouseClass owner(nullptr),enemy(nullptr);owner.ArrayIndex=0;
  Actor gi(&type,&owner);InfantryClass target(&type,&enemy),other(&type,&enemy);
  auto*cell=MapClass::Instance.TryGetCellAt(CellStruct{8,6});ASSERT_NE(cell,nullptr);
  auto token=[&](const AbstractClass* object){return !object?0:object==&target?1:object==&other?2:object==cell?3:-99;};
  std::ifstream input(RA2_INFANTRY_EXPIRATION_FIXTURE);ASSERT_TRUE(input.good());int removed;unsigned cases=0;
  while(input>>removed){
   int sensed,friendly,capture,occupier,alive,health,selling,expected[36];
   ASSERT_TRUE(bool(input>>sensed>>friendly>>capture>>occupier>>alive>>health>>selling));
   for(auto&value:expected)ASSERT_TRUE(bool(input>>value));
   gi.Owner=&owner;target.Owner=friendly?&owner:&enemy;target.IsAlive=alive!=0;target.Health=health;
   target.CurrentMission=selling?Mission::Selling:Mission::Guard;gi.CurrentMission=capture?Mission::Capture:Mission::Move;
   gi.SuspendedMission=Mission::None;type.Occupier=occupier!=0;gi.TargetingTimer.Start(0);
   while(cell->Sensors_InclHouse(0))cell->Sensors_RemOfHouse(0);if(sensed)cell->Sensors_AddOfHouse(0);
   target.Location={8*256+128,6*256+128,0};target.NextTeamMember=&other;
   gi.Target=gi.LastTarget=gi.ArchiveTarget=gi.unknown_5A0=gi.Destination=gi.LastDestination=gi.MegaDestination=gi.MegaTarget=&target;
   gi.QueueUpToEnter=gi.NextTeamMember=&target;
   DynamicVectorClass<AbstractClass*>*vectors[]{&gi.unknown_abstract_array_588,&gi.NavQueue,&gi.CurrentTargets,&gi.AttackedTargets};
   for(auto*v:vectors){v->Count=0;for(AbstractClass* item:{&other,&target,&target,&other})ASSERT_TRUE(v->AddItem(item));}
   gi.CurrentTargetThreatValues.Count=0;for(int value:{10,20,30,40})ASSERT_TRUE(gi.CurrentTargetThreatValues.AddItem(value));
   gi.assignments=0;gi.PointerExpired(&target,removed!=0);
   std::vector<int> actual{token(gi.Target),token(gi.LastTarget),token(gi.ArchiveTarget),token(gi.QueueUpToEnter),
       token(gi.unknown_5A0),token(gi.Destination),token(gi.LastDestination),token(gi.MegaDestination),token(gi.MegaTarget),token(gi.NextTeamMember),gi.assignments};
   for(auto*v:vectors){actual.push_back(v->Count);for(int i=0;i<4;++i)actual.push_back(i<v->Count?token((*v)[i]):0);}
   actual.push_back(gi.CurrentTargetThreatValues.Count);for(int i=0;i<4;++i)actual.push_back(i<gi.CurrentTargetThreatValues.Count?gi.CurrentTargetThreatValues[i]:0);
   ASSERT_EQ(actual.size(),36u);for(unsigned i=0;i<actual.size();++i)EXPECT_EQ(actual[i],expected[i])<<"case "<<cases<<" field "<<i;
   ++cases;
  }
  EXPECT_EQ(cases,256u);while(cell->Sensors_InclHouse(0))cell->Sensors_RemOfHouse(0);
 },nullptr));
}

TEST(InfantrySpace, PassengerRemovalAndReciprocalReferences) {
 InfantryClass gi(nullptr,nullptr),first(nullptr,nullptr),middle(nullptr,nullptr),last(nullptr,nullptr);
 first.NextObject=&middle;middle.NextObject=&last;
 gi.Passengers.NumPassengers=3;gi.Passengers.FirstPassenger=&first;
 gi.Passengers.RemovePassenger(&middle);
 EXPECT_EQ(first.NextObject,&last);EXPECT_EQ(middle.NextObject,&last);EXPECT_EQ(gi.Passengers.NumPassengers,2);
 gi.Passengers.RemovePassenger(&middle);EXPECT_EQ(gi.Passengers.NumPassengers,2);
 gi.Passengers.RemovePassenger(&first);EXPECT_EQ(gi.Passengers.FirstPassenger,&last);EXPECT_EQ(gi.Passengers.NumPassengers,1);
 gi.Passengers.RemovePassenger(&last);EXPECT_EQ(gi.Passengers.FirstPassenger,nullptr);EXPECT_EQ(gi.Passengers.NumPassengers,0);
 gi.DirectRockerLinkedUnit=&last;last.DirectRockerLinkedUnit=&gi;
 gi.DrainTarget=&last;last.DrainingMe=&gi;
 gi.PointerExpired(&last,true);
 EXPECT_EQ(gi.DirectRockerLinkedUnit,nullptr);EXPECT_EQ(last.DirectRockerLinkedUnit,nullptr);
 EXPECT_EQ(gi.DrainTarget,nullptr);EXPECT_EQ(last.DrainingMe,nullptr);
 first.NextObject=middle.NextObject=nullptr;
}

TEST(InfantrySpace, NativeDestructionBroadcastsBeforeReleasingIdentityAndLinks) {
 const int baseline=AbstractClass::Array.Count,types=AbstractClass::TypeExpirationListeners.Count;
 {
  InfantryTypeClass type("EXPIRY_LIFETIME");for(auto&s:type.Sequence->Sequences)s.CountFrames=1;
  InfantryClass survivor(&type,nullptr),tail(&type,nullptr);survivor.Health=tail.Health=100;
  auto victim=std::make_unique<InfantryClass>(&type,nullptr);victim->Health=100;victim->NextTeamMember=&tail;
  survivor.Target=survivor.LastTarget=survivor.Destination=survivor.LastDestination=survivor.ArchiveTarget=victim.get();
  survivor.NextTeamMember=victim.get();survivor.NavQueue.AddItem(victim.get());survivor.NavQueue.AddItem(victim.get());
  survivor.RadioLinks.SetCapacity(2);survivor.RadioLinks[0]=victim.get();survivor.RadioLinks[1]=&tail;
  AnimTypeClass anim_type("EXPIRY_ATTACHED");AnimClass anim(&anim_type,{0,0,0});anim.OwnerObject=victim.get();
  BuildingTypeClass building_type("EXPIRY_BUILDING",BuildingTypeClass::ConstructionDefaults{});
  BuildingClass building(&building_type,nullptr);building.C4AppliedBy=victim.get();building.Overpowerers.AddItem(victim.get());
  EXPECT_EQ(AbstractClass::Array.Count,baseline+5);
  victim.reset();
  EXPECT_EQ(survivor.Target,nullptr);EXPECT_EQ(survivor.LastTarget,nullptr);EXPECT_EQ(survivor.Destination,nullptr);
  EXPECT_EQ(survivor.LastDestination,nullptr);EXPECT_EQ(survivor.ArchiveTarget,nullptr);EXPECT_EQ(survivor.NavQueue.Count,0);
  EXPECT_EQ(survivor.NextTeamMember,&tail);EXPECT_EQ(survivor.RadioLinks[0],nullptr);EXPECT_EQ(survivor.RadioLinks[1],&tail);
  EXPECT_EQ(anim.OwnerObject,nullptr);EXPECT_TRUE(anim.UnableToContinue);EXPECT_FALSE(anim.TimeToDie);EXPECT_EQ(building.C4AppliedBy,nullptr);
  EXPECT_EQ(building.Overpowerers.Count,0);EXPECT_EQ(AbstractClass::Array.Count,baseline+4);
 }
 EXPECT_EQ(AbstractClass::Array.Count,baseline);EXPECT_EQ(AbstractClass::TypeExpirationListeners.Count,types);
}

TEST(InfantrySpace, GroundBridgeChainsReservationsAndDestruction) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-chains-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  InfantryTypeClass type("SPACE");type.Locomotor=LocomotionClass::CLSIDs::Walk;auto*c=MapClass::Instance.TryGetCellAt(CellStruct{8,6});ASSERT_NE(c,nullptr);
  c->Flags|=CellFlags::BridgeHead;
  // Map placement registers ownership, as in the original Unlimbo contract.
  HouseTypeClass country("CHAIN_COUNTRY");HouseClass owner(&country);
  auto ground=std::make_unique<InfantryClass>(&type,&owner),bridge=std::make_unique<InfantryClass>(&type,&owner);
  ground->Location={8*256+192,6*256+64,0};bridge->Location=ground->Location;bridge->OnBridge=true;
  game::attach_map_object(*ground);game::attach_map_object(*bridge);
  EXPECT_EQ(c->FirstObject,ground.get());EXPECT_EQ(c->AltObject,bridge.get());EXPECT_EQ(c->OccupationFlags,4u);EXPECT_EQ(c->AltOccupationFlags,4u);
  auto next=c->FindInfantrySubposition(ground->Location,false,false,false);
  EXPECT_EQ(CellClass::InfantrySubpositionIndex(next),4u);
  c->OccupationFlags|=0x18u;
  EXPECT_EQ(c->FindInfantrySubposition(ground->Location,false,false,false),CoordStruct::Empty);
  EXPECT_NE(c->FindInfantrySubposition(ground->Location,true,false,false),CoordStruct::Empty);
  c->OccupationFlags=4u;
  ground.reset();EXPECT_EQ(c->FirstObject,nullptr);EXPECT_EQ(c->OccupationFlags,0u);EXPECT_EQ(c->AltOccupationFlags,4u);
  EXPECT_EQ(c->AltObject,bridge.get());bridge.reset();EXPECT_EQ(c->AltObject,nullptr);EXPECT_EQ(c->AltOccupationFlags,0u);
  EXPECT_EQ(c->InfantryOwnerIndex,-1);EXPECT_EQ(c->AltInfantryOwnerIndex,-1);
  // Building tails are shared between foundation cells: insert infantry ahead.
  BuildingClass building(nullptr,nullptr);InfantryClass a(&type,nullptr),b(&type,nullptr);
  c->AddContent(&a,false);c->AddContent(&building,false);c->AddContent(&b,false);
  EXPECT_EQ(c->FirstObject,&b);EXPECT_EQ(b.NextObject,&a);EXPECT_EQ(a.NextObject,&building);EXPECT_EQ(building.NextObject,nullptr);
  c->RemoveContent(&a,false);EXPECT_EQ(b.NextObject,&building);EXPECT_EQ(a.NextObject,nullptr);
  c->RemoveContent(&a,false);EXPECT_EQ(b.NextObject,&building);
  c->RemoveContent(&building,false);c->RemoveContent(&b,false);EXPECT_EQ(c->FirstObject,nullptr);
  struct Occupier : TerrainClass {
   int marks=0,unmarks=0;
   Occupier():TerrainClass(nullptr,{8,6}){}
   bool IsStandingStill() const override{return true;} // original Occupies_Cells slot 0xC0
   bool IsOnBridge(TechnoClass*) const override{return false;} // distinct slot 0xBC
   void MarkAllOccupationBits(const CoordStruct&) override{++marks;}
   void UnmarkAllOccupationBits(const CoordStruct&) override{++unmarks;}
  }occupier;
  c->AddContent(&occupier,false);EXPECT_EQ(occupier.marks,1);
  c->RemoveContent(&occupier,false);EXPECT_EQ(occupier.unmarks,1);
  c->RemoveContent(&occupier,false);EXPECT_EQ(occupier.unmarks,2); // original clears even if absent
  auto& logic=LogicClass::Instance;const int oldCount=logic.Count,oldGrowth=logic.CapacityIncrement;
  struct RestoreLogic{LogicClass& list;int count,growth;~RestoreLogic(){list.Count=count;list.CapacityIncrement=growth;}}restore{logic,oldCount,oldGrowth};
  logic.Count=logic.Capacity;logic.CapacityIncrement=0;
  auto* tt=TerrainTypeClass::Array[0];const bool oldLogic=tt->IsLogic,oldSpawn=tt->SpawnsTiberium;
  struct RestoreType{TerrainTypeClass& type;bool logic,spawn;~RestoreType(){type.IsLogic=logic;type.SpawnsTiberium=spawn;}}restoreType{*tt,oldLogic,oldSpawn};
  tt->IsLogic=true;tt->SpawnsTiberium=true;
  TerrainClass rejected(tt,{8,6});rejected.Location.Z=777;
  game::attach_map_object(rejected);
  EXPECT_FALSE(rejected.IsOnMap);EXPECT_FALSE(rejected.IsInLogic);EXPECT_TRUE(rejected.InLimbo);
  EXPECT_EQ(rejected.Location.Z,777);EXPECT_EQ(c->FirstObject,nullptr);EXPECT_EQ(c->OccupationFlags,0u);
 },nullptr));
}

TEST(InfantrySpace, TagExpiryNotifiesObjectsCellsAndBothPendingLists) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-expiry-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};
 map_fixture::fixtures(root);View view(root.string());view.load("world.map");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  auto tag=std::make_unique<TagClass>(nullptr,CellStruct{8,6});
  InfantryTypeClass type("EXPIRY");InfantryClass a(&type,nullptr),b(&type,nullptr),tail(&type,nullptr);
  auto*c=MapClass::Instance.GetCellAt(CellStruct{8,6});auto*d=MapClass::Instance.GetCellAt(CellStruct{9,6});
  c->ReplaceTag(tag.get());d->ReplaceTag(tag.get());a.AttachedTag=tag.get();++tag->InstanceCount;
  EXPECT_EQ(tag->InstanceCount,3);
  LogicClass::PendingTags.AddItem(tag.get());LogicClass::PendingTags.AddItem(tag.get());
  MapClass::PendingTags.AddItem(tag.get());MapClass::PendingTags.AddItem(tag.get());
  // Map removes first occurrence, Logic removes all. Tag's two native passes
  // must run while the actual derived tag remains alive.
  auto* expired=tag.get();tag.reset();
  EXPECT_EQ(c->AttachedTag,nullptr);EXPECT_EQ(d->AttachedTag,nullptr);EXPECT_EQ(a.AttachedTag,nullptr);
  EXPECT_EQ(MapClass::Instance.TaggedCells.FindItemIndex(c->MapCoords),-1);
  EXPECT_EQ(MapClass::Instance.TaggedCells.FindItemIndex(d->MapCoords),-1);
  EXPECT_EQ(LogicClass::PendingTags.FindItemIndex(expired),-1);EXPECT_EQ(MapClass::PendingTags.FindItemIndex(expired),-1);
  ASSERT_TRUE(a.RadioLinks.SetCapacity(2));a.RadioLinks[0]=&b;a.RadioLinks[1]=&tail;
  a.NextObject=&b;b.NextObject=&tail;
  a.PointerExpired(&b,false);EXPECT_EQ(a.NextObject,&b);EXPECT_EQ(a.RadioLinks[0],&b);
  a.PointerExpired(&b,true);EXPECT_EQ(a.NextObject,&tail);EXPECT_EQ(a.RadioLinks[0],nullptr);EXPECT_EQ(a.RadioLinks[1],&tail);
  a.NextObject=nullptr;b.NextObject=nullptr;
 },nullptr));
}

TEST(VehicleMovement, FirstMapCreationRenderingAndMove) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 const int baseline=UnitClass::Array.Count;View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapViewHandle* view;UnitClass* unit=nullptr;CellStruct goal{};CoordStruct initial{};unsigned count=0;} state{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& state=*static_cast<State*>(p);
  state.count=UnitClass::Array.Count;ASSERT_GT(state.count,0u);
  for(auto* unit:UnitClass::Array){
   EXPECT_NE(unit->Locomotor,nullptr);EXPECT_TRUE(unit->IsOnMap);EXPECT_FALSE(unit->InLimbo);
   std::cout<<"VEHICLE_MAP "<<unit->Type->ID<<" owner="<<unit->Owner->PlainName<<" cell="<<unit->GetMapCoords().X<<","<<unit->GetMapCoords().Y<<"\n";
   if(!state.unit&&unit->Owner->IsControlledByCurrentPlayer())state.unit=unit;
  }
  ASSERT_NE(state.unit,nullptr);auto* unit=state.unit;state.initial=unit->Location;
  EXPECT_GT(unit->Type->ROT,0);EXPECT_GT(unit->Type->Speed,0);
  EXPECT_EQ(unit->BarrelFacing.Current().Raw,0x4000);EXPECT_EQ(unit->BarrelFacing.ROT.Raw,3*256);
  game::rebuild_world_sprites(*state.view->world);
  unsigned packets=0;for(const auto& sprite:state.view->world->impl->sprites)if(sprite.owner==unit&&sprite.voxel){++packets;EXPECT_FALSE(sprite.voxel->pixels.empty());}
  EXPECT_GE(packets,unit->Type->Turret?2u:1u);
  const auto origin=unit->GetMapCoords();auto& map=MapClass::Instance;
  bool found=false;
  for(int y=-8;y<=8&&!found;++y)for(int x=-8;x<=8&&!found;++x){
   if(std::abs(x)+std::abs(y)<6)continue;
   CellStruct end{short(origin.X+x),short(origin.Y+y)};auto* cell=map.TryGetCellAt(end);
   if(!cell||!map.IsWithinUsableArea(end,true)||cell->GetBuilding()||unit->IsCellOccupied(cell,FacingType::None,-1,nullptr,true)!=Move::OK)continue;
   if(unit->UpdatePathfinding(end,0,0)){state.goal=end;found=true;}
  }
  ASSERT_TRUE(found);unit->PathDirections[0]=-1;
  EXPECT_EQ(unit->MouseOverCell(&state.goal,false,false),Action::Move);
  auto follow=state.goal;ASSERT_TRUE(unit->CellClickedAction(Action::Move,&state.goal,&follow,false));
  ASSERT_EQ(EventClass::OutList.Count,1);
  int travelFrames=0;for(;travelFrames<1500&&unit->GetMapCoords()!=state.goal;++travelFrames){++Unsorted::CurrentFrame;game::update_map_world(*state.view->world);}
  std::cout<<"VEHICLE_TRAVEL_FRAMES "<<travelFrames<<"\n";
  EXPECT_NE(unit->Location,state.initial);EXPECT_EQ(unit->GetMapCoords(),state.goal);
  for(int frame=0;frame<100;++frame){++Unsorted::CurrentFrame;game::update_map_world(*state.view->world);}
  EXPECT_FALSE(unit->Locomotor->Is_Moving());EXPECT_EQ(unit->Destination,nullptr);
  EXPECT_TRUE(unit->IsOnMap);EXPECT_TRUE(unit->IsStandingStill());EXPECT_TRUE(unit->GetCell()->OccupationFlags&0x20u);
  std::cout<<"VEHICLE_MOVED "<<unit->Type->ID<<" to="<<unit->GetMapCoords().X<<","<<unit->GetMapCoords().Y<<"\n";
 },&state));
 auto objects=view.objects();unsigned units=0;for(const auto& object:objects)if(object.kind==game::MapObjectKind::unit){++units;EXPECT_TRUE(object.has_image);}
 EXPECT_EQ(units,state.count);view.load("ALL01UMD.MAP");EXPECT_EQ(UnitClass::Array.Count,baseline+int(state.count));
 game::destroy_map_view(view.view);EXPECT_EQ(UnitClass::Array.Count,baseline);
}

TEST(VehicleMovement, OriginalDriveControlAndTrackCorpus) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Actor:UnitClass {int speed=0;Actor():UnitClass(UnitTypeClass::Find("MTNK"),nullptr){}int GetCurrentSpeed() const override{return speed;}} actor;
  DriveLocomotionClass drive;drive.Link_To_Object(&actor);
  std::ifstream input(RA2_DRIVE_REFERENCE_FIXTURE);ASSERT_TRUE(input.good());int op,cases=0;
  while(input>>op){
   if(op==3){
    int track,index,x,y,face,rx,ry,rf;ASSERT_TRUE(bool(input>>track>>index>>x>>y>>face>>rx>>ry>>rf));
    drive.TrackNumber=track;drive.HeadToCoord={1000,2000,0};
    const int raw=DriveLocomotionClass::TurnTrack[track].NormalTrackStructIndex;
    ASSERT_GT(raw,0);const auto& point=DriveLocomotionClass::RawTrack[raw].TrackPoint[index];
    ASSERT_EQ(point.Point,(Point2D{x,y}));ASSERT_EQ(point.Face,face);
    const auto result=drive.Smooth_Turn({x,y},face);ASSERT_EQ(result,(Point2D{rx,ry}));ASSERT_EQ(face,rf);
   }else{
    int flags,expected[8],facing;double speed;ASSERT_TRUE(bool(input>>flags));for(auto& v:expected)ASSERT_TRUE(bool(input>>v));ASSERT_TRUE(bool(input>>speed>>facing));
    actor.Location={2176,1664,17};drive.DestinationCoord={2432,1664,7};drive.HeadToCoord=flags&1?CoordStruct{2200,1700,17}:CoordStruct{};
    actor.speed=flags&2?10:0;drive.movementspeed_50=0.75;actor.EMPLockRemaining=bool(flags&8);actor.BeingWarpedOut=flags&16;actor.WarpingOut=flags&32;
    actor.PrimaryFacing.SetROT(0);actor.PrimaryFacing.SetCurrent(DirStruct(0));
    auto* cell=MapClass::Instance.GetCellAt(CoordStruct{2300,1777,24});const auto saved=cell->Flags;cell->Flags=CellFlags(flags&4?0x100:0);
    if(op==0)drive.Move_To({2300,1777,24});else if(op==1)drive.Stop_Moving();else drive.Do_Turn(DirStruct(0x4000));
    cell->Flags=saved;
    const auto d=drive.DestinationCoord,h=drive.HeadToCoord;
    const int actual[]={d.X,d.Y,d.Z,h.X,h.Y,h.Z,int(drive.Is_Moving()),int(drive.Is_Moving_Now())};
    for(int i=0;i<8;++i)ASSERT_EQ(actual[i],expected[i])<<"case="<<cases<<" field="<<i;
    EXPECT_DOUBLE_EQ(drive.movementspeed_50,speed);EXPECT_EQ(actor.PrimaryFacing.Desired().Raw,facing);
   }
   ++cases;
  }
  EXPECT_EQ(cases,2297);
 },nullptr));
}

TEST(VehicleMovement, FiveTanksRedirectAndReleaseReservations) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& view=*static_cast<game::MapViewHandle*>(p);
  std::vector<UnitClass*> group;for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"MTNK")&&unit->Owner->IsControlledByCurrentPlayer())group.push_back(unit);
  ASSERT_EQ(group.size(),5u);
  auto& map=MapClass::Instance;CellStruct first{90,104},second{96,99};
  for(auto* unit:group){auto follow=first;ASSERT_TRUE(unit->CellClickedAction(Action::Move,&first,&follow,false));}
  for(int i=0;i<35;++i){++Unsorted::CurrentFrame;game::update_map_world(*view.world);}
  for(auto* unit:group){auto follow=second;ASSERT_TRUE(unit->CellClickedAction(Action::Move,&second,&follow,false));}
  for(int i=0;i<1800;++i){++Unsorted::CurrentFrame;game::update_map_world(*view.world);}
  for(auto* unit:group){
   EXPECT_FALSE(unit->Locomotor->Is_Moving());EXPECT_EQ(unit->Destination,nullptr);
   const auto delta=unit->Location-map.GetCellAt(second)->GetCoords();
   EXPECT_LE(delta.Magnitude(),RulesClass::Instance->CloseEnough+256)<<unit->GetMapCoords().X<<","<<unit->GetMapCoords().Y;
   EXPECT_TRUE(unit->GetCell()->OccupationFlags&0x20u);
   for(auto* other:group)if(other!=unit)EXPECT_NE(unit->GetMapCoords(),other->GetMapCoords());
  }
  for(auto* unit:group){auto* cell=unit->GetCell();delete unit;EXPECT_FALSE(cell->OccupationFlags&0x20u);}
 },view.view));
}

TEST(VehicleMovement, InfantrySubslotsAndBridgeVehicleReservations) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  UnitClass* tank=nullptr;InfantryClass* gi=nullptr;
  for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"MTNK")&&unit->Owner->IsControlledByCurrentPlayer()){tank=unit;break;}
  for(auto* actor:InfantryClass::Array)if(!std::strcmp(actor->Type->ID,"E1")&&actor->Owner->IsControlledByCurrentPlayer()){gi=actor;break;}
  ASSERT_NE(tank,nullptr);ASSERT_NE(gi,nullptr);
  auto* cell=CellClass::Create();ASSERT_NE(cell,nullptr);
  const auto cleanup=ra2::test::scope_exit([&]{GameDelete(cell);});
  cell->MapCoords={90,104};cell->Level=0;cell->LandType=LandType::Clear;cell->Flags=CellFlags{};
  cell->InfantryOwnerIndex=cell->AltInfantryOwnerIndex=tank->Owner->ArrayIndex;
  // Original Infantry 0x51BF90 permits sharing until all three bits are set;
  // Unit 0x73F0A0 treats any allied infantry reservation as a moving block.
  for(unsigned bits:{0x04u,0x08u,0x10u,0x0Cu,0x14u,0x18u}){
   cell->OccupationFlags=bits;
   EXPECT_EQ(gi->IsCellOccupied(cell,FacingType::None,-1,cell,true),Move::OK);
   EXPECT_EQ(tank->IsCellOccupied(cell,FacingType::None,-1,cell,true),Move::MovingBlock);
  }
  cell->OccupationFlags=0x1Cu;
  EXPECT_EQ(gi->IsCellOccupied(cell,FacingType::None,-1,cell,true),Move::MovingBlock);
  cell->OccupationFlags=0x20u;
  EXPECT_EQ(gi->IsCellOccupied(cell,FacingType::None,-1,cell,true),Move::MovingBlock);
  EXPECT_EQ(tank->IsCellOccupied(cell,FacingType::None,-1,cell,true),Move::MovingBlock);
  // Exercise the real Foot.CanReachCell in/out level correction, with an
  // independently reserved bridge deck and ground layer (no object doubles).
  cell->Flags=CellFlags(0x100u);
  for(unsigned ground:{0u,0x04u,0x1Cu,0x20u})for(unsigned deck:{0u,0x04u,0x1Cu,0x20u}){
   cell->OccupationFlags=ground;cell->AltOccupationFlags=deck;
   const auto onDeck=deck?Move::MovingBlock:Move::OK;
   EXPECT_EQ(tank->IsCellOccupied(cell,FacingType::None,-1,cell,true),onDeck)<<ground<<","<<deck;
   EXPECT_EQ(tank->IsCellOccupied(cell,FacingType::None,4,cell,true),onDeck);
   EXPECT_EQ(tank->IsCellOccupied(cell,FacingType::None,0,cell,true),ground?Move::MovingBlock:Move::OK);
  }
 },nullptr));
}

TEST(VehicleMovement, AllTanksAttackGateInitiateThenMoveToJunction) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,1280,720));
 struct State{game::MapViewHandle* view;std::vector<UnitClass*> tanks;InfantryClass* target=nullptr;}state{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  for(auto* unit:UnitClass::Array)if(unit->Owner->IsControlledByCurrentPlayer()&&!unit->Type->Harvester)s.tanks.push_back(unit);
  std::stable_sort(s.tanks.begin(),s.tanks.end(),[](auto* a,auto* b){return a->GetYSort()<b->GetYSort();});
  for(auto* unit:s.tanks)ASSERT_TRUE(unit->Select());
  for(auto* infantry:InfantryClass::Array)if(!std::strcmp(infantry->Type->ID,"INIT")){
   auto at=infantry->GetMapCoords();
   if(at.X>=115&&at.X<=116&&at.Y>=101&&at.Y<=102)
    if(!s.target||infantry->Location.X+infantry->Location.Y>s.target->Location.X+s.target->Location.Y)s.target=infantry;
  }
  ASSERT_EQ(s.tanks.size(),9u);ASSERT_NE(s.target,nullptr);
  if(const char* value=std::getenv("RA2_GATE_DELAY"))for(int i=0;i<std::atoi(value);++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  std::cout<<"GATE_ATTACK_TARGET "<<s.target->GetMapCoords().X<<","<<s.target->GetMapCoords().Y<<"\n";
 },&state));
 ASSERT_TRUE(click_world_object(*view.view,state.target));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  ASSERT_EQ(EventClass::OutList.Count,9);
  auto dump=[&](const char* phase){for(auto* unit:s.tanks){auto* d=static_cast<DriveLocomotionClass*>(unit->Locomotor);
   std::cout<<phase<<" id="<<unit->UniqueID<<" "<<unit->Type->ID<<" pos="<<unit->Location.X<<","<<unit->Location.Y<<" mission="<<int(unit->CurrentMission)<<" queued="<<int(unit->QueuedMission)
    <<" target="<<unit->Target<<" dest="<<unit->Destination<<" path="<<unit->PathDirections[0]<<","<<unit->PathDirections[1]<<" speed="<<unit->SpeedPercentage<<" frozen="<<unit->FrozenStill
    <<" track="<<d->TrackNumber<<":"<<d->TrackIndex<<" driving="<<d->IsDriving<<" locoDest="<<d->DestinationCoord.X<<","<<d->DestinationCoord.Y<<" head="<<d->HeadToCoord.X<<","<<d->HeadToCoord.Y<<"\n";
  }};
  int stable=0,frames=0;std::vector<CoordStruct> previous;for(auto* unit:s.tanks)previous.push_back(unit->Location);
  for(;frames<2400&&stable<90;++frames){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);bool same=true;
   for(unsigned i=0;i<s.tanks.size();++i){same&=previous[i]==s.tanks[i]->Location;previous[i]=s.tanks[i]->Location;}
   stable=same?stable+1:0;
  }
  std::cout<<"GATE_STOP frames="<<frames<<" stable="<<stable<<"\n";dump("GATE_STOP");ASSERT_GE(stable,90);
  CellStruct goal{122,100};if(const char* value=std::getenv("RA2_GATE_GOAL")){int x,y;if(std::sscanf(value,"%d,%d",&x,&y)==2)goal={short(x),short(y)};}auto* destination=MapClass::Instance.GetCellAt(goal);
  std::cout<<"GATE_JUNCTION cell="<<goal.X<<","<<goal.Y<<" z="<<destination->GetCoords().Z<<" land="<<int(destination->LandType)<<" objects="<<destination->FirstObject<<"\n";
  for(auto* unit:s.tanks){auto follow=goal;ASSERT_TRUE(unit->CellClickedAction(Action::Move,&goal,&follow,false));}
  for(int i=0;i<1800;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);if(i==30||i==300)dump(i==30?"GATE_MOVE_30":"GATE_MOVE_300");}
  dump("GATE_FINAL");
  for(unsigned i=0;i<s.tanks.size();++i){auto* unit=s.tanks[i];EXPECT_NE(unit->Location,previous[i])<<unit->UniqueID;EXPECT_LE((unit->Location-destination->GetCoords()).Magnitude(),RulesClass::Instance->CloseEnough+512)<<unit->UniqueID;}
 },&state));
}

TEST(VehicleMovement, TwoCellTurnUsesIntermediateRampLevel) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  UnitClass* actor=nullptr;for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"MTNK")){actor=unit;break;}
  ASSERT_NE(actor,nullptr);auto* drive=static_cast<DriveLocomotionClass*>(actor->Locomotor);
  auto& map=MapClass::Instance;
  auto* start=map.GetCellAt(CellStruct{119,99});auto* intermediate=map.GetCellAt(CellStruct{120,98});auto* end=map.GetCellAt(CellStruct{121,98});
  actor->Mark(MarkType::Up);actor->SetLocation(start->GetCoords());actor->SetHeight(0);actor->FrozenStill=true;actor->Mark(MarkType::Down);
  actor->CurrentMission=Mission::Move;actor->QueuedMission=Mission::None;
  actor->SetDestination(map.GetCellAt(CellStruct{124,98}),true);
  actor->PrimaryFacing.SetCurrent(DirStruct(0x2000));
  actor->PathDirections[0]=1;actor->PathDirections[1]=2;actor->PathDirections[2]=2;actor->PathDirections[3]=-1;
  std::cout<<"RAMP_LEVELS "<<int(start->Level)<<","<<int(intermediate->Level)<<","<<int(end->Level)<<"\n";
  EXPECT_EQ(actor->IsCellOccupied(intermediate,FacingType(1),start->Level,nullptr,true),Move::OK);
  EXPECT_EQ(actor->IsCellOccupied(end,FacingType(2),start->Level,nullptr,true),Move::No);
  EXPECT_EQ(actor->IsCellOccupied(end,FacingType(2),intermediate->Level,nullptr,true),Move::OK);
  bool stop=false;drive->Start_Of_Move(stop);EXPECT_FALSE(stop);
  EXPECT_TRUE(drive->IsDriving);EXPECT_EQ(drive->TrackNumber,10);
  EXPECT_EQ(CellClass::Coord2Cell(drive->HeadToCoord),end->MapCoords);
  EXPECT_NE(actor->Destination,nullptr);
 },nullptr));
}

TEST(UnitProduction, FirstMapSidebarBarracksAndWarFactory){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_game_view_size(*view.view,1280,720));
 struct State{game::MapViewHandle* view;Point2D gi{},tank{};int cash=0;HouseClass* owner=nullptr;InfantryClass* soldier=nullptr;UnitClass* vehicle=nullptr;BuildingClass* barracks=nullptr;BuildingClass* warfactory=nullptr;}s{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  // Production accounting is isolated from the now-active map harvesters.
  for(auto* unit:UnitClass::Array)if(unit->Type->Harvester)unit->ForceMission(Mission::Sleep);
  s.owner=HouseClass::CurrentPlayer;ASSERT_NE(s.owner,nullptr);s.cash=s.owner->Available_Money();
  std::cout<<"PRODUCTION_INITIAL credits="<<s.cash<<" tech="<<s.owner->TechLevel<<" barracks="<<s.owner->NumBarracks<<" factories="<<s.owner->NumWarFactories<<"\n";
  for(auto* b:s.owner->Buildings){if(b->Type->Factory==AbstractType::InfantryType)s.barracks=b;if(b->Type->WeaponsFactory)s.warfactory=b;}
  ASSERT_NE(s.barracks,nullptr);ASSERT_NE(s.warfactory,nullptr);
  auto* gi=InfantryTypeClass::Find("E1");auto* tank=UnitTypeClass::Find("MTNK");ASSERT_NE(gi,nullptr);ASSERT_NE(tank,nullptr);
  EXPECT_EQ(s.owner->CanBuild(gi,false,true),CanBuildResult::Buildable);EXPECT_EQ(s.owner->CanBuild(tank,false,true),CanBuildResult::Buildable);
  ASSERT_GT(s.cash,gi->Cost+tank->Cost);std::cout<<"CAMEOS gi="<<gi->ImageFile<<"/"<<gi->CameoFile<<" tank="<<tank->CameoFile<<"\n";
  ++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);
  auto& sidebar=SidebarClass::Instance;sidebar.RefreshBuildables();
  s.view->ui_resources=std::make_unique<game::UiResources>();ASSERT_TRUE(s.view->ui_resources->load(s.view->scenario.PlayerSideIndex))<<s.view->ui_resources->error();
  EXPECT_NE(gi->GetCameo(),nullptr);EXPECT_NE(tank->GetCameo(),nullptr);
  game::MapDrawingContext drawing;game::MapDrawStatistics statistics;
  drawing.plain_palette=[](void*,const BytePalette& palette,const game::DrawingPaletteHandle*& output)noexcept{output=reinterpret_cast<const game::DrawingPaletteHandle*>(&palette);return game::DrawingStatus::drawn;};
  game::GameUiFrame frame{drawing,*s.view->ui_resources,statistics};
  ASSERT_EQ(game::with_game_ui_frame(frame,[]{SidebarClass::Instance.InitializeButtons();}),game::DrawingStatus::skipped);
  for(int tab:{2,3}){auto& strip=sidebar.Tabs[tab];int slot=-1;for(int i=0;i<strip.CameoCount;++i){auto* type=TechnoTypeClass::GetByTypeAndIndex(strip.Cameos[i].ItemType,strip.Cameos[i].ItemIndex);if(type==(tab==2?static_cast<TechnoTypeClass*>(gi):tank))slot=i;}
   ASSERT_GE(slot,0);ASSERT_LT(slot,sidebar.GetUsableCameoCount());std::cout<<"PRODUCTION_CAMEO tab="<<tab<<" slot="<<slot<<"\n";const Point2D point{SidebarClass::CameoPosition.X+(slot%2)*SidebarClass::CameoPitch.X+20,SidebarClass::CameoPosition.Y+(slot/2)*SidebarClass::CameoPitch.Y+20};if(tab==2)s.gi=point;else s.tank=point;}
  sidebar.SetTab(2);
 },&s));
 game::GameInputResult result;
 for(bool down:{true,false})ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,s.gi.X,s.gi.Y,1,0,down},result));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  ASSERT_EQ(EventClass::OutList.Count,1);EXPECT_EQ(EventClass::OutList.First().Type,EventType::Produce);
  ++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);auto* f=s.owner->Primary_ForInfantry;ASSERT_NE(f,nullptr);ASSERT_NE(f->Object,nullptr);
  s.soldier=static_cast<InfantryClass*>(f->Object);EXPECT_TRUE(s.soldier->InLimbo);
  for(int i=0;i<5000&&s.owner->Primary_ForInfantry;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  ASSERT_EQ(s.owner->Primary_ForInfantry,nullptr);ASSERT_FALSE(s.soldier->InLimbo);EXPECT_EQ(s.owner->Available_Money(),s.cash-s.soldier->Type->Cost);
  std::cout<<"PRODUCTION_GI first="<<s.soldier->Location.X<<","<<s.soldier->Location.Y<<" barracks="<<s.barracks->Location.X<<","<<s.barracks->Location.Y<<"\n";
  for(int i=0;i<200;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  EXPECT_FALSE(s.soldier->IsTether);EXPECT_NE(s.soldier->GetCell()->GetBuilding(),s.barracks);
  SidebarClass::Instance.SetTab(3);
 },&s));
 for(bool down:{true,false})ASSERT_TRUE(game::submit_game_input(*view.view,{game::GameInputKind::pointer_button,s.tank.X,s.tank.Y,1,0,down},result));
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
  ASSERT_EQ(EventClass::OutList.Count,1);++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);
  auto* f=s.owner->Primary_ForVehicles;ASSERT_NE(f,nullptr);ASSERT_NE(f->Object,nullptr);s.vehicle=static_cast<UnitClass*>(f->Object);
  const int expected=s.cash-s.soldier->Type->Cost-s.vehicle->Type->Cost;
  for(int i=0;i<8000&&s.owner->Primary_ForVehicles;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  ASSERT_EQ(s.owner->Primary_ForVehicles,nullptr);ASSERT_FALSE(s.vehicle->InLimbo);EXPECT_EQ(s.owner->Available_Money(),expected);
  EXPECT_EQ(s.vehicle->Location,s.warfactory->Location+s.warfactory->Type->ExitCoord);EXPECT_EQ(s.warfactory->CurrentMission,Mission::Unload);
  std::cout<<"PRODUCTION_TANK first="<<s.vehicle->Location.X<<","<<s.vehicle->Location.Y<<" factory="<<s.warfactory->Location.X<<","<<s.warfactory->Location.Y<<"\n";
  for(int i=0;i<1000;++i){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}
  std::cout<<"TANK_TRACE pos="<<s.vehicle->Location.X<<","<<s.vehicle->Location.Y<<","<<s.vehicle->Location.Z<<" status="<<s.warfactory->MissionStatus<<" mission="<<int(s.warfactory->CurrentMission)<<" track="<<static_cast<DriveLocomotionClass*>(s.vehicle->Locomotor)->TrackNumber<<"\n";
  EXPECT_FALSE(s.vehicle->IsTether);EXPECT_FALSE(s.warfactory->IsTether);EXPECT_EQ(s.warfactory->CurrentMission,Mission::Guard);
  EXPECT_NE(s.vehicle->GetCell()->GetBuilding(),s.warfactory);EXPECT_TRUE(s.vehicle->IsAlive);EXPECT_TRUE(s.vehicle->IsOnMap);
  std::cout<<"PRODUCTION_END tank="<<s.vehicle->Location.X<<","<<s.vehicle->Location.Y<<" status="<<s.warfactory->MissionStatus<<" tether="<<s.vehicle->IsTether<<" credits="<<s.owner->Available_Money()<<"\n";
 },&s));
 ASSERT_NE(s.vehicle,nullptr);ASSERT_TRUE(click_world_object(*view.view,s.vehicle));EXPECT_TRUE(s.vehicle->IsSelected);
}

TEST(UnitProduction, ChronoMinerThenTwoIFVsLeaveFactory){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& view=*static_cast<View*>(p);
  auto* owner=HouseClass::CurrentPlayer;ASSERT_NE(owner,nullptr);
  for(auto* unit:UnitClass::Array)if(unit->Type->Harvester)unit->ForceMission(Mission::Sleep);
  BuildingClass* building=nullptr;for(auto* b:owner->Buildings)if(b->Type->WeaponsFactory){building=b;break;}
  ASSERT_NE(building,nullptr);
  auto* minerType=UnitTypeClass::Find("CMIN");auto* ifvType=UnitTypeClass::Find("FV");
  ASSERT_NE(minerType,nullptr);ASSERT_NE(ifvType,nullptr);
  const int firstUnit=UnitClass::Array.Count;
  const auto tick=[&]{++Unsorted::CurrentFrame;game::update_map_world(*view.view->world);};
  tick();ASSERT_EQ(owner->BeginProduction(AbstractType::UnitType,minerType->ArrayIndex,false),0);
  auto* miner=static_cast<UnitClass*>(owner->Primary_ForVehicles->Object);ASSERT_NE(miner,nullptr);
  for(int i=0;i<2;++i)ASSERT_EQ(owner->BeginProduction(AbstractType::UnitType,ifvType->ArrayIndex,false),0);
  bool sawMinerTrack=false;int frames=0;
  for(;frames<8000;++frames){
   tick();
   if(building->MissionStatus==3&&building->CurrentMission==Mission::Unload&&building->GetNthLink()==miner){
    IPersist* persist=nullptr;constexpr GUID iid{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};
    ASSERT_EQ(miner->Locomotor->QueryInterface(iid,reinterpret_cast<void**>(&persist)),0);
    GUID clsid{};ASSERT_EQ(persist->GetClassID(&clsid),0);persist->Release();
    ASSERT_EQ(std::memcmp(&clsid,&LocomotionClass::CLSIDs::Drive,sizeof(clsid)),0)<<"Factory departure requires Drive, not the restored Teleport locomotor";
    auto* drive=static_cast<DriveLocomotionClass*>(miner->Locomotor);
    if(!sawMinerTrack){EXPECT_EQ(drive->TrackNumber,66);EXPECT_TRUE(drive->Is_Piggybacking());sawMinerTrack=true;}
   }
   if(!owner->Primary_ForVehicles&&!building->IsTether&&building->CurrentMission==Mission::Guard)break;
  }
  ASSERT_LT(frames,8000);EXPECT_TRUE(sawMinerTrack);EXPECT_FALSE(miner->InLimbo);EXPECT_FALSE(miner->IsTether);
  int miners=0,ifvs=0;
  for(int i=firstUnit;i<UnitClass::Array.Count;++i){auto* unit=UnitClass::Array[i];
   if(unit->Type==minerType)++miners;else if(unit->Type==ifvType)++ifvs;else continue;
   EXPECT_TRUE(unit->IsAlive);EXPECT_TRUE(unit->IsOnMap);EXPECT_FALSE(unit->InLimbo);EXPECT_FALSE(unit->IsTether);
   EXPECT_NE(unit->GetCell()->GetBuilding(),building);
  }
  EXPECT_EQ(miners,1);EXPECT_EQ(ifvs,2);EXPECT_FALSE(building->HasAnyLink());
  std::cout<<"CHRONO_FACTORY frames="<<frames<<" miners="<<miners<<" ifvs="<<ifvs<<" tether="<<building->IsTether<<'\n';
 },&view))<<game::map_view_error(*view.view);
}

TEST(UnitProduction, QueuePauseRefundAndInsufficientFunds){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 struct State{game::MapViewHandle* view;}s{view.view};
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);auto* owner=HouseClass::CurrentPlayer;
  for(auto* unit:UnitClass::Array)if(unit->Type->Harvester)unit->ForceMission(Mission::Sleep);
  auto* gi=InfantryTypeClass::Find("E1");auto* engineer=InfantryTypeClass::Find("ENGINEER");ASSERT_NE(gi,nullptr);ASSERT_NE(engineer,nullptr);
  const int cash=owner->Available_Money(),factories=FactoryClass::Array.Count;
  const auto step=[&](int count){while(count--){++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);}};
  step(1);ASSERT_EQ(owner->BeginProduction(AbstractType::InfantryType,gi->ArrayIndex,false),0);
  auto* f=owner->Primary_ForInfantry;ASSERT_NE(f,nullptr);step(f->GetBuildTimeFrames()*10);
  ASSERT_GT(f->GetProgress(),0);ASSERT_LT(f->GetProgress(),54);EXPECT_LT(owner->Available_Money(),cash);
  ASSERT_EQ(owner->SuspendProduction(AbstractType::InfantryType,gi->ArrayIndex,false),0);
  const int progress=f->GetProgress(),paid=cash-owner->Available_Money();step(100);
  EXPECT_EQ(f->GetProgress(),progress);EXPECT_EQ(owner->Available_Money(),cash-paid);
  ASSERT_EQ(owner->BeginProduction(AbstractType::InfantryType,gi->ArrayIndex,false),0);EXPECT_FALSE(f->IsSuspended);
  ASSERT_EQ(owner->BeginProduction(AbstractType::InfantryType,engineer->ArrayIndex,false),0);EXPECT_EQ(f->CountTotal(engineer),1);
  ASSERT_EQ(owner->BeginProduction(AbstractType::InfantryType,engineer->ArrayIndex,false),0);EXPECT_EQ(f->CountTotal(engineer),2);
  owner->AbandonProduction(AbstractType::InfantryType,engineer->ArrayIndex,false,false);EXPECT_EQ(f->CountTotal(engineer),1);EXPECT_EQ(owner->Available_Money(),cash-paid);
  owner->AbandonProduction(AbstractType::InfantryType,engineer->ArrayIndex,false,false);EXPECT_EQ(f->CountTotal(engineer),0);
  owner->AbandonProduction(AbstractType::InfantryType,gi->ArrayIndex,false,false);EXPECT_EQ(owner->Primary_ForInfantry,nullptr);EXPECT_EQ(owner->Available_Money(),cash);EXPECT_EQ(FactoryClass::Array.Count,factories);
  owner->Balance=0;ASSERT_EQ(owner->BeginProduction(AbstractType::InfantryType,gi->ArrayIndex,false),0);
  f=owner->Primary_ForInfantry;ASSERT_NE(f,nullptr);step(100);EXPECT_EQ(f->GetProgress(),0);EXPECT_TRUE(f->OnHold);EXPECT_EQ(owner->Balance,0);
  owner->GiveMoney(gi->Cost+engineer->Cost);ASSERT_EQ(owner->BeginProduction(AbstractType::InfantryType,engineer->ArrayIndex,false),0);
  const int beforeGI=owner->FactoryProducedInfantryTypes.GetItemCount(gi->ArrayIndex),beforeEngineer=owner->FactoryProducedInfantryTypes.GetItemCount(engineer->ArrayIndex);
  for(int i=0;i<10000&&owner->Primary_ForInfantry;++i)step(1);
  EXPECT_EQ(owner->Primary_ForInfantry,nullptr);EXPECT_EQ(owner->Balance,0);EXPECT_EQ(FactoryClass::Array.Count,factories);
  EXPECT_EQ(owner->FactoryProducedInfantryTypes.GetItemCount(gi->ArrayIndex),beforeGI+1);
  EXPECT_EQ(owner->FactoryProducedInfantryTypes.GetItemCount(engineer->ArrayIndex),beforeEngineer+1);
  std::cout<<"PRODUCTION_QUEUE paused_stage="<<progress<<" refunded="<<paid<<" queued_engineer=1 final_credits="<<owner->Balance<<"\n";
 },&s));
}

TEST(UnitProduction, MapTeardownWithPendingFactory){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 const int before=FactoryClass::Array.Count;
 {View view(data,true);view.load("ALL01UMD.MAP");
  ASSERT_TRUE(game::with_map_view(*view.view,[](void*){auto* owner=HouseClass::CurrentPlayer;
   owner->UpdatePower();auto* type=UnitTypeClass::Find("MTNK");ASSERT_NE(type,nullptr);ASSERT_EQ(owner->BeginProduction(AbstractType::UnitType,type->ArrayIndex,false),0);ASSERT_NE(owner->Primary_ForVehicles,nullptr);
  },nullptr));
 }
 EXPECT_EQ(FactoryClass::Array.Count,before);
}

TEST(UnitProduction, OriginalExecutableStageAndPaymentReference){
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  std::ifstream input(RA2_PRODUCTION_FIXTURE);std::string magic;input>>magic;ASSERT_EQ(magic,"FACTORY_UPDATE_V1");
  auto* owner=HouseClass::CurrentPlayer;TechnoClass* object=nullptr;for(auto* gi:InfantryClass::Array)if(gi->Owner==owner){object=gi;break;}ASSERT_NE(object,nullptr);
  int stage,balance,cash,suspended,frame,afterStage,afterBalance,afterCash,afterSuspended,afterHold,afterChanged,cases=0;
  while(input>>stage>>balance>>cash>>suspended>>frame>>afterStage>>afterBalance>>afterCash>>afterSuspended>>afterHold>>afterChanged){
   SCOPED_TRACE(cases);FactoryClass factory;factory.Owner=owner;factory.Object=object;
   struct Release{FactoryClass& f;~Release(){f.Object=nullptr;}}release{factory};
   factory.Balance=balance;factory.IsSuspended=suspended;factory.IsDifferent=false;factory.OnHold=false;
   Unsorted::CurrentFrame=0;factory.Production.Start(3);factory.Production.Value=stage;owner->Balance=cash;
   Unsorted::CurrentFrame=frame;factory.Update();
   EXPECT_EQ(factory.Production.Value,afterStage);EXPECT_EQ(factory.Balance,afterBalance);EXPECT_EQ(owner->Balance,afterCash);
   EXPECT_EQ(factory.IsSuspended,bool(afterSuspended));EXPECT_EQ(factory.OnHold,bool(afterHold));EXPECT_EQ(factory.IsDifferent,bool(afterChanged));++cases;
  }
  EXPECT_EQ(cases,400);
 },nullptr));
}

TEST(CampaignPresets, TypePassOrderMatchesOriginalInitializer) {
 CCINIClass ai,map;
 struct Context {CCINIClass* ai;CCINIClass* map;std::vector<int> calls;} c{&ai,&map,{}};
 game::ScenarioInitializeServices services{};services.context=&c;services.ai_ini=&ai;
 services.read_objects=[](void* p,game::ScenarioObjectReader reader,CCINIClass* ini,bool global){
  auto& c=*static_cast<Context*>(p);EXPECT_EQ(ini,global?c.ai:c.map);
  c.calls.push_back(int(reader)*2+int(global));
 };
 services.step=[](void* p,game::ScenarioInitializationStep step){
  EXPECT_EQ(step,game::ScenarioInitializationStep::finish_teams);
  static_cast<Context*>(p)->calls.push_back(-1);
 };
 game::read_scenario_type_definitions(services,map);
 using R=game::ScenarioObjectReader;
 EXPECT_EQ(c.calls,(std::vector<int>{int(R::teams)*2+1,int(R::teams)*2,
  int(R::scripts)*2+1,int(R::scripts)*2,int(R::task_forces)*2+1,int(R::task_forces)*2,
  int(R::trigger_types)*2,int(R::tags)*2,int(R::ai_triggers)*2+1,int(R::ai_triggers)*2,-1}));
}

TEST(CampaignPresets, GlobalMapOverridesForwardReferencesAndAIEnable) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  CCINIClass ai,map;
  ai.WriteString("TeamTypes","0","E1000001");
  ai.WriteString("E1000001","House","Americans");
  ai.WriteString("E1000001","Name","Named Team");
  ai.WriteString("E1000001","Script","E2000001");
  ai.WriteString("E1000001","TaskForce","E3000001");
  ai.WriteInteger("E1000001","Priority",5);
  ai.WriteString("ScriptTypes","0","E2000001");ai.WriteString("E2000001","0","3,214");
  ai.WriteString("TaskForces","0","E3000001");ai.WriteString("E3000001","0","2,MTNK");
  ai.WriteString("AITriggerTypes","E4000001","global,E1000001,<all>,99,1,MTNK,0100000003000000,50.75,1,100,1,0,2,0,<none>,1,0,1");
  ai.WriteString("AITriggerTypes","E4000002","disabled by map,E1000001,<all>,99,-1,<none>,00,20,1,100,1,0,2,0,<none>,1,1,1");
  map.WriteString("TeamTypes","0","E1000001");
  map.WriteInteger("E1000001","Priority",9);
  map.WriteString("E1000001","Tag","E5000001");
  map.WriteString("TeamTypes","1","E1000002");
  map.WriteString("E1000002","House","<Player @ C>");
  map.WriteString("E1000002","Script","E2000001");map.WriteString("E1000002","TaskForce","E3000001");
  map.WriteBool("E1000002","IsBaseDefense",true);
  map.WriteString("ScriptTypes","0","E2000001");map.WriteString("E2000001","0","3,77");
  map.WriteString("TaskForces","0","E3000001");map.WriteString("E3000001","0","4,MTNK");
  map.WriteString("Triggers","E6000001","Americans,E6000002,first,0,1,1,1,0");
  map.WriteString("Triggers","E6000002","Americans,<none>,second,1,1,1,1,0");
  map.WriteString("Actions","E6000001","1,53,2,E6000002,0,0,0,0,A");
  map.WriteString("Events","E6000001","2,7,1,Named Team,7,1,E9999999");
  map.WriteString("Tags","E5000001","0,tag,E6000001");
  map.WriteString("AITriggerTypes","E4000001","local,E1000001,Americans,99,1,MTNK,0200000004000000,30.9,2,90,0,0,1,1,E1000002,0,1,0");
  map.WriteBool("AITriggerTypesEnable","E4000001",true);
  map.WriteBool("AITriggerTypesEnable","E4000002",false);
  // Observe actual intermediate objects: Team reads create the referenced
  // script/task force; later list passes must fill those same identities.
  TeamTypeClass::LoadFromINIList(&ai,1);
  auto* team=TeamTypeClass::Find("E1000001");ASSERT_NE(team,nullptr);
  auto* script=team->ScriptType;auto* task=team->TaskForce;
  ASSERT_NE(script,nullptr);ASSERT_NE(task,nullptr);
  EXPECT_EQ(script->ActionsCount,0);EXPECT_EQ(task->CountEntries,0);
  EXPECT_EQ(team->IsGlobal,1);
  game::load_scenario_type_definitions(ai,map);
  EXPECT_EQ(TeamTypeClass::Find("E1000001"),team);EXPECT_EQ(team->Priority,9);EXPECT_EQ(team->IsGlobal,0);
  EXPECT_EQ(team->ScriptType,script);EXPECT_EQ(team->TaskForce,task);
  EXPECT_EQ(script->ScriptActions[0].Argument,77);EXPECT_FALSE(script->IsGlobal);
  ASSERT_EQ(task->CountEntries,1);EXPECT_EQ(task->Entries[0].Amount,4);EXPECT_EQ(task->IsGlobal,0);
  EXPECT_EQ(team->field_EC,int(task->Entries[0].Type->MovementZone));EXPECT_TRUE(team->field_F0);EXPECT_FALSE(team->field_F1);
  EXPECT_EQ(team->Tag,TagTypeClass::Find("E5000001"));ASSERT_NE(team->Tag,nullptr);
  auto* first=TriggerTypeClass::Find("E6000001");auto* second=TriggerTypeClass::Find("E6000002");
  ASSERT_NE(first,nullptr);ASSERT_NE(second,nullptr);EXPECT_EQ(first->NextTrigger,second);
  ASSERT_NE(first->FirstAction,nullptr);EXPECT_EQ(first->FirstAction->TriggerType,second);EXPECT_EQ(team->Tag->FirstTrigger,first);
  ASSERT_NE(first->FirstEvent,nullptr);EXPECT_EQ(first->FirstEvent->TeamType,nullptr);
  ASSERT_NE(first->FirstEvent->NextEvent,nullptr);EXPECT_EQ(first->FirstEvent->NextEvent->TeamType,team);
  EXPECT_EQ(TeamTypeClass::Find("E9999999"),nullptr); // Event team lookup must not allocate.
  auto* mp=TeamTypeClass::Find("E1000002");ASSERT_NE(mp,nullptr);
  EXPECT_EQ(mp->idxHouse,HouseClass::PlayerAtC);EXPECT_EQ(mp->Owner,nullptr);EXPECT_FALSE(mp->field_F0);
  auto* trigger=AITriggerTypeClass::Find("E4000001");ASSERT_NE(trigger,nullptr);
  EXPECT_EQ(trigger->Team1,team);EXPECT_EQ(trigger->Team2,mp);EXPECT_EQ(trigger->IsGlobal,0);EXPECT_TRUE(trigger->IsEnabled);
  EXPECT_EQ(trigger->OwnerHouseType,AITriggerHouseType::Single);
  EXPECT_EQ(trigger->TechLevel,task->Entries[0].Type->TechLevel);
  EXPECT_EQ(trigger->Weight_Current,30);EXPECT_EQ(trigger->Conditions[0].ComparatorOperand,2);
  EXPECT_EQ(trigger->Conditions[0].ComparatorType,AITriggerConditionComparatorType::Greater);
  EXPECT_FALSE(trigger->Enabled_Easy);EXPECT_TRUE(trigger->Enabled_Normal);EXPECT_FALSE(trigger->Enabled_Hard);
  auto* disabled=AITriggerTypeClass::Find("E4000002");ASSERT_NE(disabled,nullptr);EXPECT_EQ(disabled->IsGlobal,1);EXPECT_FALSE(disabled->IsEnabled);
  CCINIClass enable;enable.WriteBool("AITriggerTypesEnable","E4000002",false);
  auto runtime=game::scenario_runtime();runtime.session_mode=[](void*) noexcept{return 3;};
  EXPECT_TRUE(game::with_scenario_runtime(runtime,[](void* p){AITriggerTypeClass::LoadFromINIList(static_cast<CCINIClass*>(p),0);},&enable));
  EXPECT_TRUE(disabled->IsEnabled); // Non-campaign forces listed AI triggers on.
  // The original keeps global announcements until resolution. Resolve one
  // pointer through the real swizzler, including announcements for both passes.
  void* tag=reinterpret_cast<void*>(std::uintptr_t(0xE5000001u));
  auto& swizzle=SwizzleManagerClass::Instance;
  ASSERT_EQ(swizzle.Swizzle(&tag),0);ASSERT_EQ(swizzle.Reset(),0);EXPECT_EQ(tag,team->Tag);
 },nullptr));
}

TEST(CampaignPresets, NavalTeamPostprocessAndCampaignTechLevel) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  UnitTypeClass transport("E7000001"),ship("E7000002");
  transport.Naval=ship.Naval=true;transport.Passengers=6;ship.Passengers=0;
  transport.MovementZone=MovementZone::Amphibious;ship.MovementZone=MovementZone::Water;
  transport.TechLevel=-1;ship.TechLevel=6;
  TaskForceClass task("E7000003");task.CountEntries=2;task.Entries[0]={1,&transport};task.Entries[1]={1,&ship};
  TeamTypeClass team("E7000004");team.TaskForce=&task;team.ProcessTaskForce();
  EXPECT_TRUE(team.field_F1);EXPECT_FALSE(team.field_F0);EXPECT_EQ(team.field_EC,int(MovementZone::Water));
  EXPECT_EQ(task.GetRequiredTechLevel(),6); // Campaign ignores the -1 member.
  auto services=game::scenario_runtime();services.session_mode=[](void*) noexcept{return 3;};
  EXPECT_TRUE(game::with_scenario_runtime(services,[](void* p){EXPECT_EQ(static_cast<TaskForceClass*>(p)->GetRequiredTechLevel(),11);},&task));
 },nullptr));
}

TEST(Harvesting, RefineryChimneySmokeDrawsRisesAndExpires) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* ptr){
  auto& world=*static_cast<game::MapWorld*>(ptr);
  for(auto* unit:UnitClass::Array)if(unit->Type->Harvester)unit->ForceMission(Mission::Sleep);
  BuildingClass* refinery=nullptr;for(auto* b:HouseClass::CurrentPlayer->Buildings)if(b->Type->Refinery){refinery=b;break;}
  ASSERT_NE(refinery,nullptr);
  const int initialSystems=ParticleSystemClass::Array.Count,initialParticles=ParticleClass::Array.Count;
  for(const char* id:{"GAREFN","NAREFN"}){
   auto* type=BuildingTypeClass::Find(id);ASSERT_NE(type,nullptr);ASSERT_NE(type->RefinerySmokeParticleSystem,nullptr);
   EXPECT_EQ(type->RefinerySmokeFrames,50);EXPECT_NE(type->RefinerySmokeOffsetOne,CoordStruct::Empty);
   EXPECT_NE(type->RefinerySmokeOffsetTwo,CoordStruct::Empty);EXPECT_EQ(type->RefinerySmokeOffsetThree,CoordStruct::Empty);
   // The existing placed refinery supplies owner/location; swap only during
   // the original smoke hook so each real rules section is exercised.
   auto* saved=refinery->Type;refinery->Type=type;refinery->UpdateRefinerySmokeSystems();refinery->Type=saved;
   ASSERT_EQ(ParticleSystemClass::Array.Count,initialSystems+2);
   for(int i=initialSystems;i<ParticleSystemClass::Array.Count;++i){auto* s=ParticleSystemClass::Array[i];
    EXPECT_EQ(s->Lifetime,50);EXPECT_EQ(s->Owner,refinery);
    EXPECT_EQ(s->Location,refinery->Location+(i==initialSystems?type->RefinerySmokeOffsetOne:type->RefinerySmokeOffsetTwo));
   }
   bool drawn=false,rose=false,stopped=false;int firstZ=0;unsigned firstID=0;
   const int oldDetail=GameOptionsClass::Instance.DetailLevel;GameOptionsClass::Instance.DetailLevel=2;
   auto restoreDetail=ra2::test::scope_exit([&]{GameOptionsClass::Instance.DetailLevel=oldDetail;});
   for(int tick=0;tick<250&&ParticleSystemClass::Array.Count>initialSystems;++tick){
    ++Unsorted::CurrentFrame;game::update_map_world(world);
    for(int i=initialSystems;i<ParticleSystemClass::Array.Count;++i){auto* system=ParticleSystemClass::Array[i];
     stopped|=system->TimeToDie;
     if(!system->Particles.Count)continue;
     auto* p=system->Particles[0];
     if(!firstID){firstID=p->UniqueID;firstZ=p->Location.Z;}
     if(p->UniqueID==firstID)rose|=p->Location.Z>firstZ;
     ASSERT_NE(p->GetImage(),nullptr);EXPECT_STREQ(p->Type->ImageFile,"SGRYSMK1");
     EXPECT_LT(p->StartStateAI,p->GetImage()->Frames);
     struct Capture {const ParticleClass* particle;bool called=false;} capture{p};
     game::TypeDrawingContext context;context.backend_context=&capture;
     int targetToken=0,paletteToken=0;
     context.target=reinterpret_cast<game::DrawingTargetHandle*>(&targetToken);
     context.palette=reinterpret_cast<const game::DrawingPaletteHandle*>(&paletteToken);
     context.backend.shape=[](void* opaque,const game::ShapeDrawingRequest& r){
      auto& c=*static_cast<Capture*>(opaque);c.called=true;
      EXPECT_EQ(r.image,c.particle->GetImage());EXPECT_EQ(r.frame,c.particle->StartStateAI);
      EXPECT_EQ(r.flags,0x2E04u);EXPECT_EQ(r.gradient,2);EXPECT_NE(r.palette,nullptr);
      return game::DrawingStatus::drawn;
     };
     const auto status=game::with_type_drawing(context,[](void* opaque){
      auto& c=*static_cast<Capture*>(opaque);Point2D p{200,200};RectangleStruct bounds{0,0,640,480};c.particle->DrawIt(&p,&bounds);
     },&capture);
     EXPECT_EQ(status,game::DrawingStatus::drawn);drawn|=capture.called;
    }
   }
   EXPECT_TRUE(drawn);EXPECT_TRUE(rose);EXPECT_TRUE(stopped);
   EXPECT_EQ(ParticleSystemClass::Array.Count,initialSystems);EXPECT_EQ(ParticleClass::Array.Count,initialParticles);
  }
  // Expiration detaches the owner while allowing existing smoke to dissipate.
  auto* temporary=new BuildingClass(refinery->Type,refinery->Owner);temporary->Location=refinery->Location;
  temporary->UpdateRefinerySmokeSystems();ASSERT_EQ(ParticleSystemClass::Array.Count,initialSystems+2);
  delete temporary;
  for(int i=initialSystems;i<ParticleSystemClass::Array.Count;++i){EXPECT_EQ(ParticleSystemClass::Array[i]->Owner,nullptr);EXPECT_TRUE(ParticleSystemClass::Array[i]->TimeToDie);}
  for(int i=0;i<3;++i){++Unsorted::CurrentFrame;game::update_map_world(world);}
  EXPECT_EQ(ParticleSystemClass::Array.Count,initialSystems);
 },view.view->world.get()));
}

TEST(CampaignPresets, FirstMapTimedHarriersFlyAndRender) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 const int aircraft=AircraftClass::Array.Count,teams=TeamClass::Array.Count,scripts=ScriptClass::Array.Count;
 const int tags=TagClass::Array.Count,triggers=TriggerClass::Array.Count,logicTags=LogicClass::PendingTags.Count;
 {
  View view(data,true);view.load("ALL01UMD.MAP");
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){
   auto& world=*static_cast<game::MapWorld*>(p);
   // Isolate waypoint arrival/rendering; autonomous enemy AA and casualties
   // are covered by FirstMapGattlingEngagesTimedHarriers with unchanged rules.
   for(auto* b:BuildingClass::Array)if(!std::strcmp(b->Type->ID,"YAGGUN"))b->Deactivated=true;
   ASSERT_GT(LogicClass::PendingTags.Count,0);
   auto* type=AircraftTypeClass::Find("ORCA");ASSERT_NE(type,nullptr);
   ASSERT_NE(type->MainVoxel.VXL,nullptr);EXPECT_GT(type->Speed,0);EXPECT_GT(type->GetFlightLevel(),0);
   auto* trigger=TriggerClass::GetInstance(TriggerTypeClass::Find("0C077BCC"));ASSERT_NE(trigger,nullptr);
   EXPECT_FALSE(trigger->Enabled);
   const int first=Unsorted::CurrentFrame;
   int born=-1;CoordStruct origin[2]{};std::vector<CellStruct> airborneOrigins;
   for(int frame=0;frame<1400;++frame){
    Unsorted::CurrentFrame=first+frame;game::update_map_world(world);
    const int count=AircraftClass::Array.Count-world.impl->aircraft;
    if(count&&born<0){
     born=frame;ASSERT_EQ(count,2);
     for(int i=0;i<2;++i){auto* air=AircraftClass::Array[world.impl->aircraft+i];
      EXPECT_STREQ(air->Type->ID,"ORCA");EXPECT_STREQ(air->Owner->PlainName,"Alliance");
      ASSERT_NE(air->Team,nullptr);EXPECT_NE(air->Locomotor,nullptr);
      EXPECT_EQ(air->GetMapCoords(),ScenarioClass::Instance->GetWaypointCoords(air->Team->Type->Waypoint));
      origin[i]=air->Location;
      if(air->LastMapCoords==CellStruct{})airborneOrigins.push_back(air->GetMapCoords());
     }
    }
    if(born>=0&&frame==born+60){
     ASSERT_EQ(count,2);
     game::rebuild_world_sprites(world);
     for(int i=0;i<2;++i){auto* air=AircraftClass::Array[world.impl->aircraft+i];
      EXPECT_NE(air->Location,origin[i]);EXPECT_GT(air->GetHeight(),0);
      bool body=false,shadow=false;
      for(const auto& sprite:world.impl->sprites)if(sprite.owner==air&&sprite.voxel){
       if(sprite.shadow){shadow=true;EXPECT_EQ(sprite.layer,Layer::Ground);}
       else{body=true;EXPECT_EQ(sprite.layer,Layer::Top);}
      }
      EXPECT_TRUE(body);EXPECT_TRUE(shadow);
      std::cout<<"HARRIER frame="<<frame<<" xyz="<<air->Location.X<<","<<air->Location.Y<<","<<air->Location.Z<<" dest="<<air->Destination<<" mission="<<int(air->CurrentMission)<<"\n";
     }
    }
   }
   EXPECT_GE(born,135);EXPECT_LE(born,137);
   ASSERT_EQ(AircraftClass::Array.Count,world.impl->aircraft+2);
   for(int i=world.impl->aircraft;i<AircraftClass::Array.Count;++i){
    auto* air=AircraftClass::Array[i];
    EXPECT_EQ(air->GetMapCoords(),ScenarioClass::Instance->GetWaypointCoords(214));
    EXPECT_EQ(air->GetHeight(),0);EXPECT_EQ(air->Destination,nullptr);
   }
   expect_original_adjacency_counts(airborneOrigins);
   EXPECT_EQ(TeamTypeClass::Find("0780F21C")->cntInstances,0);
   EXPECT_EQ(TeamTypeClass::Find("0780BC8C")->cntInstances,0);
  },view.view->world.get()))<<game::map_view_error(*view.view);
 }
 EXPECT_EQ(AircraftClass::Array.Count,aircraft);EXPECT_EQ(TeamClass::Array.Count,teams);EXPECT_EQ(ScriptClass::Array.Count,scripts);
 EXPECT_EQ(TagClass::Array.Count,tags);EXPECT_EQ(TriggerClass::Array.Count,triggers);EXPECT_EQ(LogicClass::PendingTags.Count,logicTags);
}

TEST(CampaignPresets, FirstMapGattlingEngagesTimedHarriers) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){
  auto& world=*static_cast<game::MapWorld*>(p);int shots=0,damaged=0,migrated=0,crashing=0;
  auto* type=BuildingTypeClass::Find("YAGGUN");ASSERT_NE(type,nullptr);
  ASSERT_TRUE(type->IsGattling);ASSERT_EQ(type->WeaponStages,3);
  EXPECT_EQ(type->RateUp,1);EXPECT_EQ(type->RateDown,50);
  for(int i=0;i<6;++i){
   ASSERT_NE(type->Weapon[i].WeaponType,nullptr);
   EXPECT_EQ(type->Weapon[i].WeaponType->Projectile->AA,bool(i%2));
  }
  // No forced targets or altered rules: let the campaign timer create the
  // two planes and the powered enemy towers discover and attack them.
  for(int frame=0;frame<700;++frame){
   ++Unsorted::CurrentFrame;game::update_map_world(world);
   for(auto* b:BuildingClass::Array)if(b->Type==type&&b->LastFireBulletFrame==Unsorted::CurrentFrame
       &&b->Target&&b->Target->WhatAmI()==AbstractType::Aircraft){
    EXPECT_TRUE(b->IsPowerOnline());EXPECT_EQ(b->SelectWeapon(b->Target)%2,1);++shots;
   }
   for(auto* air:AircraftClass::Array){
    if(air->Health<air->Type->Strength)++damaged;
    if(air->IsCrashing){++crashing;EXPECT_EQ(air->Health,0);EXPECT_GT(air->GetHeight(),0);}
    if(air->GetHeight()>208&&air->GetLastFlightMapCoords().Y<80){
     ++migrated;auto& tracker=AircraftTrackerClass::Instance;
     const int bucket=tracker.GetVectorIndex(air->GetLastFlightMapCoords());
     EXPECT_GE(tracker.TrackerVectors[bucket/20][bucket%20].FindItemIndex(air),0);
    }
   }
  }
  std::cout<<"GATTLING aircraft shots="<<shots<<" damaged frames="<<damaged<<" migrated frames="<<migrated<<"\n";
  EXPECT_GT(shots,0);EXPECT_GT(damaged,0);EXPECT_GT(migrated,0);EXPECT_GT(crashing,0);
  EXPECT_EQ(AircraftClass::Array.Count,world.impl->aircraft);
  int tracked=0;for(const auto& row:AircraftTrackerClass::Instance.TrackerVectors)
   for(const auto& bucket:row)tracked+=bucket.Count;
  EXPECT_EQ(tracked,0);
 },view.view->world.get()))<<game::map_view_error(*view.view);
}

TEST(Gattling, OriginalRateTransitions) {
 HouseTypeClass houseType("RATE_OWNER");HouseClass owner(&houseType);BuildingTypeClass type("RATE_GATTLING",BuildingTypeClass::ConstructionDefaults{});BuildingClass tower(&type,&owner);
 WeaponTypeClass weapon("RATE_WEAPON");
 type.WeaponStages=3;type.RateUp=2;type.RateDown=3;
 for(int i=0;i<6;++i)type.Weapon[i].WeaponType=&weapon;
 const int ordinary[]{5,10,15},elite[]{3,7,11};
 for(int i=0;i<3;++i){type.WeaponStage[i]=ordinary[i];type.EliteStage[i]=elite[i];}
 std::ifstream input(RA2_GATTLING_FIXTURE);std::string magic;int count=0;
 ASSERT_TRUE(bool(input>>magic>>count));ASSERT_EQ(magic,"GATTLING_V1");ASSERT_EQ(count,1344);
 for(int i=0;i<count;++i){
  int rank,down,stage,value,elapsed,playing,expectedStage,expectedValue,expectedSound,expectedUnused;
  ASSERT_TRUE(bool(input>>rank>>down>>stage>>value>>elapsed>>playing>>expectedStage>>expectedValue>>expectedSound>>expectedUnused));
  SCOPED_TRACE(i);tower.Veterancy.Veterancy=rank?2.0f:0.0f;
  tower.CurrentGattlingStage=stage;tower.GattlingValue=value;
  tower.IsGattlingSoundPlaying=tower.IsUnusedGattlingSoundPlaying=playing;
  if(down)tower.GattlingRateDown(elapsed);else tower.GattlingRateUp(elapsed);
  EXPECT_EQ(tower.CurrentGattlingStage,expectedStage);EXPECT_EQ(tower.GattlingValue,expectedValue);
  EXPECT_EQ(tower.IsGattlingSoundPlaying,bool(expectedSound));EXPECT_EQ(tower.IsUnusedGattlingSoundPlaying,bool(expectedUnused));
 }
}

TEST(Paradrop, TanyaTargetsLowFallingInfantry) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 for(int startHeight:{100,20}){
  SCOPED_TRACE(startHeight);
  View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_game_view_size(*view.view,1280,720));
  struct State{game::MapViewHandle* view;InfantryClass* tanya=nullptr;InfantryClass* victim=nullptr;int id=0,startHeight;}s{view.view,nullptr,nullptr,0,startHeight};
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   BuildingClass* base=nullptr;for(auto* b:BuildingClass::Array)if(!std::strcmp(b->Type->ID,"GAWEAP")){base=b;break;}
   ASSERT_NE(base,nullptr);s.tanya=infantry_near_building("TANY",HouseClass::CurrentPlayer,base);ASSERT_NE(s.tanya,nullptr);
   HouseClass* enemy=nullptr;for(auto* h:HouseClass::Array)if(!std::strcmp(h->Type->ID,"Arabs")){enemy=h;break;}
   ASSERT_NE(enemy,nullptr);s.victim=infantry_near_building("INIT",enemy,base);ASSERT_NE(s.victim,nullptr);
   auto at=s.victim->Location;s.victim->Limbo();at.Z=MapClass::Instance.GetCellFloorHeight(at)+s.startHeight;
   ASSERT_TRUE(s.victim->SpawnParachuted(at));s.id=s.victim->UniqueID;
   ASSERT_TRUE(s.victim->IsFallingDown);ASSERT_GT(s.victim->GetHeight(),0);ASSERT_FALSE(s.victim->IsInAir());
   EXPECT_EQ(s.victim->InWhichLayer(),Layer::Ground);
   std::cout<<"TANYA_PARA initial error="<<int(s.tanya->GetFireError(s.victim,0,true))<<" action="<<int(s.tanya->MouseOverObject(s.victim,true))<<"\n";
   EXPECT_NE(s.tanya->GetFireError(s.victim,0,true),FireError::ILLEGAL);
   ASSERT_TRUE(s.tanya->Select());
  },&s));
  ASSERT_TRUE(click_world_object(*view.view,s.victim));
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){auto& s=*static_cast<State*>(p);
   const int initialFire=s.tanya->LastFireBulletFrame;bool killed=false;int height=0;
   for(int frame=0;frame<80;++frame){
    InfantryClass* victim=nullptr;for(auto* a:InfantryClass::Array)if(a->UniqueID==s.id&&a->IsAlive){victim=a;break;}
    if(!victim){killed=true;break;}
    height=victim->GetHeight();if(!victim->IsFallingDown)break;
    ++Unsorted::CurrentFrame;game::update_map_world(*s.view->world);game::rebuild_world_sprites(*s.view->world);
   }
   std::cout<<"TANYA_PARA initial_height="<<s.startHeight<<" killed="<<killed<<" last_height="<<height<<" fired="<<(s.tanya->LastFireBulletFrame!=initialFire)<<"\n";
   EXPECT_TRUE(killed);EXPECT_GT(height,0);EXPECT_NE(s.tanya->LastFireBulletFrame,initialFire);
  },&s));
 }
}

TEST(CampaignPresets, FirstMapTimedParadrop) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 const int aircraft=AircraftClass::Array.Count,infantry=InfantryClass::Array.Count,animations=AnimClass::Array.Count;
 const int teams=TeamClass::Array.Count,scripts=ScriptClass::Array.Count;
 {
  View view(data,true);view.load("ALL01UMD.MAP");ASSERT_TRUE(game::set_map_viewport(*view.view,640,480));
  ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){
   auto& world=*static_cast<game::MapWorld*>(p);
   auto* teamType=TeamTypeClass::Find("0D2AF91C");ASSERT_NE(teamType,nullptr);
   auto* trigger=TriggerClass::GetInstance(TriggerTypeClass::Find("069B22FC"));ASSERT_NE(trigger,nullptr);
   EXPECT_FALSE(trigger->Enabled);
   struct Wave {int plane,born,firstDrop=-1,lastDrop=-1;std::vector<int> ids,falling,landed;};
   std::vector<Wave> waves;bool rendered=false,cleaned=false,walked=false;
   for(int frame=0;frame<9000;++frame){
    ++Unsorted::CurrentFrame;game::update_map_world(world);
    for(auto* air:AircraftClass::Array)if(!std::strcmp(air->Type->ID,"PDPLANE")){
     auto found=std::find_if(waves.begin(),waves.end(),[&](const auto& wave){return wave.plane==air->UniqueID;});
     if(found==waves.end()){
      waves.push_back({int(air->UniqueID),frame});auto& wave=waves.back();
      EXPECT_EQ(air->Passengers.NumPassengers,5);EXPECT_EQ(air->CurrentMission,Mission::ParadropApproach);
      EXPECT_EQ(air->Owner,teamType->Owner);EXPECT_TRUE(air->IsALoaner);
      EXPECT_EQ(air->GetMapCoords(),ScenarioClass::Instance->GetWaypointCoords(teamType->TransportWaypoint));
      for(auto* foot=air->Passengers.FirstPassenger;foot;foot=static_cast<FootClass*>(foot->NextObject)){
       wave.ids.push_back(foot->UniqueID);EXPECT_STREQ(foot->GetTechnoType()->ID,"INIT");
       EXPECT_TRUE(foot->InLimbo);ASSERT_NE(foot->Team,nullptr);EXPECT_EQ(foot->Team->Type,teamType);
      }
      EXPECT_EQ(wave.ids.size(),5u);
     }
    }
    for(auto& wave:waves)for(auto* actor:InfantryClass::Array)if(std::find(wave.ids.begin(),wave.ids.end(),actor->UniqueID)!=wave.ids.end()){
     if(actor->IsFallingDown){
      if(std::find(wave.falling.begin(),wave.falling.end(),actor->UniqueID)==wave.falling.end()){
       wave.falling.push_back(actor->UniqueID);if(wave.firstDrop<0)wave.firstDrop=frame;wave.lastDrop=frame;
      }
      ASSERT_NE(actor->Parachute,nullptr);EXPECT_TRUE(actor->HasParachute);
      auto* chute=actor->Parachute;EXPECT_EQ(chute->OwnerObject,actor);
      const auto at=chute->GetCoords();EXPECT_EQ(at,(CoordStruct{actor->Location.X,actor->Location.Y,actor->Location.Z+75}));
      EXPECT_EQ(actor->SequenceAnim,Sequence::Paradrop);
      if(!rendered&&wave.falling.size()==5){
       const auto screen=TacticalClass::CoordsToScreen(at);
       world.impl->view.tactical.TacticalPos={screen.X-320,screen.Y-240};game::rebuild_world_sprites(world);
       for(const auto& sprite:world.impl->sprites)if(sprite.owner==actor&&sprite.image==chute->Type->Image&&!sprite.shadow){
        rendered=true;EXPECT_EQ(chute->InWhichLayer(),Layer::Ground); // Original ABI behavior stays intact.
        EXPECT_EQ(sprite.layer,std::max(Layer::Air,actor->ObjectClass::InWhichLayer()));
        EXPECT_EQ(sprite.frame,chute->Animation.Value+chute->Type->Start);
        EXPECT_EQ(sprite.flags,0x2E00u); // Read-only Z, not forced overlay or a new depth writer.
        const auto index=std::size_t(&sprite-world.impl->sprites.data());ASSERT_GT(index,0u);
        const auto& body=world.impl->sprites[index-1];EXPECT_EQ(body.owner,actor);EXPECT_FALSE(body.shadow);
        EXPECT_EQ(body.image,actor->Type->Image);EXPECT_EQ(body.layer,sprite.layer);EXPECT_EQ(body.draw_group,sprite.draw_group);
        EXPECT_TRUE(sprite.color_scheme);EXPECT_EQ(sprite.palette,game::world_palette(world,world.impl->unit_palette,actor->Owner));EXPECT_EQ(sprite.intensity,chute->TintColor);
        EXPECT_EQ(sprite.position,(Point2D{320,240+chute->Type->YDrawOffset}));
       }
      }
     }else if(actor->IsOnMap&&!actor->InLimbo){
      if(std::find(wave.landed.begin(),wave.landed.end(),actor->UniqueID)==wave.landed.end())wave.landed.push_back(actor->UniqueID);
      cleaned|=!actor->Parachute&&!actor->HasParachute;
      walked|=actor->CurrentMission==Mission::Move&&actor->Destination!=nullptr;
     }
    }
   }
   ASSERT_GE(waves.size(),2u);
   for(int i=0;i<2;++i){const auto& wave=waves[i];
    EXPECT_EQ(wave.falling.size(),5u);EXPECT_EQ(wave.landed.size(),5u);EXPECT_GT(wave.lastDrop,wave.firstDrop);
    std::cout<<"PARADROP wave="<<i<<" born="<<wave.born<<" drops="<<wave.firstDrop<<".."<<wave.lastDrop<<" landed="<<wave.landed.size()<<"\n";
   }
   // Unmodified campaign timers, targets and damage: the defenders kill the
   // first wave and APDeath re-enables AP_Paradrop for another 135*15 ticks.
   EXPECT_GE(waves[0].born,2025);EXPECT_GE(waves[1].born-waves[0].born,2025);
   EXPECT_TRUE(rendered);EXPECT_TRUE(cleaned);EXPECT_TRUE(walked);
   for(auto* air:AircraftClass::Array)EXPECT_NE(air->UniqueID,waves[0].plane);
  },view.view->world.get()))<<game::map_view_error(*view.view);
 }
 EXPECT_EQ(AircraftClass::Array.Count,aircraft);EXPECT_EQ(InfantryClass::Array.Count,infantry);EXPECT_EQ(AnimClass::Array.Count,animations);
 EXPECT_EQ(TeamClass::Array.Count,teams);EXPECT_EQ(ScriptClass::Array.Count,scripts);
}

TEST(CampaignPresets, ParadropSurvivorsFollowAttackScript) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void* p){
  auto& world=*static_cast<game::MapWorld*>(p);auto* type=TeamTypeClass::Find("0D2AF91C");ASSERT_NE(type,nullptr);
  // Isolate script progression from the defenders that normally kill this
  // wave en route. The unmodified combat/repeat path is tested above.
  for(auto* actor:TechnoClass::Array)if(actor->Owner!=type->Owner)actor->Deactivated=true;
  bool attacked=false;int maxScript=-1;
  for(int frame=0;frame<11000&&!attacked;++frame){
   ++Unsorted::CurrentFrame;game::update_map_world(world);
   for(auto* team:TeamClass::Array)if(team->Type==type){
    maxScript=std::max(maxScript,team->CurrentScript->CurrentMission);
    for(auto* foot=team->FirstUnit;foot;foot=foot->NextTeamMember)
     attacked|=!foot->IsFallingDown&&foot->CurrentMission==Mission::Attack&&foot->Target&&foot->Target->WhatAmI()==AbstractType::Building;
   }
  }
  if(!attacked)for(auto* team:TeamClass::Array)if(team->Type==type)for(auto* foot=team->FirstUnit;foot;foot=foot->NextTeamMember)
   std::cout<<"PARA_ROUTE id="<<foot->UniqueID<<" pos="<<foot->GetMapCoords().X<<","<<foot->GetMapCoords().Y<<" dest="<<foot->Destination<<" mission="<<int(foot->CurrentMission)<<" health="<<foot->Health<<" under="<<team->IsUnderStrength<<"\n";
  EXPECT_TRUE(attacked)<<"script="<<maxScript;
 },view.view->world.get()));
}

TEST(Paradrop, OriginalMissionDecisions) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 View view(data,true);view.load("ALL01UMD.MAP");
 ASSERT_TRUE(game::with_map_view(*view.view,[](void*){
  struct Plane:AircraftClass {
   Plane():AircraftClass(nullptr,nullptr){}
   bool QueueMission(Mission mission,bool) override{QueuedMission=mission;return true;}
   void SetDestination(AbstractClass* target,bool) override{Destination=target;}
   void SetTarget(AbstractClass* target) override{Target=target;}
  } plane;
  struct Cargo:InfantryClass {
   mutable int attempts=0;
   Cargo():InfantryClass(InfantryTypeClass::Find("INIT"),nullptr){}
   Move IsCellOccupied(CellClass*,FacingType,int,CellClass*,bool) const override{++attempts;return Move::No;}
  } cargo;
  ASSERT_TRUE(cargo.InitializeLocomotor());
  InfantryClass target(nullptr,nullptr);RulesClass::Instance->ParadropRadius=1024;
  const auto inside=MapClass::Instance.GetCellAt(ScenarioClass::Instance->GetWaypointCoords(41))->GetCoords();
  std::ifstream input(RA2_PARADROP_FIXTURE);std::string magic;int count=0;
  ASSERT_TRUE(bool(input>>magic>>count));ASSERT_EQ(magic,"PARADROP_V1");ASSERT_EQ(count,512);
  for(int i=0;i<count;++i){
   int over,hasTarget,hasDest,passengers,distance,left,inMap,result,mission,targetOut,destOut,locked,leftOut,drops;
   ASSERT_TRUE(bool(input>>over>>hasTarget>>hasDest>>passengers>>distance>>left>>inMap>>result>>mission>>targetOut>>destOut>>locked>>leftOut>>drops));SCOPED_TRACE(i);
   plane.Location=inMap?inside:CoordStruct{-4096,-4096,0};target.Location=plane.Location;target.Location.X+=distance;
   plane.Target=hasTarget?&target:nullptr;plane.Destination=hasDest?&target:nullptr;
   plane.Passengers.NumPassengers=passengers;plane.Passengers.FirstPassenger=passengers?&cargo:nullptr;
   plane.NumParadropsLeft=left;plane.IsLocked=false;plane.QueuedMission=Mission::None;plane.Ammo=100;cargo.attempts=0;
   EXPECT_EQ(over?plane.Mission_ParaDropOverfly():plane.Mission_ParaDropApproach(),result);
   EXPECT_EQ(int(plane.QueuedMission),mission);EXPECT_EQ(bool(plane.Target),bool(targetOut));EXPECT_EQ(bool(plane.Destination),bool(destOut));
   EXPECT_EQ(plane.IsLocked,bool(locked));EXPECT_EQ(plane.NumParadropsLeft,leftOut);EXPECT_EQ(cargo.attempts,drops);
   // A blocked drop restores the same cargo and ammo for the next attempt.
   EXPECT_EQ(plane.Passengers.NumPassengers,passengers);EXPECT_EQ(plane.Ammo,100);
   if(passengers)EXPECT_EQ(plane.Passengers.FirstPassenger,&cargo);
  }
  plane.Passengers.NumPassengers=0;plane.Passengers.FirstPassenger=nullptr;
 },nullptr));
}

TEST(Paradrop, SceneCleanupDuringFlight) {
 const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA";
 const int aircraft=AircraftClass::Array.Count,infantry=InfantryClass::Array.Count,animations=AnimClass::Array.Count;
 const int teams=TeamClass::Array.Count,scripts=ScriptClass::Array.Count,feet=FootClass::Array.Count;
 for(const int ticks:{3600,4000}){
  {
   View view(data,true);view.load("ALL01UMD.MAP");
   struct Context{game::MapWorld& world;int ticks;} context{*view.view->world,ticks};
   ASSERT_TRUE(game::with_map_view(*view.view,[](void* opaque){
    auto& context=*static_cast<Context*>(opaque);
    for(int i=0;i<context.ticks;++i){++Unsorted::CurrentFrame;game::update_map_world(context.world);}
    int cargo=0,falling=0;
    for(auto* plane:AircraftClass::Array)if(!std::strcmp(plane->Type->ID,"PDPLANE"))cargo+=plane->Passengers.NumPassengers;
    for(auto* actor:InfantryClass::Array)if(actor->IsFallingDown&&actor->Parachute)++falling;
    if(context.ticks==3600){EXPECT_EQ(cargo,5);EXPECT_EQ(falling,0);}
    else{EXPECT_EQ(cargo,0);EXPECT_EQ(falling,5);}
   },&context));
  }
  EXPECT_EQ(AircraftClass::Array.Count,aircraft);EXPECT_EQ(InfantryClass::Array.Count,infantry);EXPECT_EQ(AnimClass::Array.Count,animations);
  EXPECT_EQ(TeamClass::Array.Count,teams);EXPECT_EQ(ScriptClass::Array.Count,scripts);EXPECT_EQ(FootClass::Array.Count,feet);
 }
}
