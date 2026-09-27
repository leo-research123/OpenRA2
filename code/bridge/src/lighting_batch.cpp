#include "lighting_batch.hpp"
#include "yrpp/FileFormats/SHP.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace game {
DrawingStatus decode_lighting_image(SHPStruct* image,int frame,LightingImageData& output) noexcept {
    output={};
    try {
        if(!image)return DrawingStatus::unavailable;
        if(auto* ref=image->AsReference()){ref->Load();image=ref->Data;}
        if(!image)return DrawingStatus::unavailable;
        if(frame<0||frame>=image->Frames)return DrawingStatus::invalid_argument;
        const auto rect=image->GetFrameBounds(frame);
        if(rect.Width<=0||rect.Height<=0)return DrawingStatus::skipped;
        if(rect.Width>8192||rect.Height>8192)return DrawingStatus::invalid_argument;
        const auto* pixels=image->GetPixels(frame);if(!pixels)return DrawingStatus::unavailable;
        output.bounds=rect;output.texels.resize(std::size_t(rect.Width)*rect.Height);
        // Original lighting frames are byte samples; zero is covered/dark,
        // unlike the transparency convention of ordinary palette SHP drawing.
        for(std::size_t i=0;i<output.texels.size();++i)output.texels[i]=0x10000u|pixels[i];
        return DrawingStatus::drawn;
    }catch(...){output={};return DrawingStatus::backend_failure;}
}
DrawingStatus prepare_lighting_parameters(const LightingShapeDrawingRequest& r,const RectangleStruct& image,
    int width,int height,LightingParameters& p) noexcept {
    p={};
    if(width<=0||height<=0||width>8192||height>8192)return DrawingStatus::invalid_argument;
    const auto x=std::int64_t(r.position.X)+image.X,y=std::int64_t(r.position.Y)+image.Y;
    if(x<INT32_MIN||x>INT32_MAX||y<INT32_MIN||y>INT32_MAX)return DrawingStatus::invalid_argument;
    const auto left=std::max({std::int64_t(0),x,std::int64_t(r.clip.X)});
    const auto top=std::max({std::int64_t(0),y,std::int64_t(r.clip.Y)});
    const auto right=std::min({std::int64_t(width),x+image.Width,std::int64_t(r.clip.X)+r.clip.Width});
    const auto bottom=std::min({std::int64_t(height),y+image.Height,std::int64_t(r.clip.Y)+r.clip.Height});
    if(right<=left||bottom<=top)return DrawingStatus::skipped;
    int operation=0;
    if(r.operation==RasterBlendMode::shroud)operation=1;
    else if(r.operation==RasterBlendMode::fog)operation=2;
    else if(r.operation==RasterBlendMode::alpha_shape)operation=3;
    else return DrawingStatus::invalid_argument;
    p={width,height,image.Width,image.Height,int(x),int(y),int(left),int(top),int(right-left),int(bottom-top)};
    p[19]=operation;
    return DrawingStatus::drawn;
}
int append_lighting_bins(const LightingParameters* packets,int first,int count,int width,int height,
    std::vector<std::int32_t>& bins) {
    if(!packets||first<0||count<0||count>INT32_MAX-first||width<=0||height<=0||width>8192||height>8192)
        throw std::invalid_argument("lighting batch dimensions");
    const int columns=(width+LightingBinSize-1)/LightingBinSize;
    const int rows=(height+LightingBinSize-1)/LightingBinSize,tiles=columns*rows;
    const auto start=bins.size();
    if(start+std::size_t(tiles)*2>INT32_MAX)throw std::length_error("lighting bins");
    bins.resize(start+tiles*2,0);
    const auto visit=[&](int index,auto&& action){
        const auto& p=packets[index];
        if(p[0]!=width||p[1]!=height||p[16]!=0)throw std::invalid_argument("lighting batch target");
        const auto l=std::max({std::int64_t(0),std::int64_t(p[4]),std::int64_t(p[6])});
        const auto t=std::max({std::int64_t(0),std::int64_t(p[5]),std::int64_t(p[7])});
        const auto r=std::min({std::int64_t(width),std::int64_t(p[4])+p[2],std::int64_t(p[6])+p[8]});
        const auto b=std::min({std::int64_t(height),std::int64_t(p[5])+p[3],std::int64_t(p[7])+p[9]});
        if(l>=r||t>=b)return;
        for(int y=int(t)/LightingBinSize;y<=int(b-1)/LightingBinSize;++y)
            for(int x=int(l)/LightingBinSize;x<=int(r-1)/LightingBinSize;++x)action(y*columns+x);
    };
    for(int n=0;n<count;++n)visit(first+n,[&](int tile){++bins[start+tile*2+1];});
    std::size_t next=bins.size();
    std::vector<int> cursors(tiles);
    for(int i=0;i<tiles;++i){
        if(next+std::size_t(bins[start+i*2+1])>INT32_MAX)throw std::length_error("lighting packet references");
        cursors[i]=bins[start+i*2]=int(next);next+=bins[start+i*2+1];
    }
    bins.resize(next);
    // Ascending source indices preserve every overlapping write, including
    // successive integer AlphaShape multiplications. No parallel pixel races.
    for(int n=0;n<count;++n)visit(first+n,[&](int tile){bins[cursors[tile]++]=first+n;});
    return int(start);
}
}
