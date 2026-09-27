#include "support/test_support.hpp"
#include "yrpp/RadarClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/Surface.h"
#include "yrpp/ShapeButtonClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/CCToolTip.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/MouseClass.h"
#include "api/scenario_runtime.hpp"
#include "scenario_runtime.hpp"
#include "api/filesystem.hpp"
#include "yrpp/ScenarioClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/FPSCounter.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WaypointPathClass.h"
#include "yrpp/Notifications.h"
#include "yrpp/EventClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/LocomotionClass.h"
#include "game_ui_runtime.hpp"
#include "map_world.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <memory>

namespace {

void YRPP_FASTCALL record_selection_callback(ObjectClass* object) noexcept {--object->Health;}

void read_geometry() {
    auto& radar=RadarClass::Instance;
    std::ifstream input(RA2_RADAR_FIXTURE);
    EXPECT_TRUE((bool(input))) << "open original radar fixture";
    unsigned count=0; char kind;
    while (input>>kind) {
        if (kind=='S') {
            int w,h; Point2D expected,size; std::uint32_t bits; float factor=0;
            input>>w>>h>>expected.X>>expected.Y>>bits;
            EXPECT_TRUE((RadarClass::FitTerrainRadar(w,h,size,factor) && size==expected &&
                std::bit_cast<std::uint32_t>(factor)==bits)) << "original 140x108 aspect fit and float store";
        } else {
            std::uint32_t bits; int offset,origin;
            auto& rect=radar.unknown_rect_149C;
            input>>bits>>offset>>origin>>rect.X>>rect.Y>>rect.Width>>rect.Height;
            radar.RadarSizeFactor=std::bit_cast<float>(bits);
            radar.unknown_1490=static_cast<DWORD>(offset); radar.unknown_1498=static_cast<DWORD>(origin);
            if (kind=='P') {
                CoordStruct world; Point2D expected,actual; int restrict;
                input>>world.X>>world.Y>>world.Z>>restrict>>expected.X>>expected.Y;
                EXPECT_TRUE((radar.GetCrdOnRadar(&actual,&world,restrict) && actual==expected)) << "original world to radar projection";
            } else if (kind=='I') {
                Point2D point; CellStruct expected,actual;
                input>>point.X>>point.Y>>expected.X>>expected.Y;
                EXPECT_TRUE((radar.RadarToTerrainCell(point,actual) && actual==expected)) << "original radar inverse terrain branch";
                if (point.X>=rect.X && point.Y>=rect.Y && point.X<rect.X+rect.Width && point.Y<rect.Y+rect.Height) {
                    TechnoClass* picked=nullptr;
                    EXPECT_TRUE((radar.RadarToCell(point,actual,picked) && !picked && actual==expected)) << "full radar picker retains original terrain inverse with empty tracking table";
                }
            } else if (kind=='F') {
                CellStruct center; Point2D viewport; RectangleStruct expected;
                input>>center.X>>center.Y>>viewport.X>>viewport.Y>>expected.X>>expected.Y>>expected.Width>>expected.Height;
                EXPECT_TRUE((radar.UpdateViewportFrame(center,viewport))) << "viewport frame accepts valid input";
                const auto& actual=radar.unknown_rect_14DC;
                if (std::memcmp(&actual,&expected,sizeof(actual)))
                    std::cerr<<"frame case "<<count<<" expected "<<expected.X<<','<<expected.Y<<','<<expected.Width<<','<<expected.Height
                        <<" actual "<<actual.X<<','<<actual.Y<<','<<actual.Width<<','<<actual.Height<<'\n';
                EXPECT_TRUE((!std::memcmp(&actual,&expected,sizeof(actual)))) << "original viewport frame projection and clamping";
            } else EXPECT_TRUE((false)) << "known radar fixture kind";
        }
        EXPECT_TRUE((bool(input))) << "complete original radar record"; ++count;
    }
    EXPECT_TRUE((input.eof() && count==2499)) << "consume all original radar records";
    radar.ReleaseTerrainRadar();
    std::cout<<count<<" original radar geometry cases passed\n";
}
void read_pixels() {
    static_assert(sizeof(ColorStruct)==3);
    std::ifstream input(RA2_RADAR_PIXELS,std::ios::binary);
    char magic[4]; std::uint32_t count;
    input.read(magic,4); input.read(reinterpret_cast<char*>(&count),4);
    EXPECT_TRUE((bool(input) && !std::memcmp(magic,"RDR1",4) && count==8)) << "read original radar image header";
    unsigned total=0;
    for (unsigned i=0;i<count;++i) {
        std::uint32_t header[4]; input.read(reinterpret_cast<char*>(header),sizeof(header));
        EXPECT_TRUE((bool(input) && header[0]<=2048 && header[1]<=2048 && header[2]<=140 && header[3]<=108)) << "valid radar image dimensions";
        std::vector<ColorStruct> source(header[0]*header[1]);
        std::vector<WORD> expected(header[2]*header[3]),actual(expected.size(),0x5555);
        input.read(reinterpret_cast<char*>(source.data()),source.size()*sizeof(ColorStruct));
        input.read(reinterpret_cast<char*>(expected.data()),expected.size()*sizeof(WORD));
        EXPECT_TRUE((bool(input))) << "read full original radar image";
        EXPECT_TRUE((!RadarClass::ResampleTerrainRadar(source.data(),source.size()-1,header[0],header[1],actual.data(),actual.size()) &&
            actual[0]==0x5555)) << "short source preserves output";
        EXPECT_TRUE((RadarClass::ResampleTerrainRadar(source.data(),source.size(),header[0],header[1],actual.data(),actual.size()))) << "resample valid RGB terrain";
        unsigned differences=0;
        for (std::size_t n=0;n<actual.size();++n) if (actual[n]!=expected[n]) {
            if (differences<4) std::cerr<<"image "<<i<<" at "<<n<<" expected "<<expected[n]<<" actual "<<actual[n]<<'\n';
            ++differences;
        }
        if (differences) std::cerr<<"image "<<i<<" differences "<<differences<<'\n';
        EXPECT_TRUE((!differences)) << "original radar area resampling pixels"; total+=actual.size();
    }
    EXPECT_TRUE((input.peek()==std::ifstream::traits_type::eof())) << "no untested image bytes";
    std::cout<<count<<" original radar images, "<<total<<" RGB565 pixels passed\n";
}
void read_world() {
    auto& map=MapClass::Instance; auto& radar=RadarClass::Instance;
    TacticalClass tactical({}, {},0,1,0,1,1);
    std::ifstream input(RA2_RADAR_WORLD);
    EXPECT_TRUE((bool(input))) << "open original radar world fixture";
    char kind; unsigned count=0;
    while (input>>kind) {
        if (kind=='E') {
            int width,height,pattern; RectangleStruct visible;
            input>>width>>height>>visible.X>>visible.Y>>visible.Width>>visible.Height>>pattern;
            radar.ReleaseTerrainRadar(); map.MaxLevel=13;
            EXPECT_TRUE((map.CreateEmptyCells({0,0,width,height},0))) << "create real cell data for oracle";
            map.VisibleRect=visible;
            for (int n=0;n<map.Cells.Capacity;++n) if (auto* cell=map.Cells.Items[n]) {
                const auto c=cell->MapCoords;
                cell->Level=pattern==0 ? 0 : pattern==1 ? 3 : (c.X*7+c.Y*3)%14;
                cell->SlopeIndex=pattern==2 && (c.X+c.Y)%3==0 ? 2 : 0;
            }
        } else if (kind=='B') {
            int offset,bound,origin; RectangleStruct raw; Point2D size; float factor;
            input>>offset>>bound>>origin>>raw.X>>raw.Y>>raw.Width>>raw.Height;
            EXPECT_TRUE((radar.BuildTerrainRadar())) << "build original terrain radar buffers";
            EXPECT_TRUE((RadarClass::FitTerrainRadar(raw.Width,raw.Height,size,factor))) << "fit oracle terrain bounds";
            EXPECT_TRUE((radar.unknown_1490==static_cast<DWORD>(offset) && radar.unknown_1494==static_cast<DWORD>(bound) &&
                radar.unknown_1498==static_cast<DWORD>(origin) && radar.unknown_1240==unsigned(raw.Width) &&
                radar.unknown_1244==unsigned(raw.Height) && radar.RadarSizeFactor==factor &&
                radar.unknown_rect_149C.Width==size.X && radar.unknown_rect_149C.Height==size.Y)) << "original height-aware radar bounds";
        } else if (kind=='U') {
            CellStruct point; int level,expected; input>>point.X>>point.Y>>level>>expected;
            EXPECT_TRUE((map.IsWithinUsableArea(point,level)==bool(expected))) << "original height and slope usable area";
        } else if (kind=='T') {
            RectangleStruct viewport; Point2D point; CellStruct expected,actual;
            input>>tactical.TacticalPos.X>>tactical.TacticalPos.Y>>viewport.X>>viewport.Y>>viewport.Width>>viewport.Height
                >>point.X>>point.Y>>expected.X>>expected.Y;
            EXPECT_TRUE((tactical.PickTerrainCell(point,viewport,actual) && actual==expected)) << "original terrain scan selects the same cell";
            TacticalClass::ViewBounds=viewport;
            if(viewport.X==0 && viewport.Y==0 && point.X<viewport.Width && point.Y<viewport.Height) {
                CellStruct via_radar;
                EXPECT_EQ(radar.vt_entry_CC(&via_radar,&point),&via_radar);
                EXPECT_EQ(via_radar,expected) << "original radar virtual pixel-to-cell wrapper";
            }
        } else EXPECT_TRUE((false)) << "known radar world record";
        EXPECT_TRUE((bool(input))) << "complete radar world record"; ++count;
    }
    EXPECT_TRUE((input.eof() && count==2592)) << "consume all original world radar cases";
    radar.ReleaseTerrainRadar(); map.ReleaseCellStorage();TacticalClass::ViewBounds={};
    std::cout<<count<<" original radar world cases passed\n";
}
void tracking_storage() {
    auto& radar=RadarClass::Instance;
    radar.InitRadar();
    radar.unknown_rect_149C={0,0,140,108};
    auto* table=radar.unknown_1258;
    EXPECT_TRUE((table && table->BucketCount==256)) << "original tracking hash storage";
    // Opaque identities only: never dereferenced or used as constructed Techno
    // objects. This tests record lifetime/collisions, not entity lifecycle.
    int identity[2]{};
    auto* a=reinterpret_cast<TechnoClass*>(&identity[0]);
    auto* b=reinterpret_cast<TechnoClass*>(&identity[1]);
    const RadarTrackingStruct first{a,10,10},collision{a,15,11},other{b,10,10};
    const unsigned slot=table->BucketHashFunction(first)&255u;
    EXPECT_TRUE((slot==(table->BucketHashFunction(collision)&255u))) << "original 251Y+X collision";
    auto& entries=table->Buckets[slot];
    EXPECT_TRUE((entries.AddItem({first,a}) && entries.AddItem({collision,a}) && entries.AddItem({other,b}))) << "store original pointer records";
    EXPECT_TRUE((!radar.UntrackObject(a,9,10) && entries.Count==3)) << "different coordinates retain all records";
    EXPECT_TRUE((radar.UntrackObject(a,10,10) && entries.Count==2 && entries[0].Key==collision && entries[1].Key==other)) << "remove exact key and pointer while preserving bucket order";
    EXPECT_TRUE((!radar.UntrackObject(a,10,10))) << "duplicate removal has no effect";
    EXPECT_TRUE((radar.UntrackObject(b,10,10) && entries.Count==1 && entries[0].Key==collision)) << "same pixel retains another object's record";
    EXPECT_TRUE((!radar.TrackObject(nullptr,0,0))) << "null tracking object rejected";
    radar.ReleaseTerrainRadar();
    EXPECT_EQ(radar.unknown_1258,nullptr) << "map unload releases the original tracking table";
}
}

