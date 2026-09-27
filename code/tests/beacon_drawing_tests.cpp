#include "support/test_support.hpp"
#include "api/filesystem.hpp"
#include "api/software_type_drawing.hpp"
#include "game_ui_runtime.hpp"
#include "filesystem/file_system.hpp"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/RulesClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <vector>

namespace {
constexpr int width=256,height=180;
struct Backend {
    game::TypeDrawingContext software;
    std::map<const BytePalette*,std::unique_ptr<ConvertClass>> converters;
    std::vector<game::ShapeDrawingRequest> shapes;
    std::vector<char> order;
    static game::DrawingStatus plain(void* p,const BytePalette& palette,const game::DrawingPaletteHandle*& out) noexcept {
        try {
            auto& self=*static_cast<Backend*>(p);
            auto& converter=self.converters[&palette];
            if(!converter)converter=std::make_unique<ConvertClass>(palette,palette,2,1,false);
            out=reinterpret_cast<const game::DrawingPaletteHandle*>(converter.get());return game::DrawingStatus::drawn;
        } catch(...) {return game::DrawingStatus::backend_failure;}
    }
    static game::DrawingStatus scheme(void* p,const BytePalette& palette,int shades,const game::DrawingPaletteHandle*& out) noexcept {
        auto result=plain(p,palette,out);
        if(result!=game::DrawingStatus::drawn)return result;
        auto* converter=const_cast<ConvertClass*>(reinterpret_cast<const ConvertClass*>(out));
        if(!ColorScheme::BuildColorTable(palette,static_cast<WORD*>(converter->FullColorData),256,shades,2,false))return game::DrawingStatus::backend_failure;
        return result;
    }
    static game::DrawingStatus shape(void* p,const game::ShapeDrawingRequest& request) {
        auto& self=*static_cast<Backend*>(p);self.shapes.push_back(request);self.order.push_back('S');
        return self.software.backend.shape(self.software.backend_context,request);
    }
    static game::DrawingStatus raster(void* p,const game::RasterDrawingRequest& request) {
        auto& self=*static_cast<Backend*>(p);self.order.push_back(request.width==1 && request.height==1?'E':'R');
        return self.software.backend.raster(self.software.backend_context,request);
    }
};
std::vector<WORD> pixels(BSurface& target) {
    auto* data=static_cast<WORD*>(target.Lock(0,0));std::vector<WORD> copy(data,data+width*height);target.Unlock();return copy;
}
void artifact(const std::vector<WORD>& a,const std::vector<WORD>& b,const std::vector<WORD>& c) {
    const auto* path=std::getenv("RA2_BEACON_PPM");if(!path || !*path)return;
    std::ofstream file(path,std::ios::binary);file<<"P6\n"<<width*3<<' '<<height<<"\n255\n";
    for(int y=0;y<height;++y)for(const auto* image:{&a,&b,&c})for(int x=0;x<width;++x){
        const WORD pixel=(*image)[y*width+x];const char rgb[]{char((pixel>>11)*255/31),char(((pixel>>5)&63)*255/63),char((pixel&31)*255/31)};file.write(rgb,3);
    }
    EXPECT_TRUE(file.good());
}
void exercise_drawing() {
    ASSERT_TRUE(MixFileClass::LoadTerrainMixes());
    auto& manager=BeaconManagerClass::Instance;auto& radar=RadarClass::Instance;auto& map=MapClass::Instance;
    ASSERT_EQ(manager.AllocatedCount,0);
    const auto old_player=HouseClass::CurrentPlayer;const auto old_frame=Unsorted::CurrentFrame;
    const auto old_window=DSurface::WindowBounds,old_sidebar=DSurface::SidebarBounds;
    auto* old_tactical=TacticalClass::Instance;const auto old_rules=RulesClass::Instance;
    const auto cleanup=ra2::test::scope_exit([&]{
        RadarEventClass::Clear();manager.Reset();manager.ReleaseArt();radar.ReleaseTerrainRadar();map.ReleaseCellStorage();
        HouseClass::CurrentPlayer=old_player;Unsorted::CurrentFrame=old_frame;RulesClass::Instance=old_rules;
        DSurface::WindowBounds=old_window;DSurface::SidebarBounds=old_sidebar;TacticalClass::Instance=old_tactical;
    });
    game::UiResources resources;ASSERT_TRUE(resources.load(0))<<resources.error();
    ASSERT_NE(manager.BeaconArt,nullptr);ASSERT_NE(manager.RadarBeaconArt,nullptr);
    ASSERT_GT(manager.RadarBeaconFrameCount,1);EXPECT_EQ(manager.RadarBeaconAnimPeriod,4*manager.RadarBeaconFrameCount);
    EXPECT_EQ(manager.RadarBeaconSize,Point2D(manager.RadarBeaconArt->Width,manager.RadarBeaconArt->Height));
    auto* retained=manager.RadarBeaconArt;manager.Reset();EXPECT_EQ(manager.RadarBeaconArt,retained);
    BytePalette palette{};CCFileClass palette_file("UNITTEM.PAL");ASSERT_EQ(palette_file.ReadBytes(&palette,sizeof(palette)),sizeof(palette));
    for(auto& color:palette.Entries){color.R<<=2;color.G<<=2;color.B<<=2;}
    HouseTypeClass country("BEACON_TEST");HouseClass owner(&country),ally(&country);
    HouseClass::CurrentPlayer=&owner;owner.Defeated=ally.Defeated=false;
    owner.Allies.data=ally.Allies.data=(1u<<owner.ArrayIndex)|(1u<<ally.ArrayIndex);
    const int red_index=ColorScheme::Array.Count;
    ColorScheme red("BEACON_RED",{0,255,255},palette,palette,1,true);
    const int blue_index=ColorScheme::Array.Count;
    ColorScheme blue("BEACON_BLUE",{160,255,255},palette,palette,1,true);
    owner.ColorSchemeIndex=red_index;ally.ColorSchemeIndex=blue_index;
    ASSERT_TRUE(map.CreateEmptyCells({0,0,100,100},0));map.VisibleRect={0,0,100,100};
    radar.unknown_11F0=16;radar.unknown_11F4=49;ASSERT_TRUE(radar.BuildTerrainRadar());
    DSurface::WindowBounds={0,0,width,height};DSurface::SidebarBounds={72,0,168,height};TacticalClass::Instance=nullptr;
    auto* beacon=GameCreate<BeaconClass>();beacon->SetCoordAndHouse({100*256,100*256,0},owner.ArrayIndex);
    manager.Beacons[owner.ArrayIndex][0]=beacon;manager.AllocatedCount=1;
    BSurface target(width,height,2);Backend backend;backend.software=game::make_software_type_drawing(&target,red.LightConvert);
    game::MapDrawingContext drawing;drawing.types=backend.software;drawing.types.backend_context=&backend;
    drawing.types.backend.shape=Backend::shape;drawing.types.backend.raster=Backend::raster;
    drawing.plain_palette=Backend::plain;drawing.color_scheme_palette=Backend::scheme;
    game::MapDrawStatistics stats;
    const auto render=[&](int frame){
        Unsorted::CurrentFrame=frame;backend.shapes.clear();backend.order.clear();
        game::GameUiFrame ui{drawing,resources,stats};
        EXPECT_EQ(game::with_game_ui_frame(ui,[]{EXPECT_TRUE(RadarClass::Instance.RenderRadar());}),game::DrawingStatus::drawn);
        EXPECT_EQ(Unsorted::CurrentFrame,frame);return pixels(target);
    };
    target.Fill(0x2104);beacon->Bitfield=0;const auto background=render(0);beacon->Bitfield=1;
    const int frame=manager.RadarBeaconFrameCount/2;
    const auto visible=render(frame);ASSERT_EQ(backend.shapes.size(),1u);EXPECT_NE(visible,background);
    const auto request=backend.shapes[0];const auto& rect=radar.unknown_rect_149C;
    Point2D point;ASSERT_NE(radar.GetCrdOnRadar(&point,&beacon->Coord,true),nullptr);
    EXPECT_EQ(request.position,point);EXPECT_EQ(request.frame,frame);
    EXPECT_EQ(request.flags,0x600u);EXPECT_EQ(request.clip.X,72+rect.X);EXPECT_EQ(request.clip.Y,rect.Y);
    EXPECT_EQ(render(frame),visible) << "same-frame repaint does not advance animation";
    EXPECT_EQ(render(manager.RadarBeaconFrameCount),background) << "erase frame is fully replaced by background";
    EXPECT_TRUE(backend.shapes.empty());
    EXPECT_EQ(render(manager.RadarBeaconAnimPeriod-1),background);EXPECT_TRUE(backend.shapes.empty());
    EXPECT_EQ(render(manager.RadarBeaconAnimPeriod+frame),visible);
    beacon->HouseID=ally.ArrayIndex;const auto other_color=render(frame);EXPECT_NE(other_color,visible);
    artifact(visible,background,other_color);
    ally.Defeated=true;EXPECT_EQ(render(frame),background);ally.Defeated=false;
    ally.Allies.data=0;EXPECT_EQ(render(frame),background);ally.Allies=owner.Allies;
    owner.Allies.data=1u<<owner.ArrayIndex;EXPECT_EQ(render(frame),background);owner.Allies=ally.Allies;
    // Surface entry uses unshifted content coordinates and original LightConvert.
    BSurface legacy(width,height,2);legacy.Fill(0);
    auto context=game::make_software_type_drawing(&legacy,blue.LightConvert);
    struct Call {BeaconClass* object;Surface* target;RectangleStruct bounds;} call{beacon,&legacy,{0,0,rect.Width,rect.Height}};
    EXPECT_EQ(game::with_type_drawing(context,[](void* p){auto& c=*static_cast<Call*>(p);c.object->DrawRadar(c.target,c.bounds);},&call),game::DrawingStatus::drawn);
    const auto legacy_pixels=pixels(legacy);const auto modern=render(frame);
    for(int y=0;y<rect.Height;++y)for(int x=0;x<rect.Width;++x)
        if(legacy_pixels[y*width+x])EXPECT_EQ(modern[(y+rect.Y)*width+x+72+rect.X],legacy_pixels[y*width+x]);
    // Projection clamps the marker center; sprite pixels must still clip at all edges.
    for(CoordStruct coord: {CoordStruct{0,0,0},CoordStruct{100000,0,0},CoordStruct{0,100000,0},CoordStruct{100000,100000,0}}){
        beacon->Coord=coord;const auto clipped=render(frame);
        for(int y=0;y<height;++y)for(int x=0;x<width;++x)
            if(x<72+rect.X || x>=72+rect.X+rect.Width || y<rect.Y || y>=rect.Y+rect.Height)
                EXPECT_EQ(clipped[y*width+x],background[y*width+x]);
    }
    beacon->Coord={100*256,100*256,0};
    auto rules=std::make_unique<RulesClass>();RulesClass::Instance=rules.get();
    ASSERT_TRUE(RadarEventClass::Create(RadarEventType::BeaconPlaced,{100,100}));
    auto* event=RadarEventClass::Array[0];event->Speed=12;event->ColorValue=0.5;event->ColorSpeed=0.05;
    render(frame);const auto shape_at=std::find(backend.order.begin(),backend.order.end(),'S');
    ASSERT_NE(shape_at,backend.order.end());EXPECT_NE(std::find(backend.order.begin(),shape_at,'E'),shape_at);
    EXPECT_EQ(std::find(shape_at+1,backend.order.end(),'E'),backend.order.end()) << "events precede beacon shapes";
    RadarEventClass::Clear();RulesClass::Instance=old_rules;
    // Backend failure is propagated through the borrowed UI status.
    drawing.color_scheme_palette=[](void*,const BytePalette&,int,const game::DrawingPaletteHandle*&) noexcept {return game::DrawingStatus::backend_failure;};
    game::GameUiFrame failure{drawing,resources,stats};
    EXPECT_EQ(game::with_game_ui_frame(failure,[]{RadarClass::Instance.RenderRadar();}),game::DrawingStatus::backend_failure);
    manager.Reset();
    for(int side:{1,2,0,1,0}){
        resources.clear();EXPECT_EQ(manager.BeaconArt,nullptr);EXPECT_EQ(manager.RadarBeaconArt,nullptr);
        ASSERT_TRUE(resources.load(side))<<resources.error();
        ASSERT_NE(manager.RadarBeaconArt,nullptr);EXPECT_EQ(manager.RadarBeaconFrameCount,manager.RadarBeaconArt->Frames);
        // Exercise lazy data before retirement, including its shape-registry unlink.
        auto* reference=manager.RadarBeaconArt;
        ASSERT_NE(reference->GetData(),nullptr);
    }
    game::UiResources newer;ASSERT_TRUE(newer.load(0));retained=manager.RadarBeaconArt;
    resources.clear();EXPECT_EQ(manager.RadarBeaconArt,retained) << "retired UI owner must not free newer art";
    newer.clear();EXPECT_EQ(manager.RadarBeaconArt,nullptr);
    // A real loose file is read directly. The detached physical root cannot
    // reopen MIX files; the missing second image must retain its metadata.
    CCFileClass original("PBEACON.SHP");
    std::vector<char> bytes(original.GetFileSize());ASSERT_FALSE(bytes.empty());
    ASSERT_EQ(original.ReadBytes(bytes.data(),int(bytes.size())),int(bytes.size()));
    const auto temp=std::filesystem::temp_directory_path()/
        ("ra2-beacon-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(temp));
    const auto remove_temp=ra2::test::scope_exit([&]{std::error_code ignored;std::filesystem::remove_all(temp,ignored);});
    {std::ofstream file(temp/"PBEACON.SHP",std::ios::binary);file.write(bytes.data(),bytes.size());ASSERT_TRUE(file.good());}
    {
        const int old_period=manager.RadarBeaconAnimPeriod;
        game::FileSystem loose_files(temp);game::FileSystemScope loose_scope(&loose_files);
        manager.LoadArt();ASSERT_NE(manager.BeaconArt,nullptr);EXPECT_EQ(manager.RadarBeaconArt,nullptr);
        EXPECT_FALSE(manager.BeaconArt->IsReference());EXPECT_EQ(manager.RadarBeaconAnimPeriod,old_period);
        EXPECT_EQ(manager.BeaconFrameCount,reinterpret_cast<const SHPStruct*>(bytes.data())->Frames);
        ASSERT_NE(manager.BeaconArt->GetPixels(0),nullptr);
        EXPECT_TRUE(std::filesystem::remove(temp/"PBEACON.SHP"));manager.LoadArt();
        EXPECT_EQ(manager.BeaconArt,nullptr);EXPECT_EQ(manager.RadarBeaconArt,nullptr);
    }
    // Returning to the actual resource root reloads the original MIX fallback.
    manager.LoadArt();ASSERT_NE(manager.RadarBeaconArt,nullptr);EXPECT_TRUE(manager.RadarBeaconArt->IsReference());
    ASSERT_NE(manager.BeaconArt,nullptr);EXPECT_TRUE(manager.BeaconArt->IsReference());
    manager.BeaconArt->Load();ASSERT_NE(manager.BeaconArt->GetData(),nullptr);
    manager.ReleaseArt();manager.ReleaseArt();EXPECT_EQ(manager.BeaconArt,nullptr);EXPECT_EQ(manager.RadarBeaconArt,nullptr);
}
}
TEST(BeaconDrawing, OriginalArtPixelsAnimationAndRetirement) {
    const char* data=std::getenv("RA2_GAME_DATA");
    if(!data || !*data)GTEST_SKIP()<<"RA2_GAME_DATA enables original beacon resource integration";
    game::ResourceHandle* raw=nullptr;std::string error;
    ASSERT_TRUE(game::create_resources(data,raw,error))<<error;
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(raw,game::destroy_resources);
    ASSERT_EQ(game::load_resources(*files,{},error),game::ResourceLoadResult::complete)<<error;
    EXPECT_TRUE(game::with_resources(*files,[](void*){exercise_drawing();},nullptr,error))<<error;
}
