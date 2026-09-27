#include "support/test_support.hpp"
#include "tooltip_platform.hpp"
#include "game_ui_runtime.hpp"
#include "map_view.hpp"
#include "map_runtime.hpp"
#include "api/clock.hpp"
#include "api/filesystem.hpp"
#include "yrpp/BitFont.h"
#include "yrpp/BitText.h"
#include "yrpp/MouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/CellClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <array>
#include <clocale>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
struct Font {
    BitFont font{""};BitFont* previous=BitFont::Instance;
    Font(){
        auto* data=GameCreate<BitFont::InternalData>();
        *data={5,1,7,9,2,8,nullptr,nullptr,2};
        data->SymbolTable=static_cast<short*>(YRMemory::Allocate(0x20000));
        std::fill_n(data->SymbolTable,0x10000,short(1));data->SymbolTable[L' ']=2;
        data->Bitmaps=static_cast<char*>(YRMemory::Allocate(16));
        data->Bitmaps[0]=5;std::memset(data->Bitmaps+1,0xF8,7);
        data->Bitmaps[8]=3;std::memset(data->Bitmaps+9,0,7);
        font.InternalPTR=data;font.field_18=1;font.field_1C=9;font.Unknown_28=16;
        BitFont::Instance=&font;
    }
    ~Font(){BitFont::Instance=previous;}
};
struct Tip : CCToolTip {
    const wchar_t* text=L"Unit";
    int calls=0;
    wchar_t* GetToolTipText(unsigned) override {++calls;return const_cast<wchar_t*>(text);}
};
struct LocalizedTips {
    CSFLabel labels[3]{{"Tip:MachineGun",1,0},{"Tip:Repair",1,1},{"Tip:Rocket",1,2}};
    wchar_t machine_gun[8]=L"机枪",repair[8]=L"维修",rocket[8]=L"火箭";
    wchar_t* values[3]{machine_gun,repair,rocket};
    CSFLabel* previous_labels=StringTable::Labels;
    wchar_t** previous_values=StringTable::Values;
    int previous_label_count=StringTable::LabelCount,previous_value_count=StringTable::ValueCount;
    LocalizedTips(){StringTable::Labels=labels;StringTable::Values=values;StringTable::LabelCount=StringTable::ValueCount=3;}
    ~LocalizedTips(){StringTable::Labels=previous_labels;StringTable::Values=previous_values;
        StringTable::LabelCount=previous_label_count;StringTable::ValueCount=previous_value_count;}
};
const std::array<std::wstring,7> texts{L"A",L"AAAA AAAA",L"A\r\nAA",L"A\tAA",std::wstring(110,L'A'),L"中 A 中",L"AA AA AA AA AA AA AA AA AA AA AA AA"};
void bounds(bool left){
    GameOptionsClass::Instance.SidebarMode=!left;
    DSurface::WindowBounds={0,0,800,480};DSurface::ViewBounds={left?168:0,0,632,448};
    DSurface::SidebarBounds={left?0:632,0,168,480};
}
struct Restore {
    RectangleStruct map=DSurface::ViewBounds,side=DSurface::SidebarBounds,window=DSurface::WindowBounds;
    bool right=GameOptionsClass::Instance.SidebarMode;
    ~Restore(){DSurface::ViewBounds=map;DSurface::SidebarBounds=side;DSurface::WindowBounds=window;GameOptionsClass::Instance.SidebarMode=right;}
};
struct Canvas {
    std::vector<WORD> pixels=std::vector<WORD>(800*480,0x1234);
    game::MapDrawingContext drawing;
    Canvas(){
        drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(this);drawing.types.backend_context=this;
        drawing.plain_palette=[](void* p,const BytePalette&,const game::DrawingPaletteHandle*& out)noexcept{
            out=reinterpret_cast<const game::DrawingPaletteHandle*>(p);return game::DrawingStatus::drawn;};
        drawing.types.backend.raster=[](void* p,const game::RasterDrawingRequest& r){
            auto& self=*static_cast<Canvas*>(p);
            for(int y=std::max({0,r.position.Y,r.clip.Y});y<std::min({480,r.position.Y+r.height,r.clip.Y+r.clip.Height});++y)
                for(int x=std::max({0,r.position.X,r.clip.X});x<std::min({800,r.position.X+r.width,r.clip.X+r.clip.Width});++x)
                    self.pixels[y*800+x]=r.color;
            return game::DrawingStatus::drawn;
        };
    }
    std::uint64_t hash()const{std::uint64_t h=1469598103934665603ull;for(auto p:pixels)h=(h^p)*1099511628211ull;return h;}
};
}
TEST(ToolTip, RegionOrderAndLifecycle){
    ToolTipManager tips(nullptr);ToolTip first,second;
    first.GadgetID=500;first.Bounds={10,20,30,40};second=first;second.GadgetID=501;
    ASSERT_TRUE(tips.Add(first));ASSERT_TRUE(tips.Add(second));EXPECT_FALSE(tips.Add(first));
    Point2D edge{40,60};ASSERT_NE(tips.FindFromPosition(edge),nullptr);
    EXPECT_EQ(tips.FindFromPosition(edge)->GadgetID,500u);
    ToolTip found;found.GadgetID=99;EXPECT_FALSE(tips.Find(999,found));EXPECT_EQ(found.GadgetID,99u);
    tips.CurrentToolTip=tips.FindFromPosition(edge);tips.CurrentToolTipData.Dimension={1,2,3,4};
    tips.Remove(500);EXPECT_FALSE(tips.IsToolTipShowing());EXPECT_EQ(tips.CurrentToolTipData.Dimension.Width,0);
    EXPECT_EQ(tips.FindFromPosition(edge)->GadgetID,501u);EXPECT_EQ(tips.GetToolTipCount(),1);
    edge.X=41;EXPECT_EQ(tips.FindFromPosition(edge),nullptr);
}
TEST(ToolTip, HoverTimerMovementClickAndLifetime){
    std::uint32_t now=0;game::ClockServices clock{&now,[](void* p)noexcept{return *static_cast<std::uint32_t*>(p);}};
    ASSERT_TRUE(game::with_clock(clock,[](void* p){
        auto& now=*static_cast<std::uint32_t*>(p);Restore restore;bounds(false);Font font;Tip tips;
        Point2D pointer{200,100};bool visible=true;game::ToolTipPlatform platform{&pointer,&visible};game::ToolTipScope scope(platform,tips);
        ToolTip region;region.GadgetID=500;region.Bounds=DSurface::ViewBounds;ASSERT_TRUE(tips.Add(region));tips.SetState(true);
        game::GameInputEvent move{};move.kind=game::GameInputKind::pointer_move;game::tooltip_input(move);
        now=999;game::tooltip_poll_timer();EXPECT_FALSE(tips.IsToolTipShowing());
        now=1000;game::tooltip_poll_timer();ASSERT_TRUE(tips.IsToolTipShowing());EXPECT_STREQ(tips.CurrentToolTipData.HelpText,L"Unit");
        tips.text=L"Replacement";tips.Draw(false);EXPECT_STREQ(tips.CurrentToolTipData.HelpText,L"Replacement");
        now=10999;game::tooltip_poll_timer();EXPECT_TRUE(tips.IsToolTipShowing());
        now=11000;game::tooltip_poll_timer();EXPECT_FALSE(tips.IsToolTipShowing());EXPECT_EQ(platform.timer_owner,nullptr);
        game::tooltip_input(move);now+=1000;game::tooltip_poll_timer();ASSERT_TRUE(tips.IsToolTipShowing());
        game::tooltip_input(move);EXPECT_FALSE(tips.IsToolTipShowing());
        game::GameInputEvent click{};click.kind=game::GameInputKind::pointer_button;click.code=1;click.pressed=true;
        game::tooltip_input(click);EXPECT_EQ(platform.timer_owner,nullptr);
        tips.SetTimerDelay(0);game::tooltip_input(move);EXPECT_TRUE(tips.IsToolTipShowing());
        tips.text=L"";tips.Draw(false);EXPECT_FALSE(tips.IsToolTipShowing());
        tips.text=L"Again";tips.SetTimerDelay(1000);now=0xFFFFFF00;game::tooltip_input(move);
        now+=1000;game::tooltip_poll_timer();EXPECT_TRUE(tips.IsToolTipShowing());
        tips.SetState(false);EXPECT_FALSE(tips.IsToolTipShowing());EXPECT_EQ(platform.timer_owner,nullptr);
        tips.SetState(true);game::tooltip_input(move);visible=false;now+=1000;game::tooltip_poll_timer();EXPECT_FALSE(tips.IsToolTipShowing());
    },&now));
}
TEST(ToolTip, OriginalLayoutAndDrawInstructionCorpus){
    Restore restore;Font font;CCToolTip tips;Point2D pointer{};bool visible=true;
    game::ToolTipPlatform platform{&pointer,&visible};game::ToolTipScope scope(platform,tips);
    game::UiResources resources;game::MapDrawStatistics statistics;Canvas canvas;
    game::GameUiFrame frame{canvas.drawing,resources,statistics};
    std::ifstream input(RA2_TOOLTIP_FIXTURE);ASSERT_TRUE(input.good());
    char kind;int left,text_id,layouts=0,draws=0;
    while(input>>kind>>left>>text_id){
        bounds(left);ToolTipManagerData data{};std::wcscpy(data.HelpText,texts[text_id].c_str());
        if(kind=='L'){
            int anchored,is_visible,ok,redraw;RectangleStruct expected;
            input>>pointer.X>>pointer.Y>>anchored>>is_visible>>ok>>expected.X>>expected.Y>>expected.Width>>expected.Height>>redraw;
            data.Dimension={pointer.X,pointer.Y,0,0};ToolTip region;region.Bounds={pointer.X,pointer.Y,50,30};region.field_18=anchored;
            tips.CurrentToolTip=&region;visible=is_visible;SidebarClass::Instance.SidebarNeedsRedraw=false;
            EXPECT_EQ(tips.Update(data),bool(ok))<<layouts;
            EXPECT_EQ(std::memcmp(&data.Dimension,&expected,sizeof(expected)),0)<<layouts<<" actual "<<data.Dimension.X<<","<<data.Dimension.Y<<","<<data.Dimension.Width<<","<<data.Dimension.Height;
            EXPECT_EQ(SidebarClass::Instance.SidebarNeedsRedraw,bool(redraw))<<layouts;
            tips.CurrentToolTip=nullptr;++layouts;
        }else{
            ASSERT_EQ(kind,'D');std::uint64_t expected;input>>data.Dimension.X>>data.Dimension.Y>>data.Dimension.Width>>data.Dimension.Height>>expected;
            canvas.pixels.assign(800*480,0x1234);tips.CurrentToolTipData=data;tips.FullRedraw=true;
            // The callback ABI is intentionally the same as production GameUiFrame.
            static CCToolTip* current;current=&tips;
            const auto status=game::with_game_ui_frame(frame,[]{current->DrawText(current->CurrentToolTipData);});
            EXPECT_TRUE(status==game::DrawingStatus::drawn||status==game::DrawingStatus::skipped);
            EXPECT_EQ(canvas.hash(),expected)<<"draw="<<draws<<" side="<<left<<" text="<<text_id<<" xy="<<data.Dimension.X<<","<<data.Dimension.Y;
            ++draws;
        }
    }
    EXPECT_EQ(layouts,280);EXPECT_EQ(draws,140);
}
TEST(ToolTip, ObjectNamesAndDisguise){
    HouseTypeClass country("TIP_COUNTRY");HouseClass owner(&country),enemy(&country);
    auto* old=HouseClass::CurrentPlayer;HouseClass::CurrentPlayer=&enemy;const auto restore=ra2::test::scope_exit([&]{HouseClass::CurrentPlayer=old;});
    UnitTypeClass vehicle("TIP_UNIT");InfantryTypeClass person("TIP_INF"),disguise("TIP_DISGUISE");
    BuildingTypeClass building("TIP_BUILDING",BuildingTypeClass::ConstructionDefaults{});AircraftTypeClass aircraft("TIP_AIRCRAFT");
    vehicle.UIName=L"IFV";person.UIName=L"Spy";disguise.UIName=L"GI";building.UIName=L"Power Plant";aircraft.UIName=L"Jet";
    UnitClass unit(&vehicle,&owner);InfantryClass infantry(&person,&owner);BuildingClass structure(&building,&owner);AircraftClass plane(&aircraft,&owner);
    EXPECT_STREQ(unit.GetUIName(),L"IFV");EXPECT_STREQ(infantry.GetUIName(),L"Spy");EXPECT_STREQ(structure.GetUIName(),L"Power Plant");EXPECT_STREQ(plane.GetUIName(),L"Jet");
    infantry.Disguised=true;infantry.Disguise=&disguise;infantry.DisguisedAsHouse=&enemy;
    EXPECT_STREQ(infantry.GetUIName(),L"GI");HouseClass::CurrentPlayer=&owner;EXPECT_STREQ(infantry.GetUIName(),L"Spy");
    infantry.Technician=true;EXPECT_STREQ(infantry.GetUIName(),StringTable::LoadString("TXT_TECHNICIAN"));infantry.Technician=false;
    vehicle.TurretCount=1;vehicle.HasTurretTooltips=true;unit.Passengers.FirstPassenger=&infantry;
    const auto detach=ra2::test::scope_exit([&]{unit.Passengers.FirstPassenger=nullptr;});
    person.UseOwnName=true;unit.CurrentTurretNumber=1;unit.CurrentWeaponNumber=2;EXPECT_STREQ(unit.GetUIName(),L"Spy IFV");
    unit.CurrentTurretNumber=2;unit.CurrentWeaponNumber=1;const std::wstring repair=std::wstring(StringTable::LoadString("Tip:Repair"))+L" IFV";EXPECT_STREQ(unit.GetUIName(),repair.c_str());
}
TEST(ToolTip, LocalizedIFVNamesInCLocale){
    // The host need not install a locale. Darwin swprintf("%ls") otherwise
    // rejects Chinese with EILSEQ, although CSF and BitFont accept it.
    const std::string previous_locale=std::setlocale(LC_CTYPE,nullptr);
    const auto restore_locale=ra2::test::scope_exit([&]{std::setlocale(LC_CTYPE,previous_locale.c_str());});
    ASSERT_NE(std::setlocale(LC_CTYPE,"C"),nullptr);
    LocalizedTips strings;
    HouseTypeClass country("TIP_IFV_COUNTRY");HouseClass owner(&country);
    UnitTypeClass vehicle("FV");InfantryTypeClass person("TIP_PASSENGER");
    vehicle.UIName=L"多功能步兵车";vehicle.TurretCount=4;vehicle.HasTurretTooltips=true;
    person.UIName=L"间谍";
    UnitClass unit(&vehicle,&owner);InfantryClass passenger(&person,&owner);
    EXPECT_STREQ(unit.GetUIName(),L"火箭 多功能步兵车");
    unit.Passengers.FirstPassenger=&passenger;
    const auto detach=ra2::test::scope_exit([&]{unit.Passengers.FirstPassenger=nullptr;});
    // Retail FV mappings: GI weapon 2 -> turret 1; engineer weapon 1 -> turret 2.
    vehicle.TurretWeapon[2]=1;person.IFVMode=2;unit.ReceiveGunner(&passenger);
    EXPECT_STREQ(unit.GetUIName(),L"机枪 多功能步兵车");
    vehicle.TurretWeapon[1]=2;person.IFVMode=1;unit.ReceiveGunner(&passenger);
    EXPECT_STREQ(unit.GetUIName(),L"维修 多功能步兵车");
    person.UseOwnName=true;person.IFVMode=2;unit.ReceiveGunner(&passenger);
    EXPECT_STREQ(unit.GetUIName(),L"间谍 多功能步兵车");
    person.UseOwnName=false;vehicle.TurretWeapon[5]=3;person.IFVMode=5;unit.ReceiveGunner(&passenger);
    EXPECT_STREQ(unit.GetUIName(),L"间谍 多功能步兵车");
    // A replacement name must terminate inside the original fixed buffer.
    const std::wstring long_name(300,L'中');vehicle.UIName=long_name.c_str();
    EXPECT_EQ(std::wcslen(unit.GetUIName()),0xFFu);
    vehicle.UIName=L"IFV";unit.RemoveGunner(&passenger);unit.Passengers.FirstPassenger=nullptr;
    EXPECT_STREQ(unit.GetUIName(),L"火箭 IFV");
}
TEST(ToolTip, SessionRegistrationResetAndIsolation){
    game::ResourceHandle* files=nullptr;std::string error;
    ASSERT_TRUE(game::create_resources(std::filesystem::temp_directory_path().string(),files,error));
    const auto cleanup=ra2::test::scope_exit([&]{game::destroy_resources(files);});
    game::MapViewHandle* view=nullptr;ASSERT_TRUE(game::create_map_view(*files,view));
    const auto close=ra2::test::scope_exit([&]{game::destroy_map_view(view);});
    ASSERT_TRUE(game::initialize_empty_map_view(*view,{0,0,64,64},0));view->terrain_loaded=true;
    ASSERT_TRUE(game::set_game_view_size(*view,800,600));ToolTip region;ASSERT_TRUE(view->tooltips.Find(500,region));
    EXPECT_EQ(std::memcmp(&region.Bounds,&DSurface::ViewBounds,sizeof(region.Bounds)),0);EXPECT_EQ(CCToolTip::Instance,nullptr);
    Point2D hover{200,100};view->tooltips.CurrentToolTip=view->tooltips.FindFromPosition(hover);
    auto* original_region=view->tooltips.CurrentToolTip;ASSERT_NE(original_region,nullptr);
    ASSERT_TRUE(game::set_game_view_size(*view,800,600));EXPECT_EQ(view->tooltips.CurrentToolTip,original_region);
    game::GameInputResult result;ASSERT_TRUE(game::submit_game_input(*view,{game::GameInputKind::pointer_move,200,100},result));
    EXPECT_EQ(view->tooltip_platform.timer_owner,&view->tooltips);
    ASSERT_TRUE(game::submit_game_input(*view,{game::GameInputKind::focus_lost},result));EXPECT_EQ(view->tooltip_platform.timer_owner,nullptr);
    ASSERT_TRUE(game::set_game_view_size(*view,1024,768));ASSERT_TRUE(view->tooltips.Find(500,region));EXPECT_EQ(region.Bounds.Width,856);
}

