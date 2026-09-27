#include "support/test_support.hpp"
#include "map_view.hpp"
#include "api/filesystem.hpp"
#include "api/clock.hpp"
#include "yrpp/MouseClass.h"
#include "yrpp/Surface.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/DrawingBuffers.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "game_ui_runtime.hpp"
#include <filesystem>
#include <bit>
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {

struct Probe final : GadgetClass {
    int actions=0,entered=0,left=0;
    Probe(int x,int y,int width,int height,bool sticky=true)
        : GadgetClass(x,y,width,height,static_cast<GadgetFlag>(0xff),sticky) {}
    bool Action(GadgetFlag flags,DWORD* key,KeyModifier modifier) override {
        ++actions; return GadgetClass::Action(flags,key,modifier);
    }
    void OnMouseEnter() override { ++entered; }
    void OnMouseLeave() override { ++left; }
};
void gadgets() {
    Probe first(10,20,30,40),second(60,20,30,40);
    second.AddTail(first);
    EXPECT_TRUE((first.GetNext()==&second && second.HeadOfList()==&first)) << "real Gadget/Link vtable and list lifecycle";
    first.Dispatch(0,GadgetFlag::LeftPress,10,20,KeyModifier::None);
    EXPECT_TRUE((GadgetClass::StuckOn==&first && first.entered==1)) << "inclusive top-left press captures original gadget";
    first.Dispatch(0,GadgetFlag::LeftHeld,65,25,KeyModifier::None);
    EXPECT_TRUE((first.actions==2 && second.actions==0 && first.left==1 && second.entered==1)) << "capture routes held input across neighboring gadget";
    first.Dispatch(0,GadgetFlag::LeftRelease,1000,1000,KeyModifier::None);
    EXPECT_TRUE((!GadgetClass::StuckOn)) << "out-of-bounds release ends capture";
    const auto count=first.actions;
    first.Dispatch(0,GadgetFlag::LeftPress,40,30,KeyModifier::None);
    EXPECT_TRUE((first.actions==count)) << "exclusive right edge does not hit";
    second.Disable(); first.Dispatch(0,GadgetFlag::RightPress,65,25,KeyModifier::None);
    EXPECT_TRUE((!GadgetClass::StuckOn)) << "disabled gadget cannot capture";
    second.Enable(); first.Dispatch(0,GadgetFlag::RightPress,65,25,KeyModifier::None);
    EXPECT_TRUE((GadgetClass::StuckOn==&second)) << "original right press capture";
    first.Dispatch(0,GadgetFlag::RightRelease,-100,-100,KeyModifier::None);
    EXPECT_TRUE((!GadgetClass::StuckOn)) << "original right release clears capture";
    second.SetFocus(); EXPECT_TRUE((second.IsFocused())) << "original keyboard focus";
    first.Remove(); second.Remove(); GadgetClass::ResetInput();
    EXPECT_TRUE((!GadgetClass::Hovered && !GadgetClass::Focused && !GadgetClass::LastList)) << "clear device interaction globals";
}
void input() {
    game::ResourceHandle* raw=nullptr; std::string error;
    EXPECT_TRUE((game::create_resources(std::filesystem::temp_directory_path().string(),raw,error))) << "create test resource environment";
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(raw,game::destroy_resources);
    struct Session { game::MapViewHandle* view=nullptr; ~Session(){game::destroy_map_view(view);} } session;
    EXPECT_TRUE((game::create_map_view(*files,session.view))) << "normal map session";
    auto& view=*session.view;
    EXPECT_TRUE((game::initialize_empty_map_view(view,{0,0,100,100},0))) << "real empty Cells, no substitute map model";
    auto& radar=RadarClass::Instance;
    MapClass::Instance.VisibleRect={0,0,100,100};
    EXPECT_TRUE((radar.BuildTerrainRadar())) << "radar over real empty Cells";
    Point2D dirty{4,5};
    radar.RefreshCrd(&dirty);radar.RefreshCrd(&dirty);
    EXPECT_TRUE((radar.unknown_points_125C.Count==1)) << "original dirty-pixel bitset deduplicates repeated changes";
    const int dirty_index=dirty.X+dirty.Y*radar.unknown_rect_149C.Width;
    EXPECT_TRUE(((radar.unknown_1274[dirty_index/8]&(1u<<(dirty_index%8)))!=0)) << "dirty bit matches vector entry";
    const CellStruct changed_cell{80,100};
    EXPECT_TRUE((radar.MarkTerrainCellDirty(changed_cell) && radar.MarkTerrainCellDirty(changed_cell))) << "queue actual changed Cell";
    EXPECT_TRUE((radar.unknown_cells_1124.Count==1)) << "same Cell update is coalesced";
    const int raw_x=changed_cell.X+std::bit_cast<int>(radar.unknown_1490)-changed_cell.Y;
    const int raw_y=changed_cell.X+changed_cell.Y-std::bit_cast<int>(radar.unknown_1498);
    EXPECT_TRUE((raw_x>=0 && raw_x+1<int(radar.unknown_1240) && raw_y>=0 && raw_y<int(radar.unknown_1244))) << "fixture Cell projects inside raw radar";
    auto& stale=radar.unknown_123C[raw_y*radar.unknown_1240+raw_x];stale={0,0,0};
    bool refreshed=false;
    EXPECT_TRUE((radar.UpdateTerrainRadar(refreshed) && refreshed && stale.R==60 && radar.unknown_cells_1124.Count==0)) << "refresh replaces stale RGB from canonical Cell and consumes queue";
    view.terrain_loaded=true; // Fixture bypasses file decode only.
    radar.IsAvailableNow=true; radar.unknown_14AC=radar.unknown_14B0=1;
    EXPECT_TRUE((game::set_game_view_size(view,1280,720))) << "core full canvas layout";
    EXPECT_TRUE((game::center_map_view(view,0,2200))) << "center test camera";
    GameOptionsClass::Instance.ScrollRate=0; GameOptionsClass::Instance.ScrollMethod=0;
    const auto start=view.tactical.TacticalPos;
    game::GameInputResult result;
    const auto send=[&](game::GameInputKind kind,int x,int y,unsigned code=0,bool pressed=false) {
        EXPECT_TRUE((game::submit_game_input(view,{kind,x,y,code,0,pressed,0},result))) << "raw event accepted synchronously";
    };
    send(game::GameInputKind::pointer_button,500,350,2,true);
    send(game::GameInputKind::pointer_move,580,310);
    send(game::GameInputKind::pointer_button,580,310,2,false);
    EXPECT_TRUE((view.tactical.TacticalPos==Point2D{start.X+80,start.Y-40})) << "same-frame press/move/release is preserved";
    EXPECT_TRUE((!MouseClass::Instance.unknown_byte_554A && !view.keyboard.IsKeyPressed(2))) << "release does not leave held input";
    const auto after=view.tactical.TacticalPos;
    send(game::GameInputKind::pointer_button,1200,500,2,true);
    send(game::GameInputKind::pointer_move,650,400);
    send(game::GameInputKind::pointer_button,650,400,2,false);
    EXPECT_TRUE((view.tactical.TacticalPos==after)) << "sidebar-start drag cannot pan tactical map";
    const auto panel=radar.GetPanelBounds();
    ZBuffer depth({0,0,4,4});
    struct DepthScope {
        ZBuffer* previous;ZBuffer& depth;
        ~DepthScope(){ZBuffer::Instance=previous;depth.ReleaseSurface();}
    } depth_scope{ZBuffer::Instance,depth};
    ZBuffer::Instance=&depth;depth.MaxValue=0x6543;
    view.tactical.Redrawing=false;
    const auto redraws=unsigned(radar.Redraws);
    send(game::GameInputKind::pointer_button,panel.X+70,panel.Y+65,1,true);
    EXPECT_TRUE((GadgetClass::StuckOn==&RadarClass::RadarButton && view.tactical.TacticalPos!=after)) << "core radar gadget captures and navigates";
    EXPECT_TRUE(view.tactical.Redrawing);
    EXPECT_EQ(unsigned(radar.Redraws)&0xFFu,(redraws+1)&0xFFu);
    EXPECT_EQ(depth.MaxValue,0x8000) << "radar navigation resets original depth reference";
    send(game::GameInputKind::pointer_move,panel.X+85,panel.Y+70);
    const auto dragged=view.tactical.TacticalPos;
    send(game::GameInputKind::pointer_move,500,500);
    EXPECT_TRUE((view.tactical.TacticalPos==dragged)) << "radar drag outside content cannot scroll map";
    send(game::GameInputKind::pointer_button,500,500,1,false);
    EXPECT_TRUE((!GadgetClass::StuckOn)) << "outside radar release clears capture";
    send(game::GameInputKind::pointer_button,panel.X+70,panel.Y+65,2,true);
    EXPECT_EQ(GadgetClass::StuckOn,&RadarClass::RadarButton);
    radar.unknown_14AC=2;
    const auto closing_position=view.tactical.TacticalPos;
    send(game::GameInputKind::pointer_move,panel.X+85,panel.Y+70);
    EXPECT_EQ(GadgetClass::StuckOn,nullptr) << "closing radar retires the added drag capture";
    EXPECT_EQ(view.tactical.TacticalPos,closing_position);
    send(game::GameInputKind::pointer_button,panel.X+85,panel.Y+70,2,false);
    radar.unknown_14AC=1;
    game::GameUiInput hover_input{{panel.X+70,panel.Y+65}};
    EXPECT_TRUE(game::with_game_ui_input(hover_input,[] {
        DWORD key=0x12345678;
        EXPECT_TRUE(RadarClass::RadarButton.Action(GadgetFlag::LeftUp,&key,KeyModifier::None));
        EXPECT_EQ(key,0x12345678u) << "original radar leaves the caller's key untouched";
    }));
    const auto after_closing=view.tactical.TacticalPos;
    send(game::GameInputKind::pointer_button,500,350,2,true);
    send(game::GameInputKind::focus_lost,0,0);
    send(game::GameInputKind::pointer_move,600,350);
    EXPECT_TRUE((!GadgetClass::StuckOn && view.tactical.TacticalPos==after_closing && !view.keyboard.IsKeyPressed(2))) << "focus loss cancels input, subsequent move cannot reuse capture";
    send(game::GameInputKind::focus_gained,0,0);
    const auto before_invalid=view.tactical.TacticalPos;
    game::GameInputEvent invalid{}; invalid.x=100000;
    EXPECT_TRUE((!game::submit_game_input(view,invalid,result) && view.tactical.TacticalPos==before_invalid)) << "invalid raw events leave state intact";
    EXPECT_TRUE((game::set_game_view_size(view,1600,900))) << "resize while no capture";
    EXPECT_TRUE((RadarClass::RadarButton.X==1448 && RadarClass::RadarButton.Y==49)) << "same real radar gadget repositions";
    send(game::GameInputKind::pointer_move,1432,400);
    const auto divider=view.tactical.TacticalPos;
    EXPECT_TRUE((game::update_game_view(view,0.064) && view.tactical.TacticalPos==divider)) << "map/sidebar divider is not an outer edge";
    send(game::GameInputKind::pointer_move,0,450);
    EXPECT_TRUE((game::update_game_view(view,0.032) && view.tactical.TacticalPos.X<divider.X)) << "outer edge uses original scroll acceleration";
    // Real SelectClass derives from ControlClass; no copied EXE vtable.
    auto* select=SelectClass::Array();
    EXPECT_TRUE((select[0].GetID()==202 && dynamic_cast<ControlClass*>(select)!=nullptr)) << "normal Select control lifecycle";
    auto& sidebar=SidebarClass::Instance;
    for (int i=0;i<2;++i) {
        auto& button=SidebarClass::TabButtons[i];
        button.ID=203+i; button.ToggleType=2; button.Enable();
        button.SetPosition(1460+i*29,197); button.SetDimension(29,24);
        GScreenClass::Instance.AddButton(&button);
    }
    send(game::GameInputKind::pointer_button,1490,205,1,true);
    send(game::GameInputKind::pointer_button,1490,205,1,false);
    EXPECT_TRUE((sidebar.ActiveTabIndex==1 && SidebarClass::TabButtons[1].IsOn)) << "core Toggle release switches original active strip";
    send(game::GameInputKind::pointer_button,1465,205,1,true);
    send(game::GameInputKind::pointer_button,1000,600,1,false);
    EXPECT_TRUE((sidebar.ActiveTabIndex==1 && !GadgetClass::StuckOn)) << "release outside cannot select a tab or retain capture";
    radar.unknown_14FC=32; radar.unknown_14AC=1;
    radar.SetRadarAvailability(false);
    EXPECT_TRUE((!radar.IsAvailableNow && radar.unknown_14AC==2)) << "availability loss starts original closing state";
    // Exercise the production authority from the global constructor onwards:
    // no scoped clock replacement and no extra Start(0) on the Radar timer.
    ASSERT_TRUE(game::advance_clock(0.064));
    EXPECT_TRUE((radar.AdvanceRadarAnimation() && radar.unknown_14FC==31)) << "first close frame";
    EXPECT_FALSE(radar.AdvanceRadarAnimation()) << "animation waits for four system ticks";
    for (int i=0;i<31;++i) { ASSERT_TRUE(game::advance_clock(0.064));radar.AdvanceRadarAnimation(); }
    EXPECT_TRUE((radar.unknown_14AC==0 && radar.unknown_14FC==0)) << "closing reaches original frame zero";
    radar.SetRadarAvailability(true);
    EXPECT_TRUE((radar.unknown_14AC==3)) << "restoration starts original opening state";
    for (int i=0;i<32;++i) { ASSERT_TRUE(game::advance_clock(0.064));radar.AdvanceRadarAnimation(); }
    EXPECT_TRUE((radar.unknown_14AC==1 && radar.unknown_14FC==32)) << "opening reaches active frame 32";
    send(game::GameInputKind::pointer_leave,0,0);
    const auto left=view.tactical.TacticalPos;
    EXPECT_TRUE((game::update_game_view(view,0.1) && view.tactical.TacticalPos==left)) << "pointer leave clears edge scrolling";
    EXPECT_TRUE((game::with_map_view(view,[](void*) {
        EXPECT_TRUE((RulesClass::Instance!=nullptr)) << "map session binds its actual Rules object";
        EXPECT_TRUE((RadarEventClass::Create(RadarEventType::Combat,{80,100}))) << "create event in active core scope";
    },nullptr))) << "event operation succeeds";
    const auto radius=RadarEventClass::Array[0]->Speed;
    EXPECT_TRUE((game::update_game_view(view,0.1) && game::update_game_view(view,0.1) && RadarEventClass::Array[0]->Speed<radius)) << "core frame driver advances event animation";
    const auto frame=Unsorted::CurrentFrame;
    const auto paused_radius=RadarEventClass::Array[0]->Speed;
    view.scenario.unknown_62C=1;
    EXPECT_TRUE((game::update_game_view(view,0.1) && Unsorted::CurrentFrame==frame)) << "core pause freezes event frame timers";
    EXPECT_LT(RadarEventClass::Array[0]->Speed,paused_radius) << "original presentation animation continues during logic pause";
    view.scenario.unknown_62C=0;
    for(int i=0;i<1320;++i) EXPECT_TRUE((game::update_game_view(view,0.1))) << "native event lifetime tick";
    EXPECT_TRUE((RadarEventClass::Array.Count==0)) << "core frame driver expires and releases notifications";
}
}

