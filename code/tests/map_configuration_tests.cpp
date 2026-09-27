#include "map_world_fixture.hpp"
#include "map_world_internal.hpp"
#include "yrpp/HouseClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/Powerups.h"
#include <algorithm>

TEST(MapConfiguration, RetainsUnknownFieldsParsesDefinitionsAndAppliesExistingFeatures) {
    const auto root=std::filesystem::temp_directory_path()/
        ("ra2-config-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);map_fixture::fixtures(root);
    std::ofstream(root/"RULESMD.INI",std::ios::app)
        <<"\n[General]\nMultipleFactory=0.7\nMinLowPowerProductionSpeed=0.4\nMaxLowPowerProductionSpeed=0.8\n"
          "[AudioVisual]\nRadarOn=DeferredSound\n[BLDG]\nPower=-80\n"
          "[AircraftTypes]\n0=TESTAIR\n";
    std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[Movies]\n0=INTROFILE\n";
    std::ifstream original(root/"world.map");
    const std::string plain((std::istreambuf_iterator<char>(original)),{});
    auto configured=plain;
    configured.insert(configured.find("[Basic]\n")+8,
        "Player=Neutral\nFreeRadar=yes\nHomeCell=14\nAltHomeCell=14\nNextScenario=NEXT.MAP\nIntro=INTROFILE\n"
        "TiberiumDeathToVisceroid=no\nSkipScore=yes\nUnknownFutureField=kept literally\n");
    configured += "\n[General]\nMultipleFactory=0.8\nMinLowPowerProductionSpeed=1\nMaxLowPowerProductionSpeed=1\n"
        "[SpecialFlags]\nMCVDeploy=no\nTiberiumGrows=no\n[VariableNames]\n3=Ready,1\n"
        "[Ranking]\nParTimeEasy=00:30:00\nParTimeMedium=00:20:00\nParTimeHard=00:15:00\n"
        "[Waypoints]\n14=3008\n[Triggers]\nPENDING=unimplemented,record,kept\n"
        "[ScriptTypes]\n0=PASSIVE\n[PASSIVE]\nName=Passive definition\n0=0,12\n1=6,7\n"
        "[TaskForces]\n0=TASK\n[TASK]\nName=Passive task\n0=2,TESTAIR\nGroup=3\n"
        "[Powerups]\nMoney=123,NONE,no,500\n[Guard]\nRate=0.25\n";
    std::ofstream(root/"configured.map")<<configured;
    auto broken=configured;
    broken.replace(broken.find("0=0,12"),6,"0=invalid");
    std::ofstream(root/"broken.map")<<broken;
    auto invalid=configured;
    const std::string size="Size=0,0,8,12";
    invalid.replace(invalid.find(size),size.size(),"Size=0,0,4,4");
    std::ofstream(root/"invalid.map")<<invalid;
    const auto scripts=ScriptTypeClass::Array.Count,tasks=TaskForceClass::Array.Count;
    const auto aircraft=AircraftTypeClass::Array.Count;
    const auto movie_names=MovieInfo::Array.Count;
    const auto powerup_weight=Powerups::Weights[0];
    const auto guard_rate=MissionControlClass::Find("Guard")->Rate;
    const auto live_teams=TeamClass::Array.Count;
    game::ResourceHandle* resources=nullptr;game::MapViewHandle* view=nullptr;std::string error;
    auto cleanup=ra2::test::scope_exit([&]{game::destroy_map_view(view);
        FileSystem::ClearNameCache();Destroy_All_Shapes();Unload_All_Shapes();game::destroy_resources(resources);
        std::error_code ec;std::filesystem::remove_all(root,ec);});
    ASSERT_TRUE(game::create_resources(root.string(),resources,error));
    ASSERT_TRUE(game::create_map_view(*resources,view));
    ASSERT_TRUE(game::load_map_view(*view,"configured.map",14))<<game::map_view_error(*view);
    ASSERT_TRUE(view->world);auto& world=*view->world;
    EXPECT_TRUE(world.impl->scenario_fields_parsed);
    EXPECT_TRUE(world.impl->script_fields_parsed);EXPECT_TRUE(world.impl->task_force_fields_parsed);
    char value[128]{};
    world.map_ini.ReadString("Basic","UnknownFutureField","",value,sizeof(value));
    EXPECT_STREQ(value,"kept literally");
    world.map_ini.ReadString("Triggers","PENDING","",value,sizeof(value));
    EXPECT_STREQ(value,"unimplemented,record,kept");
    EXPECT_EQ(view->scenario.HomeCell,14);EXPECT_EQ(view->scenario.AltHomeCell,14);
    EXPECT_STREQ(view->scenario.NextScenario,"NEXT.MAP");
    EXPECT_EQ(view->scenario.Intro,MovieInfo::FindIndex("INTROFILE"));
    EXPECT_GE(view->scenario.Intro,0); // Resolving a movie name never plays it.
    EXPECT_FALSE(view->scenario.TiberiumDeathToVisceroid);EXPECT_TRUE(view->scenario.SkipScore);
    EXPECT_FALSE(view->scenario.SpecialFlags.MCVDeploy);
    EXPECT_EQ(view->scenario.LocalVariables[3].Value,1);
    EXPECT_EQ(view->scenario.ParTimeEasy,30*60*60);
    EXPECT_EQ(view->scenario.ParTimeMedium,20*60*60);EXPECT_EQ(view->scenario.ParTimeDifficult,15*60*60);
    ASSERT_TRUE(view->rules);
    EXPECT_FLOAT_EQ(view->rules->MultipleFactory,0.8f);
    EXPECT_FLOAT_EQ(view->rules->MinLowPowerProductionSpeed,1.0f);
    EXPECT_FLOAT_EQ(view->rules->MaxLowPowerProductionSpeed,1.0f);
    EXPECT_NE(std::find(world.impl->deferred_sound_fields.begin(),world.impl->deferred_sound_fields.end(),
        std::pair<std::string,std::string>{"AudioVisual","RadarOn"}),world.impl->deferred_sound_fields.end());
    EXPECT_EQ(ScriptTypeClass::Array.Count,scripts+1);EXPECT_EQ(TaskForceClass::Array.Count,tasks+1);
    ASSERT_TRUE(ScriptTypeClass::Find("PASSIVE"));
    EXPECT_EQ(ScriptTypeClass::Find("PASSIVE")->ActionsCount,2);
    ASSERT_TRUE(TaskForceClass::Find("TASK"));EXPECT_EQ(TaskForceClass::Find("TASK")->CountEntries,1);
    EXPECT_EQ(TaskForceClass::Find("TASK")->Entries[0].Amount,2);
    EXPECT_EQ(Powerups::Weights[0],123);EXPECT_DOUBLE_EQ(MissionControlClass::Find("Guard")->Rate,0.25);
    EXPECT_EQ(TeamClass::Array.Count,live_teams);
    ASSERT_TRUE(game::set_game_view_size(*view,1280,720));
    ASSERT_TRUE(game::update_game_view(*view,1.0/15.0));
    EXPECT_TRUE(RadarClass::Instance.IsAvailableNow);
    EXPECT_EQ(TeamClass::Array.Count,live_teams);
    // 0x00691970 ignores the per-type LoadFromINI return. The native reader
    // rejects the malformed action without inventing one; that does not make
    // the original list entry point fail the entire map load.
    ASSERT_TRUE(game::load_map_view(*view,"broken.map",10))<<game::map_view_error(*view);
    auto* broken_script=ScriptTypeClass::Find("PASSIVE");ASSERT_NE(broken_script,nullptr);
    EXPECT_EQ(broken_script->ActionsCount,0);
    EXPECT_FALSE(broken_script->LoadFromINI(&view->world->map_ini));
    EXPECT_EQ(broken_script->ActionsCount,0);
    EXPECT_EQ(TeamClass::Array.Count,live_teams);
    // Keep rollback coverage using an actual map load failure: the map is
    // too small to contain a usable rectangle after the original border inset.
    EXPECT_FALSE(game::load_map_view(*view,"invalid.map",11));
    EXPECT_FALSE(view->world); // Failed loading releases partial definitions/tables.
    EXPECT_EQ(ScriptTypeClass::Array.Count,scripts);EXPECT_EQ(TaskForceClass::Array.Count,tasks);
    EXPECT_EQ(Powerups::Weights[0],powerup_weight);
    EXPECT_DOUBLE_EQ(MissionControlClass::Find("Guard")->Rate,guard_rate);
    ASSERT_TRUE(game::load_map_view(*view,"world.map",9))<<game::map_view_error(*view);
    EXPECT_EQ(view->scenario.HomeCell,699);EXPECT_FALSE(view->scenario.FreeRadar);
    EXPECT_FALSE(view->world->map_ini.GetSection("Triggers"));
    EXPECT_EQ(ScriptTypeClass::Array.Count,scripts);EXPECT_EQ(TaskForceClass::Array.Count,tasks);
    EXPECT_EQ(Powerups::Weights[0],powerup_weight);
    EXPECT_DOUBLE_EQ(MissionControlClass::Find("Guard")->Rate,guard_rate);
    game::destroy_map_view(view);view=nullptr;
    EXPECT_EQ(AircraftTypeClass::Array.Count,aircraft);
    EXPECT_EQ(MovieInfo::Array.Count,movie_names);
}
