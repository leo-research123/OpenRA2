// Production bitmap requests against unchanged original bitmap/blitter output.
#include "type_drawing_packets.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#if defined(RA2_VOXEL_BITMAP_SOFTWARE)
#include "api/software_type_drawing.hpp"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include "yrpp/ConvertClass.h"
#include <cstring>
#endif

namespace {
std::vector<std::uint8_t> unhex(const std::string& text) {
    if(text.size()%2)throw std::runtime_error("Invalid fixture hex");
    std::vector<std::uint8_t> bytes;
    for(std::size_t i=0;i<text.size();i+=2)bytes.push_back(std::uint8_t(std::stoul(text.substr(i,2),nullptr,16)));
    return bytes;
}
template<typename T>void hex(std::ostream& out,const T* data,std::size_t count) {
    constexpr char digits[]="0123456789abcdef";
    const auto* bytes=reinterpret_cast<const unsigned char*>(data);
    for(std::size_t i=0;i<count*sizeof(T);++i)out<<digits[bytes[i]>>4]<<digits[bytes[i]&15];
    out<<'\n';
}
}
int main(int argc,char**argv)try {
    if(argc!=3)throw std::runtime_error("Usage: ra2_voxel_bitmap_fixture reference.txt packets.txt");
    std::ifstream in(argv[1]);std::ofstream out(argv[2]);std::string token;
    if(!(in>>token)||(token!="VXL_BITMAP_REFERENCE_V1"&&token!="VXL_BITMAP_REFERENCE_V2")||!out)throw std::runtime_error("Cannot open fixture/output");
    const bool tint_fields=token=="VXL_BITMAP_REFERENCE_V2";
    game::TypeGpuTarget target;in>>target.width>>target.height>>target.shade_count>>target.depth_max;
    target.shape_z_state=true;
    out<<"SHP_GPU_PACKETS_V1\n"<<target.width<<' '<<target.height<<'\n';
    std::vector<std::uint8_t> initial[4];
    for(int i=0;i<4;++i){in>>token;initial[i]=unhex(token);out<<token<<'\n';}
#if defined(RA2_VOXEL_BITMAP_SOFTWARE)
    Drawing::SetColorMode(static_cast<RGBMode>(2));
    static const int gradients[5][6]={{1,1,1,1,-1,1},{1,1,1,1,-1,1},{2,3,2,3,-1,1},
        {1,3,1,3,1,0},{1,1,-1,-1,-1,1}};
    Drawing::ZGradientTable=gradients;
    BSurface surface(target.width,target.height,2);BytePalette colors{};
    ConvertClass convert(colors,colors,2,target.shade_count,false);
    std::memcpy(convert.FullColorData,initial[3].data()+512,target.shade_count*512);
    ABuffer alpha({0,0,target.width,target.height});ZBuffer depth({0,0,target.width,target.height});
    ABuffer::Instance=&alpha;ZBuffer::Instance=&depth;depth.MaxValue=target.depth_max;
    auto context=game::make_software_type_drawing(&surface,&convert);
    unsigned failures=0;
#endif
    int count=0;in>>count;out<<count<<'\n';
    for(int i=0;i<count;++i){
        std::string name,expected_pixels,expected_depth;
        game::IndexedDrawingRequest request;
        in>>name>>request.width>>request.height>>request.position.X>>request.position.Y
          >>request.clip.X>>request.clip.Y>>request.clip.Width>>request.clip.Height
          >>request.absolute_depth>>request.intensity;
        if(tint_fields)in>>request.flags>>request.tint;
        in>>token>>expected_pixels>>expected_depth;
        if(!in)throw std::runtime_error("Malformed case "+name);
        auto bytes=unhex(token);std::vector<std::uint32_t> pixels;
        // Deliberately varying internal VXL Z must not affect the scene blit.
        for(std::size_t p=0;p<bytes.size();++p)pixels.push_back(0x1000000u|((unsigned(p*179)&65535)<<8)|bytes[p]);
        request.pixels=pixels.data();request.pixel_count=pixels.size();request.depth_mode=game::ShapeDepthMode::legacy;
#if defined(RA2_VOXEL_BITMAP_SOFTWARE)
        auto* destination=surface.Lock(0,0);std::memcpy(destination,initial[0].data(),initial[0].size());surface.Unlock();
        std::memcpy(depth.BufferHead,initial[1].data(),initial[1].size());
        std::memcpy(alpha.BufferHead,initial[2].data(),initial[2].size());
        request.target=context.target;request.palette=context.palette;
        const auto software=game::submit_type_indexed(context,request);
        auto expected=unhex(expected_pixels),z=unhex(expected_depth);
        destination=surface.Lock(0,0);
        const bool mismatch=std::memcmp(destination,expected.data(),expected.size())||std::memcmp(depth.BufferHead,z.data(),z.size());
        surface.Unlock();
        if(software!=game::DrawingStatus::drawn||mismatch){++failures;std::cout<<"FAIL software "<<name<<'\n';}
#endif
        game::TypeGpuPacket packet;
        const auto status=game::prepare_type_indexed_parameters(request,target,packet);
        if(status!=game::DrawingStatus::drawn)throw std::runtime_error(name+": "+game::drawing_status_name(status));
        out<<name<<'\n';hex(out,packet.parameters.data(),packet.parameters.size());hex(out,pixels.data(),pixels.size());
        out<<expected_pixels<<'\n'<<expected_depth<<'\n';
    }
    if(!out)throw std::runtime_error("Output write failed");
    std::cout<<"Prepared "<<count<<" original VXL bitmap comparison packets\n";
#if defined(RA2_VOXEL_BITMAP_SOFTWARE)
    ABuffer::Instance=nullptr;ZBuffer::Instance=nullptr;alpha.ReleaseSurface();depth.ReleaseSurface();
    std::cout<<count-failures<<" software cases passed; "<<failures<<" failed\n";
    return failures?1:0;
#else
    return 0;
#endif
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