TEST(GameUiInput, Contracts) {
    gadgets(); input();
}

TEST(GameUiInput, RadarCommandsAndRetainedDrag) {
    game::ResourceHandle* raw=nullptr;std::string error;
    ASSERT_TRUE(game::create_resources(std::filesystem::temp_directory_path().string(),raw,error));
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(raw,game::destroy_resources);
    game::MapViewHandle* pointer=nullptr;ASSERT_TRUE(game::create_map_view(*files,pointer));
    std::unique_ptr<game::MapViewHandle,decltype(&game::destroy_map_view)> session(pointer,game::destroy_map_view);
    auto& view=*session;auto& radar=RadarClass::Instance;
    ASSERT_TRUE(game::initialize_empty_map_view(view,{0,0,100,100},0));
    MapClass::Instance.VisibleRect={0,0,100,100};ASSERT_TRUE(radar.BuildTerrainRadar());
    view.terrain_loaded=true;radar.IsAvailableNow=true;radar.unknown_14AC=radar.unknown_14B0=1;
    ASSERT_TRUE(game::set_game_view_size(view,1280,720));ASSERT_TRUE(game::center_map_view(view,0,2200));
    HouseTypeClass country("RADAR_INPUT");HouseClass house(&country);UnitTypeClass type("RADAR_ORDERS");
    // The hover classification is the sole actor query supplied by the fixture.
    // Real radar picking, representative selection, release, Drive/MoveOrder,
    // typed mission construction and ring writes execute through host input.
    struct Unit final:UnitClass {
        using UnitClass::UnitClass;
        bool planning_allowed=false;
        Action MouseOverCell(const CellStruct*,bool,bool)const override{return Action::Move;}
        bool CanUseWaypoint() const override{return planning_allowed;}
    } unit(&type,&house);
    type.Locomotor=LocomotionClass::CLSIDs::Drive;ASSERT_TRUE(unit.InitializeLocomotor());
    type.MovementZone=MovementZone::None;unit.Location=CellClass::Cell2Coord({60,80},0);
    const auto queue=EventClass::OutList;const auto timer=TechnoClass::ActionLineTimer;
    auto* player=HouseClass::CurrentPlayer;
    const bool planning=PlanningNodeClass::PlanningModeActive,aborted=Unsorted::DragSelectAborted;
    const bool feedback=Unsorted::MoveFeedback,attack=Game::AttackMoveMode;
    const int sw=radar.CurrentSWTypeIndex;
    const auto restore=ra2::test::scope_exit([&]{
        GadgetClass::ResetInput();ObjectClass::CurrentObjects.Clear();HouseClass::CurrentPlayer=player;
        EventClass::OutList=queue;TechnoClass::ActionLineTimer=timer;Unsorted::MoveFeedback=feedback;
        PlanningNodeClass::PlanningModeActive=planning;Unsorted::DragSelectAborted=aborted;
        Game::AttackMoveMode=attack;radar.CurrentSWTypeIndex=sw;
    });
    HouseClass::CurrentPlayer=&house;PlanningNodeClass::PlanningModeActive=false;Unsorted::MoveFeedback=false;
    Game::AttackMoveMode=false;Unsorted::DragSelectAborted=false;radar.CurrentSWTypeIndex=-1;
    ASSERT_EQ(ObjectClass::CurrentObjects.Count,0);ASSERT_TRUE(ObjectClass::CurrentObjects.AddItem(&unit));
    const auto panel=radar.GetPanelBounds();const Point2D screen{panel.X+70,panel.Y+65};
    CellStruct target;TechnoClass* hit=nullptr;
    ASSERT_TRUE(radar.RadarToCell({int(radar.unknown_11F0)+70,int(radar.unknown_11F4)+65},target,hit));
    ASSERT_EQ(hit,nullptr);MapClass::Instance.GetCellAt(target)->AltFlags|=AltCellFlags::Mapped;
    const auto send=[&](game::GameInputKind kind,Point2D point,unsigned button=0,bool down=false){
        game::GameInputResult result;
        EXPECT_TRUE(game::submit_game_input(view,{kind,point.X,point.Y,button,0,down,0},result));
        EXPECT_TRUE(result.consumed);
    };
    EventClass::OutList.Init();const auto camera=view.tactical.TacticalPos;
    send(game::GameInputKind::pointer_button,screen,1,true);
    EXPECT_EQ(GadgetClass::StuckOn,nullptr);EXPECT_EQ(EventClass::OutList.Count,0);
    EXPECT_EQ(view.tactical.TacticalPos,camera) << "selected command press does not navigate";
    for(int i=0;i<4;++i)send(game::GameInputKind::pointer_move,screen);
    EXPECT_EQ(EventClass::OutList.Count,0) << "held command input never repeats a mission";
    send(game::GameInputKind::pointer_button,screen,1,false);
    ASSERT_EQ(EventClass::OutList.Count,1);auto& event=EventClass::OutList.First();
    EXPECT_EQ(event.Type,EventType::MegaMission);EXPECT_EQ(event.MegaMission.Mission,static_cast<BYTE>(Mission::Move));
    EXPECT_EQ(event.MegaMission.Destination.As_Cell(),MapClass::Instance.GetCellAt(target));
    EXPECT_EQ(view.tactical.TacticalPos,camera);EXPECT_EQ(TechnoClass::ActionLineTimer.TimeLeft,25);
    EventClass::OutList.Init();Unsorted::DragSelectAborted=true;
    send(game::GameInputKind::pointer_button,screen,1,false);EXPECT_EQ(EventClass::OutList.Count,0);
    Unsorted::DragSelectAborted=false;
    send(game::GameInputKind::pointer_button,screen,2,true);
    EXPECT_EQ(GadgetClass::StuckOn,&RadarClass::RadarButton);EXPECT_NE(view.tactical.TacticalPos,camera);
    for(int i=0;i<4;++i)send(game::GameInputKind::pointer_move,{screen.X+10,screen.Y+5});
    send(game::GameInputKind::pointer_button,{screen.X+10,screen.Y+5},2,false);
    EXPECT_EQ(GadgetClass::StuckOn,nullptr);EXPECT_EQ(EventClass::OutList.Count,0);
    EXPECT_EQ(ObjectClass::CurrentObjects.Count,1) << "radar right release does not cancel the selection";
    PlanningNodeClass::PlanningModeActive=true;
    send(game::GameInputKind::pointer_button,screen,1,true);
    EXPECT_EQ(GadgetClass::StuckOn,&RadarClass::RadarButton) << "unsupported planning action falls back to navigation";
    send(game::GameInputKind::focus_lost,{0,0});EXPECT_EQ(GadgetClass::StuckOn,nullptr);
    EXPECT_EQ(EventClass::OutList.Count,0);
    unit.planning_allowed=true;
    send(game::GameInputKind::pointer_button,screen,1,true);
    EXPECT_EQ(GadgetClass::StuckOn,nullptr);
    send(game::GameInputKind::pointer_button,screen,1,false);
    ASSERT_EQ(EventClass::OutList.Count,1);
    EXPECT_EQ(EventClass::OutList.First().Type,EventType::MegaMission);
    EXPECT_TRUE(EventClass::OutList.First().MegaMission.IsPlanningEvent);
    EXPECT_EQ(EventClass::OutList.First().MegaMission.Destination.As_Cell(),MapClass::Instance.GetCellAt(target));
    EXPECT_EQ(EventClass::OutList.First().MegaMission.Follow.As_Cell(),MapClass::Instance.GetCellAt(target));
    EXPECT_EQ(unit.PlanningToken,nullptr) << "input queues the event; nodes belong to later event execution";
    EventClass::OutList.Init();
    PlanningNodeClass::PlanningModeActive=false;ObjectClass::CurrentObjects.Clear();
    SuperWeaponTypeClass super("RADAR_SUPER_INPUT");super.Action=static_cast<Action>(38);
    radar.CurrentSWTypeIndex=2;
    send(game::GameInputKind::pointer_button,screen,1,true);
    EXPECT_EQ(GadgetClass::StuckOn,nullptr);EXPECT_EQ(EventClass::OutList.Count,0);
    send(game::GameInputKind::pointer_button,screen,1,false);
    ASSERT_EQ(EventClass::OutList.Count,1);
    EXPECT_EQ(EventClass::OutList.First().Type,EventType::SpecialPlace);
    EXPECT_EQ(EventClass::OutList.First().SpecialPlace.ID,super.ArrayIndex);
    EXPECT_EQ(EventClass::OutList.First().SpecialPlace.Location,target);
    EventClass::OutList.Init();radar.CurrentSWTypeIndex=1; // Mapped action 37 is rejected by the original radar whitelist.
    send(game::GameInputKind::pointer_button,screen,1,true);
    EXPECT_EQ(GadgetClass::StuckOn,&RadarClass::RadarButton);
    send(game::GameInputKind::pointer_button,screen,1,false);
    EXPECT_EQ(EventClass::OutList.Count,0);
}
