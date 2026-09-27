#include "support/test_support.hpp"
#include "yrpp/MouseClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Surface.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/WarheadTypeClass.h"
#include "api/clock.hpp"
#include "scenario_runtime.hpp"
#include "map_world_fixture.hpp"
#include "game_ui_runtime.hpp"
#include <array>
#include <bit>
#include <cstdlib>
#include <cfenv>
#include <sstream>

namespace {
void power_map_fixture(const std::filesystem::path& root) {
    map_fixture::fixtures(root);
    std::ifstream input(root/"world.map");std::string text((std::istreambuf_iterator<char>(input)),{});
    const std::string border="Neutral,BLDG,128,8,3,";
    const auto at=text.find(border);ASSERT_NE(at,std::string::npos);
    // The shared drawing fixture is outside the playable 2/2/2/6 border.
    // Techno.Unlimbo correctly marks it undiscovered there, so a campaign
    // House excludes it from power. Put this test's generator inside instead.
    text.replace(at,border.size(),"Neutral,BLDG,128,8,8,");
    std::ofstream(root/"world.map")<<text;
}
struct PowerWorld {
    int rounding=std::fegetround();
    HouseTypeClass country{"POWERCOUNTRY"};
    HouseClass owner{&country};
    BuildingTypeClass generator{"POWERGEN",{}},consumer{"POWERUSE",{}},upgrade{"POWERUP",{}};
    BuildingClass a{&generator,&owner},b{&consumer,&owner};
    HouseClass* previous=HouseClass::CurrentPlayer;
    PowerWorld() {
        // The pinned reference executes with the game's 0x0E7F control word.
        // Rules integer parsing installs the same rounding mode in sessions.
        std::fesetround(FE_TOWARDZERO);
        HouseClass::CurrentPlayer=&owner;
        generator.Strength=consumer.Strength=100;
        generator.PowerBonus=200;consumer.PowerDrain=80;
        a.Health=b.Health=100;
        a.IsOnMap=b.IsOnMap=true;a.InLimbo=b.InLimbo=false;
    }
    ~PowerWorld() {
        a.IsOnMap=b.IsOnMap=false;
        a.DrainingMe=b.DrainingMe=nullptr;
        a.Passengers.NumPassengers=0;
        a.Upgrades[0]=nullptr;
        HouseClass::CurrentPlayer=previous;
        std::fesetround(rounding);
    }
};
struct FactoryProbe : FactoryClass {
    FactoryProbe():FactoryClass(noinit_t{}) {Object=nullptr;Owner=nullptr;}
    ~FactoryProbe() override {Object=nullptr;}
};
struct SuperProbe : SuperClass {
    SuperProbe():SuperClass(noinit_t{}) {Type=nullptr;Owner=nullptr;}
};
template<class F> int rows(char tag,F&& operation) {
    std::ifstream input(RA2_POWER_FIXTURE);
    EXPECT_TRUE(input.good());
    std::string line;std::getline(input,line);EXPECT_EQ(line,"POWER_REFERENCE_V1");
    int count=0;
    while(std::getline(input,line))if(!line.empty()&&line[0]==tag) {
        SCOPED_TRACE(line);
        std::istringstream row(line.substr(2));operation(row);
        EXPECT_FALSE(row.fail());++count;
    }
    return count;
}
}

TEST(Power, BuildingOutputAndDrainMatchOriginalInstructions) {
    PowerWorld w;
    EXPECT_EQ(rows('B',[&](auto& row){
        int health,strength,output,drain,bits,passengers,expectedOutput,expectedDrain;
        row>>health>>strength>>output>>drain>>bits>>passengers>>expectedOutput>>expectedDrain;
        w.a.Health=health;w.generator.Strength=strength;w.generator.PowerBonus=output;w.generator.PowerDrain=drain;
        w.generator.ExtraPowerBonus=37;w.generator.ExtraPowerDrain=13;w.generator.UnitAbsorb=true;
        w.a.HasPower=bits&1;w.a.HasExtraPowerBonus=bits&2;w.a.HasExtraPowerDrain=bits&4;
        w.a.BeingWarpedOut=bits&8;w.a.UpgradeLevel=bool(bits&16);w.a.Upgrades[0]=&w.upgrade;
        w.upgrade.PowerBonus=53;w.upgrade.PowerDrain=11;w.a.Passengers.NumPassengers=passengers;
        EXPECT_EQ(w.a.GetPowerOutput(),expectedOutput);EXPECT_EQ(w.a.GetPowerDrain(),expectedDrain);
    }),3846);
}

TEST(Power, IntegerConversionPreservesOriginalModeChange) {
    PowerWorld w;
    EXPECT_EQ(rows('N',[&](auto& row){
        int control,health,strength,nominal,expected,after;row>>control>>health>>strength>>nominal>>expected>>after;
        std::fesetround(control==0x027F?FE_TONEAREST:FE_TOWARDZERO);
        w.a.Health=health;w.generator.Strength=strength;w.generator.PowerBonus=nominal;
        EXPECT_EQ(w.a.GetPowerOutput(),expected);EXPECT_EQ(after,0x0E7F);
        EXPECT_EQ(std::fegetround(),FE_TOWARDZERO);
    }),16);
}

