#include "support/test_support.hpp"
#include "support/map_test_support.hpp"
#include "api/filesystem.hpp"
#include "api/map_objects.hpp"
#include "map_view.hpp"
#include "map_world.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/FileFormats/SHP.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
using map_fixture::binary;
using map_fixture::shp;

void building_coordinates() {
    std::ifstream input(RA2_BUILDING_COORDINATES_FIXTURE);
    EXPECT_TRUE((bool(input))) << "open original building coordinate fixture";
    BuildingTypeClass type("GEOMETRY",BuildingTypeClass::ConstructionDefaults{});
    BuildingClass building(&type,nullptr);
    int foundation,count=0;
    while(input>>foundation) {
        CoordStruct location,render,center,targetCoords;Point2D screen;
        input>>location.X>>location.Y>>location.Z>>render.X>>render.Y>>render.Z
            >>center.X>>center.Y>>center.Z>>screen.X>>screen.Y
            >>targetCoords.X>>targetCoords.Y>>targetCoords.Z;
        EXPECT_TRUE((bool(input))) << "complete original building coordinate row";
        building.Location=location;type.Foundation=static_cast<Foundation>(foundation);type.TargetCoordOffset={15,-31,64};
        // Original Building vtable 0x7E3EBC + 0x48 dispatches to 0x447AC0.
        // Testing only the named center helper missed a wrong override slot.
        const AbstractClass* target=&building;CoordStruct actual;
        ASSERT_EQ(target->GetCoords(&actual),&actual);
        ASSERT_EQ(actual,center) << "building virtual GetCoords, foundation " << foundation;
        const ObjectClass* object=&building;
        ASSERT_EQ(object->GetTargetCoords(&actual),&actual);ASSERT_EQ(actual,targetCoords);
        EXPECT_TRUE((building.GetRenderCoords()==render)) << "original building sprite origin";
        EXPECT_TRUE((building.GetCenterCoords()==center)) << "original building foundation center";
        EXPECT_TRUE((TacticalClass::CoordsToScreen(building.GetRenderCoords())==screen)) << "original building height projection";
        ++count;
    }
    EXPECT_TRUE((input.eof()&&count==1056)) << "all original building coordinate samples";
}
void fixtures(const std::filesystem::path& root) {
    map_fixture::fixtures(root);
    map_fixture::edit(root/"RULESMD.INI", "[TerrainTypes]",
        "[SmudgeTypes]\n0=CRATER\n[CRATER]\nTheater=no\nWidth=3\nHeight=2\nCrater=yes\n[TerrainTypes]");
    map_fixture::shp(root/"CRATER.SHP",1,60,60);
    map_fixture::edit(root/"world.map", "[CATIME]", "[Smudge]\n0=CRATER,8,5,0\n[CATIME]");
    // Preserve this fixture's original missing-fog input.
    std::filesystem::remove(root/"FOG.SHP");
}
}

TEST(MapWorld, BuildingCoordinateVirtualDispatch) { building_coordinates(); }