TEST(MapRadar, Contracts) {
    read_geometry(); read_pixels(); read_world(); tracking_storage();
}

TEST(MapRadar, CacheOwnershipCoordinatesAndRebuild) {
    auto& map=MapClass::Instance; auto& radar=RadarClass::Instance;
    struct Cleanup { ~Cleanup(){RadarClass::Instance.ReleaseTerrainRadar();MapClass::Instance.ReleaseCellStorage();} } cleanup;
    ASSERT_TRUE(map.CreateEmptyCells({0,0,100,100},0));
    map.VisibleRect={0,0,100,100};
    radar.unknown_11F0=16; radar.unknown_11F4=49;
    ASSERT_TRUE(radar.BuildTerrainRadar());
    const auto rect=radar.unknown_rect_149C;
    EXPECT_EQ(rect.X,16+(140-rect.Width)/2);
    EXPECT_EQ(rect.Y,49+(108-rect.Height)/2);
    ASSERT_NE(radar.unknown_1220,nullptr);
    std::array<WORD,140*108> first{},second{};
    ASSERT_TRUE(radar.CopyTerrainRadar(first.data(),first.size()));
    second.fill(0x5555);
    EXPECT_FALSE(radar.CopyTerrainRadar(second.data(),rect.Width*rect.Height-1));
    EXPECT_EQ(second[0],0x5555);
    const CellStruct cell{80,100};
    const int x=cell.X+std::bit_cast<int>(radar.unknown_1490)-cell.Y;
    const int y=cell.X+cell.Y-std::bit_cast<int>(radar.unknown_1498);
    radar.unknown_123C[y*radar.unknown_1240+x]={0,0,0};
    second.fill(0);
    ASSERT_TRUE(radar.CopyTerrainRadar(second.data(),second.size()));
    EXPECT_EQ(first,second) << "presentation reads the cached background without resampling RGB";
    ASSERT_TRUE(radar.MarkTerrainCellDirty(cell));
    bool changed=false;
    ASSERT_TRUE(radar.UpdateTerrainRadar(changed)); ASSERT_TRUE(changed);
    ASSERT_TRUE(radar.CopyTerrainRadar(second.data(),second.size()));
    EXPECT_EQ(first,second) << "canonical cell update rebuilds the original background pixels";
    auto* surface=radar.unknown_1220;
    ASSERT_TRUE(radar.UpdateTerrainRadar(changed)); EXPECT_FALSE(changed);
    EXPECT_EQ(surface,radar.unknown_1220) << "unchanged terrain retains its surface";
    const auto width=radar.unknown_1240;
    radar.unknown_1240=0;
    EXPECT_FALSE(radar.RebuildTerrainRadarCache());
    EXPECT_EQ(radar.unknown_1220,nullptr);
    radar.unknown_1240=width;
    ASSERT_TRUE(radar.UpdateTerrainRadar(changed));
    EXPECT_NE(radar.unknown_1220,nullptr) << "missing cache is retried without a second terrain change";
    BuildingTypeClass type("RADAR_REBUILD",BuildingTypeClass::ConstructionDefaults{});
    BuildingClass building(&type,nullptr);
    ASSERT_TRUE(radar.TrackObject(&building,2,3));
    building.IsRadarTracked=true;
    ASSERT_TRUE(radar.BuildTerrainRadar());
    EXPECT_FALSE(building.IsRadarTracked) << "rebuilding permits live objects to register again";
    for(int i=0;i<radar.unknown_1258->BucketCount;++i) EXPECT_EQ(radar.unknown_1258->Buckets[i].Count,0);
    Point2D point{2,3}; radar.RefreshCrd(&point);
    const auto timer=radar.unknown_timer_1500;
    radar.ClearRadar();
    EXPECT_EQ(radar.unknown_121C,nullptr); EXPECT_EQ(radar.unknown_1220,nullptr);
    EXPECT_EQ(radar.unknown_123C,nullptr); EXPECT_EQ(radar.unknown_1258,nullptr); EXPECT_EQ(radar.unknown_1274,nullptr);
    EXPECT_EQ(radar.unknown_points_125C.Count,1) << "ClearRadar frees resources without broadening its original write set";
    EXPECT_EQ(radar.unknown_rect_149C.X,rect.X);
    EXPECT_EQ(radar.unknown_timer_1500.StartTime,timer.StartTime);
    EXPECT_EQ(radar.unknown_timer_1500.TimeLeft,timer.TimeLeft);
    radar.ClearRadar(); // Idempotent resource release.
}

TEST(MapRadar, FailedPresentationKeepsInvalidations) {
    auto& map=MapClass::Instance; auto& radar=RadarClass::Instance;
    struct Cleanup { ~Cleanup(){RadarClass::Instance.ReleaseTerrainRadar();MapClass::Instance.ReleaseCellStorage();} } cleanup;
    ASSERT_TRUE(map.CreateEmptyCells({0,0,100,100},0)); map.VisibleRect={0,0,100,100};
    ASSERT_TRUE(radar.BuildTerrainRadar());
    Point2D point{2,3}; radar.RefreshCrd(&point);
    game::MapDrawingContext drawing;
    drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(&drawing);
    DSurface::WindowBounds={0,0,1280,720};
    DSurface::SidebarBounds={1112,0,168,720};
    drawing.plain_palette=[](void*,const BytePalette&,const game::DrawingPaletteHandle*&) noexcept {
        return game::DrawingStatus::drawn;
    };
    drawing.types.backend.raster=[](void*,const game::RasterDrawingRequest&) {return game::DrawingStatus::backend_failure;};
    game::UiResources resources; game::MapDrawStatistics statistics;
    game::GameUiFrame frame{drawing,resources,statistics};
    EXPECT_EQ(game::with_game_ui_frame(frame,[]{EXPECT_FALSE(RadarClass::Instance.RenderRadar());}),
        game::DrawingStatus::backend_failure);
    EXPECT_TRUE(radar.unknown_bool_14D9); EXPECT_TRUE(radar.unknown_bool_14DA);
    EXPECT_EQ(radar.unknown_points_125C.Count,1);
    frame.status=game::DrawingStatus::skipped;
    EXPECT_EQ(game::with_game_ui_frame(frame,[]{RadarClass::Instance.RadarClass::Draw(0);}),
        game::DrawingStatus::unavailable) << "missing art reports failure without dereferencing null resources";
    EXPECT_EQ(radar.unknown_points_125C.Count,1);
}

