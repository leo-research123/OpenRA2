// Test-only access to the production VXL rasterizer and GPU packet encoder.
// Inputs/expected indexes come from unchanged fixed-EXE instructions. Matching
// input matrices isolate rasterization; this does not validate Building::DrawIt.
#include "api/filesystem.hpp"
#include "building_voxel.hpp"
#include "type_drawing_packets.hpp"
#include "yrpp/CCFileClass.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
std::vector<std::uint8_t> unhex(const std::string& text) {
    if (text.size()%2) throw std::runtime_error("Odd hex length");
    std::vector<std::uint8_t> out(text.size()/2);
    for (std::size_t i=0;i<out.size();++i)
        out[i]=static_cast<std::uint8_t>(std::stoul(text.substr(i*2,2),nullptr,16));
    return out;
}
template<class T> void hex(std::ostream& out,const T* values,std::size_t count) {
    constexpr char digits[]="0123456789abcdef";
    const auto* bytes=reinterpret_cast<const unsigned char*>(values);
    for(std::size_t i=0;i<count*sizeof(T);++i) out<<digits[bytes[i]>>4]<<digits[bytes[i]&15];
    out<<'\n';
}
struct Run {
    std::ifstream input;
    std::ofstream packets,results;
    std::filesystem::path output;
    unsigned count=0,failed=0;
};
void execute(void* pointer) {
    auto& run=*static_cast<Run*>(pointer);
    std::string token;
    auto next=[&](){if(!(run.input>>token))throw std::runtime_error("Truncated reference");return token;};
    const auto format=next();
    if(format!="VXL_RASTER_REFERENCE_V2"&&format!="VXL_RASTER_REFERENCE_V3"&&format!="VXL_RASTER_REFERENCE_V4")throw std::runtime_error("Wrong reference format");
    constexpr int width=256,height=256;
    run.packets<<"SHP_GPU_PACKETS_V1\n"<<width<<' '<<height<<'\n';
    for(int i=0;i<4;++i)run.packets<<next()<<'\n';
    run.count=static_cast<unsigned>(std::stoul(next()));run.packets<<run.count<<'\n';
    game::VoxelPalette palette;
    if(!game::load_voxel_palette(palette))throw std::runtime_error("Cannot load VOXELS.VPL");
    run.results<<"case\tstatus\toriginal_covered\tnative_covered\tindex_mismatches\tcoverage_mismatches\trectangle_mismatches\tfirst_x\tfirst_y\n";
    for(unsigned c=0;c<run.count;++c) {
        const auto name=next(),asset=next();const auto frame=static_cast<unsigned>(std::stoul(next()));
        const bool buffered=std::stoul(next())!=0;
        const bool shadow=format!="VXL_RASTER_REFERENCE_V2"&&std::stoul(next())!=0;
        int expected_rect[4]{};
        if(format=="VXL_RASTER_REFERENCE_V4")for(auto& value:expected_rect)value=std::stoi(next());
        const auto matrix=unhex(next()),expected=unhex(next());
        const auto expected_color=next(),expected_z=next();
        if(matrix.size()!=48||expected.size()!=width*height)throw std::runtime_error("Invalid case size");
        CCFileClass vf((asset+".vxl").c_str()),hf((asset+".hva").c_str());
        VoxLib voxel(&vf,false);MotLib hva(&hf);
        if(voxel.Initialized||hva.LoadedFailed||!voxel.CountHeaders)throw std::runtime_error("Cannot load "+asset);
        hva.Scale(voxel.leaSectionTailer(0,0)->HVAMultiplier);
        VoxelStruct resource{&voxel,&hva};game::BuildingVoxelPart part;
        part.resource=&resource;part.frame=frame;part.use_buffer=buffered;part.shadow=shadow;std::memcpy(part.local.row,matrix.data(),48);
        game::VoxelSurface surface;
        const auto status=game::render_building_voxel(part,palette,surface);
        if(status!=game::DrawingStatus::drawn)throw std::runtime_error(name+": "+game::drawing_status_name(status));
        std::vector<std::uint8_t> actual(width*height);
        for(int y=0;y<surface.height;++y)for(int x=0;x<surface.width;++x) {
            const auto pixel=surface.pixels[y*surface.width+x];if(!(pixel&0x01000000u))continue;
            const int dx=128+surface.offset.X+x,dy=128+surface.offset.Y+y;
            if(dx<0||dy<0||dx>=width||dy>=height)throw std::runtime_error("Canvas too small for "+name);
            actual[dy*width+dx]=static_cast<std::uint8_t>(pixel);
        }
        unsigned mismatch=0,coverage=0,original_count=0,actual_count=0;int first=-1;
        for(int i=0;i<width*height;++i) {
            original_count+=expected[i]!=0;actual_count+=actual[i]!=0;
            coverage+=(expected[i]!=0)!=(actual[i]!=0);
            if(expected[i]!=actual[i]){++mismatch;if(first<0)first=i;}
        }
        const bool rectangle_mismatch=format=="VXL_RASTER_REFERENCE_V4"&&(surface.offset.X!=expected_rect[0]||surface.offset.Y!=expected_rect[1]||surface.width!=expected_rect[2]||surface.height!=expected_rect[3]);
        run.failed+=mismatch!=0||rectangle_mismatch;
        run.results<<name<<'\t'<<((mismatch||rectangle_mismatch)?"FAIL":"PASS")<<'\t'<<original_count<<'\t'<<actual_count
            <<'\t'<<mismatch<<'\t'<<coverage<<'\t'<<rectangle_mismatch<<'\t'<<(first<0?-1:first%width)<<'\t'<<(first<0?-1:first/width)<<'\n';
        std::ofstream pixels(run.output/(name+".native.indices"),std::ios::binary);
        pixels.write(reinterpret_cast<const char*>(actual.data()),actual.size());
        game::IndexedDrawingRequest request;
        request.width=surface.width;request.height=surface.height;
        request.position={128+surface.offset.X,128+surface.offset.Y};request.clip={0,0,width,height};
        request.pixels=surface.pixels.data();request.pixel_count=static_cast<std::uint32_t>(surface.pixels.size());
        // Controlled neutral palette; do not imply validation of world Z/light.
        request.depth_mode=game::ShapeDepthMode::none;
        game::TypeGpuPacket packet;
        if(game::prepare_type_indexed_parameters(request,{width,height,1,0,0x8000,true},packet)!=game::DrawingStatus::drawn)
            throw std::runtime_error("Cannot prepare packet "+name);
        run.packets<<name<<'\n';hex(run.packets,packet.parameters.data(),packet.parameters.size());
        hex(run.packets,surface.pixels.data(),surface.pixels.size());
        run.packets<<expected_color<<'\n'<<expected_z<<'\n';
        if(!run.packets||!run.results||!pixels)throw std::runtime_error("Cannot write case "+name);
    }
}
}
int main(int argc,char** argv)try {
    if(argc!=4)throw std::runtime_error("Usage: ra2_voxel_raster_fixture assets reference.txt output-directory");
    Run run;run.output=std::filesystem::absolute(argv[3]);std::filesystem::create_directories(run.output);
    run.input.open(argv[2]);run.packets.open(run.output/"gpu-packets.txt");run.results.open(run.output/"native-results.tsv");
    if(!run.input||!run.packets||!run.results)throw std::runtime_error("Cannot open input/output");
    game::ResourceHandle* handle=nullptr;std::string error;
    if(!game::create_resources(std::filesystem::absolute(argv[1]).string(),handle,error))throw std::runtime_error(error);
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> owner(handle,game::destroy_resources);
    if(!game::with_resources(*handle,execute,&run,error))throw std::runtime_error(error);
    std::cout<<run.count-run.failed<<" passed; "<<run.failed<<" failed; "<<run.count<<" original VXL raster cases\n";
    return run.failed?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}