TEST(Power, HouseAccountingMatchesOriginalInstructions) {
    PowerWorld w;
    EXPECT_EQ(rows('H',[&](auto& row){
        int mode,control,bits,blackout,output,drain,drained,factoryCalls,superCalls;
        row>>mode>>control>>bits>>blackout>>output>>drain>>drained>>factoryCalls>>superCalls;
        w.owner.IsHumanPlayer=control&1;w.owner.IsInPlayerControl=control&2;
        w.owner.PowerBlackoutTimer.StartTime=-1;w.owner.PowerBlackoutTimer.TimeLeft=blackout;
        w.a.HasPower=bits&1;w.a.InLimbo=bits&2;w.a.IsOnMap=bits&4;
        w.a.DiscoveredByCurrentPlayer=bits&8;w.b.DiscoveredByCurrentPlayer=bits&16;
        w.a.BeingWarpedOut=bits&32;w.a.DrainingMe=(bits&64)?&w.b:nullptr;
        w.owner.PowerOutput=200;w.owner.PowerDrain=80;
        w.owner.RecheckPower=true;w.owner.RecheckRadar=false;
        auto runtime=game::default_scenario_runtime();runtime.context=&mode;
        runtime.session_mode=[](void* p)noexcept{return *static_cast<int*>(p);};
        ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void* p){static_cast<HouseClass*>(p)->UpdatePower();},&w.owner));
        EXPECT_EQ(w.owner.PowerOutput,output);EXPECT_EQ(w.owner.PowerDrain,drain);
        EXPECT_EQ(w.owner.IsBeingDrained,drained!=0);EXPECT_FALSE(w.owner.RecheckPower);EXPECT_TRUE(w.owner.RecheckRadar);
        EXPECT_EQ(w.owner.Power_Output(),output);EXPECT_EQ(w.owner.Power_Drain(),drain);
        // Side-effect callees are independently exercised below; this case
        // checks accounting with no active production or superweapon entries.
        EXPECT_EQ(factoryCalls,1);EXPECT_EQ(superCalls,output<drain&&drain?1:0);
    }),2048);
}

TEST(Power, RadarAvailabilityMatchesOriginalInstructions) {
    PowerWorld w;ScenarioClass scenario;auto* previous=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
    auto restore=ra2::test::scope_exit([&]{ScenarioClass::Instance=previous;});
    EXPECT_EQ(rows('R',[&](auto& row){
        int mode,control,first,low,free,blackout,expected;
        row>>mode>>control>>first>>low>>free>>blackout>>expected;
        w.owner.IsHumanPlayer=control&1;w.owner.IsInPlayerControl=control&2;
        w.owner.PowerOutput=low?40:200;w.owner.PowerDrain=80;
        w.owner.RadarBlackoutTimer.StartTime=-1;w.owner.RadarBlackoutTimer.TimeLeft=blackout;
        scenario.FreeRadar=free;
        for(auto* b:{&w.a,&w.b}) {
            b->Type->Radar=true;b->HasPower=true;b->InLimbo=false;b->IsOnMap=true;
            b->DiscoveredByCurrentPlayer=true;b->CurrentMission=Mission::Guard;b->QueuedMission=Mission::None;
            b->EMPLockRemaining=0;b->BeingWarpedOut=false;
        }
        if(first==1)w.a.HasPower=false;
        if(first==2)w.a.InLimbo=true;
        if(first==3)w.a.IsOnMap=false;
        if(first==4)w.a.DiscoveredByCurrentPlayer=false;
        if(first==5)w.a.CurrentMission=Mission::Selling;
        if(first==6)w.a.QueuedMission=Mission::Selling;
        if(first==7)w.a.EMPLockRemaining=1;
        if(first==8)w.a.BeingWarpedOut=true;
        if(first==9)w.generator.Radar=false;
        auto runtime=game::default_scenario_runtime();runtime.context=&mode;
        runtime.session_mode=[](void* p)noexcept{return *static_cast<int*>(p);};
        RadarClass::Instance.IsAvailableNow=false;
        ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void* p){static_cast<HouseClass*>(p)->UpdateRadarAvailability();},&w.owner));
        EXPECT_EQ(RadarClass::Instance.IsAvailableNow,expected!=0);
        EXPECT_FALSE(w.owner.RecheckRadar);
    }),640);
}

