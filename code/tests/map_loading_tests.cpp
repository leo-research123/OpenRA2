#include "support/test_support.hpp"
#include "map_world_fixture.hpp"
#include "api/filesystem.hpp"
#include "map_view.hpp"
#include "yrpp/CCINIClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/Pipes.h"
#include "yrpp/Straws.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

struct View { game::MapViewHandle* pointer=nullptr; ~View() { game::destroy_map_view(pointer); } };
void put(std::vector<byte>& bytes,int offset,std::uint32_t value) { std::memcpy(bytes.data()+offset,&value,4); }
void tile_file(const std::filesystem::path& path,int terrain,int slope) {
    std::vector<byte> bytes(20+52+900);
    put(bytes,0,1); put(bytes,4,1); put(bytes,8,60); put(bytes,12,30); put(bytes,16,20);
    bytes[20+41]=static_cast<byte>(terrain); bytes[20+42]=static_cast<byte>(slope);
    bytes[20+43]=50; bytes[20+44]=100; bytes[20+45]=150;
    std::fill(bytes.begin()+72,bytes.end(),42);
    std::ofstream file(path,std::ios::binary); file.write(reinterpret_cast<char*>(bytes.data()),bytes.size());
}
void parse(CCINIClass& ini,const char* text) {
    BufferStraw input(const_cast<char*>(text),static_cast<int>(std::strlen(text)));
    EXPECT_TRUE((ini.ReadStraw(input)>0)) << "fixture INI";
}
void write_map(const std::filesystem::path& path,const char* theater,bool corrupt=false) {
    CCINIClass ini;
    parse(ini,"[Map]\nSize=0,0,8,12\nLocalSize=2,2,4,4\nLevel=2\n[Basic]\nNewINIFormat=4\n[Waypoints]\n0=1008\n"
        "[Lighting]\nAmbient=.8\nRed=1.5\nGreen=1\nBlue=.5\nGround=.2\nLevel=.1\n");
    ini.WriteString("Map","Theater",theater);
    // Pack3 then Pack5 target the same cell. The latter must win, with its ice byte.
    byte earlier[]{8,0,1,0,1,0,0,0,0,4,0,0,0,0};
    EXPECT_TRUE((ini.WriteUUBlock("IsoMapPack3",earlier,sizeof(earlier)))) << "write earlier map format";
    byte record[]{8,0,1,0,1,0,0,0,0,3,5,8,0,2,0,0,0,0,0,7,2,0,0,0,0,0};
    byte compressed[256]; BufferPipe packed(compressed,sizeof(compressed));
    LZOPipe lzo(0,8192); lzo.Put_To(packed); lzo.Put(record,corrupt ? 8 : sizeof(record)); lzo.Flush();
    EXPECT_TRUE((ini.WriteUUBlock("IsoMapPack5",compressed,packed.Index))) << "write final map format";
    std::vector<byte> output(4096); BufferPipe target(output.data(),static_cast<int>(output.size()));
    EXPECT_TRUE((ini.WritePipe(target)>0)) << "serialize fixture INI";
    std::ofstream file(path,std::ios::binary); file.write(reinterpret_cast<char*>(output.data()),target.Index);
}
void load(game::MapViewHandle& view,const char* filename) {
    if (!game::load_map_view(view,filename,static_cast<unsigned>(std::strlen(filename))))
        throw std::runtime_error(std::string(filename)+": "+game::map_view_error(view));
}
void synthetic() {
    const auto root=std::filesystem::temp_directory_path()/
        ("ra2-map-loading-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(root);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code ec; std::filesystem::remove_all(path,ec); } } cleanup{root};
    std::ofstream(root/"KEYBOARDMD.INI") << "[Hotkey]\nDeployObject=68\nStopObject=83\n";
    map_fixture::shp(root/"SHROUD.SHP",48,60,30,127);
    const char* catalog="[General]\nClearTile=0\n[TileSet0000]\nTilesInSet=2\nFileName=T\n";
    std::ofstream(root/"TEMPERATMD.INI") << catalog; std::ofstream(root/"SNOWMD.INI") << catalog;
    std::vector<byte> palette(768);
    for (std::size_t i=0;i<palette.size();++i) palette[i]=static_cast<byte>(i%64);
    for (const auto* name : {"ISOTEM.PAL","ISOSNO.PAL"}) {
        std::ofstream file(root/name,std::ios::binary); file.write(reinterpret_cast<const char*>(palette.data()),palette.size());
    }
    tile_file(root/"T01.TEM",0,0); tile_file(root/"T02.TEM",7,2); tile_file(root/"T02.SNO",9,4);
    tile_file(root/"T01a.TEM",0,0); tile_file(root/"T01b.TEM",0,0);
    write_map(root/"temperate.map","TEMPERATE"); write_map(root/"snow.map","SNOW");
    write_map(root/"broken.map","TEMPERATE",true);
    game::ResourceHandle* handle=nullptr; std::string error;
    EXPECT_TRUE((game::create_resources(root.string(),handle,error))) << "create map resources";
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(handle,game::destroy_resources);
    View owner; EXPECT_TRUE((game::create_map_view(*handle,owner.pointer))) << "create map world";
    auto& view=*owner.pointer;
    for (int i=0;i<6;++i) {
        load(view,i%2 ? "snow.map" : "temperate.map");
        game::MapViewInfo info{}; EXPECT_TRUE((game::get_map_view_info(view,info))) << "inspect loaded terrain";
        EXPECT_TRUE((info.state==game::MapViewState::ready && info.terrain_loaded && info.generation==static_cast<unsigned>(i+1) &&
            info.width==8 && info.height==12 && info.visible_x==2 && info.visible_y==2 && info.visible_width==4 && info.visible_height==4)) << "publish dimensions, visible rectangle and completed terrain only";
        const auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{8,1});
        EXPECT_TRUE((cell && cell->IsoTileTypeIndex==1 && cell->Height==0 && cell->Level==3 && cell->IsIceGrowthAllowed==5)) << "later pack overwrites tile, level and ice in the real Cell";
        const auto color=cell->GetTerrainRadarColor();
        EXPECT_TRUE((color.R==(i%2 ? 20 : 25) && color.G==(i%2 ? 40 : 50) && color.B==(i%2 ? 60 : 75))) << "terrain radar uses TMP RGB and current theater brightness, independent of main terrain tint";
        game::MapRadarInfo radar{};
        EXPECT_TRUE((game::get_map_radar_info(view,radar) && radar.width>0 && radar.height>0 && !radar.viewport_valid)) << "loaded radar exists before the first main-map viewport";
        std::vector<std::uint16_t> radar_pixels(radar.width*radar.height);
        EXPECT_TRUE((game::copy_map_radar_pixels(view,radar_pixels.data(),radar_pixels.size()) &&
            !game::copy_map_radar_pixels(view,radar_pixels.data(),radar_pixels.size()-1))) << "copy radar pixels checks destination capacity";
        EXPECT_TRUE((cell->SlopeIndex==(i%2 ? 4 : 2) && static_cast<int>(cell->LandType)==(i%2 ? 2 : 3))) << "slope and land type come from the selected theater TMP";
        EXPECT_TRUE((cell->Intensity==98304 && cell->Intensity_Normal==900 && cell->Intensity_Terrain==1350 &&
            cell->Color1_Blue==1950 && cell->Color2_Red==1000 && cell->Color2_Green==666 && cell->Color2_Blue==333)) << "original lighting splits normalized tint and cell brightness";
        EXPECT_TRUE((FileSystem::ISOx_PAL.Entries[0].G==4 && FileSystem::ISOx_PAL.Entries[0].B==8 &&
            FileSystem::TEMPERAT_PAL.Entries[255].R==255 && FileSystem::TEMPERAT_PAL.Entries[255].G==0 &&
            FileSystem::TEMPERAT_PAL.Entries[255].B==252)) << "ISO palette shift and original missing-screen-palette gradient";
        EXPECT_TRUE((view.scenario.Waypoints[0]==CellStruct{8,1} && (cell->Flags&CellFlags::IsWaypoint)!=CellFlags::Empty)) << "waypoints attach to the same normally constructed cell";
        const auto* hole=MapClass::Instance.TryGetCellAt(CellStruct{8,2});
        EXPECT_TRUE((hole && hole->IsoTileTypeIndex==0xffff && !hole->Height && !hole->SlopeIndex)) << "invalid subtile becomes original clear sentinel";
        EXPECT_TRUE((IsometricTileTypeClass::Array.Count==2 && Theater::LastTheater==view.scenario.Theater &&
            !ScenarioClass::Instance)) << "reload owns one catalog and restores Scenario scope";
        EXPECT_TRUE((cell->UniqueID==1000001 && view.scenario.UniqueID==1010181)) << "cells precede the original theater ID reservation";
        DWORD usage=0;
        for (int type=0;type<IsometricTileTypeClass::Array.Count;++type)
            for (auto* tile=IsometricTileTypeClass::Array[type];tile;tile=tile->NextVariant) {
                usage+=tile->unk_308;
                EXPECT_TRUE((!tile->unk_308 || tile->Image)) << "every used variant is resident after loading";
            }
        EXPECT_TRUE((usage==180 && IsometricTileTypeClass::Array[0]->unk_2F0==3 &&
            IsometricTileTypeClass::Array[0]->NextVariant->unk_308>0)) << "actual cells populate the original variant usage counters";
        if (i==0) {
            struct Recorder {
                int last_y=INT32_MIN,last_x=INT32_MIN;
                unsigned draws=0,shrouds=0;
                bool valid=true,saw_clear_shroud=false;
            } recorder;
            game::MapDrawingContext context;
            context.types.backend_context=&recorder;
            context.types.target=reinterpret_cast<game::DrawingTargetHandle*>(&recorder);
            context.types.backend.lighting_shape=[](void* data,const game::LightingShapeDrawingRequest& request) {
                auto& recorder=*static_cast<Recorder*>(data);
                // 0x004801F0 submits even clear frame zero, before TMP draws.
                recorder.valid&=!recorder.draws && request.operation==game::RasterBlendMode::shroud;
                auto* image=request.image;
                if(auto* ref=image->AsReference()){ref->Load();image=ref->Data;}
                if(!image || request.frame<0 || request.frame>=image->Frames || !image->GetPixels(request.frame))
                    return game::DrawingStatus::unavailable;
                ++recorder.shrouds;recorder.saw_clear_shroud|=request.frame==0;
                return game::DrawingStatus::drawn;
            };
            context.terrain_palette=[](void* data,const BytePalette&,int r,int g,int b,int shades,
                const game::DrawingPaletteHandle*& output) noexcept {
                auto& recorder=*static_cast<Recorder*>(data);
                recorder.valid&=r==992 && g==640 && b==320 && shades==27;
                output=reinterpret_cast<const game::DrawingPaletteHandle*>(data);
                return game::DrawingStatus::drawn;
            };
            context.types.backend.tile=[](void* data,const game::TileDrawingRequest& request) {
                auto& recorder=*static_cast<Recorder*>(data);
                const int y=request.position.Y+15*request.level;
                recorder.valid&=y>recorder.last_y || (y==recorder.last_y && request.position.X>=recorder.last_x);
                recorder.valid&=request.clip.Width==200 && request.clip.Height==140 && request.use_depth && request.resource && request.image;
                recorder.last_y=y; recorder.last_x=request.position.X; ++recorder.draws;
                return game::DrawingStatus::drawn;
            };
            game::MapDrawStatistics stats;
            EXPECT_TRUE((!game::set_map_viewport(view,0,140) && game::set_map_viewport(view,200,140))) << "viewport rejects invalid dimensions";
            EXPECT_EQ(game::draw_map_view(view,context,stats),game::DrawingStatus::unavailable);
            EXPECT_NE(std::strstr(game::map_view_error(view),"shape palette callback"),nullptr);
            context.shape_palette=[](void* data,const BytePalette&,int shades,const game::DrawingPaletteHandle*& output) noexcept {
                auto& recorder=*static_cast<Recorder*>(data);recorder.valid&=shades==1 || shades==53;
                output=reinterpret_cast<const game::DrawingPaletteHandle*>(data);
                return game::DrawingStatus::drawn;
            };
            recorder={};
            // 0x006D7560 visits a viewport band, not all 180 map cells.
            // x86/check_map_tile_visits.py executes its original loop for
            // this camera/origin and verifies 154 unique legal candidates.
            EXPECT_EQ(view.tactical.TacticalPos,(Point2D{-100,215}));
            EXPECT_EQ(view.tactical.ApplyMatrix_Pixel(view.tactical.TacticalPos),(Point2D{1408,2261}));
            EXPECT_TRUE((game::draw_map_view(view,context,stats)==game::DrawingStatus::drawn && recorder.valid &&
                stats.visited==154 && stats.visited==stats.drawn+stats.skipped &&
                stats.drawn==recorder.draws && stats.drawn>0 && stats.skipped>0 &&
                recorder.shrouds>0 && recorder.saw_clear_shroud)) << "map draw uses original viewport traversal, shroud before tiles, clipping and quantized light palette";
            game::MapViewInfo before{},after{}; game::get_map_view_info(view,before);
            EXPECT_TRUE((game::set_map_viewport(view,100,80))) << "resize map viewport"; game::get_map_view_info(view,after);
            EXPECT_TRUE((before.camera_x+100==after.camera_x+50 && before.camera_y+70==after.camera_y+40)) << "viewport resize preserves camera center in Tactical";
            EXPECT_TRUE((game::scroll_map_view(view,INT_MAX,INT_MIN))) << "extreme mouse motion is clamped without overflow";
            game::get_map_view_info(view,after);
            EXPECT_TRUE((after.camera_x==20 && after.camera_y==105 &&
                view.tactical.TacticalCoord1==Point2D{70,145})) << "scroll uses original visible bounds and real center fields";
            const auto revision=after.camera_generation;
            EXPECT_TRUE((game::scroll_map_view(view,INT_MAX,INT_MIN))) << "repeat blocked direction";
            game::get_map_view_info(view,after);
            EXPECT_TRUE((after.camera_generation==revision)) << "clamped motion does not produce endless new frames";
            EXPECT_TRUE((game::set_map_viewport(view,1280,720))) << "viewport larger than small map";
            const auto center=view.tactical.TacticalCoord1;
            EXPECT_TRUE((game::scroll_map_view(view,INT_MIN,INT_MAX) && view.tactical.TacticalCoord1==center)) << "oversized viewport keeps the small map centered on both axes";
        }
    }
    EXPECT_TRUE((!game::load_map_view(view,"missing.map",11) &&
        std::strstr(game::map_view_error(view),"Map file not found"))) << "missing map has a file lookup diagnostic";
    std::ofstream(root/"no-map.map") << "[Basic]\nName=Missing map section\n";
    EXPECT_TRUE((!game::load_map_view(view,"no-map.map",10) &&
        std::strstr(game::map_view_error(view),"missing the [Map] section"))) << "readable INI without Map has a distinct diagnostic";
    EXPECT_TRUE((!game::load_map_view(view,"broken.map",10))) << "incomplete packed record fails";
    game::MapViewInfo info{}; game::get_map_view_info(view,info);
    EXPECT_TRUE((info.state==game::MapViewState::failed && !info.terrain_loaded && !MapClass::Instance.Cells.Items &&
        !IsometricTileTypeClass::Array.Count && Theater::LastTheater==TheaterType::None)) << "failed reload releases partial terrain and resources";
    load(view,"temperate.map");
    const auto screen_before=FileSystem::TEMPERAT_PAL,iso_before=FileSystem::ISOx_PAL;
    std::ofstream(root/"ISOTEM.PAL",std::ios::binary) << "bad";
    EXPECT_TRUE((game::with_map_view(view,[](void*) {
        EXPECT_TRUE((!FileSystem::LoadTheaterPalettes(TheaterType::Temperate))) << "short ISO palette fails";
    },nullptr))) << "palette failure resource scope";
    EXPECT_TRUE((!std::memcmp(&screen_before,&FileSystem::TEMPERAT_PAL,sizeof(screen_before)) &&
        !std::memcmp(&iso_before,&FileSystem::ISOx_PAL,sizeof(iso_before)))) << "palette failure publishes neither partial palette";
    {
        std::ofstream file(root/"ISOTEM.PAL",std::ios::binary);
        file.write(reinterpret_cast<const char*>(palette.data()),palette.size());
    }
    std::filesystem::remove(root/"T02.TEM");
    EXPECT_TRUE((!game::load_map_view(view,"temperate.map",13) && !IsometricTileTypeClass::Array.Count)) << "referenced missing TMP fails explicitly";
    game::destroy_map_view(owner.pointer);
    EXPECT_TRUE((!TacticalClass::Instance && !MapClass::Instance.Cells.Items && !IsometricTileTypeClass::Array.Count)) << "world close releases actual objects";
    EXPECT_TRUE((!RadarClass::Instance.unknown_123C)) << "world close releases original radar buffer";
    std::cout << "Terrain map loading, ordered packs, slope, waypoint, theater switching and failure cleanup passed\n";
}
void real_files(int argc,char** argv) {
    game::ResourceHandle* handle=nullptr; std::string error;
    EXPECT_TRUE((game::create_resources(argv[1],handle,error))) << "create actual resources";
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(handle,game::destroy_resources);
    EXPECT_TRUE((game::load_resources(*handle,{},error)==game::ResourceLoadResult::complete)) << "bootstrap actual resources";
    View owner; EXPECT_TRUE((game::create_map_view(*handle,owner.pointer))) << "create actual world";
    std::vector<std::string> names;
    if (argc==3 && !std::strcmp(argv[2],"--campaigns")) {
        EXPECT_TRUE((game::with_resources(*handle,[](void* data) {
            auto& names=*static_cast<std::vector<std::string>*>(data);
            EXPECT_TRUE((MixFileClass::LoadMapMixes())) << "mount mission archives";
            for (const char* side : {"ALL","SOV"}) for (int n=1;n<=7;++n) for (char theater : {'U','D','S','T','L'}) {
                char name[32]; std::snprintf(name,sizeof(name),"%s%02d%cMD.MAP",side,n,theater);
                CCFileClass file(name); if (file.Exists()) names.emplace_back(name);
            }
        },&names,error))) << "discover existing mission files";
        EXPECT_TRUE((!names.empty())) << "mission archive contains known campaign filenames";
    } else for (int i=2;i<argc;++i) names.emplace_back(argv[i]);
    for (const auto& name : names) {
        load(*owner.pointer,name.c_str());
        game::MapViewInfo info{}; game::get_map_view_info(*owner.pointer,info);
        int cells=0,slopes=0,raised=0;
        for (int n=0;n<MapClass::Instance.Cells.Capacity;++n) if (auto* cell=MapClass::Instance.Cells.Items[n]) {
            ++cells; slopes+=cell->SlopeIndex!=0; raised+=cell->Level!=0;
        }
        std::cout << name << ": " << info.width << 'x' << info.height << ", theater=" << info.theater << ", cells=" << cells
            << ", slopes=" << slopes << ", raised=" << raised << ", visible=" << info.visible_x << ',' << info.visible_y << ','
            << info.visible_width << ',' << info.visible_height << '\n';
    }
}
}

TEST(MapLoading, Contracts) {
    const auto [argc, argv] = ra2::test::arguments();

         synthetic(); if (argc>1) real_files(argc,argv);
}