TEST(MapRadar, OriginalButtonsAndArtOwnership) {
    auto& radar=RadarClass::Instance;
    struct Cleanup {~Cleanup(){
        auto& r=RadarClass::Instance;r.SetRadarButtonsVisible(false);
        RadarClass::DiplomacyButton.SetShape(nullptr,0,0);RadarClass::OptionsButton.SetShape(nullptr,0,0);
        r.DisposeOfArt();GadgetClass::ResetInput();
    }} cleanup;
    radar.SetRadarButtonsVisible(false);radar.DisposeOfArt();
    SHPStruct diplomacy,options;diplomacy.Width=72;diplomacy.Height=18;options.Width=68;options.Height=18;
    RadarClass::DiplomacyShape=&diplomacy;RadarClass::OptionsShape=&options;
    RadarClass::DiplomacyBounds={11,20,72,18};RadarClass::OptionsBounds={83,20,68,18};
    Unsorted::ArmageddonMode=false;
    radar.Init_IO();
    EXPECT_EQ(RadarClass::DiplomacyButton.ID,242);EXPECT_EQ(RadarClass::OptionsButton.ID,243);
    EXPECT_EQ(RadarClass::DiplomacyButton.Flags,static_cast<GadgetFlag>(5));
    EXPECT_EQ(RadarClass::DiplomacyButton.ShapeData,&diplomacy);
    EXPECT_EQ(RadarClass::OptionsButton.ShapeData,&options);
    radar.SetRadarButtonsVisible(true);radar.SetRadarButtonsVisible(true);
    int diplomacy_count=0,options_count=0;
    for(auto* button=GScreenClass::Buttons;button;button=button->GetNext()) {
        diplomacy_count+=button==&RadarClass::DiplomacyButton;options_count+=button==&RadarClass::OptionsButton;
    }
    EXPECT_EQ(diplomacy_count,1);EXPECT_EQ(options_count,1);
    ASSERT_TRUE(game::with_game_ui_input({{20,25}},[]{
        DWORD key=0;
        RadarClass::DiplomacyButton.Action(GadgetFlag::LeftPress,&key,KeyModifier::None);
        EXPECT_EQ(key,0u);EXPECT_EQ(GadgetClass::StuckOn,&RadarClass::DiplomacyButton);
        RadarClass::DiplomacyButton.Action(GadgetFlag::LeftRelease,&key,KeyModifier::None);
        EXPECT_EQ(key,0x80F2u);EXPECT_EQ(GadgetClass::StuckOn,nullptr);
    }));
    {
        CCToolTip tips;
        auto* old_tips=CCToolTip::Instance;
        const auto old_bounds=TacticalClass::ViewBounds;
        const bool old_side=GameOptionsClass::Instance.SidebarMode;
        struct Restore { CCToolTip* tips;RectangleStruct bounds;bool side;
            ~Restore(){CCToolTip::Instance=tips;TacticalClass::ViewBounds=bounds;GameOptionsClass::Instance.SidebarMode=side;}
        } restore{old_tips,old_bounds,old_side};
        CCToolTip::Instance=&tips;TacticalClass::ViewBounds.Width=1112;
        GameOptionsClass::Instance.SidebarMode=true;
        auto runtime=game::default_scenario_runtime();
        SessionClass session{};
        game::ScenarioHouseServices houses;houses.session=&session;runtime.houses=&houses;
        runtime.session_mode=[](void*) noexcept {return 0;};
        ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void*) {
            RadarClass::Instance.InitGUI();RadarClass::Instance.InitGUI();
        },nullptr));
        EXPECT_EQ(tips.GetToolTipCount(),2);
        struct KeyCheck {SessionClass& session;} key_check{session};
        ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void* p) {
            auto& session=static_cast<KeyCheck*>(p)->session;
            for(auto mode:{GameMode::Campaign,GameMode::Skirmish,GameMode::LAN,GameMode::Internet}){
                session.GameMode=mode;
                Game::SpecialDialog=42;
                TabClass::Instance.ProcessButtonKey(0x80F2);
                EXPECT_EQ(Game::SpecialDialog,mode==GameMode::Campaign?9:8);
                EXPECT_EQ(Unsorted::SpecialDialog,Game::SpecialDialog);
                TabClass::Instance.ProcessButtonKey(0x80F3);EXPECT_EQ(Game::SpecialDialog,1);
                TabClass::Instance.ProcessButtonKey(0);EXPECT_EQ(Game::SpecialDialog,1);
            }
            Game::SpecialDialog=0;
        },&key_check));
        ToolTip tip;
        ASSERT_TRUE(tips.Find(242,tip));
        EXPECT_STREQ(tip.Text,"Tip:BriefingButton");EXPECT_EQ(tip.Bounds.X,1123);
        EXPECT_EQ(tip.Bounds.Width,72);EXPECT_FALSE(tip.field_18);
        EXPECT_EQ(RadarClass::DiplomacyButton.DrawPosition.X,-1112);
        GameOptionsClass::Instance.SidebarMode=false;
        runtime.session_mode=[](void*) noexcept {return 5;};
        ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void*){RadarClass::Instance.InitGUI();},nullptr));
        ASSERT_TRUE(tips.Find(242,tip));
        EXPECT_STREQ(tip.Text,"Tip:DiplomacyButton");EXPECT_EQ(tip.Bounds.X,11);
        ASSERT_TRUE(tips.Find(243,tip));EXPECT_STREQ(tip.Text,"Tip:OptionsButton");
        EXPECT_EQ(RadarClass::DiplomacyButton.DrawPosition.X,0);
        EXPECT_EQ(tips.GetToolTipCount(),2);
    }
    radar.SetRadarButtonsVisible(false);
    EXPECT_FALSE(GScreenClass::Instance.SetButtons(&RadarClass::DiplomacyButton));
    EXPECT_FALSE(GScreenClass::Instance.SetButtons(&RadarClass::OptionsButton));
    radar.DisposeOfArt();
    EXPECT_EQ(RadarClass::DiplomacyShape,nullptr);EXPECT_EQ(diplomacy.Width,72);
    RadarClass::DiplomacyShape=static_cast<SHPStruct*>(YRMemory::Allocate(sizeof(SHPStruct)));
    RadarClass::OptionsShape=static_cast<SHPStruct*>(YRMemory::Allocate(sizeof(SHPStruct)));
    ASSERT_NE(RadarClass::DiplomacyShape,nullptr);ASSERT_NE(RadarClass::OptionsShape,nullptr);
    RadarClass::OwnsDiplomacyShape=RadarClass::OwnsOptionsShape=true;
    radar.DisposeOfArt();radar.DisposeOfArt();
    EXPECT_EQ(RadarClass::OptionsShape,nullptr);EXPECT_FALSE(RadarClass::OwnsDiplomacyShape);EXPECT_FALSE(RadarClass::OwnsOptionsShape);
}

TEST(MapRadar, BorrowedArtReloadWithOriginalAssets) {
    const char* data=std::getenv("RA2_GAME_DATA");
    if(!data || !*data)GTEST_SKIP()<<"RA2_GAME_DATA enables original sidebar art lifecycle regression";
    game::ResourceHandle* raw=nullptr;std::string error;
    ASSERT_TRUE(game::create_resources(data,raw,error))<<error;
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(raw,game::destroy_resources);
    ASSERT_EQ(game::load_resources(*files,{},error),game::ResourceLoadResult::complete)<<error;
    ASSERT_TRUE(game::with_resources(*files,[](void*) {
        ScenarioClass scenario;
        auto* old=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
        struct Restore{ScenarioClass* old;~Restore(){ScenarioClass::Instance=old;}} restore{old};
        game::UiResources resources;game::MapDrawStatistics stats;game::MapDrawingContext drawing;
        drawing.plain_palette=[](void*,const BytePalette&,const game::DrawingPaletteHandle*&) noexcept{return game::DrawingStatus::drawn;};
        DSurface::SidebarBounds={1112,158,168,562};TacticalClass::ViewBounds={0,0,1112,688};
        SidebarClass::Instance.IsSidebarActive=true;
        for(int side:{0,1,2,0}){
            scenario.PlayerSideIndex=side;
            ASSERT_TRUE(resources.load(side))<<resources.error();
            game::GameUiFrame frame{drawing,resources,stats};
            EXPECT_EQ(game::with_game_ui_frame(frame,[]{
                RadarClass::Instance.InitializeLayout();RadarClass::Instance.InitializeButtons();
                SidebarClass::Instance.UpdateLayout();SidebarClass::Instance.InitializeButtons();
            }),game::DrawingStatus::skipped);
            EXPECT_EQ(SidebarClass::ToggleRepairButton.ShapeData,resources.image(game::UiImage::repair));
            EXPECT_EQ(SidebarClass::ToggleSellButton.ShapeData,resources.image(game::UiImage::sell));
            EXPECT_EQ(SidebarClass::ToggleRepairButton.ID,101);EXPECT_EQ(SidebarClass::ToggleSellButton.ID,102);
            EXPECT_EQ(SidebarClass::ToggleRepairButton.X,SidebarClass::RepairPosition.X);
            EXPECT_EQ(SidebarClass::ToggleSellButton.X,SidebarClass::RepairPosition.X+SidebarClass::RepairPitch);
            EXPECT_TRUE(GScreenClass::Instance.SetButtons(&SidebarClass::ToggleRepairButton));
            EXPECT_TRUE(GScreenClass::Instance.SetButtons(&SidebarClass::ToggleSellButton));
            EXPECT_EQ(RadarClass::DiplomacyShape,resources.image(game::UiImage::briefing));
            EXPECT_EQ(RadarClass::OptionsShape,resources.image(game::UiImage::options));
            EXPECT_EQ(RadarClass::RadarAnim,resources.image(game::UiImage::radar));
            EXPECT_FALSE(RadarClass::OwnsDiplomacyShape);EXPECT_FALSE(RadarClass::OwnsOptionsShape);
            EXPECT_EQ(RadarClass::DiplomacyButton.X,1112+(side?14:11));
            EXPECT_EQ(RadarClass::DiplomacyButton.Y,side?21:20);
            GadgetClass::StuckOn=&RadarClass::DiplomacyButton;
            GadgetClass::Hovered=&RadarClass::OptionsButton;
            RadarClass::DiplomacyButton.IsPressed=true;
            resources.clear();
            EXPECT_EQ(SidebarClass::ToggleRepairButton.ShapeData,nullptr);EXPECT_EQ(SidebarClass::ToggleSellButton.ShapeData,nullptr);
            EXPECT_FALSE(GScreenClass::Instance.SetButtons(&SidebarClass::ToggleRepairButton));
            EXPECT_FALSE(GScreenClass::Instance.SetButtons(&SidebarClass::ToggleSellButton));
            EXPECT_EQ(RadarClass::DiplomacyShape,nullptr);EXPECT_EQ(RadarClass::RadarAnim,nullptr);
            EXPECT_EQ(RadarClass::DiplomacyButton.ShapeData,nullptr);
            EXPECT_EQ(GadgetClass::StuckOn,nullptr);EXPECT_EQ(GadgetClass::Hovered,nullptr);
            EXPECT_FALSE(RadarClass::DiplomacyButton.IsPressed);
            EXPECT_FALSE(GScreenClass::Instance.SetButtons(&RadarClass::DiplomacyButton));
        }
        RadarClass::Instance.RemoveButton(&RadarClass::RadarButton);
    },nullptr,error))<<error;
}

TEST(MapRadar, SelectionRepresentativeUsesOriginalObjects) {
    auto& selected=ObjectClass::CurrentObjects;
    ASSERT_EQ(selected.Count,0);
    HouseTypeClass country("RADAR_SELECTION");HouseClass house(&country);
    WeaponTypeClass weapon("RADAR_SELECTION_WEAPON");weapon.Damage=10;weapon.AmbientDamage=0;
    UnitTypeClass armed("RADAR_SELECTION_ARMED"),unarmed("RADAR_SELECTION_UNARMED");
    armed.Weapon[0].WeaponType=&weapon;
    UnitClass far(&armed,&house),near(&unarmed,&house),target(&unarmed,&house);
    struct FaultUnit final:UnitClass {
        explicit FaultUnit(HouseClass* h):UnitClass(nullptr,h){}
        bool IsUnderEMP() const override {throw std::runtime_error("selection query failed");}
    } fault(&house);
    struct RestoreSelection{~RestoreSelection(){ObjectClass::CurrentObjects.Clear();}} restore;
    EXPECT_EQ(Unsorted::BestSelectedObject(nullptr,nullptr),nullptr);
    ASSERT_TRUE(selected.AddItem(&far));ASSERT_TRUE(selected.AddItem(&near));
    target.Location={128,128,0};far.Location={1000,1000,0};near.Location={130,128,0};
    EXPECT_EQ(Unsorted::BestSelectedObject(nullptr,&target),&far) << "armed priority wins before distance";
    unarmed.Weapon[0].WeaponType=&weapon;
    EXPECT_EQ(Unsorted::BestSelectedObject(nullptr,nullptr),&far) << "no target preserves first equal-priority object";
    EXPECT_EQ(Unsorted::BestSelectedObject(nullptr,&target),&near);
    const CellStruct cell{0,0};target.Location=far.Location;
    EXPECT_EQ(Unsorted::BestSelectedObject(&cell,&target),&near) << "cell center takes precedence over object target";
    near.EMPLockRemaining=1;
    EXPECT_EQ(Unsorted::BestSelectedObject(&cell,nullptr),&far);
    near.EMPLockRemaining=0;near.IsImmobilized=true;
    EXPECT_EQ(Unsorted::BestSelectedObject(&cell,nullptr),&near) << "YR tests EMP, not chronosphere immobilization";
    near.Berzerk=true;
    EXPECT_EQ(Unsorted::BestSelectedObject(&cell,nullptr),&far);
    near.Berzerk=false;near.IsImmobilized=false;far.Location={130,129,0};
    EXPECT_EQ(Unsorted::BestSelectedObject(&cell,nullptr),&far) << "truncated distance tie preserves original ordering";
    selected.Clear();ASSERT_TRUE(selected.AddItem(&fault));
    EXPECT_EQ(Unsorted::BestSelectedObject(nullptr,nullptr),nullptr) << "query exceptions stay within the entry boundary";
}