TEST(Power, FactoryRatesAndProductionTimeMatchOriginalInstructions) {
    PowerWorld w;RulesClass rules;auto* previous=RulesClass::Instance;RulesClass::Instance=&rules;
    FactoryProbe factory;factory.Owner=&w.owner;factory.Object=&w.a;FactoryClass::Array.AddItem(&factory);
    auto cleanup=ra2::test::scope_exit([&]{FactoryClass::Array.Remove(&factory);RulesClass::Instance=previous;});
    rules.MinLowPowerProductionSpeed=0.1f;rules.MaxLowPowerProductionSpeed=0.8f;
    rules.LowPowerPenaltyModifier=0.9f;rules.MultipleFactory=0.8f;rules.WallBuildSpeedCoefficient=0.2;
    w.generator.BuildCat=BuildCat::Tech;w.owner.PowerDrain=80;
    EXPECT_EQ(rows('T',[&](auto& row){
        int cost,output,factories,wall,time,rate,start,left;std::uint64_t baseSpeed;std::uint32_t countrySpeed,multiplier;
        row>>cost>>baseSpeed>>countrySpeed>>multiplier>>output>>factories>>wall>>time>>rate>>start>>left;
        w.generator.Cost=cost;rules.BuildSpeed=std::bit_cast<double>(baseSpeed);w.country.BuildtimeBuildingsMult=std::bit_cast<float>(countrySpeed);
        w.generator.BuildTimeMultiplier=std::bit_cast<float>(multiplier);w.owner.PowerOutput=output;
        w.owner.NumConYards=factories;w.generator.Wall=wall;
        EXPECT_EQ(w.a.TimeToBuild(),time);
        factory.Production.Rate=37;factory.Production.Timer.StartTime=71;factory.Production.Timer.TimeLeft=19;
        FactoryClass::UpdateBuildSpeed(&w.owner);
        EXPECT_EQ(factory.Production.Rate,rate);EXPECT_EQ(factory.Production.Timer.StartTime,start);
        EXPECT_EQ(factory.Production.Timer.TimeLeft,left);
    }),720);
}

TEST(Power, SuperweaponPowerTransitionsMatchOriginalInstructions) {
    PowerWorld w;SuperWeaponTypeClass type("POWERSW");SuperProbe super;super.Type=&type;super.Owner=&w.owner;
    const bool active=Game::IsActive;SessionClass session{};game::ScenarioHouseServices houses;houses.session=&session;
    auto runtime=game::default_scenario_runtime();runtime.houses=&houses;
    w.owner.Supers.AddItem(&super);SuperClass::Array.AddItem(&super);
    auto cleanup=ra2::test::scope_exit([&]{w.owner.Supers.Clear();SuperClass::Array.Remove(&super);Game::IsActive=active;});
    w.generator.SuperWeapon=0;w.generator.SuperWeapon2=-1;type.AuxBuilding=nullptr;
    w.owner.PowerDrain=80;SidebarClass::Instance.ActiveTabIndex=1;Unsorted::CurrentFrame=100;
    EXPECT_EQ(rows('S',[&](auto& row){
        int bits,mode,supply;row>>bits>>mode>>supply;
        Game::IsActive=mode!=0;w.owner.Defeated=mode==2;session.Config.SWAllowed=mode!=3;
        super.IsPresent=bits&1;super.IsOneTime=bits&2;super.CanHold=bits&4;super.IsSuspended=bits&8;
        type.IsPowered=bits&16;type.ManualControl=bits&32;type.DisableableFromShell=bits&64;super.IsReady=bits&128;
        super.RechargeTimer.StartTime=10;super.RechargeTimer.TimeLeft=200;
        w.owner.PowerOutput=supply==1?40:200;w.a.HasPower=supply!=2;
        w.owner.RecheckTechTree=false;DisplayClass::Instance.CurrentSWTypeIndex=0;
        SidebarClass::Instance.SidebarNeedsRedraw=false;SidebarClass::Instance.Tabs[1].NeedsRedraw=false;
        ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void* p){static_cast<HouseClass*>(p)->UpdateSuperWeapons();},&w.owner));
        const int fields[]{super.IsPresent,super.IsReady,super.IsSuspended,super.RechargeTimer.StartTime,
            super.RechargeTimer.TimeLeft,DisplayClass::Instance.CurrentSWTypeIndex,w.owner.RecheckTechTree,
            SidebarClass::Instance.SidebarNeedsRedraw};
        int index=0;for(int actual:fields){int expected;row>>expected;EXPECT_EQ(actual,expected)<<"field "<<index++;}
    }),3072);
}