TEST(ToolTip, WorldObjectQueryVisibilityAndRemoval){
    game::ResourceHandle* files=nullptr;std::string error;
    ASSERT_TRUE(game::create_resources(std::filesystem::temp_directory_path().string(),files,error));
    const auto cleanup=ra2::test::scope_exit([&]{game::destroy_resources(files);});
    game::MapViewHandle* view=nullptr;ASSERT_TRUE(game::create_map_view(*files,view));
    const auto close=ra2::test::scope_exit([&]{game::destroy_map_view(view);});
    ASSERT_TRUE(game::initialize_empty_map_view(*view,{0,0,64,64},0));view->terrain_loaded=true;
    ASSERT_TRUE(game::set_game_view_size(*view,800,600));
    view->pointer={316,284};view->pointer_inside=true;
    const auto center=TacticalClass::CoordsToScreen({64*256+128,64*256+128,0});
    view->tactical.TacticalPos={center.X-view->pointer.X,center.Y-view->pointer.Y};
    ASSERT_TRUE(game::with_map_view(*view,[](void* p){
        auto& view=*static_cast<game::MapViewHandle*>(p);
        auto runtime=game::map_runtime();runtime.has_window=[]()noexcept{return true;};
        ASSERT_TRUE(game::with_map_runtime(runtime,[](void* p){
            auto& view=*static_cast<game::MapViewHandle*>(p);Font font;
            HouseTypeClass country("TIP_QUERY_COUNTRY");HouseClass owner(&country),player(&country);
            auto* previous=HouseClass::CurrentPlayer;HouseClass::CurrentPlayer=&player;
            const auto restore=ra2::test::scope_exit([&]{HouseClass::CurrentPlayer=previous;});
            UnitTypeClass type("TIP_QUERY_UNIT");type.UIName=L"Tank";type.Strength=100;
            UnitClass unit(&type,&owner);
            CellStruct cell;ASSERT_TRUE(view.tactical.PickTerrainCell(view.pointer,DSurface::ViewBounds,cell));
            auto* tile=MapClass::Instance.TryGetCellAt(cell);ASSERT_NE(tile,nullptr);tile->GetCellCoords(&unit.Location);tile->AltFlags|=AltCellFlags::Clear;
            unit.InLimbo=false;unit.IsAlive=true;unit.IsOwnedByCurrentPlayer=false;
            view.tactical.SelectableObjects[0]={&unit,view.pointer.X+view.tactical.TacticalPos.X,view.pointer.Y+view.tactical.TacticalPos.Y};
            view.tactical.SelectableCount=1;
            const auto detach=ra2::test::scope_exit([&]{view.tactical.SelectableCount=0;unit.InLimbo=true;});
            Point2D cursor;ASSERT_TRUE(game::tooltip_pointer(cursor));EXPECT_EQ(cursor,view.pointer);
            CellStruct picked_cell;CoordStruct picked_coord;ObjectClass* picked=nullptr;
            ASSERT_TRUE(game::tooltip_pick(view.pointer,picked_cell,picked_coord,picked));EXPECT_EQ(picked,&unit);
            EXPECT_STREQ(unit.GetUIName(),L"Tank");
            EXPECT_STREQ(DisplayClass::Instance.DisplayClass::GetToolTip(500),L"Tank");
            EXPECT_STREQ(CCToolTip::Instance->GetToolTipText(500),L"Tank");
            type.Invisible=true;EXPECT_EQ(CCToolTip::Instance->GetToolTipText(500),nullptr);type.Invisible=false;
            unit.CloakState=CloakState::Cloaked;EXPECT_EQ(CCToolTip::Instance->GetToolTipText(500),nullptr);
            tile->Sensors_AddOfHouse(player.ArrayIndex);EXPECT_STREQ(CCToolTip::Instance->GetToolTipText(500),L"Tank");
            tile->Sensors_RemOfHouse(player.ArrayIndex);unit.CloakState=CloakState::Uncloaked;
            tile->AltFlags&=~AltCellFlags::Clear;EXPECT_STREQ(CCToolTip::Instance->GetToolTipText(500),StringTable::LoadString("TXT_SHADOW"));
            tile->AltFlags|=AltCellFlags::Clear;
            Unsorted::UserInputLocked=true;EXPECT_EQ(CCToolTip::Instance->GetToolTipText(500),nullptr);Unsorted::UserInputLocked=false;
            view.tactical.field_D8=1;EXPECT_EQ(CCToolTip::Instance->GetToolTipText(500),nullptr);view.tactical.field_D8=0;
            EXPECT_EQ(CCToolTip::Instance->GetToolTipText(499),nullptr);
            ToolTip region;region.GadgetID=500;region.Bounds=DSurface::ViewBounds;
            view.tooltips.CurrentToolTip=view.tooltips.FindFromPosition(view.pointer);view.tooltips.CurrentMousePosition=view.pointer;
            ASSERT_TRUE(view.tooltips.Process());
            // Reach the actual hover manager with a localized, empty IFV;
            // a successful direct GetUIName alone does not prove tip delivery.
            LocalizedTips strings;
            type.UIName=L"多功能步兵车";type.TurretCount=4;type.HasTurretTooltips=true;
            EXPECT_STREQ(CCToolTip::Instance->GetToolTipText(500),L"火箭 多功能步兵车");
            ASSERT_TRUE(view.tooltips.Process());
            EXPECT_STREQ(view.tooltips.CurrentToolTipData.HelpText,L"火箭 多功能步兵车");
            view.tactical.SelectableCount=0;ASSERT_FALSE(view.tooltips.Process());EXPECT_EQ(view.tooltips.CurrentToolTip,nullptr);
        },p));
    },view));
}