TEST(MapRadar, PlanningWaypointLookupAndOwnedLifecycle) {
    const int paths_before=WaypointPathClass::Array.Count;
    const int listeners_before=PointerExpiredNotification::NotifyInvalidWaypoint.Array.Count;
    HouseTypeClass country("RADAR_PATHS");
    {
        HouseClass house(&country);
        EXPECT_EQ(PointerExpiredNotification::NotifyInvalidWaypoint.Array.Count,listeners_before+1);
        CellStruct missing{-2,3};
        EXPECT_EQ(house.GetPlanningWaypointAt(&missing),nullptr);
        ASSERT_EQ(WaypointPathClass::Array.Count,paths_before+12);
        for(auto* path:house.PlanningPaths) {
            ASSERT_NE(path,nullptr);EXPECT_EQ(path->CurrentWaypointIndex,-1);EXPECT_EQ(path->Waypoints.Count,0);
        }
        auto* path=house.PlanningPaths[2];
        ASSERT_TRUE(path->Waypoints.AddItem({{-511,896,208}}));
        ASSERT_TRUE(path->Waypoints.AddItem({{-511,896,416}}));
        CellStruct match{-1,3};
        EXPECT_EQ(house.GetPlanningWaypointAt(&match),path->GetWaypoint(0));
        EXPECT_EQ(house.GetPlanningWaypointAt(&missing),nullptr) << "negative leptons divide toward zero";
        int path_index=99;BYTE waypoint_index=99;
        ASSERT_TRUE(house.GetPlanningWaypointProperties(path->GetWaypoint(1),path_index,waypoint_index));
        EXPECT_EQ(path_index,2);EXPECT_EQ(waypoint_index,1);
        WaypointClass same_coords{{-511,896,416}};
        EXPECT_FALSE(house.GetPlanningWaypointProperties(&same_coords,path_index,waypoint_index));
        EXPECT_EQ(path_index,-1);EXPECT_EQ(waypoint_index,0) << "properties use pointer identity";
        GameDelete(path);
        EXPECT_EQ(house.PlanningPaths[2],nullptr) << "typed original notification clears the owner's pointer";
        EXPECT_EQ(WaypointPathClass::Array.Count,paths_before+11);
        auto* rebuilt=house.EnsurePlanningPathExists(2);
        EXPECT_EQ(rebuilt,house.PlanningPaths[2]);
        EXPECT_EQ(house.EnsurePlanningPathExists(2),rebuilt) << "reuse returns the same original path";
        EXPECT_EQ(WaypointPathClass::Array.Count,paths_before+12);
        {
            HouseClass early(&country);early.EnsurePlanningPathExists(0);
            ASSERT_TRUE(early.PlanningPaths[0]->Waypoints.AddItem({{-256,896,0}}));
            EXPECT_EQ(early.GetPlanningWaypointAt(&match),early.PlanningPaths[0]->GetWaypoint(0));
            for(int i=1;i<12;++i)EXPECT_EQ(early.PlanningPaths[i],nullptr) << "early hit does not allocate later paths";
        }
        EXPECT_EQ(WaypointPathClass::Array.Count,paths_before+12);
        {
            WaypointPathClass unregistered(-1);
            EXPECT_EQ(WaypointPathClass::Array.Count,paths_before+12);
            CLSID id{};EXPECT_EQ(unregistered.GetClassID(nullptr),static_cast<HRESULT>(0x80004003u));
            EXPECT_EQ(unregistered.GetClassID(&id),0);
            constexpr DWORD expected[]{0xF73125BAu,0x11D21054u,0x60007281u,0xB55B0508u};
            EXPECT_EQ(std::memcmp(&id,expected,sizeof(expected)),0);
            EXPECT_EQ(unregistered.WhatAmI(),AbstractType::Waypoint);
        }
    }
    EXPECT_EQ(WaypointPathClass::Array.Count,paths_before);
    EXPECT_EQ(PointerExpiredNotification::NotifyInvalidWaypoint.Array.Count,listeners_before);
}

TEST(MapRadar, ActiveClickReachesOriginalMissionQueue) {
    auto& map=MapClass::Instance;auto& selected=ObjectClass::CurrentObjects;
    ASSERT_EQ(selected.Count,0);
    struct MapCleanup{~MapCleanup(){MapClass::Instance.ReleaseCellStorage();}} cleanup;
    ASSERT_TRUE(map.CreateEmptyCells({0,0,100,100},0));map.VisibleRect={2,2,96,92};
    HouseTypeClass country("RADAR_ORDERS");HouseClass house(&country);
    UnitTypeClass type("RADAR_ORDER_UNIT");
    struct OrderingUnit final:UnitClass {
        Action action;
        OrderingUnit(UnitTypeClass* type,HouseClass* owner,Action a):UnitClass(type,owner),action(a){}
        Action MouseOverCell(const CellStruct*,bool,bool) const override {return action;}
    } attacker(&type,&house,Action::Attack),harvester(&type,&house,Action::Harvest);
    auto* old_player=HouseClass::CurrentPlayer;auto* old_input=InputManagerClass::Instance;
    const bool old_feedback=Unsorted::MoveFeedback,old_attack=Game::AttackMoveMode;
    const bool old_planning=PlanningNodeClass::PlanningModeActive;const auto old_debug=Unsorted::ArmageddonMode;
    const auto old_queue=EventClass::OutList;
    struct Restore {
        HouseClass* player;InputManagerClass* input;bool feedback,attack,planning;byte debug;
        const QueueClass<EventClass,EventClass::MAX_EVENTS>& queue;
        ~Restore(){ObjectClass::CurrentObjects.Clear();HouseClass::CurrentPlayer=player;InputManagerClass::Instance=input;
            Unsorted::MoveFeedback=feedback;Game::AttackMoveMode=attack;PlanningNodeClass::PlanningModeActive=planning;
            Unsorted::ArmageddonMode=debug;EventClass::OutList=queue;}
    } restore{old_player,old_input,old_feedback,old_attack,old_planning,old_debug,old_queue};
    HouseClass::CurrentPlayer=&house;InputManagerClass::Instance=nullptr;
    Unsorted::MoveFeedback=false;Game::AttackMoveMode=false;PlanningNodeClass::PlanningModeActive=false;
    Unsorted::ArmageddonMode=0;EventClass::OutList.Init();
    ASSERT_TRUE(selected.AddItem(&attacker));ASSERT_TRUE(selected.AddItem(&harvester));
    const CellStruct target{80,100};house.EnsurePlanningPathExists(5);
    ASSERT_TRUE(house.PlanningPaths[5]->Waypoints.AddItem({CellClass::Cell2Coord(target,208)}));
    attacker.unknown_bool_430=harvester.unknown_bool_430=true;
    DisplayClass::Instance.ActiveClick(nullptr,target,Action::Attack);
    ASSERT_EQ(EventClass::OutList.Count,2);
    auto& first=EventClass::OutList[0];auto& second=EventClass::OutList[1];
    EXPECT_EQ(first.Type,EventType::MegaMission);EXPECT_EQ(first.MegaMission.Mission,static_cast<unsigned char>(Mission::Attack));
    EXPECT_EQ(second.Type,EventType::MegaMission);EXPECT_EQ(second.MegaMission.Mission,static_cast<unsigned char>(Mission::Harvest));
    EXPECT_EQ(first.MegaMission.Target.As_Cell(),map.GetCellAt(target));
    EXPECT_EQ(second.MegaMission.Destination.As_Cell(),map.GetCellAt(target));
    for(auto* actor:{&attacker,&harvester}) {
        EXPECT_EQ(actor->PlanningPathIdx,5);EXPECT_EQ(actor->WaypointIndex,0);
        EXPECT_EQ(actor->WaypointCell,target);EXPECT_EQ(actor->WaypointNearbyAccessibleCellDelta,CellStruct(0,0));
        EXPECT_FALSE(actor->unknown_bool_430);
    }
    EXPECT_TRUE(Unsorted::MoveFeedback);
    EXPECT_EQ(selected.Count,2) << "orders preserve selection";
}

TEST(MapRadar, ClickEventWireAndSuperWeaponLookup) {
    const int old_frame=Unsorted::CurrentFrame;
    struct Restore {int frame;~Restore(){Unsorted::CurrentFrame=frame;}} restore{old_frame};
    Unsorted::CurrentFrame=0x12345678;
    HouseTypeClass country("RADAR_EVENT");HouseClass house(&country);
    UnitTypeClass unit_type("RADAR_EVENT_UNIT");UnitClass unit(&unit_type,&house);
    TargetClass target(&unit);
    EventClass event;event.IsExecuted=true;
    std::memset(event.DataBuffer,0xA5,sizeof(event.DataBuffer));
    ::new(static_cast<void*>(&event)) EventClass(house.ArrayIndex,EventType::Sell,target.m_ID,int(target.m_RTTI));
    EXPECT_EQ(event.Type,EventType::Sell);EXPECT_TRUE(event.IsExecuted);
    EXPECT_EQ(event.Frame,0x12345678u);EXPECT_EQ(event.Sell.Whom.As_Object(),&unit);
    EXPECT_EQ(static_cast<unsigned char>(event.DataBuffer[5]),0xA5);
    QueueClass<EventClass,4> queued;
    ASSERT_TRUE(queued.Add(event,0xFFFFFFF7));
    EXPECT_EQ(std::memcmp(&queued.First(),&event,sizeof(event)),0);
    const CellStruct cell{32767,-32768};
    ::new(static_cast<void*>(&event)) EventClass(house.ArrayIndex,EventType::Place,AbstractType::Building,cell);
    EXPECT_EQ(event.Place.RTTIType,AbstractType::Building);EXPECT_EQ(event.Place.HeapID,-1);
    EXPECT_EQ(event.Place.IsNaval,0);EXPECT_EQ(event.Place.Location,cell);
    ::new(static_cast<void*>(&event)) EventClass(house.ArrayIndex,EventType::Place,AbstractType::Unit,17,cell);
    EXPECT_EQ(event.Place.HeapID,17);EXPECT_EQ(event.Place.IsNaval,0);
    ::new(static_cast<void*>(&event)) EventClass(house.ArrayIndex,EventType::Place,AbstractType::Unit,19,1,cell);
    EXPECT_EQ(event.Place.HeapID,19);EXPECT_EQ(event.Place.IsNaval,1);
    ASSERT_EQ(SuperWeaponTypeClass::Array.Count,0);
    SuperWeaponTypeClass first("RADAR_SW1"),second("RADAR_SW2");
    first.Action=Action::PsychicReveal;second.Action=Action::PsychicReveal;
    auto* special=SuperWeaponTypeClass::FindFirstOfAction(Action::PsychicReveal);
    ASSERT_EQ(special,&first) << "first matching original array entry wins";
    EXPECT_EQ(SuperWeaponTypeClass::FindFirstOfAction(Action::Nuke),nullptr);
    ::new(static_cast<void*>(&event)) EventClass(house.ArrayIndex,EventType::SpecialPlace,special->ArrayIndex,cell);
    EXPECT_EQ(event.SpecialPlace.ID,first.ArrayIndex);EXPECT_EQ(event.SpecialPlace.Location,cell);
    const EventClass before(event);
    ::new(static_cast<void*>(&event)) EventClass(-1,EventType::SellCell,cell);
    EXPECT_EQ(event.Type,EventType::Empty);EXPECT_EQ(event.HouseIndex,-1);
    EXPECT_TRUE(event.IsExecuted);EXPECT_EQ(event.Frame,0x12345678u);
    EXPECT_EQ(std::memcmp(event.DataBuffer,before.DataBuffer,sizeof(event.DataBuffer)),0);
}

