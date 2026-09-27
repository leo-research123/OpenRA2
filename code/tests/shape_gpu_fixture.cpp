// Prepare production GPU packets from an original-x86 pixel fixture. Rendering
// and pixel/Z comparison happen in TestShapeGpuReference on a real GPU.
#include "type_drawing_packets.hpp"
#include "yrpp/FileFormats/SHP.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
std::vector<std::uint8_t> unhex(const std::string& text) {
    if(text.size()%2)throw std::runtime_error("Invalid fixture hex");
    std::vector<std::uint8_t> bytes;
    for(std::size_t i=0;i<text.size();i+=2)bytes.push_back(std::uint8_t(std::stoul(text.substr(i,2),nullptr,16)));
    return bytes;
}
template<typename T>void hex(std::ostream& out,const T* data,std::size_t count) {
    constexpr char digits[]="0123456789abcdef";
    const auto*bytes=reinterpret_cast<const unsigned char*>(data);
    for(std::size_t i=0;i<count*sizeof(T);++i)out<<digits[bytes[i]>>4]<<digits[bytes[i]&15];
    out<<'\n';
}
}
int main(int argc,char**argv)try{
    if(argc!=3)throw std::runtime_error("Usage: ra2_shape_gpu_fixture reference.txt packets.txt");
    std::ifstream in(argv[1]);std::ofstream out(argv[2]);std::string token;
    if(!(in>>token)||(token!="SHP_GPU_REFERENCE_V1"&&token!="SHP_GPU_REFERENCE_V2"&&token!="SHP_GPU_REFERENCE_V3")||!out)throw std::runtime_error("Cannot open fixture/output");
    const bool frames=token=="SHP_GPU_REFERENCE_V2";
    const bool tint=token=="SHP_GPU_REFERENCE_V3";
    game::TypeGpuTarget target;in>>target.width>>target.height>>target.shade_count>>target.depth_max;
    target.shape_z_state=true;
    std::vector<std::uint8_t> shapes[2],aux;
    for(auto&shape:shapes){in>>token;shape=unhex(token);}
    in>>token;aux=unhex(token);
    out<<"SHP_GPU_PACKETS_V1\n"<<target.width<<' '<<target.height<<'\n';
    for(int i=0;i<4;++i){in>>token;out<<token<<'\n';}
    int count=0;in>>count;out<<count<<'\n';
    for(int i=0;i<count;++i){
        std::string name,flags,expected_pixels,expected_depth;int compressed,use_aux;
        game::ShapeDrawingRequest request;
        in>>name>>compressed;
        if(frames)in>>request.frame;
        in>>flags>>request.gradient>>request.position.X>>request.position.Y
          >>request.clip.X>>request.clip.Y>>request.clip.Width>>request.clip.Height
          >>request.depth_adjustment>>use_aux>>request.depth_offset.X>>request.depth_offset.Y;
        if(frames)in>>request.intensity;
        if(tint)in>>request.tint;
        in>>expected_pixels>>expected_depth;
        if(!in||compressed<0||compressed>1)throw std::runtime_error("Malformed case "+name);
        request.flags=std::stoul(flags,nullptr,0);
        request.image=reinterpret_cast<SHPStruct*>(shapes[compressed].data());
        if(use_aux)request.depth_image=reinterpret_cast<SHPStruct*>(aux.data());
        game::TypeGpuPacket packet;
        const auto status=game::prepare_type_shape(request,target,packet);
        if(status==game::DrawingStatus::skipped){
            packet={};packet.parameters[0]=target.width;packet.parameters[1]=target.height;
            packet.parameters[2]=packet.parameters[3]=1;packet.texels={0};
        }else if(status!=game::DrawingStatus::drawn)throw std::runtime_error(name+": "+game::drawing_status_name(status));
        out<<name<<'\n';hex(out,packet.parameters.data(),packet.parameters.size());
        hex(out,packet.texels.data(),packet.texels.size());
        out<<expected_pixels<<'\n'<<expected_depth<<'\n';
    }
    if(!out)throw std::runtime_error("Output write failed");
    std::cout<<"Prepared "<<count<<" production packets for original-x86 GPU comparison\n";
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