TEST(Power, ActiveSidebarBuildLimitsMatchOriginalInstructions) {
    PowerWorld w;
    RulesClass rules;auto* previous=RulesClass::Instance;RulesClass::Instance=&rules;
    AircraftTypeClass aircraft{"POWERJET"};InfantryTypeClass infantry{"POWERTHIEF"};UnitTypeClass vehicle{"POWERCAR"};
    UnitClass hijackedA{&vehicle,&w.owner},hijackedB{&vehicle,&w.owner};
    hijackedA.HijackerInfantryType=hijackedB.HijackerInfantryType=-1;
    FactoryProbe factory;factory.Owner=&w.owner;
    rules.PadAircraft.AddItem(&aircraft);
    auto cleanup=ra2::test::scope_exit([&]{
        w.owner.Primary_ForAircraft=w.owner.Primary_ForInfantry=w.owner.Primary_ForBuildings=nullptr;
        w.owner.Primary_ForVehicles=w.owner.Primary_ForShips=nullptr;
        RulesClass::Instance=previous;
    });
    EXPECT_TRUE(w.owner.HasReachedBuildLimit(nullptr));
    EXPECT_EQ(rows('Q',[&](auto& row){
        int kind,limit,owned,ever,queued,product,naval,special,extra,total,expected;
        row>>kind>>limit>>owned>>ever>>queued>>product>>naval>>special>>extra>>total>>expected;
        TechnoTypeClass* type=kind==3?static_cast<TechnoTypeClass*>(&aircraft):kind==7?static_cast<TechnoTypeClass*>(&w.generator):
            kind==16?static_cast<TechnoTypeClass*>(&infantry):static_cast<TechnoTypeClass*>(&vehicle);
        type->BuildLimit=limit;type->Naval=naval;
        aircraft.AirportBound=special&&kind==3;infantry.VehicleThief=special&&kind==16;
        CounterClass* current=kind==3?&w.owner.OwnedAircraftTypes:kind==7?&w.owner.OwnedBuildingTypes:
            kind==16?&w.owner.OwnedInfantryTypes:&w.owner.OwnedUnitTypes;
        CounterClass* history=kind==3?&w.owner.FactoryProducedAircraftTypes:kind==7?&w.owner.FactoryProducedBuildingTypes:
            kind==16?&w.owner.FactoryProducedInfantryTypes:&w.owner.FactoryProducedUnitTypes;
        const int index=type->GetArrayIndex();ASSERT_TRUE(current->EnsureItem(index));current->Items[index]=owned;
        ASSERT_TRUE(history->EnsureItem(index));history->Items[index]=ever;
        ASSERT_TRUE(w.owner.ActiveAircraftTypes.EnsureItem(aircraft.ArrayIndex));
        w.owner.ActiveAircraftTypes.Items[aircraft.ArrayIndex]=owned;
        w.owner.AirportDocks=extra;
        hijackedA.HijackerInfantryType=hijackedB.HijackerInfantryType=extra?infantry.ArrayIndex:-1;
        w.owner.Primary_ForAircraft=w.owner.Primary_ForInfantry=w.owner.Primary_ForBuildings=nullptr;
        w.owner.Primary_ForVehicles=w.owner.Primary_ForShips=nullptr;
        if(kind==3)w.owner.Primary_ForAircraft=&factory;
        else if(kind==7)w.owner.Primary_ForBuildings=&factory;
        else if(kind==16)w.owner.Primary_ForInfantry=&factory;
        else (naval?w.owner.Primary_ForShips:w.owner.Primary_ForVehicles)=&factory;
        // The original routine uses only the product's virtual type getter.
        // A real Building carries that pointer in its normal Type field.
        struct Product : BuildingClass {
            TechnoTypeClass* product;
            Product(BuildingTypeClass* building,HouseClass* owner,TechnoTypeClass* value):BuildingClass(building,owner),product(value){}
            TechnoTypeClass* GetTechnoType() const override {return product;}
        } currentProduct{&w.consumer,&w.owner,type};
        factory.Object=product?&currentProduct:nullptr;factory.QueuedObjects.Clear();
        for(int i=0;i<queued;++i)factory.QueuedObjects.AddItem(type);
        EXPECT_EQ(factory.CountTotal(type),total);
        EXPECT_EQ(w.owner.HasReachedBuildLimit(type),bool(expected));
        factory.Object=nullptr;
    }),896);
}

TEST(Power, DesiredLevelsMatchOriginalX87Instructions) {
    PowerWorld w;auto& power=PowerClass::Instance;
    EXPECT_EQ(rows('L',[&](auto& row){
        int height,nominalOutput,nominalDrain,output,drain,desired,maximum,green,yellow,red;
        row>>height>>nominalOutput>>nominalDrain>>output>>drain>>desired>>maximum>>green>>yellow>>red;
        SidebarClass::CameoHeight=height;w.generator.PowerBonus=nominalOutput;w.consumer.PowerDrain=nominalDrain;
        w.owner.PowerOutput=output;w.owner.PowerDrain=drain;
        EXPECT_EQ(power.DesiredPowerHeight(),desired);
        int actualGreen,actualYellow,actualRed;
        EXPECT_EQ(power.DesiredPowerLevels(actualGreen,actualYellow,actualRed),maximum);
        EXPECT_EQ(actualGreen,green);EXPECT_EQ(actualYellow,yellow);EXPECT_EQ(actualRed,red);
    }),1800);
}