TEST(MapRadar, OriginalTypeSelectionAndRectangleDispatch) {
    ASSERT_EQ(ObjectClass::CurrentObjects.Count,0);
    TacticalClass tactical({}, {},0,1,0,1,1);
    HouseTypeClass country("RADAR_SELECT");HouseClass house(&country),foreign(&country);
    UnitTypeClass type("RADAR_SELECT_UNIT"),other_type("RADAR_SELECT_OTHER");
    type.Selectable=other_type.Selectable=true;
    UnitClass visible(&type,&house),offscreen(&type,&house),enemy(&type,&foreign),other(&other_type,&house);
    BuildingTypeClass building_type("RADAR_SELECT_BUILDING",BuildingTypeClass::ConstructionDefaults{});
    building_type.Selectable=true;building_type.UndeploysInto=&type;building_type.Foundation=Foundation::_1x1;
    BuildingClass building(&building_type,&house);
    ObjectClass* objects[]={&visible,&offscreen,&enemy,&other,&building};
    const auto old_selectable=TacticalClass::SelectableObjects[0];
    struct Restore {
        TacticalClass* tactical;HouseClass* house;TacticalSelectableStruct selectable;
        bool whole,active,attack,feedback;int command;ObjectClass** objects;
        ~Restore(){
            for(int i=0;i<5;++i){objects[i]->Deselect();objects[i]->InLimbo=true;}
            TacticalClass::Instance=tactical;HouseClass::CurrentPlayer=house;
            TacticalClass::SelectableObjects[0]=selectable;
            Game::TypeSelectionIncludesMap=whole;Game::TypeSelectionActive=active;
            Game::AttackMoveMode=attack;Game::SelectionCommandMode=command;Unsorted::MoveFeedback=feedback;
        }
    } restore{TacticalClass::Instance,HouseClass::CurrentPlayer,old_selectable,
        Game::TypeSelectionIncludesMap,Game::TypeSelectionActive,Game::AttackMoveMode,
        Unsorted::MoveFeedback,Game::SelectionCommandMode,objects};
    TacticalClass::Instance=&tactical;HouseClass::CurrentPlayer=&house;
    for(auto* object:objects){object->IsAlive=true;object->Health=100;object->IsOnMap=true;object->InLimbo=false;}
    tactical.SelectableCount=1;tactical.TacticalPos={10,20};
    TacticalClass::SelectableObjects[0]={&visible,15,25};
    struct Context {UnitClass& visible;UnitClass& offscreen;UnitClass& enemy;UnitClass& other;
        BuildingClass& building;BuildingTypeClass& building_type;TacticalClass& tactical;HouseClass& house;
    } context{visible,offscreen,enemy,other,building,building_type,tactical,house};
    auto runtime=game::default_scenario_runtime();
    runtime.session_mode=[](void*) noexcept{return int(GameMode::Skirmish);};
    ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void* opaque){
        auto& c=*static_cast<Context*>(opaque);
        Game::TypeSelectionIncludesMap=false;Unsorted::MoveFeedback=true;
        Game::UICommands_TypeSelect_7327D0("radar_select_unit");
        EXPECT_TRUE(c.visible.IsSelected);EXPECT_FALSE(c.offscreen.IsSelected);
        EXPECT_TRUE(Unsorted::MoveFeedback);
        Game::TypeSelectionIncludesMap=true;
        Game::UICommands_TypeSelect_7327D0("RADAR_SELECT_UNIT");
        EXPECT_TRUE(c.offscreen.IsSelected);EXPECT_FALSE(c.enemy.IsSelected);EXPECT_FALSE(c.other.IsSelected);
        Game::UICommands_TypeDeselect("radar_select_unit");
        EXPECT_EQ(ObjectClass::CurrentObjects.Count,0);
        Game::TypeSelectionActive=true;
        const auto callback=&record_selection_callback;
        c.tactical.SelectThese({5,5,1,1},callback);
        EXPECT_TRUE(c.visible.IsSelected);EXPECT_TRUE(c.offscreen.IsSelected);
        EXPECT_EQ(c.visible.Health,100) << "type selection precedes the rectangle callback";
        Game::SelectionCommandMode=7;Game::AttackMoveMode=true;
        MapClass::UnselectAll();
        EXPECT_EQ(ObjectClass::CurrentObjects.Count,0);EXPECT_EQ(Game::SelectionCommandMode,0);EXPECT_FALSE(Game::AttackMoveMode);
        Game::TypeSelectionActive=false;
        c.tactical.SelectThese({5,5,1,1},callback);
        EXPECT_EQ(c.visible.Health,99);EXPECT_FALSE(c.visible.IsSelected);
        c.tactical.SelectThese({6,5,1,1},nullptr);EXPECT_FALSE(c.visible.IsSelected);
        c.tactical.SelectThese({5,5,1,1},nullptr);EXPECT_TRUE(c.visible.IsSelected);
        MapClass::UnselectAll();
        TacticalClass::SelectableObjects[0]={&c.building,15,25};
        c.tactical.SelectThese({5,5,1,1},nullptr);EXPECT_TRUE(c.building.IsSelected);
        MapClass::UnselectAll();c.building_type.Foundation=Foundation::_2x2;
        c.tactical.SelectThese({5,5,1,1},nullptr);EXPECT_FALSE(c.building.IsSelected);
        c.building_type.Foundation=Foundation::_1x1;c.building_type.UndeploysInto=nullptr;
        c.tactical.SelectThese({5,5,1,1},nullptr);EXPECT_FALSE(c.building.IsSelected);
        EXPECT_TRUE(Unsorted::MoveFeedback);
    },&context));
    runtime.session_mode=[](void*) noexcept{return int(GameMode::Campaign);};
    ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void* opaque){
        auto& c=*static_cast<Context*>(opaque);
        Game::TypeSelectionIncludesMap=true;c.house.IsHumanPlayer=true;c.house.IsInPlayerControl=false;
        Game::UICommands_TypeSelect_7327D0("RADAR_SELECT_UNIT");
        EXPECT_FALSE(c.visible.IsSelected) << "campaign group selection specifically requires IsInPlayerControl";
        c.house.IsInPlayerControl=true;
        Game::UICommands_TypeSelect_7327D0("RADAR_SELECT_UNIT");
        EXPECT_TRUE(c.visible.IsSelected);EXPECT_TRUE(c.offscreen.IsSelected);EXPECT_FALSE(c.enemy.IsSelected);
        MapClass::UnselectAll();
    },&context));
}

