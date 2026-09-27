#include "support/test_support.hpp"
#include "type_drawing_packets.hpp"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/FileFormats/SHP.h"
#include "api/filesystem.hpp"
#include <cassert>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {

void reference_residency() {
    auto root=std::filesystem::temp_directory_path()/
        ("ra2-packet-residency-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    struct Cleanup { std::filesystem::path root; ~Cleanup() {
        Unload_All_Shapes();std::error_code error;std::filesystem::remove_all(root,error);
    } } cleanup{root};
    auto write=[&](const char* name,bool rle) {
        const int width=rle?2:8;
        std::vector<byte> data(32+(rle?8:64),3);
        SHPStruct header;header.Width=header.Height=rle?4:8;header.Frames=1;
        SHPFrame frame{};frame.Width=frame.Height=width;frame.Offset=32;frame.Flags=rle?2:0;
        std::memcpy(data.data(),&header,8);std::memcpy(data.data()+8,&frame,24);
        if(rle){const byte rows[]={4,0,7,8,4,0,9,10};std::memcpy(data.data()+32,rows,8);}
        std::ofstream file(root/name,std::ios::binary);file.write(reinterpret_cast<const char*>(data.data()),data.size());
        EXPECT_TRUE((bool(file))) << "write resident SHP fixture";
    };
    write("body.shp",true);write("depth.shp",false);write("scratch.shp",false);
    game::ResourceHandle* raw=nullptr;std::string error;
    EXPECT_TRUE((game::create_resources(root.string(),raw,error))) << "create packet resources";
    const auto release=[](game::ResourceHandle* handle){Unload_All_Shapes();game::destroy_resources(handle);};
    std::unique_ptr<game::ResourceHandle,decltype(release)> resources(raw,release);
    const bool ok=game::with_resources(*raw,[](void* context) {
        const auto& root=*static_cast<const std::filesystem::path*>(context);
        SHPReference body("body.shp"),depth("depth.shp"),scratch("scratch.shp");
        game::ShapeDrawingRequest request;request.image=&body;request.depth_image=&depth;
        request.depth_mode=game::ShapeDepthMode::legacy;request.flags=0x6E00;
        request.depth_offset={4,4};request.position={20,20};request.clip={0,0,100,100};
        game::TypeGpuTarget target{100,100,33,0,65535,true};game::TypeGpuPacket first,next;
        EXPECT_TRUE((game::prepare_type_shape(request,target,first)==game::DrawingStatus::drawn)) << "reference body and auxiliary decode";
        EXPECT_TRUE((first.texels==std::vector<std::uint32_t>({0x10307,0x10308,0x10309,0x1030A}))) << "body pixels survive resolving a differently sized auxiliary SHP";
        // Warm drawing must keep working without another file read, even when
        // an unrelated original GetData call overwrites the shared scratch.
        EXPECT_TRUE((std::filesystem::remove(root/"body.shp")&&std::filesystem::remove(root/"depth.shp"))) << "remove backing SHPs";
        EXPECT_TRUE((scratch.GetData()!=nullptr)) << "evict shared scratch";
        EXPECT_TRUE((game::prepare_type_shape(request,target,next)==game::DrawingStatus::drawn&&
            next.parameters==first.parameters&&next.texels==first.texels)) << "warm packet reuses resident body and depth bytes";
        body.Unload();depth.Unload();
        EXPECT_TRUE((game::prepare_type_shape(request,target,next)==game::DrawingStatus::unavailable)) << "explicit unload releases resident data and preserves missing-file failure";
    },&root,error);
    EXPECT_TRUE((ok)) << error.c_str();
}
}