TEST(Power, EveryUiTransitionMatchesOriginalInstructions) {
    PowerWorld w;auto& power=PowerClass::Instance;auto& sidebar=SidebarClass::Instance;
    SidebarClass::CameoHeight=251;w.generator.PowerBonus=300;
    struct State {std::uint32_t milliseconds=0;PowerClass* power;bool initialize=false;} state{0,&power,true};
    const game::ClockServices clock{&state,[](void* p)noexcept{return static_cast<State*>(p)->milliseconds;}};
    const auto update=[](void* p){auto& s=*static_cast<State*>(p);
        if(s.initialize)s.power->PowerClass::Init_Clear();
        else {const int key=0;s.power->PowerClass::Update(key,{0,0});}
    };
    ASSERT_TRUE(game::with_clock(clock,update,&state));state.initialize=false;
    EXPECT_EQ(rows('U',[&](auto& row){
        int tick,active,output,drain;row>>tick>>active>>output>>drain;
        state.milliseconds=static_cast<std::uint32_t>(tick)*16;
        Unsorted::CurrentFrame=tick*7+91;
        sidebar.IsSidebarActive=active;sidebar.SidebarNeedsRedraw=false;power.PowerNeedRedraw=false;
        w.owner.PowerOutput=output;w.owner.PowerDrain=drain;
        ASSERT_TRUE(game::with_clock(clock,update,&state));
        const int fields[]{std::bit_cast<int>(power.unknown_152C),std::bit_cast<int>(power.unknown_1530),
            std::bit_cast<int>(power.unknown_1534),std::bit_cast<int>(power.unknown_151C),power.unknown_bool_1538,
            power.PowerOutput,power.PowerDrain,power.unknown_timer_1510.StartTime,power.unknown_timer_1510.TimeLeft,
            power.unknown_timer_1520.StartTime,power.unknown_timer_1520.TimeLeft,power.PowerNeedRedraw,sidebar.SidebarNeedsRedraw};
        int index=0;for(int actual:fields){int expected;row>>expected;EXPECT_EQ(actual,expected)<<"field "<<index++;}
    }),1800);
    power.PowerClass::Init_Clear();
}

TEST(Power, BlackoutAndRadarSelection) {
    PowerWorld w;ScenarioClass scenario;auto* previous=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
    auto restore=ra2::test::scope_exit([&]{ScenarioClass::Instance=previous;});
    w.owner.PowerOutput=200;w.owner.PowerDrain=80;w.generator.Radar=w.consumer.Radar=true;
    w.a.CurrentMission=w.b.CurrentMission=Mission::Guard;w.a.QueuedMission=w.b.QueuedMission=Mission::None;
    EXPECT_EQ(w.owner.GetPowerPercentage(),1.0);
    Unsorted::CurrentFrame=100;w.owner.CreatePowerOutage(30);
    EXPECT_TRUE(w.owner.RecheckPower);EXPECT_EQ(w.owner.PowerBlackoutTimer.StartTime,100);
    EXPECT_EQ(w.owner.PowerBlackoutTimer.TimeLeft,30);
    w.owner.UpdatePower();EXPECT_EQ(w.owner.PowerOutput,0);EXPECT_EQ(w.owner.GetPowerPercentage(),0.0);
    w.owner.UpdateRadarAvailability();EXPECT_FALSE(RadarClass::Instance.IsAvailableNow);
    scenario.FreeRadar=true;w.owner.UpdateRadarAvailability();EXPECT_TRUE(RadarClass::Instance.IsAvailableNow);
    w.owner.CreateRadarOutage(20);w.owner.UpdateRadarAvailability();EXPECT_FALSE(RadarClass::Instance.IsAvailableNow);
    w.owner.RadarBlackoutTimer.Stop();scenario.FreeRadar=false;w.owner.PowerBlackoutTimer.Stop();w.owner.UpdatePower();
    w.a.EMPLockRemaining=1;w.owner.UpdateRadarAvailability();EXPECT_FALSE(RadarClass::Instance.IsAvailableNow);
    w.a.HasPower=false;w.owner.UpdateRadarAvailability();EXPECT_TRUE(RadarClass::Instance.IsAvailableNow);
    w.owner.PowerOutput=40;EXPECT_EQ(w.owner.GetPowerPercentage(),0.5);
}