TEST(MapRadar, PlacementFoundationCopyAndUpgradeDependencies) {
    auto& display=DisplayClass::Instance;
    ASSERT_EQ(display.CurrentFoundation_Data,nullptr);
    ASSERT_EQ(display.CurrentFoundationCopy_Data,nullptr);
    const auto center=display.CurrentFoundation_CenterCell,copy_center=display.CurrentFoundationCopy_CenterCell;
    const auto offset=display.CurrentFoundation_TopLeftOffset,copy_offset=display.CurrentFoundationCopy_TopLeftOffset;
    struct Cleanup {DisplayClass& display;CellStruct center,copy_center,offset,copy_offset;
        TechnoClass* first;TechnoClass* second;
        ~Cleanup(){
            display.SetActiveFoundation(nullptr);display.SetActiveFoundationCopy(nullptr);
            display.CurrentFoundation_CenterCell=center;display.CurrentFoundationCopy_CenterCell=copy_center;
            display.CurrentFoundation_TopLeftOffset=offset;display.CurrentFoundationCopy_TopLeftOffset=copy_offset;
            Game::SidebarTabObjects[0]=first;Game::SidebarTabObjects[1]=second;
            RadarClass::Instance.ReleaseTerrainRadar();MapClass::Instance.ReleaseCellStorage();
        }
    } cleanup{display,center,copy_center,offset,copy_offset,Game::SidebarTabObjects[0],Game::SidebarTabObjects[1]};
    ASSERT_TRUE(display.CreateEmptyCells({0,0,64,64},0));
    display.CurrentFoundation_CenterCell={64,64};display.CurrentFoundationCopy_CenterCell={64,64};
    BuildingTypeClass type("RADAR_UPGRADE_BASE",BuildingTypeClass::ConstructionDefaults{});
    type.Foundation=Foundation::_3x3;type.FoundationData=game::native_building_foundation(int(type.Foundation));
    const auto* source=type.GetFoundationData(true);
    display.SetActiveFoundation(source);
    EXPECT_EQ(display.CurrentFoundation_Data,DisplayClass::ActiveFoundationBuffer);
    EXPECT_NE(display.CurrentFoundation_Data,source);
    EXPECT_EQ(display.CurrentFoundation_TopLeftOffset,CellStruct(-1,-1));
    for(int y=63;y<=65;++y)for(int x=63;x<=65;++x)
        EXPECT_NE(display.GetCellAt(CellStruct(short(x),short(y)))->AltFlags&AltCellFlags::ContainsBuilding,AltCellFlags{});
    display.SetActiveFoundationCopy(source);
    EXPECT_EQ(display.CurrentFoundationCopy_Data,DisplayClass::PendingFoundationBuffer);
    EXPECT_NE(display.CurrentFoundationCopy_Data,display.CurrentFoundation_Data);
    EXPECT_EQ(display.CurrentFoundationCopy_TopLeftOffset,CellStruct(-1,-1));
    auto* corner=display.GetCellAt(CellStruct(63,63));
    const auto unrelated=corner->AltFlags&~(AltCellFlags::ContainsBuilding|AltCellFlags::Unknown_4);
    display.SetActiveFoundation(nullptr);
    EXPECT_EQ(corner->AltFlags&AltCellFlags::ContainsBuilding,AltCellFlags{});
    EXPECT_NE(corner->AltFlags&AltCellFlags::Unknown_4,AltCellFlags{});
    EXPECT_EQ(display.CurrentFoundation_TopLeftOffset,CellStruct(0,0));
    display.SetActiveFoundationCopy(nullptr);
    EXPECT_EQ(corner->AltFlags,unrelated);
    // Every canonical source supports the original fixed 120/50-cell copy,
    // including the sentinel-only foundation and the uninitialized type.
    for(int index=0;index<22;++index){
        auto* cells=game::native_building_foundation(index);
        display.SetActiveFoundation(cells);display.SetActiveFoundationCopy(cells);
        EXPECT_EQ(std::memcmp(display.CurrentFoundation_Data,cells,120*sizeof(CellStruct)),0);
        EXPECT_EQ(std::memcmp(display.CurrentFoundationCopy_Data,cells,50*sizeof(CellStruct)),0);
    }
    BuildingTypeClass upgrade("RADAR_UPGRADE",BuildingTypeClass::ConstructionDefaults{});
    display.SetActiveFoundation(upgrade.GetFoundationData());
    EXPECT_EQ(display.FoundationBoundsSize(display.CurrentFoundation_Data),CellStruct(1,1));
    HouseTypeClass country("RADAR_UPGRADE_HOUSE");HouseClass house(&country),other(&country);
    BuildingClass building(&type,&house);
    std::strcpy(upgrade.PowersUpBuilding,"radar_upgrade_base");upgrade.PowersUpToLevel=-1;type.Upgrades=3;
    building.UpgradeLevel=2;EXPECT_TRUE(building.CanUpgrade(&upgrade,&house));
    EXPECT_FALSE(building.CanUpgrade(&upgrade,&other));
    building.UpgradeLevel=3;EXPECT_FALSE(building.CanUpgrade(&upgrade,&house));
    building.UpgradeLevel=0xFF;EXPECT_TRUE(building.CanUpgrade(&upgrade,&house));
    building.UpgradeLevel=0;type.Upgrades=0;EXPECT_TRUE(building.CanUpgrade(&upgrade,&house));
    upgrade.PowersUpToLevel=4;EXPECT_FALSE(building.CanUpgrade(&upgrade,&house));
    upgrade.PowersUpToLevel=1;EXPECT_TRUE(building.CanUpgrade(&upgrade,&house));
    Game::SidebarTabObjects[0]=Game::SidebarTabObjects[1]=&building;
    building.ClearSidebarTabObject();
    EXPECT_EQ(Game::SidebarTabObjects[0],nullptr);EXPECT_EQ(Game::SidebarTabObjects[1],&building);
    EXPECT_EQ(Game::ClearSidebarTabObject(nullptr),0);
    EXPECT_EQ(Game::SidebarTabObjects[0],nullptr);EXPECT_EQ(Game::SidebarTabObjects[1],nullptr);
}

TEST(MapRadar, OriginalBeaconManagerOwnsSelection) {
    auto& manager=BeaconManagerClass::Instance;
    ASSERT_EQ(manager.AllocatedCount,0);ASSERT_EQ(ObjectClass::CurrentObjects.Count,0);
    EXPECT_EQ(&BeaconClass::Array,&manager.Beacons);
    EXPECT_EQ(&BeaconClass::Count,&manager.AllocatedCount);
    HouseTypeClass country("RADAR_BEACON");HouseClass player(&country),ally(&country);
    auto* old_player=HouseClass::CurrentPlayer;HouseClass::CurrentPlayer=&player;
    const int old_rounding=std::fegetround();
    struct Cleanup {HouseClass* player;int rounding,command;bool attack;
        ~Cleanup(){BeaconManagerClass::Instance.Reset();HouseClass::CurrentPlayer=player;
            std::fesetround(rounding);Game::SelectionCommandMode=command;Game::AttackMoveMode=attack;}
    } cleanup{old_player,old_rounding,Game::SelectionCommandMode,Game::AttackMoveMode};
    auto* first=GameCreate<BeaconClass>();auto* second=GameCreate<BeaconClass>();
    manager.Beacons[player.ArrayIndex][0]=first;manager.Beacons[ally.ArrayIndex][0]=second;manager.AllocatedCount=2;
    first->Bitfield=0xF0;second->Bitfield=0xA0;
    first->SetCoordAndHouse({128,0,0},player.ArrayIndex);second->SetCoordAndHouse({127,0,0},ally.ArrayIndex);
    EXPECT_TRUE(first->VisibleToPlayer());EXPECT_FALSE(second->VisibleToPlayer());
    player.Allies.data|=1u<<ally.ArrayIndex;EXPECT_FALSE(second->VisibleToPlayer());
    ally.Allies.data|=1u<<player.ArrayIndex;EXPECT_TRUE(second->VisibleToPlayer());
    ally.Defeated=true;EXPECT_FALSE(second->VisibleToPlayer());ally.Defeated=false;
    first->Bitfield|=2;Game::SelectionCommandMode=7;Game::AttackMoveMode=true;
    EXPECT_TRUE(manager.SelectBeacon(0,0,0));
    EXPECT_EQ(first->Bitfield,0xF1);EXPECT_EQ(second->Bitfield,0xA3);
    EXPECT_EQ(Game::SelectionCommandMode,0);EXPECT_FALSE(Game::AttackMoveMode);
    EXPECT_EQ(std::fegetround(),FE_TOWARDZERO);
    const CoordStruct previous=first->Coord;
    first->SetCoordAndHouse({0,0,0},8);
    EXPECT_EQ(first->Coord,previous);EXPECT_EQ(first->HouseID,player.ArrayIndex);
    std::array<wchar_t,140> text;text.fill(L'文');text.back()=0;
    first->SetText(text.data());EXPECT_EQ(first->Text[126],L'文');EXPECT_EQ(first->Text[127],0);
    first->SetText(nullptr);EXPECT_EQ(first->Text[0],0);EXPECT_EQ(first->Text[126],0);
    EXPECT_TRUE(manager.CanPlaceBeacon(player.ArrayIndex));
    for(int i=1;i<3;++i){manager.Beacons[player.ArrayIndex][i]=GameCreate<BeaconClass>();++manager.AllocatedCount;}
    EXPECT_FALSE(manager.CanPlaceBeacon(player.ArrayIndex));
    manager.Reset();EXPECT_EQ(BeaconClass::Count,0);
    for(const auto& house:manager.Beacons)for(auto* beacon:house)EXPECT_EQ(beacon,nullptr);
    manager.Reset();EXPECT_EQ(manager.AllocatedCount,0);
}

TEST(MapRadar, VisibleRangeParentAndReentryDispatch) {
    auto& map=MapClass::Instance;
    struct Cleanup{~Cleanup(){RadarClass::Instance.ReleaseTerrainRadar();MapClass::Instance.ReleaseCellStorage();}} cleanup;
    ASSERT_TRUE(map.CreateEmptyCells({0,0,100,100},0));
    HouseTypeClass country("RADAR_RANGE");HouseClass house(&country);
    auto* old_player=HouseClass::CurrentPlayer;HouseClass::CurrentPlayer=&house;
    struct Restore{HouseClass* player;~Restore(){HouseClass::CurrentPlayer=player;}} restore{old_player};
    struct ObservedUnit:UnitClass {
        int seen=0;DWORD first=99,second=99;
        explicit ObservedUnit(HouseClass* owner):UnitClass(nullptr,owner){}
        void See(DWORD a,DWORD b) override {++seen;first=a;second=b;}
    } unit(&house);
    unit.Location={80*256+128,100*256+128,0};unit.IsAlive=true;unit.InLimbo=false;
    const RectangleStruct small{2,2,10,10},all{-20,-20,140,140};
    map.MapClass::SetVisibleRect(small);EXPECT_FALSE(unit.IsInPlayfield);
    const unsigned redraws=unsigned(map.Redraws);
    map.MapClass::SetVisibleRect(all);
    EXPECT_EQ(map.VisibleRect.X,2);EXPECT_EQ(map.VisibleRect.Y,2);
    EXPECT_EQ(map.VisibleRect.Width,96);EXPECT_EQ(map.VisibleRect.Height,92);
    EXPECT_TRUE(unit.IsInPlayfield);EXPECT_EQ(unit.seen,1);
    EXPECT_EQ(unit.first,0u);EXPECT_EQ(unit.second,0u);
    EXPECT_EQ(unsigned(map.Redraws)&0xFFu,(redraws+1)&0xFFu);
    map.MapClass::SetVisibleRect(all);EXPECT_EQ(unit.seen,1) << "only outside-to-inside reentry reveals";
    for(int mode=0;mode<3;++mode){
        map.MapClass::SetVisibleRect(small);
        unit.IsAlive=mode!=0;unit.InLimbo=mode==1;
        HouseClass::CurrentPlayer=mode==2?nullptr:&house;
        map.MapClass::SetVisibleRect(all);EXPECT_TRUE(unit.IsInPlayfield);EXPECT_EQ(unit.seen,1);
    }
    unit.InLimbo=true;
}