TEST(MapWorld, Contracts) {
    const auto root=std::filesystem::temp_directory_path()/("ra2-world-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    struct Cleanup { std::filesystem::path p; ~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);} } cleanup{root};
    fixtures(root);
    game::ResourceHandle* raw=nullptr;std::string error;
    EXPECT_TRUE((game::create_resources(root.string(),raw,error))) << "resource fixture";
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> resources(raw,game::destroy_resources);
    game::MapViewHandle* view=nullptr;EXPECT_TRUE((game::create_map_view(*raw,view))) << "map allocation";
    struct Close { game::MapViewHandle*& p;~Close(){game::destroy_map_view(p);} } close{view};
    auto load=[&] {if(!game::load_map_view(*view,"world.map",9))throw std::runtime_error(game::map_view_error(*view));};
    load();
    EXPECT_TRUE(game::with_map_view(*view,[](void*){
        auto* type=SmudgeTypeClass::Find("CRATER");ASSERT_NE(type,nullptr);
        for(int y=0;y<2;++y)for(int x=0;x<3;++x){
            auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(8+x),short(5+y)});ASSERT_NE(cell,nullptr);
            EXPECT_EQ(cell->SmudgeTypeIndex,type->ArrayIndex);EXPECT_EQ(cell->SmudgeData,x+3*y);
        }
    },nullptr));
    EXPECT_TRUE((BuildingTypeClass::Find("CATIME")&&BuildingTypeClass::Find("CATIME")->Strength==0)) << "unused map-only type overrides keep defaults without rejecting the map";
    game::MapWorldSnapshot world{};EXPECT_TRUE((game::get_map_world_snapshot(*view,world)&&world.objects_loaded)) << "world loaded";
    EXPECT_TRUE((world.buildings==1&&world.terrain_objects==1&&world.animations==1)) << "real object and attachment counts";
    std::uint32_t count=0;EXPECT_TRUE((game::copy_map_objects(*view,nullptr,0,count)&&count==2)) << "object count ABI";
    game::MapObjectSnapshot one{};one.health=987;
    EXPECT_TRUE((!game::copy_map_objects(*view,&one,1,count)&&count==2&&one.health==987)) << "short buffer writes no partial snapshot";
    std::vector<game::MapObjectSnapshot> objects(count);EXPECT_TRUE((game::copy_map_objects(*view,objects.data(),count,count))) << "copy objects";
    auto building=objects[0].kind==game::MapObjectKind::building?objects[0]:objects[1];
    const auto id=building.id;
    EXPECT_TRUE((building.health==50&&building.max_health==100&&building.frame==1)) << "map HP uses 256 ratio and damaged body";
    EXPECT_TRUE((!std::strcmp(building.name,"Native Building"))) << "native fallback name";
    EXPECT_TRUE((AnimTypeClass::Find("SPIN")->Rate==900)) << "ART Rate=1 is 900 logical ticks before normalization";
    EXPECT_TRUE((BuildingClass::Array.Count==1&&BuildingClass::Array[0]->Health==50)) << "snapshot uses actual BuildingClass";
    EXPECT_TRUE((BuildingClass::Array[0]->Anims[3]&&!std::strcmp(BuildingClass::Array[0]->Anims[3]->Type->ID,"SPINDAM"))) << "damaged attachment";
    EXPECT_TRUE((game::set_map_object_health(*view,id,1000)&&game::get_map_object(*view,id,building)&&building.health==100)) << "clamp health to Strength";
    EXPECT_TRUE((!std::strcmp(BuildingClass::Array[0]->Anims[3]->Type->ID,"SPIN"))) << "healthy attachment switch";
    EXPECT_TRUE((BuildingClass::Array[0]->Anims[3]->ZAdjust==-20&&BuildingClass::Array[0]->Anims[3]->YSortAdjust==11)) << "building slot replaces animation type Z/Y defaults";
    EXPECT_TRUE((game::set_map_building_enabled(*view,id,false)&&!BuildingClass::Array[0]->StuffEnabled)) << "native enable state";
    EXPECT_TRUE((game::set_map_building_enabled(*view,id,true))) << "enable restore";
    const auto old_generation=view->generation;
    for(int i=0;i<12;++i)EXPECT_TRUE((game::update_game_view(*view,1.0/60.0))) << "world update";
    EXPECT_TRUE((game::get_map_world_snapshot(*view,world)&&world.simulation_tick>0&&world.resource_revision>0&&world.resource_cells>0)) << "drill wraps and creates real ore";
    EXPECT_TRUE((view->generation==old_generation)) << "animation must not invalidate entire world";
    bool harvested=false;
    for(int y=6;y<=8;++y)for(int x=7;x<=9;++x){
        game::MapResourceSnapshot ore{};
        if(game::get_map_resource(*view,x,y,ore)&&ore.units>0){
            EXPECT_TRUE((ore.total_value==std::int64_t(ore.units)*25)) << "ore value from actual cell quantity";
            int units=0;std::int64_t value=0;
            EXPECT_TRUE((game::harvest_map_resource(*view,x,y,1000,units,value)&&units==ore.frame&&value==std::int64_t(ore.frame)*ore.value_per_unit)) << "bulk harvest returns original credited yield, excluding the terminal frame";
            EXPECT_TRUE((game::get_map_resource(*view,x,y,ore)&&ore.units==0)) << "empty resource cell";harvested=true;
        }
    }
    EXPECT_TRUE((harvested)) << "drill creates ore in a neighbour";
    EXPECT_TRUE((game::with_map_view(*view,[](void*){
        auto*c=MapClass::Instance.TryGetCellAt(CellStruct{9,7});
        EXPECT_TRUE((c&&c->IncreaseTiberium(0,3))) << "visible ore drawing fixture";
        c->OverlayTypeIndex=113; // Draw must reselect the image from cell coordinates.
    },nullptr))) << "resource drawing setup";
    EXPECT_TRUE((game::set_map_viewport(*view,640,480))) << "viewport";
    struct Draw { Point2D body{},camera{};bool found=false,building_shadow=false,animation=false,animation_shadow=false,ore=false;unsigned shapes=0,rasters=0,smudges=0;char last=' '; } recorded;
    recorded.camera=view->tactical.TacticalPos;
    game::MapDrawingContext drawing;drawing.types.backend_context=&recorded;
    drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(&recorded);
    drawing.terrain_palette=[](void*,const BytePalette& source,int,int,int,int,const game::DrawingPaletteHandle*& out)noexcept{out=reinterpret_cast<const game::DrawingPaletteHandle*>(&source);return game::DrawingStatus::drawn;};
    drawing.shape_palette=[](void*,const BytePalette& source,int shades,const game::DrawingPaletteHandle*& out)noexcept{if(shades!=1&&shades!=53)return game::DrawingStatus::invalid_argument;out=reinterpret_cast<const game::DrawingPaletteHandle*>(&source);return game::DrawingStatus::drawn;};
    drawing.color_scheme_palette=drawing.shape_palette;
    drawing.types.backend.tile=[](void* p,const game::TileDrawingRequest&){static_cast<Draw*>(p)->last='T';return game::DrawingStatus::drawn;};
    drawing.types.backend.shape=[](void* p,const game::ShapeDrawingRequest& r){
        auto& d=*static_cast<Draw*>(p);++d.shapes;
        if(r.image==SmudgeTypeClass::Array[0]->Image){
            ++d.smudges;EXPECT_TRUE((d.last=='T')) << "smudge immediately follows its terrain, before world objects";
            EXPECT_TRUE((r.frame==0&&r.flags==0xE00&&r.depth_mode==game::ShapeDepthMode::legacy)) << "footprint cells redraw the original single-frame smudge";
            auto point=TacticalClass::CoordsToScreen(CoordStruct{8*256,5*256,0});
            point.X-=d.camera.X;point.Y-=d.camera.Y;
            EXPECT_TRUE((r.position==point)) << "loaded map smudge uses original top vertex and multi-cell offset";
        }
        d.last='S';
        if(reinterpret_cast<const BytePalette*>(r.palette)->Entries[42].R==44){
            d.ore=true;EXPECT_TRUE((r.flags==0x4E00&&r.gradient==0&&r.depth_adjustment==-2&&r.intensity==1000)) << "flat resources use original depth and global theater palette";
            bool cell_found=false;
            for(int i=0;i<MapClass::Instance.Cells.Capacity;++i)if(auto*c=MapClass::Instance.Cells[i]){
                if(!TiberiumClass::Find(c->OverlayTypeIndex))continue;
                auto point=TacticalClass::CoordsToScreen(CoordStruct{c->MapCoords.X*256+128,c->MapCoords.Y*256+128,0});
                point.X-=d.camera.X;point.Y-=d.camera.Y+12;
                if(point!=r.position)continue;
                cell_found=true;const int index=102+(c->MapCoords.X*c->MapCoords.Y)%12;
                EXPECT_TRUE((r.image==OverlayTypeClass::Array[index]->Image)) << "resource image variant is selected by coordinates";
            }
            EXPECT_TRUE((cell_found)) << "resource is drawn 12 pixels above cell center";
        }
        if(r.image==BuildingClass::Array[0]->Type->Image){
            EXPECT_TRUE((r.depth_mode==game::ShapeDepthMode::legacy)) << "building uses original SHP depth";
            if(r.flags&1){
                d.building_shadow=true;
                EXPECT_TRUE((r.frame==1&&r.flags==0x6E01&&r.gradient==0&&r.depth_adjustment==-4&&!r.depth_image)) << "original building shadow request";
            }else{
                d.body=r.position;d.found=true;
                EXPECT_TRUE((r.flags==0x6E00&&r.gradient==2&&r.depth_adjustment==-22)) << "original building body request including NormalZAdjust";
                EXPECT_TRUE((r.depth_image&&r.depth_offset==Point2D{200,413})) << "original ZShapePointMove and foundation sampling offset";
                auto*bytes=r.depth_image->GetPixels(0);
                EXPECT_TRUE((bytes[0]==0&&bytes[2*396+2]==BYTE(42-0x41))) << "BUILDNGZ.SHA signed bias preserves zero pixels";
                EXPECT_TRUE((r.intensity==MapClass::Instance.TryGetCellAt(BuildingClass::Array[0]->Location)->Intensity_Normal-10)) << "signed ExtraLight";
            }
        }
        if(r.image==BuildingClass::Array[0]->Anims[3]->Type->Image){
            EXPECT_TRUE((r.depth_mode==game::ShapeDepthMode::legacy)) << "animation uses original SHP depth";
            const auto*palette=reinterpret_cast<const BytePalette*>(r.palette);
            EXPECT_TRUE((palette->Entries[42].R==248)) << "building animation uses owner unit palette instead of ANIM.PAL";
            if(r.flags&1){d.animation_shadow=true;EXPECT_TRUE((r.flags==0x1E01&&r.gradient==0&&r.depth_adjustment==-2&&r.position==d.body)) << "animation shadow uses constructor Z flags and unshifted anchor";}
            else{d.animation=true;EXPECT_TRUE((r.flags==0x3E04&&r.gradient==2&&r.depth_adjustment==-15&&r.intensity==1000)) << "animation combines slot Z, YDrawOffset, translucency and normal light";
                EXPECT_TRUE((r.position==Point2D{d.body.X,d.body.Y+7})) << "slot pixel offset passes through original inverse projection";}
        }
        if(r.image==TerrainClass::Array[0]->Type->Image){EXPECT_TRUE((r.depth_mode==game::ShapeDepthMode::legacy)) << "terrain uses original SHP depth";EXPECT_TRUE(((r.flags==0x2E00&&r.gradient==2&&r.depth_adjustment==-12)||(r.flags==0x2E01&&r.gradient==0&&r.depth_adjustment==-3))) << "original terrain body and shadow requests";}
        return game::DrawingStatus::drawn;
    };
    drawing.types.backend.raster=[](void* p,const game::RasterDrawingRequest&){++static_cast<Draw*>(p)->rasters;return game::DrawingStatus::drawn;};
    game::MapDrawStatistics stats;EXPECT_TRUE((game::draw_map_view(*view,drawing,stats)==game::DrawingStatus::drawn&&recorded.found&&recorded.shapes>=3)) << "native object draw requests";
    EXPECT_TRUE((recorded.building_shadow)) << "original second-half building shadow is submitted";
    EXPECT_TRUE((recorded.animation&&recorded.animation_shadow)) << "building animation body and shadow submitted";
    EXPECT_TRUE((recorded.ore)) << "resource drawing request observed";
    EXPECT_TRUE((recorded.smudges==6)) << "each of the six footprint cells redraws the smudge after its terrain";
    // Original 0x00459EF0 -> 0x006D1F10 for Location=(2176,896,0).
    EXPECT_TRUE((recorded.body==Point2D{150-view->tactical.TacticalPos.X,165-view->tactical.TacticalPos.Y})) << "world body uses original render origin rather than foundation center";
    game::GameInputEvent input;game::GameInputResult result;
    input.kind=game::GameInputKind::pointer_move;input.x=recorded.body.X;input.y=recorded.body.Y;
    EXPECT_TRUE((game::submit_game_input(*view,input,result))) << "hover input";
    input.kind=game::GameInputKind::pointer_button;input.code=1;input.pressed=true;
    EXPECT_TRUE((game::submit_game_input(*view,input,result))) << "press input";input.pressed=false;
    EXPECT_TRUE((game::submit_game_input(*view,input,result))) << "release input";
    EXPECT_TRUE((game::get_map_object(*view,id,building)&&building.selected&&building.hovered)) << "opaque SHP pixel selects and hovers native building";
    EXPECT_TRUE((game::draw_map_view(*view,drawing,stats)==game::DrawingStatus::drawn&&recorded.rasters>=3)) << "health bar and selection primitives";
    EXPECT_TRUE((game::set_map_object_health(*view,id,0)&&BuildingClass::Array[0]->IsOnMap&&BuildingClass::Array[0]->C4Timer.GetTimeLeft()==8)) << "death starts original eight-tick removal";
    EXPECT_TRUE((!game::set_map_object_health(*view,id,1))) << "dead object cannot be revived by health command";
    load();EXPECT_TRUE((!game::get_map_object(*view,id,building)&&!game::set_map_object_health(*view,id,50))) << "reload rejects stale world IDs";
    binary(root/"DRILL.SHP",{0});
    EXPECT_TRUE((!game::load_map_view(*view,"world.map",9)&&
        std::strstr(game::map_view_error(*view),"Cannot load type DRILL"))) << "type failure identifies the failing type after rollback";
    EXPECT_TRUE((BuildingClass::Array.Count==0&&TerrainClass::Array.Count==0&&AnimClass::Array.Count==0)) << "failed type loading releases world ownership";
    shp(root/"DRILL.SHP",4);load();
    game::destroy_map_view(view);
    EXPECT_TRUE((BuildingClass::Array.Count==0&&TerrainClass::Array.Count==0&&AnimClass::Array.Count==0&&!MapClass::Instance.Cells.Items)) << "world teardown releases native ownership";
}