TEST(TypeDrawingPacket, Contracts) {
    reference_residency();
    game::TypeGpuTarget target{100,100,33,0,65535,true};
    game::TypeGpuPacket p,e;
    std::vector<byte> bytes(sizeof(SHPStruct)+sizeof(SHPFrame)+16,0);
    auto* sh=new(bytes.data()) SHPStruct;sh->Width=8;sh->Height=8;sh->Frames=1;
    auto* fr=reinterpret_cast<SHPFrame*>(bytes.data()+sizeof(SHPStruct));
    fr->Width=4;fr->Height=4;fr->Left=2;fr->Top=1;fr->Offset=int(bytes.size()-16);
    bytes[fr->Offset]=7;
    game::ShapeDrawingRequest shape;shape.image=sh;shape.flags=0xE00;shape.position={20,30};shape.clip={5,6,80,80};
    EXPECT_TRUE((game::prepare_type_shape(shape,target,p)==game::DrawingStatus::drawn));
    EXPECT_TRUE((p.parameters[4]==23 && p.parameters[5]==33 && p.parameters[10]==1));
    EXPECT_TRUE((p.texels[0]==0x10007 && p.texels[1]==0 && p.parameters[13]==0));
    shape.flags=0xE10;EXPECT_TRUE((game::prepare_type_shape(shape,target,p)==game::DrawingStatus::unsupported));
    shape.flags=0x600;shape.position={INT32_MAX,0};EXPECT_TRUE((game::prepare_type_shape(shape,target,p)==game::DrawingStatus::invalid_argument));
    shape.position={20,30}; shape.depth_mode=game::ShapeDepthMode::read_write;
    shape.absolute_depth=12345; shape.blend_mode=game::ShapeBlendMode::shadow;
    EXPECT_TRUE((game::prepare_type_shape_parameters(shape,target,p)==game::DrawingStatus::drawn));
    EXPECT_TRUE((p.texels.empty() && p.parameters[13]==257 && p.parameters[14]==12345));
    EXPECT_TRUE((game::decode_type_shape(shape,p.texels)==game::DrawingStatus::drawn && p.texels[0]==0x10007));
    shape.depth_mode=game::ShapeDepthMode::read; shape.blend_mode=game::ShapeBlendMode::translucent50;
    EXPECT_TRUE((game::prepare_type_shape(shape,target,p)==game::DrawingStatus::drawn && p.parameters[13]==770));
    shape.absolute_depth=-1;
    EXPECT_TRUE((game::prepare_type_shape(shape,target,p)==game::DrawingStatus::invalid_argument));
    shape.absolute_depth=0; shape.blend_mode=static_cast<game::ShapeBlendMode>(99);
    EXPECT_TRUE((game::prepare_type_shape(shape,target,p)==game::DrawingStatus::unsupported));
    std::vector<byte> tile(sizeof(TMPImage)+900+900+4+4,0);
    auto* im=reinterpret_cast<TMPImage*>(tile.data());im->Flags=3;im->ZOffset=sizeof(TMPImage)+900;
    im->ExtraOffset=sizeof(TMPImage)+1800;im->ExtraZOffset=im->ExtraOffset+4;
    im->ExtraWidth=2;im->ExtraHeight=2;im->ExtraX=25;im->ExtraY=-2;
    tile[im->ExtraOffset]=9;tile[im->ExtraZOffset]=27;
    TMPStruct resource{1,1,60,30};game::TileDrawingRequest r;
    r.resource=&resource;r.image=im;r.position={10,40};r.clip={0,0,100,100};r.level=2;r.use_depth=true;
    EXPECT_TRUE((game::prepare_type_tile(r,target,p,e)==game::DrawingStatus::drawn));
    int count=0;for(auto v:p.texels)if(v&0x10000)++count;EXPECT_TRUE((count==900));
    EXPECT_TRUE((p.texels[28]==0x10000 && p.parameters[14]==65435 && p.parameters[13]==1));
    EXPECT_TRUE((e.parameters[4]==35 && e.parameters[5]==38 && e.parameters[15]==1));
    EXPECT_TRUE((e.texels[0]==(0x10009u|(27u<<8)) && e.texels[1]==0));
    r.flag16=true;EXPECT_TRUE((game::prepare_type_tile(r,target,p,e)==game::DrawingStatus::unsupported));
    r.flag16=false;r.level=INT32_MIN;EXPECT_TRUE((game::prepare_type_tile(r,target,p,e)==game::DrawingStatus::invalid_argument));
}
