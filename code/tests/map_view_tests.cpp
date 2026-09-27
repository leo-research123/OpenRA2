#include "support/test_support.hpp"
#include "api/filesystem.hpp"
#include "api/clock.hpp"
#include "map_view.hpp"
#include "scenario_runtime.hpp"
#include "yrpp/CCFileClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/Unsorted.h"
#include <array>
#include <bit>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

struct Session {
    game::MapViewHandle* view = nullptr;
    ~Session() { game::destroy_map_view(view); }
};
void identity(AbstractClass& object) {
    void* output = nullptr;
    for (DWORD first : {0u,0x109u,0x10cu}) {
        DWORD words[]{first,0,0xc0,0x46000000}; IID iid; std::memcpy(&iid,words,sizeof(iid));
        EXPECT_TRUE((object.QueryInterface(iid,&output)==0 && output==static_cast<IPersistStream*>(&object))) << "original COM identity through actual virtual dispatch";
    }
    DWORD words[]{0x170dac82,0x11d212e4,0x60007581,0xb55b0508}; IID iid; std::memcpy(&iid,words,sizeof(iid));
    EXPECT_TRUE((object.QueryInterface(iid,&output)==0 && output==static_cast<IRTTITypeInfo*>(&object))) << "adjusted RTTI pointer";
    EXPECT_TRUE((static_cast<IRTTITypeInfo*>(output)->What_Am_I()==object.WhatAmI())) << "RTTI dispatch";
    words[0]=42; std::memcpy(&iid,words,sizeof(iid));
    EXPECT_TRUE((object.QueryInterface(iid,&output)==static_cast<HRESULT>(0x80004002u) && !output)) << "failed query clears output";
    EXPECT_TRUE((object.QueryInterface(iid,nullptr)==static_cast<HRESULT>(0x80004003u))) << "query rejects missing output";
}
TEST(MapView, Contracts) {
    const auto root = std::filesystem::temp_directory_path() /
        ("ra2-map-view-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(root);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code ec; std::filesystem::remove_all(path,ec); } } cleanup{root};
    std::ofstream(root/"map-scope.txt") << "world";
    game::ResourceHandle* resource=nullptr; std::string error;
    EXPECT_TRUE((game::create_resources(root.string(),resource,error))) << error.c_str();
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(resource,game::destroy_resources);
    auto* map=&MouseClass::Instance;
    Session session;
    EXPECT_TRUE((!ScenarioClass::Instance && !TacticalClass::Instance)) << "no external world at startup";
    EXPECT_TRUE((game::create_map_view(*files,session.view))) << "create map session";
    EXPECT_TRUE((TacticalClass::Instance==&session.view->tactical && !ScenarioClass::Instance)) << "normal Tactical lifetime and scoped Scenario";
    game::MapViewHandle* duplicate=nullptr;
    EXPECT_TRUE((!game::create_map_view(*files,duplicate) && !duplicate)) << "reject overlapping session ownership";
    EXPECT_TRUE((!game::create_map_view(*files,session.view))) << "reject replacement of a live output";
    game::MapViewInfo info{};
    EXPECT_TRUE((game::get_map_view_info(*session.view,info) && info.state==game::MapViewState::empty && !info.generation)) << "new session has no published cells";
    identity(session.view->tactical);
    // check_tactical_startup.py executes the actual nonzero-angle constructor.
    constexpr std::array<std::uint32_t,12> matrix{
        0x3defffff,0xbdefffff,0,0,0x3d701b32,0x3d701b32,0xbe12f29b,0,
        0x3dcfd0bd,0x3dcfd0bd,0x3da9c7e0,0};
    for (std::size_t i=0;i<matrix.size();++i)
        EXPECT_TRUE((std::bit_cast<std::uint32_t>(session.view->tactical.Unused_Matrix3D.Data[i])==matrix[i])) << "native Tactical matrix matches original float stores";
    const auto* old_runtime=&game::scenario_runtime();
    EXPECT_TRUE((game::with_map_view(*session.view,[](void* pointer) {
        auto& view=*static_cast<game::MapViewHandle*>(pointer);
        EXPECT_TRUE((ScenarioClass::Instance==&view.scenario && game::scenario_runtime().context==&view)) << "actual scoped services";
        CCFileClass file("map-scope.txt"); char buffer[5]{};
        EXPECT_TRUE((file.ReadBytes(buffer,5)==5 && !std::memcmp(buffer,"world",5))) << "resource environment follows the session";
        EXPECT_TRUE((!game::initialize_empty_map_view(view,{0,0,8,12},0))) << "reentrant reload rejected";
        auto* alias=&view; game::destroy_map_view(alias);
        EXPECT_TRUE((alias==&view && game::map_view_error(view)[0])) << "reentrant destruction leaves owner live";
    },session.view))) << "scoped operation";
    EXPECT_TRUE((!ScenarioClass::Instance && &game::scenario_runtime()==old_runtime)) << "scope restored after operation";
    for (bool standard : {false,true}) {
        EXPECT_TRUE((!game::with_map_view(*session.view,[](void* pointer) {
            if (*static_cast<bool*>(pointer)) throw std::runtime_error("map callback failure");
            throw 7;
        },&standard))) << "callback exception contained";
        EXPECT_TRUE((!ScenarioClass::Instance && &game::scenario_runtime()==old_runtime && game::map_view_error(*session.view)[0])) << "exception restores original globals and records an error";
    }
    for (int i=0;i<12;++i) {
        auto& beacons=BeaconManagerClass::Instance;
        ASSERT_EQ(beacons.AllocatedCount,0);
        beacons.Beacons[0][0]=GameCreate<BeaconClass>();beacons.AllocatedCount=1;
        auto& messages=MessageListClass::Instance;
        ASSERT_EQ(messages.NumMessages(),0);
        messages.MessageList=new TextLabelClass(messages.MessageBuffers[0],0,0,0,TextPrintType(0));
        messages.BufferAvail[0]=0;
        ASSERT_TRUE(game::advance_clock(0.016));
        auto time_before=game::clock_milliseconds();
        const int tick_start=Game::TickCount.StartTime,tick_elapsed=Game::TickCount.GetTimeElapsed();
        const int radar_start=map->unknown_timer_1500.StartTime;
        const int radar_left=map->unknown_timer_1500.TimeLeft;
        EXPECT_TRUE((game::initialize_empty_map_view(*session.view,{0,0,8+i,12},static_cast<char>(i%4)))) << "reload session cells";
        EXPECT_EQ(beacons.AllocatedCount,0);EXPECT_EQ(beacons.Beacons[0][0],nullptr);
        EXPECT_EQ(messages.NumMessages(),0);EXPECT_EQ(messages.BufferAvail[0],1);
        EXPECT_EQ(Game::TickCount.StartTime,tick_start);EXPECT_EQ(Game::TickCount.GetTimeElapsed(),tick_elapsed);
        EXPECT_EQ(game::clock_milliseconds(),time_before) << "reload cannot reset the authority";
        EXPECT_EQ(map->unknown_timer_1500.StartTime,radar_start) << "original Radar Init_Clear retains timer";
        EXPECT_EQ(map->unknown_timer_1500.TimeLeft,radar_left);
        EXPECT_EQ(map->unknown_14AC,0u);
        EXPECT_EQ(map->unknown_14FC,0u);
        EXPECT_FALSE(map->IsAvailableNow);
        EXPECT_TRUE(game::with_map_view(*session.view,[](void* p){
            EXPECT_EQ(game::clock_milliseconds(),*static_cast<const std::uint32_t*>(p));
            EXPECT_FALSE(game::advance_clock(1));
        },&time_before));
        EXPECT_TRUE((game::get_map_view_info(*session.view,info) && info.state==game::MapViewState::ready &&
            info.generation==static_cast<unsigned>(i+1) && info.width==8+i && info.height==12 && info.slot_capacity==262144)) << "only completed geometry is published";
        EXPECT_TRUE((&MouseClass::Instance==map && !ScenarioClass::Instance && TacticalClass::Instance==&session.view->tactical)) << "root identity survives reload and Scenario binding does not leak";
        identity(*map->TryGetCellAt(CellStruct{static_cast<short>(8+i),1}));
        EXPECT_TRUE((session.view->scenario.UniqueID==1000000+(2*(8+i)-1)*12+1)) << "reload resets Scenario identities";
    }
    EXPECT_TRUE((!game::initialize_empty_map_view(*session.view,{0,0,1,12},0))) << "invalid reload rejected";
    EXPECT_TRUE((game::get_map_view_info(*session.view,info) && info.state==game::MapViewState::failed && !info.width &&
        !map->Cells.Items && game::map_view_error(*session.view)[0])) << "failed reload unpublishes and frees previous map";
    const auto before_close=game::clock_milliseconds();
    BeaconManagerClass::Instance.Beacons[0][0]=GameCreate<BeaconClass>();BeaconManagerClass::Instance.AllocatedCount=1;
    game::destroy_map_view(session.view); game::destroy_map_view(session.view);
    EXPECT_EQ(BeaconManagerClass::Instance.AllocatedCount,0);
    EXPECT_EQ(BeaconManagerClass::Instance.Beacons[0][0],nullptr);
    EXPECT_EQ(game::clock_milliseconds(),before_close);
    EXPECT_TRUE((!session.view && !TacticalClass::Instance && !ScenarioClass::Instance && !map->Cells.Items)) << "repeated close";
    EXPECT_TRUE((game::create_map_view(*files,session.view))) << "reopen using same resource owner";
    EXPECT_EQ(game::clock_milliseconds(),before_close);
    // Cells and the first 1764-byte navigation buffer fit. The following
    // 4410-byte buffer fails, exercising cleanup after substantial allocation.
    EXPECT_TRUE((YRMemory::ConfigureFailureRecovery(nullptr,nullptr,4096))) << "enable late allocation failure fixture";
    EXPECT_TRUE((!game::initialize_empty_map_view(*session.view,{0,0,8,12},0) && !map->Cells.Items &&
        !map->LevelAndPassability && !map->LevelAndPassabilityStruct2pointer_70 &&
        session.view->scenario.UniqueID==1000180)) << "late allocation failure releases cells and the first navigation buffer";
}
}