TEST(Power, OriginalDrawingWithGameResources) {
    const char* data=std::getenv("RA2_GAME_DATA");if(!data)GTEST_SKIP()<<"Set RA2_GAME_DATA for original UI assets";
    game::ResourceHandle* resources=nullptr;std::string error;
    ASSERT_TRUE(game::create_resources(data,resources,error));
    ASSERT_EQ(game::load_resources(*resources,{},error),game::ResourceLoadResult::complete)<<error;
    game::MapViewHandle* view=nullptr;ASSERT_TRUE(game::create_map_view(*resources,view));
    auto cleanup=ra2::test::scope_exit([&]{game::destroy_map_view(view);FileSystem::ClearNameCache();Destroy_All_Shapes();Unload_All_Shapes();game::destroy_resources(resources);});
    ASSERT_TRUE(game::load_map_view(*view,"ALL01UMD.MAP",12))<<game::map_view_error(*view);
    ASSERT_TRUE(game::with_map_view(*view,[](void*){
        EXPECT_TRUE(ScenarioClass::Instance->FreeRadar); // Real ALL01UMD.MAP [Basic].
        EXPECT_TRUE(RadarClass::Instance.IsAvailableNow);
    },nullptr));
    ASSERT_TRUE(game::set_game_view_size(*view,1280,720));
    ASSERT_TRUE(SidebarClass::Instance.IsSidebarActive);
    struct State {game::MapViewHandle* view;} state{view};
    ASSERT_TRUE(game::with_map_view(*view,[](void* p){auto& v=*static_cast<State*>(p)->view;
        game::UiResources resources;ASSERT_TRUE(resources.load(0));
        std::vector<game::ShapeDrawingRequest> draws;
        game::MapDrawingContext drawing;game::MapDrawStatistics statistics;
        drawing.types.backend_context=&draws;drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(&draws);
        drawing.types.backend.shape=[](void* p,const game::ShapeDrawingRequest& request){static_cast<std::vector<game::ShapeDrawingRequest>*>(p)->push_back(request);return game::DrawingStatus::drawn;};
        drawing.plain_palette=[](void*,const BytePalette& palette,const game::DrawingPaletteHandle*& out)noexcept{out=reinterpret_cast<const game::DrawingPaletteHandle*>(&palette);return game::DrawingStatus::drawn;};
        SidebarClass::CameoHeight=50;
        DSurface::SidebarBounds={600,165,168,600};DSurface::WindowBounds={0,0,800,800};
        EXPECT_EQ(rows('D',[&](auto& row){
            int side,green,yellow,red,flash,count;row>>side>>green>>yellow>>red>>flash>>count;
            v.scenario.PlayerSideIndex=side;auto& power=PowerClass::Instance;
            power.unknown_152C=green;power.unknown_1530=yellow;power.unknown_1534=red;power.unknown_151C=flash;
            draws.clear();game::GameUiFrame frame{drawing,resources,statistics};
            EXPECT_EQ(game::with_game_ui_frame(frame,[]{PowerClass::Instance.PowerClass::Draw(1);}),game::DrawingStatus::drawn);
            ASSERT_EQ(draws.size(),static_cast<std::size_t>(count));
            for(const auto& draw:draws){int index,x,y;row>>index>>x>>y;EXPECT_EQ(draw.frame,index);EXPECT_EQ(draw.position,(Point2D{x+600,y}));}
        }),64);
    },&state));
}

