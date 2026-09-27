#include "support/test_support.hpp"
#include "yrpp/MouseClass.h"
#include "yrpp/WWMouseClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/Drawing.h"
#include "yrpp/HouseClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/WaypointPathClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/MixFileClass.h"
#include "api/filesystem.hpp"
#include "api/clock.hpp"
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <vector>

namespace {
// Only the OS mouse driver is observed. All Display/Mouse/House/Map methods,
// original resource decoding, palette conversion and shape painting are real.
class PaintedMouse final:public WWMouseClass {
public:
    BSurface target{96,96,2};
    std::vector<int> frames;
    Point2D hot{};
    void Draw(const Point2D& hotspot,const SHPStruct* shape,int frame) override {
        frames.push_back(frame);hot=hotspot;
        const Point2D position{48-hot.X,48-hot.Y};const RectangleStruct clip{0,0,96,96};
        target.Fill(0);
        CC_Draw_Shape(&target,FileSystem::MOUSE_PAL,const_cast<SHPStruct*>(shape),frame,
            &position,&clip,BlitterFlags(0),nullptr,0,ZGradient(-1),1000,0,nullptr,0,0,0);
    }
    bool has_pixels() {
        const auto* pixels=static_cast<const WORD*>(target.Lock(0,0));
        const bool result=std::any_of(pixels,pixels+96*96,[](WORD pixel){return pixel!=0;});
        target.Unlock();return result;
    }
};
void load_palette(const char* name,BytePalette& palette) {
    CCFileClass file(name);ASSERT_EQ(file.ReadBytes(&palette,sizeof(palette)),sizeof(palette))<<name;
    for(auto& color:palette.Entries){color.R<<=2;color.G<<=2;color.B<<=2;}
}
void exercise_cursor() {
    ASSERT_TRUE(MixFileClass::LoadTerrainMixes()); // Includes original CONQMD/CONQUER.
    BytePalette palette{},waypoint{};load_palette("MOUSEPAL.PAL",palette);load_palette("WAYPOINT.PAL",waypoint);
    ConvertClass converter(palette,palette,2,1,false);
    // YR One_Time (0x005BDF30) loads MOUSE.SHA; OpenTS uses MOUSE.SHP.
    SHPReference art("MOUSE.SHA");art.Load();ASSERT_NE(art.GetData(),nullptr);
    PaintedMouse driver;
    auto& mouse=MouseClass::Instance;
    ASSERT_EQ(ObjectClass::CurrentObjects.Count,0);
    HouseTypeClass country("CURSOR_TEST");HouseClass house(&country);
    UnitTypeClass type("CURSOR_UNIT");UnitClass unit(&type,&house);
    struct Restore {
        ConvertClass* palette=FileSystem::MOUSE_PAL;
        BytePalette waypoints=FileSystem::WAYPOINT_PAL;
        SHPStruct* shape=MouseClass::CursorShape;
        WWMouseClass* driver=WWMouseClass::Instance;
        HouseClass* player=HouseClass::CurrentPlayer;
        bool initialized=MouseClass::CursorInitialized,attack=Game::AttackMoveMode;
        byte debug=Unsorted::ArmageddonMode;
        SysTimerClass timer=MouseClass::CursorTimer;
        bool mini=MouseClass::Instance.MouseCursorIsMini,planning=MouseClass::Instance.PlanningMode;
        bool repair=mouse().RepairMode,sell=mouse().SellMode,power=mouse().PowerToggleMode,beacon=mouse().PlaceBeaconMode;
        ObjectTypeClass* pending=mouse().CurrentBuildingType;
        int command=Game::SelectionCommandMode;
        MouseCursorType index=MouseClass::Instance.MouseCursorIndex,last=MouseClass::Instance.MouseCursorLastIndex;
        int frame=MouseClass::Instance.MouseCursorCurrentFrame;
        BYTE red=mouse().WaypointColorRed,green=mouse().WaypointColorGreen,blue=mouse().WaypointColorBlue;
        static MouseClass& mouse(){return MouseClass::Instance;}
        ~Restore(){
            ObjectClass::CurrentObjects.Clear();FileSystem::MOUSE_PAL=palette;FileSystem::WAYPOINT_PAL=waypoints;
            MouseClass::CursorShape=shape;WWMouseClass::Instance=driver;HouseClass::CurrentPlayer=player;
            MouseClass::CursorInitialized=initialized;MouseClass::CursorTimer=timer;
            Game::AttackMoveMode=attack;Unsorted::ArmageddonMode=debug;
            mouse().MouseCursorIsMini=mini;mouse().MouseCursorIndex=index;mouse().MouseCursorLastIndex=last;
            mouse().MouseCursorCurrentFrame=frame;mouse().PlanningMode=planning;
            mouse().RepairMode=repair;mouse().SellMode=sell;mouse().PowerToggleMode=power;mouse().PlaceBeaconMode=beacon;
            mouse().CurrentBuildingType=pending;Game::SelectionCommandMode=command;
            mouse().WaypointColorRed=red;mouse().WaypointColorGreen=green;mouse().WaypointColorBlue=blue;
        }
    } restore;
    FileSystem::MOUSE_PAL=&converter;FileSystem::WAYPOINT_PAL=waypoint;
    MouseClass::CursorInitialized=false;MouseClass::CursorShape=nullptr;WWMouseClass::Instance=nullptr;
    EXPECT_FALSE(mouse.SetCursor(MouseCursorType::Move,true));EXPECT_FALSE(MouseClass::CursorInitialized);
    MouseClass::CursorShape=art.GetData();
    EXPECT_FALSE(mouse.UpdateCursor(MouseCursorType::Move,true));EXPECT_FALSE(MouseClass::CursorInitialized);
    MouseClass::CursorShape=art.GetData();WWMouseClass::Instance=&driver;
    HouseClass::CurrentPlayer=&house;Game::AttackMoveMode=false;Unsorted::ArmageddonMode=0;
    MouseClass::CursorInitialized=false;mouse.PlanningMode=false;
    FileSystem::MOUSE_PAL=nullptr;
    EXPECT_FALSE(mouse.ConvertAction({0,0},false,nullptr,Action::Move,true));
    EXPECT_TRUE(driver.frames.empty());
    FileSystem::MOUSE_PAL=&converter;
    mouse.WaypointColorRed=mouse.WaypointColorGreen=mouse.WaypointColorBlue=0;
    const auto ticks=SystemTimer::GetTime();
    ASSERT_TRUE(mouse.ConvertAction({0,0},false,nullptr,Action::Move,true));
    EXPECT_EQ(mouse.GetLastMouseCursor(),MouseCursorType::Move);
    const auto& move=MouseCursor::GetCursor(MouseCursorType::Move);
    EXPECT_EQ(driver.frames.back(),move.MiniFrame);EXPECT_EQ(MouseClass::CursorTimer.StartTime,ticks);
    EXPECT_EQ(MouseClass::CursorTimer.TimeLeft,move.Interval);EXPECT_TRUE(driver.has_pixels());
    EXPECT_EQ(driver.hot,Point2D(art.GetData()->Width/2,art.GetData()->Height/2));
    const auto count=driver.frames.size();
    EXPECT_FALSE(mouse.ConvertAction({0,0},false,nullptr,Action::Move,true));
    EXPECT_EQ(driver.frames.size(),count) << "unchanged action does not restart the cursor timer";
    EXPECT_TRUE(mouse.UpdateCursor(MouseCursorType::NoMove,false));
    EXPECT_EQ(mouse.GetLastMouseCursor(),MouseCursorType::Move);
    EXPECT_TRUE(mouse.RestoreCursor());EXPECT_EQ(driver.frames.back(),move.Frame);
    Game::AttackMoveMode=true;
    EXPECT_TRUE(mouse.ConvertAction({0,0},false,nullptr,Action::Move,true));
    EXPECT_EQ(mouse.GetLastMouseCursor(),MouseCursorType::AttackOutOfRange2);
    Game::AttackMoveMode=false;
    ASSERT_TRUE(ObjectClass::CurrentObjects.AddItem(&unit));type.MoveToShroud=true;
    mouse.ConvertAction({0,0},true,nullptr,Action::NoMove,true);
    EXPECT_EQ(mouse.GetLastMouseCursor(),MouseCursorType::Move);
    type.MoveToShroud=false;mouse.ConvertAction({0,0},true,nullptr,Action::NoMove,true);
    EXPECT_EQ(mouse.GetLastMouseCursor(),MouseCursorType::NoMove);
    unit.IsMouseHovering=false;
    mouse.ConvertAction({0,0},false,&unit,Action::Select,true);
    EXPECT_TRUE(unit.IsMouseHovering);EXPECT_EQ(mouse.GetLastMouseCursor(),MouseCursorType::Select);
    auto* path=house.EnsurePlanningPathExists(5);ASSERT_NE(path,nullptr);
    ASSERT_TRUE(path->Waypoints.AddItem({{40*256+128,50*256+128,0}}));
    const auto before=driver.frames.size();
    mouse.ConvertAction({40,50},true,&unit,Action::Enter,true);
    EXPECT_EQ(house.SelectedPathIndex,5);ASSERT_EQ(driver.frames.size(),before+2);
    EXPECT_EQ(mouse.GetLastMouseCursor(),MouseCursorType::Beacon);EXPECT_TRUE(driver.has_pixels());
    for(int i=0;i<8;++i) {
        const auto& color=waypoint.Entries[5*8+i];
        EXPECT_EQ(static_cast<WORD*>(converter.PaletteData)[i+1],Drawing::RGB_To_Int(color.R,color.G,color.B));
    }
    // Beacon mode uses the same initialized original cursor chain and actual
    // software painting. The OS driver remains the observed boundary above.
    ObjectClass::CurrentObjects.Clear();
    auto& manager=BeaconManagerClass::Instance;
    ASSERT_EQ(manager.AllocatedCount,0);
    const auto reset_beacons=ra2::test::scope_exit([&manager]{manager.Reset();});
    auto* beacon=GameCreate<BeaconClass>();beacon->Bitfield=3;
    manager.Beacons[house.ArrayIndex][0]=beacon;manager.AllocatedCount=1;
    mouse.CurrentBuildingType=nullptr;mouse.RepairMode=false;mouse.PlaceBeaconMode=false;
    mouse.SellMode=mouse.PowerToggleMode=mouse.PlanningMode=true;
    mouse.SetBeaconMode(1);
    EXPECT_TRUE(mouse.PlaceBeaconMode);EXPECT_FALSE(mouse.RepairMode);EXPECT_FALSE(mouse.SellMode);
    EXPECT_FALSE(mouse.PowerToggleMode);EXPECT_FALSE(mouse.PlanningMode);
    EXPECT_EQ(beacon->Bitfield,1) << "mode activation cancels beacon selection";
    EXPECT_EQ(mouse.GetLastMouseCursor(),MouseCursorType::Default);EXPECT_TRUE(driver.has_pixels());
    mouse.SetBeaconMode(-1);EXPECT_TRUE(mouse.PlaceBeaconMode) << "toggle starts from RepairMode";
    mouse.SetBeaconMode(0);EXPECT_FALSE(mouse.PlaceBeaconMode);
    for(int i=1;i<3;++i){manager.Beacons[house.ArrayIndex][i]=GameCreate<BeaconClass>();++manager.AllocatedCount;}
    mouse.SetBeaconMode(1);EXPECT_FALSE(mouse.PlaceBeaconMode) << "all three house slots occupied";

    // Release dispatch reaches real selection and the loaded original cursor.
    // Only the device-level WWMouse drawing receiver above is substituted.
    const bool type_select=Game::TypeSelectionActive,scenario=Unsorted::ScenarioStarted;
    const bool band=mouse.LeftPressAndDraggingRectangle,tentative=mouse.unknown_bool_11D0;
    const auto action_timer=TechnoClass::ActionLineTimer;
    auto* current=mouse.CurrentBuilding;
    const auto selection_restore=ra2::test::scope_exit([&]{
        unit.Deselect();unit.InLimbo=true;unit.IsOnMap=false;
        Game::TypeSelectionActive=type_select;Unsorted::ScenarioStarted=scenario;
        mouse.LeftPressAndDraggingRectangle=band;mouse.unknown_bool_11D0=tentative;
        mouse.CurrentBuilding=current;TechnoClass::ActionLineTimer=action_timer;
    });
    Game::TypeSelectionActive=false;Unsorted::ScenarioStarted=false;
    mouse.CurrentBuilding=nullptr;mouse.LeftPressAndDraggingRectangle=false;mouse.unknown_bool_11D0=true;
    type.Selectable=true;unit.IsAlive=true;unit.Health=100;unit.IsOnMap=true;unit.InLimbo=false;
    mouse.SetCursor(MouseCursorType::Move,true);Game::AttackMoveMode=true;
    mouse.LeftMouseButtonUp({},CellStruct{0,0},&unit,Action::Select,1);
    ASSERT_EQ(ObjectClass::CurrentObjects.Count,1);EXPECT_EQ(ObjectClass::CurrentObjects[0],&unit);
    EXPECT_TRUE(unit.IsSelected);EXPECT_FALSE(Game::AttackMoveMode);EXPECT_FALSE(mouse.unknown_bool_11D0);
    EXPECT_EQ(TechnoClass::ActionLineTimer.TimeLeft,25);
    EXPECT_EQ(mouse.GetLastMouseCursor(),MouseCursorType::Default);EXPECT_TRUE(driver.has_pixels());
    EXPECT_EQ(driver.frames.back(),MouseCursor::GetCursor(MouseCursorType::Default).MiniFrame);

    TacticalClass tactical({}, {},0,1,0,1,1);InputManagerClass keyboard;
    auto* old_tactical=TacticalClass::Instance;auto* old_keyboard=InputManagerClass::Instance;
    const auto candidate=TacticalClass::SelectableObjects[0];const bool aborted=Unsorted::DragSelectAborted;
    const auto band_restore=ra2::test::scope_exit([&]{
        TacticalClass::Instance=old_tactical;InputManagerClass::Instance=old_keyboard;
        TacticalClass::SelectableObjects[0]=candidate;Unsorted::DragSelectAborted=aborted;
    });
    TacticalClass::Instance=&tactical;InputManagerClass::Instance=&keyboard;
    tactical.Band={5,5,10,10};tactical.TacticalPos={0,0};tactical.SelectableCount=1;tactical.Redrawing=false;
    TacticalClass::SelectableObjects[0]={&unit,7,7};house.IsHumanPlayer=true;
    mouse.LeftPressAndDraggingRectangle=true;mouse.unknown_bool_11D0=true;
    Unsorted::DragSelectAborted=false;Game::AttackMoveMode=true;
    const auto event_count=EventClass::OutList.Count;
    mouse.LeftMouseButtonUp({},CellStruct{0,0},nullptr,Action::Move,1);
    EXPECT_TRUE(tactical.Redrawing);EXPECT_EQ(tactical.Band.Left,0);EXPECT_EQ(tactical.Band.Top,0);
    EXPECT_TRUE(unit.IsSelected);EXPECT_EQ(ObjectClass::CurrentObjects.Count,1);
    EXPECT_FALSE(mouse.LeftPressAndDraggingRectangle);EXPECT_FALSE(mouse.unknown_bool_11D0);
    EXPECT_TRUE(Unsorted::DragSelectAborted);EXPECT_FALSE(Game::AttackMoveMode);
    EXPECT_EQ(EventClass::OutList.Count,event_count)<<"completed band selection must not dispatch the supplied move action";
}
}
TEST(RadarCursor, OriginalResourcesAndNativeActionChain) {
    const char* data=std::getenv("RA2_GAME_DATA");
    if(!data || !*data)GTEST_SKIP()<<"RA2_GAME_DATA enables original cursor resource integration";
    game::ResourceHandle* raw=nullptr;std::string error;
    ASSERT_TRUE(game::create_resources(data,raw,error))<<error;
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(raw,game::destroy_resources);
    ASSERT_EQ(game::load_resources(*files,{},error),game::ResourceLoadResult::complete)<<error;
    EXPECT_TRUE(game::with_resources(*files,[](void*){exercise_cursor();},nullptr,error))<<error;
}
