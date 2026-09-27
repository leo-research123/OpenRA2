#include "support/test_support.hpp"
#include "yrpp/YRPPCore.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/CampaignClass.h"
#include "yrpp/Surface.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/WinModemClass.h"
#include "api/scenario_runtime.hpp"
#include "api/ini_runtime.hpp"
#include "api/clock.hpp"
#include <algorithm>
#include <climits>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct CampaignFixture : CampaignClass {
    CampaignFixture() : CampaignClass(noinit_t{}) { idxCD = 2; std::strcpy(Scenario, "Campaign.map"); }
    HRESULT YRPP_STDCALL GetClassID(CLSID*) override { throw std::runtime_error("unexpected campaign COM query"); }
    HRESULT YRPP_STDCALL Load(IStream*) override { throw std::runtime_error("unexpected campaign load"); }
    HRESULT YRPP_STDCALL Save(IStream*, BOOL) override { throw std::runtime_error("unexpected campaign save"); }
    AbstractType WhatAmI() const override { return AbstractType::Campaign; }
    int Size() const override { return sizeof(*this); }
};
struct MemoryFile : FileClass {
    std::string bytes = "[Basic]\nIntro=opening\nBrief=brief\n";
    size_t position = 0;
    bool open = false, exists = true;
    int closes = 0;
    const char* GetFileName() const override { return "fixture.map"; }
    const char* SetFileName(const char*) override { throw std::runtime_error("unexpected rename"); }
    BOOL CreateFile() override { throw std::runtime_error("unexpected create"); }
    BOOL DeleteFile() override { throw std::runtime_error("unexpected delete"); }
    bool Exists(bool) override { return exists; }
    bool HasHandle() override { return open; }
    bool Open(FileAccessMode access) override {
        EXPECT_TRUE((access == FileAccessMode::Read)) << "preflight file is read-only";
        position = 0; return open = exists;
    }
    bool OpenEx(const char*, FileAccessMode) override { throw std::runtime_error("unexpected openex"); }
    int ReadBytes(void* data, int count) override {
        EXPECT_TRUE((open && count >= 0)) << "valid file read";
        const auto take = std::min(size_t(count), bytes.size() - position);
        std::memcpy(data, bytes.data() + position, take); position += take;
        return int(take);
    }
    int Seek(int, FileSeekMode) override { throw std::runtime_error("unexpected seek"); }
    int GetFileSize() override { return int(bytes.size()); }
    int WriteBytes(void*, int) override { throw std::runtime_error("unexpected write"); }
    void Close() override { ++closes; open = false; }
    void CDCheck(DWORD, bool, const char*) override { throw std::runtime_error("unexpected file CD check"); }
};
struct SurfaceFixture : Surface {
    std::vector<std::string>& events;
    explicit SurfaceFixture(std::vector<std::string>& events) : events(events) {}
    bool Fill(COLORREF color) override { EXPECT_TRUE((color == 0)) << "clear hidden surface to black"; events.emplace_back("fill"); return true; }
};
struct Fixture {
    std::vector<std::string> events;
    SessionClass session{};
    CampaignFixture campaign;
    DynamicVectorClass<CampaignClass*> campaigns;
    MemoryFile file;
    SurfaceFixture surface{events};
    Surface* hidden = &surface;
    ScenarioClass* scenario = nullptr;
    unsigned char alternate = 0;
    int mode = 0, depth = 3, clock_calls = 0, loaded_theme = 15, dropships = 1;
    bool force = true, load = true, started = false, active = false, has_disc = true;
    bool lease_available = true, mode_changes = false;
    bool world_controller = false;
    Point2D current{640,480}, preferred{800,600};
    std::string opened_name, loaded_name;
    game::ScenarioStartServices start;
    game::ScenarioRuntimeServices runtime;
    static Fixture& self(void* p) { return *static_cast<Fixture*>(p); }
    Fixture() {
        session.Config.ScenarioIndex = -1;
        campaigns.AddItem(&campaign);
        start = {
            .context = this, .session = &session, .campaigns = &campaigns,
            .alternate_campaign = &alternate, .alternate_filename = "Alternate.map",
            .media_check_depth = &depth, .current_resolution = &current, .preferred_resolution = &preferred,
            .hidden_surface = &hidden, .scenario_started = &started, .game_active = &active,
            .request_disc = [](void* p,int disc) { self(p).events.emplace_back("disc " + std::to_string(disc)); },
            .current_disc = [](void* p,int timeout,int& out) { EXPECT_TRUE((timeout == 60)) << "disc timeout"; self(p).events.emplace_back("current disc"); out=2; return true; },
            .mission_has_disc = [](void* p,MultiMission*,int disc,bool& out) { EXPECT_TRUE((disc==2)) << "current disc passed"; self(p).events.emplace_back("mission disc"); out=self(p).has_disc; return true; },
            .mission_first_disc = [](void* p,MultiMission*,int& out) { self(p).events.emplace_back("first disc"); out=1; return true; },
            .force_disc = [](void* p) { self(p).events.emplace_back("force"); return self(p).force; },
            .hide_cursor = [](void* p) { self(p).events.emplace_back("hide"); },
            .show_cursor = [](void* p) { self(p).events.emplace_back("show"); },
            .acquire_file = [](void* p,const char* name,FileClass*& out) { auto& f=self(p); f.events.emplace_back("file"); f.opened_name=name; if(!f.lease_available)return false; out=&f.file; return true; },
            .release_file = [](void* p,FileClass* file) { auto& f=self(p); EXPECT_TRUE((file==&f.file && !f.file.open)) << "release closed file"; f.events.emplace_back("release"); },
            .play_movie = [](void* p,int movie,int theme,bool a,bool b,bool c) { EXPECT_TRUE((theme==-1 && a && b && c)) << "movie arguments"; self(p).events.emplace_back("movie " + std::to_string(movie)); },
            .stop_theme = [](void* p,bool immediate) { self(p).events.emplace_back(immediate?"stop immediate":"stop"); },
            .find_theme = [](void* p,const char* name,int& out) { EXPECT_TRUE((!std::strcmp(name,"LOADING"))) << "loading theme name"; self(p).events.emplace_back("find loading"); out=10; return true; },
            .play_theme = [](void* p,int index) { EXPECT_TRUE((index==10)) << "loading theme index"; self(p).events.emplace_back("loading theme"); },
            .queue_theme = [](void* p,int index) { self(p).events.emplace_back("queue " + std::to_string(index)); },
            .load_world = [](void* p,const char* name) { auto& f=self(p); f.events.emplace_back("world"); f.loaded_name=name; f.scenario->Action=12; f.scenario->ThemeIndex=f.loaded_theme; f.scenario->StartingDropships=f.dropships; if(f.mode_changes)f.mode=4; return f.load; },
            .dropship_dialog = [](void* p) { self(p).events.emplace_back("dropships"); },
            .resize_display = [](void* p,int x,int y) { EXPECT_TRUE((x==800 && y==600)) << "display size"; self(p).events.emplace_back("resize"); },
            .release_menu_assets = [](void* p) { self(p).events.emplace_back("menu"); },
            .apply_options = [](void* p) { self(p).events.emplace_back("options"); },
            .present_surface = [](void* p,Surface* surface,bool captured) { EXPECT_TRUE((surface==self(p).hidden && captured)) << "presentation arguments"; self(p).events.emplace_back("present"); },
            .disable_ime = [](void* p) { self(p).events.emplace_back("ime"); },
            .clear_online_state = [](void* p) { self(p).events.emplace_back("online"); }
        };
        runtime = {this, [](void* p) noexcept { return self(p).mode; }, [](void*,bool,int){},
            [](void*,const char*,int,const wchar_t*& out) { out=L"fixture"; return true; }};
        runtime.start = &start;
    }
    bool run(const char* filename="Mixed.map", bool briefing=true, int index=-1, bool ticking=false, bool wait_only=false) {
        struct Call { Fixture* fixture; const char* name; bool briefing; int index; bool ticking; bool wait_only; bool result=false; } call{this,filename,briefing,index,ticking,wait_only};
        game::ClockServices clock{this, [](void* p) noexcept -> std::uint32_t { ++self(p).clock_calls; return 16000; }};
        EXPECT_TRUE((game::with_clock(clock, [](void* p) {
            auto& call=*static_cast<Call*>(p); auto& f=*call.fixture;
            EXPECT_TRUE((game::with_scenario_runtime(f.runtime, [](void* p) {
                auto& call=*static_cast<Call*>(p); auto& f=*call.fixture;
                const int selected_mode=f.mode; f.mode=0;
                ScenarioClass value; f.scenario=&value; f.mode=selected_mode;
                auto* previous=ScenarioClass::Instance; ScenarioClass::Instance=&value;
                struct Restore { ScenarioClass* old; ~Restore(){ ScenarioClass::Instance=old; } } restore{previous};
                value.ElapsedTimer.StartTime=call.ticking?900:-1; value.ElapsedTimer.TimeElapsed=42;
                f.clock_calls=0;
                const char* name=call.name;
                if(name && !std::strcmp(name,"@alias")) { std::strcpy(value.FileName,"Alias.map"); name=value.FileName; }
                call.result=call.wait_only?ScenarioClass::WaitForPlayers():ScenarioClass::StartScenario(name,call.briefing,call.index);
                if(call.result && !call.wait_only) {
                    EXPECT_TRUE((value.ElapsedTimer.TimeElapsed==42 && value.ElapsedTimer.StartTime==(call.ticking?900:1000))) << "resume elapsed without reset";
                    if(!f.world_controller)EXPECT_TRUE((f.clock_calls==(call.ticking?0:1))) << "only paused timer samples clock";
                    EXPECT_TRUE((value.CampaignIndex==call.index)) << "campaign index retained";
                }
                if(!f.loaded_name.empty()) {
                    auto upper=f.loaded_name; for(char& c:upper)if(c>='a'&&c<='z')c=char(c-'a'+'A');
                    EXPECT_TRUE(((f.world_controller?f.loaded_name:upper)==value.FileName)) << "saved filename follows active loader";
                }
            },&call))) << "bind scenario start dependencies";
        },&call))) << "bind startup clock";
        scenario=nullptr;
        return call.result;
    }
};
struct WorldCountry : HouseTypeClass {
    WorldCountry() : HouseTypeClass(noinit_t{}) { SideIndex=2; }
};
struct WorldHouse : HouseClass {
    WorldHouse() : HouseClass(nullptr) {}
};
struct WorldFixture {
    Fixture f;
    WorldCountry country;
    WorldHouse first_house,second_house;
    NodeNameType first{},second{};
    DynamicVectorClass<NodeNameType*> players;
    DynamicVectorClass<HouseTypeClass*> countries;
    DynamicVectorClass<HouseClass*> houses;
    RulesClass rules;
    RulesClass* rules_pointer=&rules;
    game::ScenarioHouseServices house_services;
    game::ScenarioIniServices ini;
    game::ScenarioLoadScreenServices screen;
    game::ScenarioNetworkLoadServices network;
    game::ScenarioWorldServices world;
    int depth=2,tournament=0,manager_cookie=0,side=-1;
    unsigned game_id=12345;
    unsigned short ports[8]{1000,1001};
    bool armageddon=false,delay=false,load=true,manager_attached=false,shroud=false;
    double fraction=1.0;
    WinModemClass* modem=nullptr;
    int modem_queries=0;
    RectangleStruct rect{10,20,640,480};
    LoadProgressManager* manager=reinterpret_cast<LoadProgressManager*>(&manager_cookie);
    static WorldFixture& self(void* p){return *static_cast<WorldFixture*>(p);}
    void event(const char* name){f.events.emplace_back(name);}
    bool wait_for_players(){return f.run("Mixed.map",false,-1,false,true);}
    WorldFixture() {
        f.world_controller=true;
        first.Country=-3; second.Country=0; first.HouseIndex=0;second.HouseIndex=1;
        unsigned ip=0x04030201;std::memcpy(&second.Address.sin_addr,&ip,sizeof(ip));
        players.AddItem(&first);players.AddItem(&second);countries.AddItem(&country);
        houses.AddItem(&first_house);houses.AddItem(&second_house);rules.Shroud=false;
        house_services.session=&f.session;house_services.players=&players;house_services.countries=&countries;
        house_services.houses=&houses;house_services.rules=&rules_pointer;
        ini={this,&armageddon,[](void* p,int percent){self(p).f.events.emplace_back("progress "+std::to_string(percent));},[](void* p){self(p).event("pump");},[](void*){}};
        screen={
            .context=this,.tournament=&tournament,.game_id=&game_id,
            .format_game_id=[](void*,const wchar_t*,unsigned id,wchar_t* out,unsigned capacity){EXPECT_TRUE((id==12345 && capacity==130)) << "game id";std::wcscpy(out,L"Game 12345");return true;},
            .begin=[](void* p,double maximum,unsigned char count){EXPECT_TRUE((maximum==100.0 && count==(self(p).f.mode==0||self(p).f.mode==5?1:2))) << "progress maximum and player count";self(p).event("begin");},
            .get_manager=[](void* p,LoadProgressManager*& out){out=self(p).manager;self(p).event("manager");return true;},
            .prepare_manager=[](void* p,LoadProgressManager* m){EXPECT_TRUE((m==self(p).manager)) << "prepare current manager";self(p).event("prepare manager");},
            .attach_manager=[](void* p,LoadProgressManager* m){self(p).manager_attached=m!=nullptr;self(p).event(m?"attach":"detach");},
            .set_side=[](void* p,int side){self(p).side=side;self(p).event("side");},
            .prepare_surface=[](void* p,LoadProgressManager* m){EXPECT_TRUE((m==self(p).manager)) << "prepare surface manager";self(p).event("prepare surface");},
            .campaign_palette=[](void*,ConvertClass*& out){out=nullptr;return true;},
            .load_bar=[](void* p,const char* name,ConvertClass*){EXPECT_TRUE((!std::strcmp(name,self(p).f.mode==0?"SPLDBR.SHP":"PROGBARM.SHP"))) << "loading bar asset";self(p).event("bar");},
            .position=[](void* p,LoadProgressManager* m,Point2D& out){EXPECT_TRUE((m==self(p).manager)) << "position uses retained manager";out={84,7};return true;},
            .configure_text=[](void* p,const Point2D& point,const wchar_t* text,bool a,bool b){auto& w=self(p);EXPECT_TRUE((point==Point2D{84,7} && a==(w.f.mode!=0) && b==a)) << "loading text flags";EXPECT_TRUE((bool(text)==bool(w.f.mode==4&&w.tournament))) << "optional numeric game id";},
            .extent=[](void*,LoadProgressManager*,int& out){out=406;return true;},
            .set_extent=[](void*,int extent){EXPECT_TRUE((extent==406)) << "bar extent";},
            .fraction=[](void* p,double& out){out=self(p).fraction;return true;},
            .set_player_progress=[](void* p,int slot,double value,double secondary){EXPECT_TRUE((slot>=0&&slot<2&&value==100.0&&std::bit_cast<std::uint64_t>(secondary)==UINT64_MAX)) << "complete progress payload including NaN bits";self(p).fraction=1.0;self(p).event("complete player");},
            .finish=[](void* p){self(p).event("finish progress");},
            .release_manager=[](void* p,LoadProgressManager* m,bool success){auto& w=self(p);EXPECT_TRUE((m==w.manager&&!w.manager_attached)) << "detach before release";w.event(success?"release graphics":"destroy manager");}
        };
        network={
            .context=this,.delay_before_sync=&delay,.peer_ports=ports,
            .has_datagram_transport=[](void*){return true;},
            .clear_broadcast_addresses=[](void* p){self(p).event("clear addresses");},
            .add_broadcast_address=[](void* p,const char* ip,unsigned short port){EXPECT_TRUE((!std::strcmp(ip,"1.2.3.4")&&port==1001)) << "borrowed address bytes and port";self(p).event("address");},
            .poll_input=[](void* p){self(p).event("input");},
            .modem_status=[](void* p,int& status){auto& w=self(p);++w.modem_queries;
                if(!w.modem)return false;status=w.modem->Get_Modem_Status();return true;},
            .player_fraction=[](void*,int,double& out){out=1.0;return true;},
            .drop_connection=[](void*,int,int){throw std::runtime_error("unexpected peer drop");},
            .is_game_host=[](void*,HouseClass*,bool& out){out=false;return true;},
            .reset_game_host=[](void*){throw std::runtime_error("unexpected host reset");},
            .delay=[](void* p,unsigned milliseconds,bool value){EXPECT_TRUE((milliseconds==15000&&value)) << "online delay";self(p).event("delay");},
            .finish_online_load=[](void* p){self(p).event("online finished");}
        };
        world={
            .context=this,.initialization_depth=&depth,.message_rect=&rect,.screen=&screen,.network=&network,
            .load_seed=[](void* p,const char* filename){self(p).f.loaded_name=filename;self(p).event("seed");return self(p).load;},
            .generate_map=[](void* p){self(p).event("generate");},
            .legacy_starting_units=[](void* p,bool official){EXPECT_TRUE((official)) << "random map units official flag";self(p).event("units");},
            .initialize_world_ini=[](void* p,CCINIClass& ini,bool skip){auto& w=self(p);EXPECT_TRUE((ini.Exists("Basic","Intro")&&!skip)) << "world receives parsed scenario and false skip_units";w.f.loaded_name=w.f.scenario->FileName;w.event("world ini");return w.load;},
            .legacy_finish_world=[](void* p){auto& w=self(p);w.event("finish world");w.f.scenario->Action=12;w.f.scenario->ThemeIndex=15;w.f.scenario->StartingDropships=0;},
            .set_shroud=[](void* p,bool value){EXPECT_TRUE((!value)) << "disable shroud";self(p).shroud=true;self(p).event("shroud");},
            .fade_loading_palette=[](void* p){self(p).event("fade");},
            .show_load_error=[](void* p,const wchar_t*,const wchar_t*){self(p).event("error");},
            .configure_messages=[](void*,int x,int y,int width){EXPECT_TRUE((x==13&&y==20&&width==634)) << "message layout";},
            .finish_house=[](void* p,HouseClass* house){auto& w=self(p);w.event(house==&w.second_house?"house 1":"house 0");}
        };
        f.runtime.houses=&house_services;f.runtime.ini=&ini;f.runtime.world=&world;
        f.start.load_world=[](void*,const char* filename){return ScenarioClass::ReadScenario(filename);};
    }
};
void world_tests() {
    for(int mode:{0,3,4,5})for(bool armageddon:{false,true})for(bool random:{false,true})for(bool loaded:{false,true}) {
        WorldFixture w;w.f.mode=mode;w.armageddon=armageddon;w.load=loaded;
        w.f.session.SawCompletion=w.f.session.OutOfSync=true;
        EXPECT_TRUE((w.f.run(random?"Mixed.SeD":"Mixed.map")==loaded)) << "integrated startup and world result";
        EXPECT_TRUE((w.depth==2 && !w.manager_attached && w.shroud)) << "world depth, manager and shroud cleanup";
        auto& events=w.f.events;
        const int addresses = !armageddon && mode == 4 ? 1 : 0;
        EXPECT_TRUE((std::count(events.begin(),events.end(),"clear addresses")==addresses &&
            std::count(events.begin(),events.end(),"address")==addresses)) << "complete WinSock services retain original address setup";
        if(!armageddon)EXPECT_TRUE((w.side==(mode==0?0:2))) << "loading side uses original country sentinel";
        if(loaded) {
            EXPECT_TRUE((!w.f.session.SawCompletion&&!w.f.session.OutOfSync)) << "reset synchronization flags";
            auto a=std::find(events.begin(),events.end(),"house 1"),b=std::find(events.begin(),events.end(),"house 0");
            EXPECT_TRUE((a!=events.end()&&b!=events.end()&&a<b)) << "finish houses in descending order";
        } else {
            auto a=std::find(events.begin(),events.end(),"fade"),b=std::find(events.begin(),events.end(),"error");
            EXPECT_TRUE((a!=events.end()&&b!=events.end()&&a<b)) << "load failure UI order";
            EXPECT_TRUE((std::find(events.begin(),events.end(),"finish progress")==events.end())) << "failure skips successful progress finalization";
            EXPECT_TRUE(((std::find(events.begin(),events.end(),"destroy manager")!=events.end())==!armageddon)) << "failure manager destruction is conditional";
        }
        EXPECT_TRUE((Unsorted::CurrentFrame==0)) << "world startup resets frame";
    }
    for(int mode:{0,3,4,5}) {
        WorldFixture absent;absent.f.mode=mode;
        absent.network.peer_ports=nullptr;absent.network.has_datagram_transport=nullptr;
        absent.network.clear_broadcast_addresses=nullptr;absent.network.add_broadcast_address=nullptr;
        EXPECT_TRUE((absent.f.run()&&absent.depth==2&&!absent.manager_attached)) << "absent WinSock services allow world loading and cleanup";
        const auto& events=absent.f.events;
        EXPECT_TRUE((std::find(events.begin(),events.end(),"clear addresses")==events.end() &&
            std::find(events.begin(),events.end(),"address")==events.end())) << "absent WinSock services skip address setup";
    }
    for(unsigned supplied=1;supplied<15;++supplied) {
        WorldFixture partial;partial.f.mode=4;
        if(!(supplied&1))partial.network.peer_ports=nullptr;
        if(!(supplied&2))partial.network.has_datagram_transport=nullptr;
        if(!(supplied&4))partial.network.clear_broadcast_addresses=nullptr;
        if(!(supplied&8))partial.network.add_broadcast_address=nullptr;
        EXPECT_TRUE((!partial.f.run()&&partial.depth==2&&!partial.manager_attached)) << "partially supplied WinSock services fail before world side effects";
        EXPECT_TRUE((std::find(partial.f.events.begin(),partial.f.events.end(),"begin")==partial.f.events.end())) << "incomplete WinSock group is rejected before the loading screen starts";
    }
    WorldFixture w;w.f.mode=4;w.fraction=0.9996;w.tournament=1;w.delay=true;
    EXPECT_TRUE((w.f.run()&&std::count(w.f.events.begin(),w.f.events.end(),"complete player")==2)) << "post-sync finish applies every player progress";
    for(bool random:{false,true}) {
        WorldFixture campaign;campaign.players.Count=0;
        EXPECT_TRUE((campaign.f.run(random?"Mixed.SeD":"Mixed.map")&&campaign.depth==2&&!campaign.manager_attached)) << "campaign startup accepts no multiplayer players and completes world cleanup";
        EXPECT_TRUE((campaign.side==0 && std::count(campaign.f.events.begin(),campaign.f.events.end(),"begin")==1)) << "campaign loading uses its own side and one progress slot without multiplayer nodes";
    }
    for(int mode:{1,3,4,5})for(int count:{0,9}) {
        WorldFixture invalid;invalid.f.mode=mode;invalid.players.Count=count;
        EXPECT_TRUE((!invalid.f.run()&&invalid.depth==2&&!invalid.manager_attached)) << "non-campaign loading still rejects empty or oversized player lists before world load";
        invalid.players.Count=2;
    }
}
void modem_tests() {
    EXPECT_TRUE((!WinModemClass::Instance)) << "core modem defaults to absent";
    {
        WinModemClass modem;
        EXPECT_TRUE((!WinModemClass::Instance)) << "construction does not publish a device";
        WinModemClass::Instance=&modem;
        {
            WinModemClass other;
            EXPECT_TRUE((other.Open_Serial_Port("COM1",9600,0,8,1)==WinModemClass::InvalidHandle &&
                other.Get_Port_Handle()==WinModemClass::InvalidHandle && !other.Get_Modem_Status())) << "no-device modem cannot open a port or claim carrier";
            other.Close_Serial_Port();other.Close_Serial_Port();
        }
        EXPECT_TRUE((WinModemClass::Instance==&modem)) << "destroying another modem preserves the borrowed instance";
    }
    EXPECT_TRUE((!WinModemClass::Instance)) << "destruction clears the selected borrowed modem";
    for(int mode:{0,1,3,4,5}) {
        WorldFixture absent;absent.f.mode=mode;
        EXPECT_TRUE((absent.wait_for_players() && absent.modem_queries==(mode==1?1:0))) << "an absent modem does not abort loading and only mode 1 queries it";
        WinModemClass modem;
        WorldFixture unavailable;unavailable.f.mode=mode;unavailable.modem=&modem;
        EXPECT_TRUE((unavailable.wait_for_players()==(mode!=1) && unavailable.modem_queries==(mode==1?1:0))) << "a no-carrier device aborts only the legacy modem loading path";
    }
    // Exercise the actual Scenario consumer with every hardware signal-bit
    // combination. A future device backend returns positive masks, not int8.
    class Signals final : public WinModemClass {
    public:
        explicit Signals(int value) : value(value) {}
        int Get_Modem_Status() noexcept override {return value;}
        int value;
    };
    for(int signals=0;signals<=0xF0;signals+=0x10) {
        Signals modem(signals);
        WorldFixture w;w.f.mode=1;w.modem=&modem;
        EXPECT_TRUE((w.wait_for_players()==((signals&WinModemClass::CarrierDetect)!=0) && w.modem_queries==1)) << "carrier bit, rather than sign or other modem signals, controls loading";
    }
}
struct InitializeFixture : WorldFixture {
    struct MapFixture : DisplayClass {
        ~MapFixture() override { Cells.Clear(); } // Fixture slots borrow its sibling Cell.
        bool SetCursor(MouseCursorType,bool) override {return false;}
        bool UpdateCursor(MouseCursorType,bool) override {return false;}
        bool RestoreCursor() override {return false;}
        void UpdateCursorMinimapState(bool) override {}
        MouseCursorType GetLastMouseCursor() override {return {};}
    } map;
    struct CellFixture : CellClass { CellFixture():CellClass(){} } cell;
    struct AuxiliaryFile : MemoryFile {
        std::string name,metadata="[Mixed.map]\nUIName=TXT:Title\nBriefing=TXT:Brief\nLSLoadMessage=Loading\n";
        const char* SetFileName(const char* value) override {
            EXPECT_TRUE((!open)) << "rename closed auxiliary file";name=value;
            exists=name=="MISSIONMD.INI";bytes=exists?metadata:"";return name.c_str();
        }
        const char* GetFileName() const override {return name.c_str();}
    } auxiliary;
    CCINIClass source,rules_ini,ai_ini,ui_ini;
    CCINIClass* rules_ini_pointer=&rules_ini;
    HouseClass* current=nullptr;
    game::ScenarioInitializeServices initialize;
    game::ScenarioRenderServices render;
    ScenarioFlags flags{};
    unsigned crc=12345;
    int difficulty=1,tactical_cookie=0,clear_calls=0,side_calls=0,leases=0,releases=0,refreshed=0,failure=0;
    bool building=true,custom_ai=false,result=false,missing_saved_title=true;
    TacticalClass* tactical=reinterpret_cast<TacticalClass*>(&tactical_cookie);
    std::wstring title=L"Title",brief=L"Briefing";
    static InitializeFixture& self(void* p){return *static_cast<InitializeFixture*>(p);}
    InitializeFixture() {
        first_house.Type=second_house.Type=&country;country.ArrayIndex2=3;
        std::strcpy(first_house.PlainName,"Americans");std::strcpy(second_house.PlainName,"Second");
        house_services.current_player=&current;
        map.MapRect={0,0,2,2};map.VisibleRect={0,0,2,2};
        EXPECT_TRUE((map.Cells.SetCapacity(2048))) << "initialize map fixture storage";
        std::fill_n(map.Cells.Items,2048,nullptr);map.Cells.Items[1025]=&cell;
        source.WriteString("Basic","Player","Americans");source.WriteInteger("Basic","InitTime",321);
        source.WriteString("Map","Theater","SNOW");rules_ini.WriteString("VariableNames","2","Ready");
        f.runtime.context=this;
        f.runtime.session_mode=[](void*) noexcept{return 0;};
        f.runtime.stringtable=[](void* p,const char* label,int,const wchar_t*& out) {
            auto& w=self(p);
            out=!std::strcmp(label,"TXT:Brief")?w.brief.c_str():!std::strcmp(label,"TXT:TitleSav")&&w.missing_saved_title?L"MISSING:TXT:TitleSav":w.title.c_str();
            return true;
        };
        f.start.context=this;
        f.start.acquire_file=[](void* p,const char* name,FileClass*& out) {
            auto& w=self(p);EXPECT_TRUE((!name)) << "initializer acquires default CCFile";
            if(w.failure==3)return false;
            ++w.leases;out=&w.auxiliary;return true;
        };
        f.start.release_file=[](void* p,FileClass* file) {
            auto& w=self(p);EXPECT_TRUE((file==&w.auxiliary&&!w.auxiliary.open)) << "auxiliary file lifetime";++w.releases;
        };
        render.context=this;render.map=&map;render.redraw_sidebar=[](void*,int mode){EXPECT_TRUE((mode==2)) << "initializer sidebar mode";};
        initialize={
            .context=this,.campaign_difficulty=&difficulty,.special_flags=&flags,.scenario_crc=&crc,
            .building_read_flag=&building,.custom_ai=&custom_ai,.rules_ini=&rules_ini_pointer,
            .ai_ini=&ai_ini,.ui_ini=&ui_ini,.tactical=&tactical,
            .clear_world=[](void* p){++self(p).clear_calls;},
            .rules=[](void* p,game::ScenarioRulesPass pass,RulesClass*,CCINIClass* ini,bool multiplayer){
                auto& w=self(p);EXPECT_TRUE((!multiplayer)) << "campaign command bar";
                if(pass==game::ScenarioRulesPass::initialize)EXPECT_TRUE((ini==&w.rules_ini)) << "global rules source";
                w.event("rules");
            },
            .read_country=[](void*,HouseTypeClass*,CCINIClass*){},
            .set_map_size=[](void*,const RectangleStruct&,bool,bool,bool){},
            .set_local_size=[](void*,const RectangleStruct&){},
            .prepare_mode=[](void*,MPGameModeClass*,bool){},.start_mode=[](void*,MPGameModeClass*,bool){},
            .start_special_mode=[](void*,bool){},.mode_ready=[](void*,MPGameModeClass*){},
            .draw_load_screen=[](void* p,LoadProgressManager* manager){EXPECT_TRUE((manager==self(p).manager)) << "initialize load manager";},
            .destroy_tactical=[](void* p,TacticalClass* tactical){EXPECT_TRUE((tactical==self(p).tactical)) << "destroy current tactical";self(p).event("destroy tactical");},
            .create_tactical=[](void* p,TacticalClass*& out){auto& w=self(p);if(w.failure==4)return false;out=reinterpret_cast<TacticalClass*>(&w.tactical_cookie);return true;},
            .tactical_rect=[](void*,TacticalClass*,const RectangleStruct& rect){EXPECT_TRUE((rect.Width==640)) << "tactical viewport";},
            .theater=[](void*,int index){EXPECT_TRUE((index==1)) << "initialize snow theater";},
            .side=[](void* p,int index){auto& w=self(p);EXPECT_TRUE((index==2)) << "campaign house side";++w.side_calls;return w.failure!=w.side_calls;},
            .find_house=[](void* p,const char* name,HouseClass*& out){EXPECT_TRUE((!std::strcmp(name,"Americans"))) << "campaign player name";out=&self(p).first_house;return true;},
            .read_objects=[](void* p,game::ScenarioObjectReader reader,CCINIClass*,bool){if(reader==game::ScenarioObjectReader::buildings)EXPECT_TRUE((!self(p).building)) << "building read flag disabled around loader";},
            .step=[](void* p,game::ScenarioInitializationStep step){auto& w=self(p);if(step==game::ScenarioInitializationStep::operational_buildings){EXPECT_TRUE((w.depth==0)) << "temporarily clear initialization depth";if(w.failure==5)throw std::runtime_error("operational fixture failure");}},
            .refresh_cell=[](void* p,CellClass* cell,int value){auto& w=self(p);EXPECT_TRUE((cell==&w.cell&&value==-1)) << "refresh actual map cell";++w.refreshed;},
            .map_total_value=[](void*,bool value,int& out){EXPECT_TRUE((!value)) << "resource value flag";out=1234;return true;}
        };
        f.runtime.render=&render;f.runtime.initialize=&initialize;
    }
    void run() {
        game::ClockServices clock{nullptr,[](void*) noexcept -> std::uint32_t{return 16000;}};
        EXPECT_TRUE((game::with_clock(clock,[](void* p){auto& w=self(p);
            EXPECT_TRUE((game::with_scenario_runtime(w.f.runtime,[](void* p){auto& w=self(p);
                ScenarioClass scenario;
                auto* old=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
                struct Restore {ScenarioClass* old;~Restore(){ScenarioClass::Instance=old;}} restore{old};
                std::strcpy(scenario.FileName,"Mixed.map");scenario.UINameLoaded[0]=L'Z';scenario.UINameLoaded[1]=0;
                if(w.failure==6)std::memset(scenario.FileName,'X',sizeof(scenario.FileName));
                try { w.result=ScenarioClass::InitializeWorldINI(&w.source,false); }
                catch(const std::runtime_error& error){
                    EXPECT_TRUE((w.failure==5&&!std::strcmp(error.what(),"operational fixture failure"))) << "expected downstream exception";
                    EXPECT_TRUE((w.depth==2)) << "exception restores temporarily suspended initialization depth";return;
                }
                EXPECT_TRUE((w.result==(w.failure==0))) << "initialize result including failures";
                if(w.result){
                    EXPECT_TRUE((scenario.InitTime==321&&scenario.PlayerSideIndex==2&&scenario.Difficulty1==1&&scenario.Difficulty2==1)) << "initializer scalar fields";
                    EXPECT_TRUE((!std::strcmp(scenario.GlobalVariables[2].Name,"Ready")&&w.current==&w.first_house)) << "global variables and real ReadINI player selection";
                    if(w.auxiliary.metadata.find("UIName=")!=std::string::npos) {
                        if(w.title.size()>44)EXPECT_TRUE((scenario.Name[42]==L'T'&&scenario.Name[43]==0xD83D&&scenario.Name[44]==0)) << "UTF-16 truncation keeps target code-unit limit on wide hosts";
                        else EXPECT_TRUE((!std::wcscmp(scenario.Name,L"Title"))) << "mission title";
                        EXPECT_TRUE((!std::wcscmp(scenario.Name,scenario.UINameLoaded))) << "missing saved title falls back to display name";
                    } else EXPECT_TRUE((!scenario.Name[0]&&scenario.UINameLoaded[0]==L'Z')) << "missing UIName retains saved display name";
                }
            },p))) << "bind initializer services";
        },this))) << "bind initializer clock";
        EXPECT_TRUE((depth==2&&leases==releases)) << "initializer restores nesting and releases every file";
        if(result)EXPECT_TRUE((clear_calls==1&&leases==2&&side_calls==2&&refreshed==1&&crc==0&&map.TotalValue==1234&&building)) << "campaign two-pass initialization and world completion state";
    }
};
void initialize_tests() {
    for(int failure=0;failure<=6;++failure){InitializeFixture w;w.failure=failure;w.run();}
    InitializeFixture long_title;long_title.title=std::wstring(43,L'T')+L"\U0001F600 tail";long_title.run();
    InitializeFixture missing;missing.auxiliary.metadata="[Other.map]\nName=Other\n";missing.run();
    CCINIClass ini;int defaults[4]{1,2,30,40},out[4];
    ini.Read4Integers(out,"Map","Size",defaults);
    EXPECT_TRUE((std::all_of(out,out+4,[](int value){return value==0;}))) << "missing rectangle key uses literal zero default";
    ini.WriteString("Map","Size","7,x,9,10");ini.Read4Integers(out,"Map","Size",defaults);
    EXPECT_TRUE((out[0]==7&&out[1]==2&&out[2]==30&&out[3]==40)) << "partial rectangle conversion keeps remaining defaults";
}
struct ClearFixture : InitializeFixture {
    struct ObjectFixture : ObjectClass {
        ClearFixture& world;
        int references;
        AbstractType kind;
        ObjectFixture(ClearFixture& w,int refs,AbstractType type):ObjectClass(),world(w),references(refs),kind(type) {}
        HRESULT YRPP_STDCALL GetClassID(CLSID*) override {return 0;}
        HRESULT YRPP_STDCALL Save(IStream*,BOOL) override {return 0;}
        AbstractType WhatAmI() const override {return kind;}
        int Size() const override {return sizeof(*this);}
        ULONG YRPP_STDCALL Release() override {
            ++world.object_releases;
            const int remaining=--references;
            if(!remaining)delete this;
            return remaining;
        }
        ~ObjectFixture() override {
            world.correct_flags=world.correct_flags&&!world.building;
            world.objects.Remove(this);++world.object_deletes;
        }
    };
    struct TagFixture : TagTypeClass {
        ClearFixture& world;
        explicit TagFixture(ClearFixture& w):TagTypeClass(noinit_t{}),world(w){
            // The fixture skips registration, but the real base destructor still
            // owns FirstTrigger. An empty fixture must own an empty chain.
            FirstTrigger=nullptr;
        }
        ~TagFixture() override {
            world.correct_flags=world.correct_flags&&world.building;
            world.tag_types.Remove(this);++world.tag_deletes;
        }
    };
    struct ParticleFixture : ParticleSystemClass {
        ClearFixture& world;
        explicit ParticleFixture(ClearFixture& w):ParticleSystemClass(noinit_t{}),world(w){
            // The real base destructor owns this union member even in the fixture.
            new (&Particles) DynamicVectorClass<ParticleClass*>();
        }
        ~ParticleFixture() override {++world.particle_deletes;}
    };
    IsometricTileTypeClass tile{0,-65,0,"clear fixture tile",0};
    DynamicVectorClass<AbstractTypeClass*> types;
    DynamicVectorClass<IsometricTileTypeClass*> tiles;
    DynamicVectorClass<ObjectClass*> objects,current_objects;
    DynamicVectorClass<TagTypeClass*> tag_types;
    DynamicVectorClass<TagClass*> map_tags,logic_tags;
    ObjectClass* borrowed_objects[2]{};
    TagClass* borrowed_tags[2]{};
    ParticleSystemClass* particle=nullptr;
    CellStruct empty_cell{7,9};
    VolumeStruct volume{};
    VolumeStruct* volume_pointer=&volume;
    game::ScenarioPauseServices pause;
    game::ScenarioClearServices clear;
    int object_releases=0,object_deletes=0,tag_deletes=0,particle_deletes=0,notifications=0;
    bool correct_flags=true,integrated=false;
    static ClearFixture& self(void* p){return *static_cast<ClearFixture*>(p);}
    ClearFixture() {
        f.runtime.context=this;f.runtime.variable_changed=[](void* p,bool global,int index){
            auto& w=self(p);EXPECT_TRUE((global&&ScenarioClass::Instance->GlobalVariables[index].Value==0&&ScenarioClass::Instance->VariablesChanged)) << "clear notification sees updated state";++w.notifications;
        };
        types.AddItem(&country);types.AddItem(&tile);types.AddItem(&country);tiles.AddItem(&tile);
        objects.AddItem(new ObjectFixture(*this,3,AbstractType::Bullet));
        objects.AddItem(new ObjectFixture(*this,1,AbstractType::Building));
        tag_types.AddItem(new TagFixture(*this));particle=new ParticleFixture(*this);
        current_objects.SetCapacity(2,borrowed_objects);current_objects.Count=2;
        map_tags.SetCapacity(2,borrowed_tags);map_tags.Count=2;
        logic_tags.AddItem(nullptr);map.ZoneConnections.AddItem({});
        pause.volume=&volume_pointer;f.runtime.pause=&pause;
        clear={
            .context=this,.types=&types,.tile_types=&tiles,.objects=&objects,.tag_types=&tag_types,
            .map_tags=&map_tags,.logic_tags=&logic_tags,.current_objects=&current_objects,
            .particle_system=&particle,.empty_cell=&empty_cell,
            .destroy_world_objects=[](void* p){auto& w=self(p);
                EXPECT_TRUE((w.types.Count==2&&w.types[0]==&w.country&&w.types[1]==&w.country)) << "tile type detached before global object destruction";
                w.types.Count=0;
            },
            .step=[](void* p,game::ScenarioClearStep step){auto& w=self(p);
                if(step==game::ScenarioClearStep::stop_audio)EXPECT_TRUE((ScenarioClass::Instance->Random.unknown_00&&w.current==&w.first_house)) << "audio stopped before clearing current player";
                if(step==game::ScenarioClearStep::map_objects)EXPECT_TRUE((w.types.Count==1&&w.types[0]==&w.tile)) << "preserved tile type restored";
                if(step==game::ScenarioClearStep::map_initialize)EXPECT_TRUE((!w.map.MapRect.X&&!w.map.MapRect.Y&&!w.map.MapRect.Width&&!w.map.MapRect.Height)) << "clear map rectangle before initializing map hierarchy";
                if(step==game::ScenarioClearStep::sidebar_timers)EXPECT_TRUE((!ScenarioClass::Instance->Random.unknown_00)) << "reset random flag before sidebar timers";
            },
            .drain_particles=[](void* p,ParticleSystemClass* particle){EXPECT_TRUE((particle==self(p).particle)) << "drain current particle system";},
            .reset_map_start_positions=[](void*){ScenarioClass::Instance->UniqueID=123;return -7;}
        };
        f.runtime.clear=&clear;
        initialize.clear_world=[](void*){ScenarioClass::ClearWorld();};
    }
    void run() {
        game::ClockServices clock{nullptr,[](void*) noexcept -> std::uint32_t{return 16000;}};
        EXPECT_TRUE((game::with_clock(clock,[](void* p){auto& w=self(p);
            EXPECT_TRUE((game::with_scenario_runtime(w.f.runtime,[](void* p){auto& w=self(p);
                ScenarioClass scenario;auto* old=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
                struct Restore {ScenarioClass* old;~Restore(){ScenarioClass::Instance=old;}} restore{old};
                std::strcpy(scenario.FileName,"Mixed.map");w.current=&w.first_house;
                scenario.GlobalVariables[0].Value=1;scenario.GlobalVariables[49].Value=-1;
                if(w.integrated) {
                    EXPECT_TRUE((ScenarioClass::InitializeWorldINI(&w.source,false))) << "initialization executes shared ClearWorld and ReadINI";
                    EXPECT_TRUE((w.current==&w.first_house&&scenario.InitTime==321)) << "initializer restores player and map metadata after cleanup";
                } else {
                    EXPECT_TRUE((ScenarioClass::ClearWorld()==-7)) << "original map reset return value";
                    EXPECT_TRUE((scenario.UniqueID==1000000&&!scenario.Random.unknown_00&&!w.current)) << "clear final instance state";
                    EXPECT_TRUE((std::all_of(std::begin(scenario.Waypoints),std::end(scenario.Waypoints),[&](auto value){return value==w.empty_cell;}))) << "clear uses original empty-cell value for every waypoint";
                }
                EXPECT_TRUE((w.notifications==2&&w.object_releases==3&&w.object_deletes==2&&w.tag_deletes==1&&w.particle_deletes==1)) << "COM release, deletion and global notification counts";
                EXPECT_TRUE((w.objects.Count==0&&w.tag_types.Count==0&&!w.particle&&w.correct_flags&&w.building)) << "owning destructors remove original objects with correct flags";
                EXPECT_TRUE((w.current_objects.Items==w.borrowed_objects&&w.map_tags.Items==w.borrowed_tags&&w.current_objects.Capacity==0&&w.map_tags.Count==0)) << "borrowed list slots retained after clear";
                EXPECT_TRUE((!w.logic_tags.Items&&!w.map.ZoneConnections.Items)) << "owned list slot allocations released";
                EXPECT_TRUE((w.volume.GetVolume()==0x4000&&ScenarioClass::PausedAudioVolume==0x4000)) << "clear restores audio volume";
            },p))) << "bind clear-world dependencies";
        },this))) << "bind clear-world clock";
    }
};
void clear_tests() {
    ClearFixture direct;direct.run();
    ClearFixture integrated;integrated.integrated=true;integrated.run();
}
void tests(void*) {
    for(int mode:{0,3,4,5}) for(int failure=0;failure<3;++failure) for(bool briefing:{false,true}) for(int movie=0;movie<3;++movie) {
        Fixture f; f.mode=mode; f.force=failure!=1; f.load=failure!=2;
        if(movie==1)f.file.bytes="[Basic]\nBrief=brief\n";
        if(movie==2)f.file.bytes="[Basic]\nIntro=unknown\n";
        EXPECT_TRUE((f.run("Mixed.map",briefing)==(failure==0))) << "startup result";
        std::vector<std::string> expected{"disc -1","force"};
        if(failure!=1) {
            expected.insert(expected.end(),{"hide","file"});
            if(movie!=2)expected.insert(expected.end(),{"stop",movie==0?"movie 7":"movie 8"});
            expected.insert(expected.end(),{"release","find loading","loading theme","world"});
            if(failure==2)expected.emplace_back("show");
            else {
                expected.emplace_back("dropships"); if(briefing)expected.emplace_back("movie 12");
                expected.insert(expected.end(),{"resize","menu","options","fill","present","queue 15","ime"});
            }
            EXPECT_TRUE((f.opened_name=="Mixed.map" && f.loaded_name=="Mixed.map" && f.file.closes==1)) << "file spelling and lifetime";
        }
        if(mode==4)expected.emplace_back("online");
        EXPECT_TRUE((f.events==expected)) << "complete startup dependency order";
        EXPECT_TRUE((f.depth==(failure==1?4:3) && f.started==(failure==0) && f.active==(failure==0))) << "failure and success state";
    }
    for(unsigned char flag:{0,1,2}) {
        Fixture f; f.alternate=flag;
        EXPECT_TRUE((f.run(nullptr,false,0) && f.opened_name==(flag==1?"Alternate.map":"Campaign.map"))) << "campaign filename exact flag";
        EXPECT_TRUE((f.events[1]=="disc 2")) << "campaign required disc";
    }
    for(bool available:{false,true}) {
        Fixture f; f.mode=5; f.has_disc=available; f.session.Config.ScenarioIndex=0;
        // Identity only; MultiMission contents belong to the external session module.
        f.session.MultiMission.AddItem(reinterpret_cast<MultiMission*>(&f.session));
        EXPECT_TRUE((f.run())) << "multiplayer mission media selection";
        EXPECT_TRUE(((std::find(f.events.begin(),f.events.end(),"disc 1")!=f.events.end())==!available)) << "request first disc only when needed";
    }
    {
        Fixture f; f.mode_changes=true; f.loaded_theme=-1; f.current=f.preferred; f.dropships=0; f.file.exists=false;
        EXPECT_TRUE((f.run("@alias",false,-1,true) && f.loaded_name=="ALIAS.MAP")) << "alias and absent preflight file";
        EXPECT_TRUE((f.events.back()=="online" && std::find(f.events.begin(),f.events.end(),"stop immediate")!=f.events.end())) << "live mode and no mission theme";
        EXPECT_TRUE((std::find(f.events.begin(),f.events.end(),"resize")==f.events.end())) << "same resolution skips resize";
    }
    {
        Fixture f; f.depth=INT_MAX; f.force=false;
        EXPECT_TRUE((!f.run() && f.depth==INT_MIN)) << "media depth wraps on failed check";
    }
    {
        Fixture f; f.lease_available=false; EXPECT_TRUE((!f.run())) << "file acquisition failure";
        EXPECT_TRUE((f.events.back()=="show" && !f.started)) << "file acquisition failure restores cursor";
    }
    {
        Fixture f; std::string name(260,'x');
        EXPECT_TRUE((!f.run(name.c_str()) && !f.run(nullptr) && !f.run(nullptr,false,3) && f.events.empty())) << "invalid filename or campaign rejected before mutation";
        f.start.load_world=nullptr;
        EXPECT_TRUE((!f.run() && f.events.empty())) << "missing world loader cannot report startup complete";
    }
    world_tests();
    modem_tests();
    initialize_tests();
    clear_tests();
}
}

TEST(ScenarioStart, Contracts) {
    game::IniRuntimeServices ini{nullptr,
        [](void*,game::IniTypeKind,int,const char*&){return false;},
        [](void*,game::IniTypeKind kind,const char* name,bool allocate,game::IniTypeResult& out) {
            EXPECT_TRUE((kind==game::IniTypeKind::movie && !allocate)) << "preflight only resolves movies";
            if(!std::strcmp(name,"opening"))out.index=7;
            else if(!std::strcmp(name,"brief"))out.index=8;
            else return false;
            return true;
        }, [](void*,const char*,const wchar_t*& out){out=L"fixture"; return true;}};
    std::string error;
    EXPECT_TRUE((game::with_ini_runtime(ini,tests,nullptr,error))) << error.c_str();
}