TEST(MapRadar, ResetAndPostLoadTransientRepair) {
    auto& map=MapClass::Instance;auto& radar=RadarClass::Instance;
    struct Cleanup{~Cleanup(){RadarClass::Instance.ReleaseTerrainRadar();MapClass::Instance.ReleaseCellStorage();}} cleanup;
    ASSERT_TRUE(map.CreateEmptyCells({0,0,100,100},0));map.VisibleRect={2,2,96,92};
    ASSERT_TRUE(radar.BuildTerrainRadar());
    BuildingTypeClass type("RADAR_POSTLOAD",BuildingTypeClass::ConstructionDefaults{});
    BuildingClass object(&type,nullptr);
    ASSERT_TRUE(radar.TrackObject(&object,2,3));object.IsRadarTracked=true;
    radar.RadarBackground({80,100});
    const auto point_count=radar.unknown_points_125C.Count,cell_count=radar.unknown_cells_1124.Count;
    radar.unknown_timer_1500.Start(17);const auto timer=radar.unknown_timer_1500;
    radar.unknown_14B0=2;radar.unknown_14AC=1;radar.unknown_14FC=32;
    radar.ResetRadar();
    EXPECT_FALSE(object.IsRadarTracked);
    ASSERT_NE(radar.unknown_1258,nullptr);ASSERT_NE(radar.unknown_1220,nullptr);
    EXPECT_EQ(radar.unknown_points_125C.Count,point_count);EXPECT_EQ(radar.unknown_cells_1124.Count,cell_count);
    EXPECT_EQ(radar.unknown_14B0,2u);EXPECT_EQ(radar.unknown_14AC,1u);EXPECT_EQ(radar.unknown_14FC,32u);
    EXPECT_EQ(radar.unknown_timer_1500.StartTime,timer.StartTime);EXPECT_EQ(radar.unknown_timer_1500.TimeLeft,timer.TimeLeft);
    for(int i=0;i<radar.unknown_1258->BucketCount;++i)EXPECT_EQ(radar.unknown_1258->Buckets[i].Count,0);
    // Retire the live resources at the coordinator boundary, then emulate
    // stale serialized addresses. PostLoad must not delete any of these.
    radar.ClearRadar();
    radar.unknown_121C=reinterpret_cast<Surface*>(0x11111111u);
    radar.unknown_1220=reinterpret_cast<Surface*>(0x22222222u);
    radar.unknown_123C=reinterpret_cast<ColorStruct*>(0x33333333u);
    radar.unknown_1258=reinterpret_cast<decltype(radar.unknown_1258)>(0x44444444u);
    radar.unknown_1274=reinterpret_cast<byte*>(0x55555555u);
    object.IsRadarTracked=true;
    RulesClass rules;rules.DetailMinFrameRateNormal=13;
    auto* old_rules=RulesClass::Instance;const auto old_detail=Detail::MinFrameRate;
    struct Restore{RulesClass* rules;unsigned detail;~Restore(){RulesClass::Instance=rules;Detail::MinFrameRate=detail;}} restore{old_rules,old_detail};
    RulesClass::Instance=&rules;Detail::MinFrameRate=20;
    radar.unknown_14B0=3;radar.unknown_14B4=2;radar.unknown_14AC=1;
    radar.PostLoadRadarFixup();
    ASSERT_NE(radar.unknown_1220,nullptr);ASSERT_NE(radar.unknown_123C,nullptr);ASSERT_NE(radar.unknown_1274,nullptr);
    EXPECT_FALSE(object.IsRadarTracked);EXPECT_EQ(radar.unknown_14B0,2u);
    EXPECT_EQ(radar.unknown_14AC,4u);EXPECT_EQ(radar.unknown_14FC,25u);EXPECT_EQ(Detail::MinFrameRate,13u);
    EXPECT_EQ(radar.unknown_points_125C.Count,point_count);EXPECT_EQ(radar.unknown_cells_1124.Count,cell_count);
    EXPECT_EQ(radar.unknown_timer_1500.StartTime,timer.StartTime);EXPECT_EQ(radar.unknown_timer_1500.TimeLeft,timer.TimeLeft);
}

TEST(MapRadar, OriginalScreenRedrawPropagation) {
    auto& screen=GScreenClass::Instance;auto& map=MapClass::Instance;
    const int old_bitfield=screen.Bitfield;const auto old_redraws=map.Redraws;
    TacticalClass tactical({}, {},0,1,0,1,1);
    auto* old=TacticalClass::Instance;TacticalClass::Instance=&tactical;
    struct Restore{TacticalClass* old;int bitfield;BOOL redraws;~Restore(){
        TacticalClass::Instance=old;GScreenClass::Instance.Bitfield=bitfield;MapClass::Instance.Redraws=redraws;
    }} restore{old,old_bitfield,old_redraws};
    screen.Bitfield=0;map.Redraws=0x123456FF;tactical.Redrawing=false;
    screen.MarkNeedsRedraw(0);
    EXPECT_TRUE(tactical.Redrawing);EXPECT_EQ(screen.Bitfield,0);EXPECT_EQ(map.Redraws,0x123456FF);
    screen.MarkNeedsRedraw(1);
    EXPECT_EQ(screen.Bitfield,1);EXPECT_EQ(map.Redraws,0x12345600);
    screen.MarkNeedsRedraw(2);screen.MarkNeedsRedraw(1);
    EXPECT_EQ(screen.Bitfield,2);EXPECT_EQ(map.Redraws,0x12345602);
}

TEST(MapRadar, RangeReentryRevealsTerrainAndDiscoversUnit) {
    auto& map=MapClass::Instance;
    struct Cleanup{~Cleanup(){RadarClass::Instance.ReleaseTerrainRadar();MapClass::Instance.ReleaseCellStorage();}} cleanup;
    RulesClass rules;rules.LeptonsPerSightIncrease=50;rules.RevealByHeight=false;
    ScenarioClass scenario;scenario.SpecialFlags.FogOfWar=false;
    TacticalClass tactical({}, {},0,1,0,1,1);
    auto* old_rules=RulesClass::Instance;auto* old_scenario=ScenarioClass::Instance;
    auto* old_tactical=TacticalClass::Instance;auto* old_player=HouseClass::CurrentPlayer;
    struct Restore {RulesClass* rules;ScenarioClass* scenario;TacticalClass* tactical;HouseClass* player;
        ~Restore(){RulesClass::Instance=rules;ScenarioClass::Instance=scenario;TacticalClass::Instance=tactical;HouseClass::CurrentPlayer=player;}
    } restore{old_rules,old_scenario,old_tactical,old_player};
    RulesClass::Instance=&rules;ScenarioClass::Instance=&scenario;TacticalClass::Instance=&tactical;
    ASSERT_TRUE(map.CreateEmptyCells({0,0,64,64},0));
    HouseTypeClass country("RADAR_SIGHT");country.MultiplayPassive=false;
    HouseClass house(&country);HouseClass::CurrentPlayer=&house;
    UnitTypeClass type("RADAR_SCOUT");type.Sight=4;
    UnitClass scout(&type,&house);scout.Location={64*256+128,64*256+128,0};
    scout.IsAlive=true;scout.InLimbo=false;scout.IsInPlayfield=false;
    scout.IsOwnedByCurrentPlayer=true;
    auto* center=map.GetCellAt(CellStruct{64,64});center->FirstObject=&scout;
    struct Detach{UnitClass& scout;CellClass* cell;~Detach(){cell->FirstObject=nullptr;scout.InLimbo=true;}} detach{scout,center};
    EXPECT_TRUE(map.IsLocationShrouded(scout.Location));
    map.MapClass::SetVisibleRect({2,2,8,8});EXPECT_FALSE(scout.IsInPlayfield);
    map.MapClass::SetVisibleRect({0,0,64,64});
    EXPECT_TRUE(scout.IsInPlayfield);EXPECT_FALSE(map.IsLocationShrouded(scout.Location));
    EXPECT_TRUE(scout.DiscoveredByCurrentPlayer);
    EXPECT_EQ(center->ShroudCounter,-1);
    EXPECT_NE(center->AltFlags&AltCellFlags::Mapped,AltCellFlags{});
    const auto outside=CellClass::Cell2Coord({54,64},0);
    EXPECT_TRUE(map.IsLocationShrouded(outside)) << "revealing a unit's sight must not reveal the entire map";
    // Repeated range assignment does not acquire an extra sight reference.
    map.MapClass::SetVisibleRect({0,0,64,64});EXPECT_EQ(center->ShroudCounter,-1);
    const CoordStruct position=scout.Location;
    auto mutable_position=position;
    map.RevealArea2(&mutable_position,4,&house,0,0,0,0,1);
    EXPECT_EQ(center->ShroudCounter,0);
    map.RevealArea2(&mutable_position,4,&house,0,0,0,0,0);
    EXPECT_EQ(center->ShroudCounter,-1);
    // Production acquisition records the old position once. The vehicle's
    // actual cell-arrival method must release it and acquire at the new one.
    scout.UpdateSight(false,0,false,nullptr,0);
    ASSERT_TRUE(scout.unknown_bool_250);EXPECT_EQ(scout.LastSightCoords,position);
    EXPECT_EQ(scout.LastSightRange,4);EXPECT_EQ(center->ShroudCounter,-2);
    scout.UpdateSight(false,0,false,nullptr,0);EXPECT_EQ(center->ShroudCounter,-2);
    auto* destination=map.GetCellAt(CellStruct{76,64});
    center->FirstObject=nullptr;destination->FirstObject=&scout;detach.cell=destination;
    scout.Location=CellClass::Cell2Coord({76,64},0);
    EXPECT_TRUE(map.IsLocationShrouded(scout.Location));
    scout.UnitClass::UpdatePosition(PCPType::End);
    EXPECT_EQ(center->ShroudCounter,-1);
    EXPECT_EQ(scout.LastSightCoords,scout.Location);EXPECT_EQ(scout.LastSightRange,4);
    EXPECT_FALSE(map.IsLocationShrouded(scout.Location));EXPECT_EQ(destination->ShroudCounter,-1);
    scout.vt_entry_48C(false,0,false,nullptr);
    EXPECT_FALSE(scout.unknown_bool_250);EXPECT_EQ(destination->ShroudCounter,0);
    // The original house argument is a pointer, including on a 64-bit host;
    // the explicit radius must be saved for the matching release.
    scout.UpdateSight(false,0,true,&house,2);
    EXPECT_TRUE(scout.unknown_bool_250);EXPECT_EQ(scout.LastSightRange,2);
    EXPECT_EQ(destination->ShroudCounter,-1);
    scout.vt_entry_48C(false,0,true,&house);
    EXPECT_FALSE(scout.unknown_bool_250);EXPECT_EQ(destination->ShroudCounter,0);
}