TEST(Power, MapFreeRadarSurvivesPowerChangesAndResetsOnReload) {
    const auto root=std::filesystem::temp_directory_path()/
        ("ra2-free-radar-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);power_map_fixture(root);
    std::ifstream input(root/"world.map");const std::string original((std::istreambuf_iterator<char>(input)),{});
    const auto basic=original.find("[Basic]\n");ASSERT_NE(basic,std::string::npos);
    for(const auto& entry:std::array<std::pair<const char*,const char*>,3>{{
        {"free.map","FreeRadar=yes\n"},{"paid.map","FreeRadar=no\n"},{"default.map",""}}}) {
        auto text=original;text.insert(basic+8,std::string("Player=Neutral\n")+entry.second);
        std::ofstream(root/entry.first)<<text;
    }
    std::ofstream(root/"RULESMD.INI",std::ios::app)<<"\n[BLDG]\nPower=-80\nRadar=no\n";
    game::ResourceHandle* resources=nullptr;std::string error;game::MapViewHandle* view=nullptr;
    auto cleanup=ra2::test::scope_exit([&]{game::destroy_map_view(view);FileSystem::ClearNameCache();Destroy_All_Shapes();Unload_All_Shapes();
        game::destroy_resources(resources);std::error_code ec;std::filesystem::remove_all(root,ec);});
    ASSERT_TRUE(game::create_resources(root.string(),resources,error));ASSERT_TRUE(game::create_map_view(*resources,view));
    const auto load=[&](const char* name){return game::load_map_view(*view,name,std::strlen(name));};
    ASSERT_TRUE(load("free.map"));ASSERT_TRUE(game::set_game_view_size(*view,1280,720));
    ASSERT_TRUE(game::with_map_view(*view,[](void*){
        ASSERT_TRUE(HouseClass::CurrentPlayer);EXPECT_TRUE(ScenarioClass::Instance->FreeRadar);
        ASSERT_EQ(HouseClass::CurrentPlayer->Buildings.Count,1);
        const auto* building=HouseClass::CurrentPlayer->Buildings[0];
        ASSERT_TRUE(building->IsInPlayfield);ASSERT_TRUE(building->DiscoveredByCurrentPlayer);
        EXPECT_EQ(HouseClass::CurrentPlayer->PowerOutput,0);EXPECT_EQ(HouseClass::CurrentPlayer->PowerDrain,80);
        EXPECT_TRUE(RadarClass::Instance.IsAvailableNow);
        HouseClass::CurrentPlayer->CreatePowerOutage(4);
    },nullptr));
    for(int i=0;i<6;++i) {
        ASSERT_TRUE(game::update_game_view(*view,1.0/15.0));
        EXPECT_TRUE(RadarClass::Instance.IsAvailableNow);
    }
    // Turning the only consumer off restores full power. The free radar
    // remains active through both branches of the House power transition.
    ASSERT_TRUE(game::with_map_view(*view,[](void*){
        auto* player=HouseClass::CurrentPlayer;
        for(auto* building:player->Buildings)building->HasPower=false;
        player->RecheckPower=true;
    },nullptr));
    ASSERT_TRUE(game::update_game_view(*view,1.0/15.0));
    EXPECT_TRUE(RadarClass::Instance.IsAvailableNow);
    ASSERT_TRUE(game::with_map_view(*view,[](void*){
        EXPECT_EQ(HouseClass::CurrentPlayer->GetPowerPercentage(),1.0);
        HouseClass::CurrentPlayer->CreateRadarOutage(4);
    },nullptr));
    ASSERT_TRUE(game::update_game_view(*view,1.0/15.0));
    EXPECT_FALSE(RadarClass::Instance.IsAvailableNow); // Radar blackout still takes priority.
    for(int i=0;i<5;++i)ASSERT_TRUE(game::update_game_view(*view,1.0/15.0));
    EXPECT_TRUE(RadarClass::Instance.IsAvailableNow);
    for(const char* name:{"paid.map","free.map","default.map"}) {
        ASSERT_TRUE(load(name));
        bool free=std::strcmp(name,"free.map")==0;
        ASSERT_TRUE(game::with_map_view(*view,[](void* p){
            EXPECT_EQ(ScenarioClass::Instance->FreeRadar,*static_cast<const bool*>(p));
        },&free));
        EXPECT_EQ(RadarClass::Instance.IsAvailableNow,free);
    }
}

TEST(Power, MapDamageShutdownBlackoutAndReloadDriveRealPower) {
    const auto root=std::filesystem::temp_directory_path()/
        ("ra2-power-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);power_map_fixture(root);
    const auto insert=[&](const char* file,const char* anchor,const char* added) {
        std::ifstream input(root/file);std::string text((std::istreambuf_iterator<char>(input)),{});
        const auto at=text.find(anchor);ASSERT_NE(at,std::string::npos);
        text.insert(at+std::strlen(anchor),added);std::ofstream(root/file)<<text;
    };
    insert("RULESMD.INI","[BuildingTypes]\n","2=LOAD\n");
    insert("world.map","[Basic]\n","Player=Neutral\n");
    insert("world.map","[Structures]\n","1=Neutral,LOAD,256,8,6,0,None,1,0,1,0,0,None,None,None,0,0\n");
    std::ofstream(root/"RULESMD.INI",std::ios::app)<<
        "\n[BLDG]\nPower=200\n"
        "[LOAD]\nStrength=100\nPower=-80\nPowered=yes\nRadar=yes\nTogglePower=yes\n"
        "ExtraPower=25\nUnitAbsorb=yes\nInfantryAbsorb=yes\n";
    std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[LOAD]\nFoundation=1x1\nNewTheater=no\n";
    map_fixture::shp(root/"LOAD.SHP",2);
    game::ResourceHandle* resources=nullptr;std::string error;game::MapViewHandle* view=nullptr;
    auto cleanup=ra2::test::scope_exit([&]{game::destroy_map_view(view);FileSystem::ClearNameCache();Destroy_All_Shapes();Unload_All_Shapes();
        game::destroy_resources(resources);std::error_code ec;std::filesystem::remove_all(root,ec);});
    ASSERT_TRUE(game::create_resources(root.string(),resources,error));ASSERT_TRUE(game::create_map_view(*resources,view));
    ASSERT_TRUE(game::load_map_view(*view,"world.map",9))<<game::map_view_error(*view);
    ASSERT_TRUE(game::set_game_view_size(*view,1280,720));
    ASSERT_TRUE(SidebarClass::Instance.IsSidebarActive);
    // Do not seed the UI activation flag: production startup owns it. Also
    // verify that the ordinary loop reaches Power.Update before damage.
    ASSERT_TRUE(game::update_game_view(*view,1.0/15.0));
    EXPECT_EQ(PowerClass::Instance.PowerOutput,80);
    EXPECT_EQ(PowerClass::Instance.PowerDrain,100);
    EXPECT_GT(PowerClass::Instance.unknown_152C+PowerClass::Instance.unknown_1530+
        PowerClass::Instance.unknown_1534,0u);
    struct State {BuildingClass* generator=nullptr;BuildingClass* consumer=nullptr;} state;
    ASSERT_TRUE(game::with_map_view(*view,[](void* p){auto& s=*static_cast<State*>(p);
        for(auto* building:HouseClass::CurrentPlayer->Buildings)
            if(building->Type->PowerBonus>0)s.generator=building;else s.consumer=building;
        ASSERT_NE(s.generator,nullptr);ASSERT_NE(s.consumer,nullptr);
        ASSERT_TRUE(s.generator->IsInPlayfield);ASSERT_TRUE(s.generator->DiscoveredByCurrentPlayer);
        ASSERT_TRUE(s.consumer->IsInPlayfield);ASSERT_TRUE(s.consumer->DiscoveredByCurrentPlayer);
        EXPECT_EQ(HouseClass::CurrentPlayer->PowerOutput,100);EXPECT_EQ(HouseClass::CurrentPlayer->PowerDrain,80);
        EXPECT_TRUE(s.consumer->IsPowerOnline());EXPECT_TRUE(RadarClass::Instance.IsAvailableNow);
        EXPECT_TRUE(s.consumer->Type->TogglePower);EXPECT_TRUE(s.consumer->Type->UnitAbsorb);
        EXPECT_TRUE(s.consumer->Type->InfantryAbsorb);EXPECT_EQ(s.consumer->Type->ExtraPowerBonus,25);
        WarheadTypeClass warhead("POWERHIT");int damage=40;
        s.generator->ReceiveDamage(&damage,0,&warhead,nullptr,true,true,nullptr);
        EXPECT_EQ(s.generator->Health,10);
    },&state));
    const auto tick=[&]{ASSERT_TRUE(game::update_game_view(*view,1.0/15.0));};
    tick();tick();
    ASSERT_TRUE(game::with_map_view(*view,[](void* p){auto& s=*static_cast<State*>(p);
        EXPECT_EQ(HouseClass::CurrentPlayer->PowerOutput,s.generator->GetPowerOutput());
        EXPECT_LT(HouseClass::CurrentPlayer->PowerOutput,80);
        EXPECT_FALSE(s.consumer->IsPowerOnline());EXPECT_FALSE(RadarClass::Instance.IsAvailableNow);
        s.generator->Health=50; // Repair also goes through the original cached-health notification.
    },&state));
    tick();tick();
    ASSERT_TRUE(game::with_map_view(*view,[](void* p){auto& s=*static_cast<State*>(p);
        EXPECT_EQ(HouseClass::CurrentPlayer->PowerOutput,100);EXPECT_TRUE(s.consumer->IsPowerOnline());
        EXPECT_TRUE(RadarClass::Instance.IsAvailableNow);HouseClass::CurrentPlayer->CreatePowerOutage(4);
    },&state));
    tick();
    ASSERT_TRUE(game::with_map_view(*view,[](void*){EXPECT_EQ(HouseClass::CurrentPlayer->PowerOutput,0);},nullptr));
    for(int i=0;i<5;++i)tick();
    ASSERT_TRUE(game::with_map_view(*view,[](void*){
        EXPECT_EQ(HouseClass::CurrentPlayer->PowerOutput,100);EXPECT_EQ(HouseClass::CurrentPlayer->PowerBlackoutTimer.GetTimeLeft(),0);
        EXPECT_TRUE(RadarClass::Instance.IsAvailableNow);
    },nullptr));
    std::uint32_t count=0;ASSERT_TRUE(game::copy_map_objects(*view,nullptr,0,count));
    std::vector<game::MapObjectSnapshot> objects(count);ASSERT_TRUE(game::copy_map_objects(*view,objects.data(),count,count));
    for(const auto& object:objects)if(std::strcmp(object.type_id,"BLDG")==0) {
        ASSERT_TRUE(game::set_map_building_enabled(*view,object.id,false));
    }
    tick();
    ASSERT_TRUE(game::with_map_view(*view,[](void*){EXPECT_EQ(HouseClass::CurrentPlayer->PowerOutput,0);},nullptr));
    ASSERT_TRUE(game::load_map_view(*view,"world.map",9));
    EXPECT_FALSE(SidebarClass::Instance.IsSidebarActive);
    ASSERT_TRUE(game::with_map_view(*view,[](void*){
        EXPECT_EQ(HouseClass::CurrentPlayer->PowerOutput,100);EXPECT_EQ(HouseClass::CurrentPlayer->PowerDrain,80);
        EXPECT_EQ(PowerClass::Instance.unknown_152C,0u);EXPECT_EQ(PowerClass::Instance.unknown_1534,0u);
        EXPECT_EQ(PowerClass::Instance.PowerOutput,-1);EXPECT_EQ(PowerClass::Instance.PowerDrain,-1);
    },nullptr));
    ASSERT_TRUE(game::set_game_view_size(*view,1600,900));
    EXPECT_TRUE(SidebarClass::Instance.IsSidebarActive);
    ASSERT_TRUE(game::update_game_view(*view,1.0/15.0));
    EXPECT_EQ(PowerClass::Instance.PowerDrain,100);
    game::destroy_map_view(view);view=nullptr;
    EXPECT_FALSE(SidebarClass::Instance.IsSidebarActive);
}
