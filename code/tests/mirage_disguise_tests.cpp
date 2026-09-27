#include "support/test_support.hpp"
#include "yrpp/UnitClass.h"
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "scenario_runtime.hpp"
#include <array>
#include <fstream>
#include <sstream>

TEST(MirageDisguise, OriginalInstructionCorpus) {
 auto runtime=game::default_scenario_runtime();runtime.session_mode=[](void*)noexcept{return int(GameMode::Campaign);};
 ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void*){
  RulesClass rules;ScenarioClass scenario;
  auto* oldRules=RulesClass::Instance;auto* oldScenario=ScenarioClass::Instance;auto* oldPlayer=HouseClass::CurrentPlayer;
  const bool oldActive=Game::IsActive;Game::IsActive=true;
  const int oldBridge=IsometricTileTypeClass::BridgeSet;IsometricTileTypeClass::BridgeSet=-1;
  const auto oldGuard=MissionControlClass::Array[int(Mission::Guard)];
  const int oldFrame=Unsorted::CurrentFrame;
  RulesClass::Instance=&rules;ScenarioClass::Instance=&scenario;
  auto restore=ra2::test::scope_exit([&]{MapClass::Instance.ReleaseCellStorage();MissionControlClass::Array[int(Mission::Guard)]=oldGuard;Game::IsActive=oldActive;IsometricTileTypeClass::BridgeSet=oldBridge;RulesClass::Instance=oldRules;ScenarioClass::Instance=oldScenario;HouseClass::CurrentPlayer=oldPlayer;Unsorted::CurrentFrame=oldFrame;});
  ASSERT_TRUE(MapClass::Instance.CreateEmptyCells({0,0,64,64},0));
  HouseTypeClass country("MIRAGE_COUNTRY");HouseClass owner(&country),viewer(&country),other(&country);
  owner.ArrayIndex=0;viewer.ArrayIndex=1;other.ArrayIndex=2;
  UnitTypeClass type("MIRAGE_TEST"),actorType("DEFENDER_TEST");type.Strength=actorType.Strength=100;
  TerrainTypeClass tree0("TREE_TEST0"),tree1("TREE_TEST1"),tree2("TREE_TEST2"),tree3("TREE_TEST3");
  std::array<TerrainTypeClass*,4> trees{&tree0,&tree1,&tree2,&tree3};
  for(auto* tree:trees)rules.DefaultMirageDisguises.AddItem(tree);
  rules.InfantryBlinkDisguiseTime=20;
  for(int chance:{15,5,2})rules.DisabledDisguiseDetectionPercent.AddItem(chance);
  WeaponTypeClass weapon("MIRAGE_WEAPON");WarheadTypeClass warhead("MIRAGE_WARHEAD");
  weapon.Damage=10;weapon.Warhead=&warhead;weapon.Range=4096;for(auto& v:warhead.Verses)v=1.0;
  struct Actor:UnitClass {
   WeaponStruct slot{};
   Actor(UnitTypeClass* t,HouseClass* h,WeaponTypeClass* w):UnitClass(t,h){slot.WeaponType=w;}
   WeaponStruct* GetWeapon(int)const override{return const_cast<WeaponStruct*>(&slot);}
   int SelectWeapon(AbstractClass*)const override{return 0;}
   FireError GetFireErrorWithoutRange(AbstractClass*,int)const override{return FireError::OK;}
   bool IsArmed()const override{return true;}
  } unit(&type,&owner,&weapon),actor(&actorType,&viewer,&weapon);
  struct Driver:DriveLocomotionClass {bool moving=false;bool YRPP_STDCALL Is_Moving()override{return moving;}} driver;
  InfantryTypeClass infType("NEARBY_TEST");InfantryClass infantry(&infType,&viewer);
  AnimClass ring(nullptr,CoordStruct::Empty);
  unit.Locomotor=&driver;unit.RadioLinks.SetCapacity(1);unit.RadioLinks[0]=nullptr;
  const auto detach=ra2::test::scope_exit([&]{unit.Locomotor=nullptr;unit.MindControlRingAnim=nullptr;MapClass::Instance.GetCellAt(CellStruct{40,39})->FirstObject=nullptr;});
  unit.Location=actor.Location={10368,10368,0};auto* cell=unit.GetCell();auto* neighbor=MapClass::Instance.GetCellAt(CellStruct{40,39});
  auto identity=[&](const void* p){if(!p)return 0;for(int i=0;i<4;++i)if(p==trees[i])return i+1;return p==&type?5:p==&owner?6:p==&viewer?7:8;};
  auto reset=[&]{
   scenario.Random=Randomizer(12345);Unsorted::CurrentFrame=1000;HouseClass::CurrentPlayer=&viewer;
   owner.IsHumanPlayer=viewer.IsHumanPlayer=false;owner.Allies.data=viewer.Allies.data=other.Allies.data=0;
   if(cell->DisguiseSensors_InclHouse(1))cell->DisguiseSensors_RemOfHouse(1);neighbor->FirstObject=nullptr;unit.Disguised=false;unit.Disguise=&tree0;unit.DisguisedAsHouse=nullptr;
   unit.DisguiseCreationFrame=900;unit.InfantryBlinkTimer.StartTime=990;unit.InfantryBlinkTimer.TimeLeft=0;
   unit.DisguiseBlinkTimer.StartTime=990;unit.DisguiseBlinkTimer.TimeLeft=0;unit.MindControlRingAnim=nullptr;ring.Invisible=false;
   unit.Health=unit.EstimatedHealth=actor.Health=actor.EstimatedHealth=100;
   unit.InLimbo=actor.InLimbo=false;unit.IsInPlayfield=actor.IsInPlayfield=true;unit.IsAlive=actor.IsAlive=true;
   unit.DiscoveredByCurrentPlayer=true;unit.CurrentMission=actor.CurrentMission=Mission::Guard;
   unit.Target=actor.Target=nullptr;type.LegalTarget=true;actorType.CanRetaliate=true;
   MissionControlClass::Array[int(Mission::Guard)].Retaliate=true;MissionControlClass::Array[int(Mission::Guard)].NoThreat=false;
  };
  std::ifstream input(RA2_MIRAGE_DISGUISE_FIXTURE);ASSERT_TRUE(input.good());std::string line;std::array<int,3> counts{};
  while(std::getline(input,line)) {
   std::istringstream row(line);char kind;row>>kind;if(kind=='F'||kind=='D'||kind=='V')continue;reset();
   SCOPED_TRACE(line);
   int flags;row>>flags;
   if(kind=='U') {
    int frame;row>>frame;std::array<int,9> expected;for(auto& v:expected)ASSERT_TRUE(bool(row>>v));
    scenario.Random=Randomizer(12345+flags);Unsorted::CurrentFrame=frame;unit.Disguised=flags&1;driver.moving=flags&2;type.DisguiseWhenStill=flags&4;
    unit.RadioLinks[0]=flags&8?&actor:nullptr;
    if(flags&(16|32)){neighbor->FirstObject=&infantry;infantry.Owner=flags&16?&viewer:&owner;}
    unit.InfantryBlinkTimer.StartTime=0;unit.InfantryBlinkTimer.TimeLeft=flags&64?20:0;
    unit.MindControlRingAnim=flags&128?&ring:nullptr;owner.IsHumanPlayer=flags&256;
    unit.UpdateDisguise();
    std::array<int,9> actual{unit.Disguised,identity(unit.Disguise),identity(unit.DisguisedAsHouse),int(unit.DisguiseCreationFrame),
     unit.InfantryBlinkTimer.StartTime,unit.InfantryBlinkTimer.TimeLeft,ring.Invisible,scenario.Random.Next1,scenario.Random.Next2};
    // Nine output fields follow the two inputs in the binary fixture.
    EXPECT_EQ(actual,expected);++counts[0];
   } else if(kind=='Q') {
    int age;row>>age;std::array<int,5> expected;for(auto& v:expected)ASSERT_TRUE(bool(row>>v));
    unit.Disguised=flags&1;owner.IsHumanPlayer=flags&2;HouseClass::CurrentPlayer=flags&2?&owner:&viewer;
    owner.Allies.data=flags&4?2:0;if(flags&8)cell->DisguiseSensors_AddOfHouse(1);
    unit.DisguisedAsHouse=flags&16?&viewer:flags&32?&other:nullptr;viewer.Allies.data=flags&64?4:0;
    unit.DisguiseCreationFrame=1000-age;unit.DisguiseBlinkTimer.TimeLeft=flags&128?12:0;
    std::array<int,5> actual{unit.IsDisguisedAs(&viewer),unit.IsClearlyVisibleTo(&viewer),int(unit.GetDisguiseFlags(0x2E00)),identity(unit.GetDisguise(false)),identity(unit.GetDisguiseHouse(false))};
    EXPECT_EQ(actual,expected);++counts[1];
   } else if(kind=='A') {
    int difficulty,seed;row>>difficulty>>seed;std::array<int,4> expected;for(auto& v:expected)ASSERT_TRUE(bool(row>>v));
    unit.Disguised=flags&1;viewer.IsHumanPlayer=flags&2;if(flags&4)cell->DisguiseSensors_AddOfHouse(1);
    owner.Allies.data=flags&8?2:0;unit.DisguiseBlinkTimer.TimeLeft=flags&16?12:0;actorType.DetectDisguise=flags&32;
    viewer.AIDifficulty=AIDifficulty(difficulty);scenario.Random=Randomizer(seed);int value=0;
    const bool retaliation=actor.CanRetaliateToAttacker(&unit,nullptr);
    const bool acquisition=actor.CanAutoTargetObject(ThreatType(0),2,4096,&unit,&value,DWORD(-1),&CoordStruct::Empty);
    std::array<int,4> actual{retaliation,acquisition,scenario.Random.Next1,scenario.Random.Next2};
    EXPECT_EQ(actual,expected);++counts[2];
   } else FAIL()<<"Unknown fixture kind";
  }
  EXPECT_EQ(counts,(std::array<int,3>{3584,5376,4608}));
 },nullptr));
}

