// Exercise production lighting geometry and bin construction; the GPU test
// checks results against the existing original-x86 ABUFREF1 lookup fixture.
#include "lighting_batch.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
template<class T> void write(std::ostream& out,const std::vector<T>& words) {
    out.write(reinterpret_cast<const char*>(words.data()),words.size()*sizeof(T));
}
void make_case(std::ostream& out,int width,int height,int operation,bool overlap) {
    std::vector<game::LightingParameters> packets;
    std::vector<std::uint32_t> atlas,initial(width*height);
    for(int i=0;i<width*height;++i)initial[i]=overlap?(i*53)%256:i/256;
    const int count=overlap?73:1;
    for(int n=0;n<count;++n){
        game::LightingShapeDrawingRequest request;
        const int op=overlap?1+n%3:operation;
        request.operation=op==1?game::RasterBlendMode::shroud:op==2?game::RasterBlendMode::fog:game::RasterBlendMode::alpha_shape;
        request.position=overlap?Point2D{(n*37)%(width+30)-30,(n*23)%(height+20)-20}:Point2D{};
        request.clip=overlap?RectangleStruct{n%9-4,n%7-3,width-n%13,height-n%11}:RectangleStruct{0,0,width,height};
        const RectangleStruct bounds=overlap?RectangleStruct{-3,2,61,47}:RectangleStruct{0,0,width,height};
        game::LightingParameters p;
        const auto status=game::prepare_lighting_parameters(request,bounds,width,height,p);
        if(status==game::DrawingStatus::skipped)continue;
        if(status!=game::DrawingStatus::drawn)throw std::runtime_error("lighting geometry failed");
        p[17]=int(atlas.size());
        for(int i=0;i<bounds.Width*bounds.Height;++i)
            atlas.push_back(std::uint32_t((i+n*29)%256)|(overlap&&i%17==0?0u:0x10000u));
        packets.push_back(p);
    }
    std::vector<std::int32_t> bins,offsets;
    // Multiple contiguous runs also exercise nonzero bin-header offsets.
    const int split=overlap?int(packets.size())/2:int(packets.size());
    offsets.push_back(game::append_lighting_bins(packets.data(),0,split,width,height,bins));
    if(split<int(packets.size()))offsets.push_back(game::append_lighting_bins(packets.data(),split,int(packets.size())-split,width,height,bins));
    // Independent destination scan verifies the bins cover exactly the same
    // contributing requests, in order, at every pixel (including bin edges).
    for(int y=0;y<height;++y)for(int x=0;x<width;++x){
        std::vector<int> wanted,actual;
        const auto covers=[&](int index){const auto& p=packets[index];return x>=p[4]&&y>=p[5]&&x<p[4]+p[2]&&y<p[5]+p[3]
            &&x>=p[6]&&y>=p[7]&&x<p[6]+p[8]&&y<p[7]+p[9];};
        for(int n=0;n<int(packets.size());++n)if(covers(n))wanted.push_back(n);
        for(int offset:offsets){
            const int tile=(y/game::LightingBinSize)*((width+game::LightingBinSize-1)/game::LightingBinSize)+x/game::LightingBinSize;
            const int start=bins[offset+tile*2],size=bins[offset+tile*2+1];
            for(int n=0;n<size;++n)if(covers(bins[start+n]))actual.push_back(bins[start+n]);
        }
        if(actual!=wanted)throw std::runtime_error("lighting bin coverage/order differs");
    }
    write(out,std::vector<std::int32_t>{width,height,int(packets.size()),int(atlas.size()),int(bins.size()),int(offsets.size())});
    write(out,initial);write(out,packets);write(out,atlas);write(out,bins);write(out,offsets);
}
}
int main(int argc,char** argv)try{
    if(argc!=2)throw std::runtime_error("Usage: ra2_lighting_gpu_fixture packets.bin");
    std::ofstream out(argv[1],std::ios::binary);
    out.write("LIGHTBT1",8);write(out,std::vector<std::int32_t>{4});
    for(int operation=1;operation<=3;++operation)make_case(out,256,256,operation,false);
    make_case(out,133,97,0,true);
    if(!out)throw std::runtime_error("fixture write failed");
    std::cout<<"Lighting geometry/bin coverage and order passed; 4 GPU cases prepared\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