TEST(MapRadar, LeftReleaseDispatchesMissionsAndHonorsPlanningRejection) {
    auto& map=MapClass::Instance;auto& display=DisplayClass::Instance;
    ASSERT_EQ(ObjectClass::CurrentObjects.Count,0);ASSERT_EQ(display.CurrentBuilding,nullptr);
    ASSERT_TRUE(map.CreateEmptyCells({0,0,100,100},0));map.VisibleRect={2,2,96,92};
    const auto cells=ra2::test::scope_exit([&]{map.ReleaseCellStorage();});
    HouseTypeClass country("RELEASE_ORDERS");HouseClass house(&country);
    UnitTypeClass type("RELEASE_UNIT");
    // Classification is the observed input to real original actor command
    // methods, mission construction and ring queue; none of those is replaced.
    struct Unit:UnitClass {
        using UnitClass::UnitClass;
        Action MouseOverCell(const CellStruct*,bool,bool)const override{return Action::Move;}
    } unit(&type,&house);
    type.Locomotor=LocomotionClass::CLSIDs::Drive;ASSERT_TRUE(unit.InitializeLocomotor());
    unit.Location=CellClass::Cell2Coord({60,80},0);
    // A revealed destination and an unrestricted movement type exercise real
    // MoveOrder without pretending this empty-cell fixture built region zones.
    type.MovementZone=MovementZone::None;
    map.GetCellAt(CellStruct{80,100})->AltFlags|=AltCellFlags::Mapped;
    auto* player=HouseClass::CurrentPlayer;auto* input=InputManagerClass::Instance;
    const auto queue=EventClass::OutList;const auto timer=TechnoClass::ActionLineTimer;
    const bool attack=Game::AttackMoveMode,planning=PlanningNodeClass::PlanningModeActive;
    const bool error=Game::PlanningErrorReported,feedback=Unsorted::MoveFeedback;
    const bool band=display.LeftPressAndDraggingRectangle,tentative=display.unknown_bool_11D0;
    const auto debug=Unsorted::ArmageddonMode;const int members=Game::PlanningMemberCounts[house.ArrayIndex];
    const auto restore=ra2::test::scope_exit([&]{
        ObjectClass::CurrentObjects.Clear();HouseClass::CurrentPlayer=player;InputManagerClass::Instance=input;
        EventClass::OutList=queue;TechnoClass::ActionLineTimer=timer;Game::AttackMoveMode=attack;
        PlanningNodeClass::PlanningModeActive=planning;Game::PlanningErrorReported=error;
        Unsorted::MoveFeedback=feedback;Unsorted::ArmageddonMode=debug;
        display.LeftPressAndDraggingRectangle=band;display.unknown_bool_11D0=tentative;
        Game::PlanningMemberCounts[house.ArrayIndex]=members;
    });
    HouseClass::CurrentPlayer=&house;InputManagerClass::Instance=nullptr;Unsorted::ArmageddonMode=0;
    PlanningNodeClass::PlanningModeActive=false;Game::AttackMoveMode=false;Unsorted::MoveFeedback=false;
    display.LeftPressAndDraggingRectangle=false;display.unknown_bool_11D0=true;
    EventClass::OutList.Init();ASSERT_TRUE(ObjectClass::CurrentObjects.AddItem(&unit));
    const CellStruct at{80,100};display.LeftMouseButtonUp({},at,nullptr,Action::Move,1);
    ASSERT_EQ(EventClass::OutList.Count,1);auto& event=EventClass::OutList.First();
    EXPECT_EQ(event.Type,EventType::MegaMission);
    EXPECT_EQ(event.MegaMission.Mission,static_cast<unsigned char>(Mission::Move));
    EXPECT_EQ(event.MegaMission.Destination.As_Cell(),map.GetCellAt(at));
    EXPECT_EQ(TechnoClass::ActionLineTimer.TimeLeft,25);EXPECT_FALSE(display.unknown_bool_11D0);
    EXPECT_EQ(ObjectClass::CurrentObjects.Count,1);
    // Reject before ActiveClick; no mission or action-line restart is emitted.
    EventClass::OutList.Init();PlanningNodeClass::PlanningModeActive=true;
    Game::PlanningErrorReported=true;Game::PlanningMemberCounts[house.ArrayIndex]=128;
    TechnoClass::ActionLineTimer.Start(3);Game::AttackMoveMode=true;display.unknown_bool_11D0=true;
    display.LeftMouseButtonUp({},at,nullptr,Action::Move,1);
    EXPECT_EQ(EventClass::OutList.Count,0);EXPECT_EQ(TechnoClass::ActionLineTimer.TimeLeft,3);
    EXPECT_FALSE(Game::AttackMoveMode);EXPECT_FALSE(display.unknown_bool_11D0);
    // Full original ring: command state still runs and attack-move clears,
    // while the previously queued events remain byte-for-byte unchanged.
    PlanningNodeClass::PlanningModeActive=false;
    EventClass marker;for(int i=0;i<128;++i)ASSERT_TRUE(EventClass::OutList.Add(marker,123));
    const auto full=EventClass::OutList;Game::AttackMoveMode=true;Unsorted::MoveFeedback=false;
    display.LeftMouseButtonUp({},at,nullptr,Action::Move,1);
    EXPECT_EQ(std::memcmp(&EventClass::OutList,&full,sizeof(full)),0);
    EXPECT_FALSE(Game::AttackMoveMode);EXPECT_EQ(TechnoClass::ActionLineTimer.TimeLeft,25);
}

TEST(MapRadar, LeftReleaseQueuesUpgradeAndHandsOffFoundation) {
    auto& d=DisplayClass::Instance;
    ASSERT_EQ(d.CurrentBuilding,nullptr);ASSERT_EQ(d.CurrentFoundation_Data,nullptr);
    ASSERT_EQ(d.CurrentFoundationCopy_Data,nullptr);
    ASSERT_TRUE(d.CreateEmptyCells({0,0,64,64},0));
    const auto cells=ra2::test::scope_exit([&]{RadarClass::Instance.ReleaseTerrainRadar();d.ReleaseCellStorage();});
    HouseTypeClass country("RELEASE_BUILD");HouseClass house(&country);
    BuildingTypeClass base("RELEASE_BASE",BuildingTypeClass::ConstructionDefaults{});
    BuildingTypeClass upgrade("RELEASE_UPGRADE",BuildingTypeClass::ConstructionDefaults{});
    base.Foundation=upgrade.Foundation=Foundation::_1x1;
    base.FoundationData=upgrade.FoundationData=game::native_building_foundation(int(Foundation::_1x1));
    base.Upgrades=1;std::strcpy(upgrade.PowersUpBuilding,base.ID);upgrade.PowersUpToLevel=1;upgrade.Naval=true;
    BuildingClass building(&base,&house),pending(&upgrade,&house);
    const auto queue=EventClass::OutList;auto* player=HouseClass::CurrentPlayer;
    const bool scenario=Unsorted::ScenarioStarted,active=Game::IsActive,attack=Game::AttackMoveMode;
    const byte debug=Unsorted::ArmageddonMode;const bool prox=d.unknown_1180,shroud=d.unknown_1181,tentative=d.unknown_bool_11D0;
    const auto center=d.CurrentFoundation_CenterCell,copyCenter=d.CurrentFoundationCopy_CenterCell;
    const auto offset=d.CurrentFoundation_TopLeftOffset,copyOffset=d.CurrentFoundationCopy_TopLeftOffset;
    auto* currentType=d.CurrentBuildingType;const int owner=d.CurrentBuildingOwnerArrayIndex;
    auto* copy=d.CurrentBuildingCopy;auto* copyType=d.CurrentBuildingTypeCopy;const int copyOwner=d.CurrentBuildingOwnerArrayIndexCopy;
    auto* sidebar0=Game::SidebarTabObjects[0];auto* sidebar1=Game::SidebarTabObjects[1];
    const auto restore=ra2::test::scope_exit([&]{
        d.SetActiveFoundation(nullptr);d.SetActiveFoundationCopy(nullptr);
        d.CurrentBuilding=nullptr;d.CurrentBuildingType=currentType;d.CurrentBuildingOwnerArrayIndex=owner;
        d.CurrentBuildingCopy=copy;d.CurrentBuildingTypeCopy=copyType;d.CurrentBuildingOwnerArrayIndexCopy=copyOwner;
        d.CurrentFoundation_CenterCell=center;d.CurrentFoundationCopy_CenterCell=copyCenter;
        d.CurrentFoundation_TopLeftOffset=offset;d.CurrentFoundationCopy_TopLeftOffset=copyOffset;
        d.unknown_1180=prox;d.unknown_1181=shroud;d.unknown_bool_11D0=tentative;
        HouseClass::CurrentPlayer=player;Unsorted::ScenarioStarted=scenario;Game::IsActive=active;
        Game::AttackMoveMode=attack;Unsorted::ArmageddonMode=debug;EventClass::OutList=queue;
        Game::SidebarTabObjects[0]=sidebar0;Game::SidebarTabObjects[1]=sidebar1;
    });
    HouseClass::CurrentPlayer=&house;Unsorted::ScenarioStarted=false;Game::IsActive=true;Unsorted::ArmageddonMode=0;
    d.CurrentFoundation_CenterCell={64,64};d.SetActiveFoundation(upgrade.GetFoundationData(true));
    d.CurrentBuilding=&pending;d.CurrentBuildingType=&upgrade;d.CurrentBuildingOwnerArrayIndex=house.ArrayIndex;
    d.unknown_1180=false;d.unknown_1181=true;d.unknown_bool_11D0=true;Game::AttackMoveMode=true;
    Game::SidebarTabObjects[0]=&pending;Game::SidebarTabObjects[1]=&building;EventClass::OutList.Init();
    d.LeftMouseButtonUp({},CellStruct{64,64},&building,Action::None,0);
    ASSERT_EQ(EventClass::OutList.Count,1);const auto& event=EventClass::OutList.First();
    EXPECT_EQ(event.Type,EventType::Place);EXPECT_EQ(event.Place.RTTIType,AbstractType::Building);
    EXPECT_EQ(event.Place.HeapID,upgrade.ArrayIndex);EXPECT_EQ(event.Place.IsNaval,1);
    EXPECT_EQ(event.Place.Location,CellStruct(64,64));EXPECT_FALSE(event.IsExecuted);
    EXPECT_TRUE(d.unknown_1180)<<"actual CanUpgrade overrides failed proximity";
    EXPECT_EQ(d.CurrentBuilding,nullptr);EXPECT_EQ(d.CurrentBuildingType,nullptr);EXPECT_EQ(d.CurrentBuildingOwnerArrayIndex,-1);
    EXPECT_EQ(d.CurrentBuildingCopy,&pending);EXPECT_EQ(d.CurrentBuildingTypeCopy,&upgrade);
    EXPECT_EQ(d.CurrentFoundationCopy_CenterCell,CellStruct(64,64));
    EXPECT_EQ(d.CurrentFoundationCopy_Data,DisplayClass::PendingFoundationBuffer);EXPECT_EQ(d.CurrentFoundation_Data,nullptr);
    EXPECT_EQ(Game::SidebarTabObjects[0],nullptr);EXPECT_EQ(Game::SidebarTabObjects[1],&building);
    EXPECT_TRUE(d.unknown_bool_11D0)<<"placement preserves tentative selection";EXPECT_FALSE(Game::AttackMoveMode);
}