TEST(UnitTargeting, OriginalWeaponCategoryExpansionCorpus) {
 auto runtime=game::default_scenario_runtime();runtime.session_mode=[](void*)noexcept{return int(GameMode::Campaign);};
 ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void*){
  RulesClass rules;ScenarioClass scenario;auto* oldRules=RulesClass::Instance;auto* oldScenario=ScenarioClass::Instance;auto* oldPlayer=HouseClass::CurrentPlayer;
  RulesClass::Instance=&rules;ScenarioClass::Instance=&scenario;
  auto restore=ra2::test::scope_exit([&]{MapClass::Instance.ReleaseCellStorage();RulesClass::Instance=oldRules;ScenarioClass::Instance=oldScenario;HouseClass::CurrentPlayer=oldPlayer;});
  ASSERT_TRUE(MapClass::Instance.CreateEmptyCells({0,0,64,64},0));
  HouseTypeClass country("SCAN_COUNTRY");HouseClass owner(&country),enemy(&country);HouseClass::CurrentPlayer=&owner;
  owner.ArrayIndex=0;enemy.ArrayIndex=1;owner.Allies.data=enemy.Allies.data=0;
  UnitTypeClass type("SCAN_UNIT"),victimType("SCAN_VICTIM");type.Strength=victimType.Strength=100;victimType.LegalTarget=true;
  BulletTypeClass p0("SCAN_P0"),p1("SCAN_P1"),p2("SCAN_P2");
  WeaponTypeClass w0("SCAN_W0"),w1("SCAN_W1"),w2("SCAN_W2"),neutral("SCAN_NEUTRAL");
  BulletTypeClass* projectiles[]{&p0,&p1,&p2};WeaponTypeClass* weapons[]{&w0,&w1,&w2};
  for(int i=0;i<3;++i){weapons[i]->Projectile=projectiles[i];weapons[i]->Damage=10;weapons[i]->Range=4096;}
  neutral.Damage=10;neutral.Range=4096;
  unsigned short zone=0;auto* oldZones=MapClass::Instance.MovementZones[int(type.MovementZone)];
  MapClass::Instance.MovementZones[int(type.MovementZone)]=&zone;
  auto restoreZones=ra2::test::scope_exit([&]{MapClass::Instance.MovementZones[int(type.MovementZone)]=oldZones;});
  struct Actor:UnitClass {
   WeaponStruct slots[3]{};
   Actor(UnitTypeClass* t,HouseClass* h):UnitClass(t,h){}
   WeaponStruct* GetWeapon(int index)const override{return const_cast<WeaponStruct*>(&slots[index]);}
   int SelectWeapon(AbstractClass*)const override{return 0;}
   FireError GetFireErrorWithoutRange(AbstractClass*,int)const override{return FireError::OK;}
   bool IsArmed()const override{return true;}
  } unit(&type,&owner),victim(&victimType,&enemy);
  type.Locomotor=victimType.Locomotor=LocomotionClass::CLSIDs::Drive;
  ASSERT_TRUE(unit.InitializeLocomotor());ASSERT_TRUE(victim.InitializeLocomotor());
  victim.slots[0].WeaponType=&neutral;unit.CurrentWeaponNumber=2;
  for(auto* actor:{&unit,&victim}){
   actor->Location={10368,10368,0};actor->Health=actor->EstimatedHealth=100;
   actor->InLimbo=false;actor->IsInPlayfield=true;actor->IsAlive=true;actor->LastLayer=Layer::Ground;
   actor->DiscoveredByCurrentPlayer=true;actor->CurrentMission=Mission::Guard;
  }
  auto* control=unit.CurrentMissionControl();const auto previous=*control;control->NoThreat=false;
  auto restoreControl=ra2::test::scope_exit([&]{*control=previous;});
  std::ifstream input(RA2_UNIT_TARGETING_FIXTURE);ASSERT_TRUE(input.good());int mode,primary,secondary,threat,human,expected,cases=0;
  while(input>>mode>>primary>>secondary>>threat>>human>>expected){
   SCOPED_TRACE(::testing::Message()<<mode<<' '<<primary<<' '<<secondary<<' '<<threat<<' '<<human);
   owner.IsHumanPlayer=human;type.TurretCount=mode==1||mode==2?1:0;type.IsGattling=mode==2;type.DeployToFire=mode==3;
   const int configs[]{primary,secondary,3};
   for(int i=0;i<3;++i){unit.slots[i].WeaponType=configs[i]==4?nullptr:weapons[i];projectiles[i]->AA=configs[i]&1;projectiles[i]->AG=configs[i]&2;}
   auto origin=unit.Location;auto* target=unit.GreatestThreat(ThreatType(threat),&origin,false);
   EXPECT_EQ(target==&victim,bool(expected));EXPECT_TRUE(!target||target==&victim);++cases;
  }
  EXPECT_EQ(cases,1200);
 },nullptr));
}
