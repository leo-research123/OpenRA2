// Resource decoding follows the existing SHP decoder and XCC TMP traversal in
// IsometricTileTypeClassDraw.cpp. YR lighting/depth expressions: 547CF0,
// AlphaLightingRemapClass 420140. No Surface, ConvertClass or rasterizer linked.
#include "type_drawing_packets.hpp"
#include "api/images.hpp"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/IsometricTileTypeClass.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <string>
namespace game {
DrawingStatus prepare_type_particle_parameters(const RasterDrawingRequest& r,const TypeGpuTarget& target,TypeGpuPacket& packet) noexcept {
 if(r.blend_mode!=RasterBlendMode::particle||r.width!=1||r.height!=1||r.pixels||r.line_rgb>0xFFFFFFu)return DrawingStatus::invalid_argument;
 const int x=r.position.X,y=r.position.Y;
 if(x<0||y<0||x>=target.width||y>=target.height||x<r.clip.X||y<r.clip.Y||std::int64_t(x)>=std::int64_t(r.clip.X)+r.clip.Width||std::int64_t(y)>=std::int64_t(r.clip.Y)+r.clip.Height)return DrawingStatus::skipped;
 packet.parameters={target.width,target.height,1,1,x,y,x,y,1,1};
 packet.parameters[11]=int(r.line_rgb);packet.parameters[19]=8;
 packet.parameters[14]=int(std::uint16_t(unsigned(target.depth_max)+unsigned(target.depth_bounds_y)-unsigned(y)))+r.line_z;
 packet.parameters[16]=target.depth_bounds_y;
 return DrawingStatus::drawn;
}
namespace {
SHPFile* shape_data(SHPStruct* image) {
    if (!image) return nullptr;
    // Keep body and auxiliary data alive together. GetData's shared scratch
    // buffer is overwritten by the next reference; GetPixels on the returned
    // raw SHP cannot promote the original reference to persistent storage.
    if (auto* reference=image->AsReference()) {
        reference->Load();
        return static_cast<SHPFile*>(reference->Data);
    }
    return static_cast<SHPFile*>(image);
}
bool integer(std::int64_t n) { return n >= INT32_MIN && n <= INT32_MAX; }
bool setup(TypeGpuPacket& p, const TypeGpuTarget& t, const RectangleStruct& r,
    std::int64_t x, std::int64_t y, int w, int h, int intensity, bool lit) {
    p = {};
    if (t.width < 1 || t.height < 1 || t.width > 8192 || t.height > 8192 ||
        t.shade_count < 1 || t.shade_count > 256 || w < 0 || h < 0 || w > 4096 || h > 4096 ||
        !integer(x) || !integer(y) || !integer(x+w) || !integer(y+h)) return false;
    const auto right = std::clamp<std::int64_t>(std::int64_t(r.X)+r.Width, 0, t.width);
    const auto bottom = std::clamp<std::int64_t>(std::int64_t(r.Y)+r.Height, 0, t.height);
    const int left = std::clamp(r.X, 0, t.width), top = std::clamp(r.Y, 0, t.height);
    auto& a = p.parameters;
    a[0]=t.width; a[1]=t.height; a[2]=w; a[3]=h; a[4]=int(x); a[5]=int(y);
    a[6]=left; a[7]=top; a[8]=int(std::max<std::int64_t>(0,right-left));
    a[9]=int(std::max<std::int64_t>(0,bottom-top));
    a[10]=lit ? 1 : 0;
    a[11]=int(std::min<std::int64_t>(254,261ll*std::max(intensity,0)/2048));
    a[12]=t.shade_count;
    return true;
}
bool visible(const TypeGpuPacket& p) {
    const auto& a=p.parameters;
    return a[8]>0 && a[9]>0 && std::int64_t(a[4])+a[2]>a[6] &&
        std::int64_t(a[5])+a[3]>a[7] && a[4]<a[6]+a[8] && a[5]<a[7]+a[9];
}
DrawingStatus auxiliary_bounds(const ShapeDrawingRequest& r, SHPFile& shape,
        RectangleStruct& body, Point2D& offset) {
    if (!r.depth_image) return DrawingStatus::drawn;
    auto* z=shape_data(r.depth_image);
    if (!z) return DrawingStatus::unavailable;
    if (r.depth_mode!=ShapeDepthMode::legacy ||
        r.depth_frame<0 || r.depth_frame>=z->Frames || z->HasCompression(r.depth_frame))
        return DrawingStatus::unsupported;
    // Unlit RLE palette read/write has separate check/write delta streams.
    // Current world auxiliary requests select the calibrated alpha variants.
    if(shape.HasCompression(r.frame)&&(r.flags&0x4000)&&!(r.flags&0x807))return DrawingStatus::unsupported;
    const auto b=z->GetFrameBounds(r.depth_frame);
    const auto x=std::int64_t(r.depth_offset.X)-shape.Width/2+body.X+b.X;
    const auto y=std::int64_t(r.depth_offset.Y)-shape.Height/2+body.Y+b.Y;
    // The original row dispatcher requires its auxiliary lock to succeed.
    if (x<0 || y<0) return DrawingStatus::unsupported;
    if (x>=b.Width || y>=b.Height) return DrawingStatus::skipped;
    offset={int(x),int(y)};
    body.Width=std::min(body.Width,b.Width-offset.X);
    body.Height=std::min(body.Height,b.Height-offset.Y);
    return body.Width>0 && body.Height>0 ? DrawingStatus::drawn : DrawingStatus::skipped;
}
bool original_shape_mode(const ShapeDrawingRequest& r, const TypeGpuTarget& t,
        bool rle, TypeGpuPacket& packet) {
    // YR ConvertClass::Select*Blitter, with standard ZFlags=0x3000.
    // Remap and warp variants still require their own inputs.
    // Warp (0x8) reads an offset destination pixel (e.g. 0x00495590), unlike
    // ordinary translucency. Keep it unsupported until GPU destination reads
    // reproduce the original row phase/order; accepting the bit alone is wrong.
    if (r.flags & ~0x7F07u) return false;
    if ((r.flags & 0x100) && !(r.flags & 0x7000)) return false;
    const unsigned mode=(r.flags & 0x4000) ? 1 : (r.flags & 0x3000) ? 2 : 0;
    const unsigned blend=(r.flags & 6) ? ((r.flags & 6)==6 ? 4 : (r.flags & 6)==4 ? 3 : 2)
        : (r.flags & 1) ? 1 : 0;
    auto& a=packet.parameters;
    const bool auxiliary=rle&&r.depth_image;
    a[13]=int((mode?0x10000u:0u)|(auxiliary?0x20000u:0u)|mode|(blend<<8));
    // Only alpha palette Z-read, and RLE alpha palette Z-read/write, tint.
    // Translucency, shadows, unlit and plain Z-write ignore Draw_Shape's tint.
    if ((r.flags&0x800) && !blend && (mode==2 || (rle && mode==1)))
        a[10]=std::bit_cast<int>(unsigned(a[10]) | (unsigned(r.tint)<<16));
    if (!mode) return true;
    if (!t.shape_z_state || r.gradient<0 || r.gradient>3) return false;
    // Exact descriptors at 0x008176F8, also bound by the software reference.
    constexpr int gradients[4][6]={{1,1,1,1,-1,1},{2,3,2,3,-1,1},
        {1,3,1,3,1,0},{1,1,-1,-1,-1,1}};
    const auto& g=gradients[r.gradient];
    const int top=std::max(a[5],a[7]);
    const int bottom=std::min(a[5]+a[3],a[7]+a[9]);
    const int ratio=g[3]/g[2];
    std::int64_t base=0;int phase=0;
    if (g[5]) {
        base=std::int64_t(r.depth_adjustment)+std::uint16_t(std::int64_t(t.depth_bounds_y)+t.depth_max-top);
        base=g[1]*(base/g[1]);
    } else {
        const int last=rle ? a[5]+a[3] : bottom;
        const int rows=last-top;
        base=std::int64_t(r.depth_adjustment)+std::uint16_t(std::int64_t(t.depth_bounds_y)+t.depth_max-last+1);
        if (!auxiliary) {
            base=ratio*(base/ratio)-rows/ratio;
            phase=g[3]-rows%ratio;
            if (phase==g[3]) { phase=0;base+=g[4]; }
        }
    }
    if (!integer(base)) return false;
    a[14]=int(base);
    // Normalize the original negative unit slope to a positive step/limit.
    const unsigned step=unsigned(std::abs(g[2])),limit=unsigned(std::abs(g[3]));
    a[15]=std::int32_t(unsigned(phase)|(step<<8)|(limit<<16)|(unsigned(std::uint8_t(g[4]))<<24));
    a[18]=top;
    return true;
}
}
DrawingStatus prepare_type_shape_parameters(const ShapeDrawingRequest& r, const TypeGpuTarget& t,
        TypeGpuPacket& output) noexcept {
    output={};
    try {
        if (r.depth_mode!=ShapeDepthMode::legacy &&
            r.flags!=0 && r.flags!=0x400 && r.flags!=0x600 && r.flags!=0xE00) return DrawingStatus::unsupported;
        if (!r.image) return DrawingStatus::skipped;
        auto* shape=shape_data(r.image);
        if (!shape) return DrawingStatus::unavailable;
        if (r.frame<0 || r.frame>=shape->Frames) return DrawingStatus::invalid_argument;
        auto b=shape->GetFrameBounds(r.frame);
        Point2D auxiliary_offset;
        const auto aux=auxiliary_bounds(r,*shape,b,auxiliary_offset);
        if (aux!=DrawingStatus::drawn) return aux;
        if (b.Width<=0 || b.Height<=0 || r.clip.Width<=0 || r.clip.Height<=0) return DrawingStatus::skipped;
        // YR's SHP equal-size blit treats the desired position relative to the
        // intersected clipping rectangle. TMP positions are instead absolute.
        const auto x=std::int64_t(std::max(r.clip.X,0))+r.position.X-((r.flags&0x200) ? shape->Width/2 : 0)+b.X;
        const auto y=std::int64_t(std::max(r.clip.Y,0))+r.position.Y-((r.flags&0x200) ? shape->Height/2 : 0)+b.Y;
        if (!setup(output,t,r.clip,x,y,b.Width,b.Height,r.intensity,(r.flags&0x800)!=0))
            return DrawingStatus::invalid_argument;
        if (!visible(output)) { output={}; return DrawingStatus::skipped; }
        const auto depth=static_cast<unsigned>(r.depth_mode);
        const auto blend=static_cast<unsigned>(r.blend_mode);
        if (depth>static_cast<unsigned>(ShapeDepthMode::read_write) ||
            blend>static_cast<unsigned>(ShapeBlendMode::translucent75)) return DrawingStatus::unsupported;
        if (r.depth_mode!=ShapeDepthMode::legacy && r.depth_mode!=ShapeDepthMode::none) {
            if (r.absolute_depth<0 || r.absolute_depth>65535) return DrawingStatus::invalid_argument;
            output.parameters[13]=r.depth_mode==ShapeDepthMode::read_write ? 1 : 2;
            output.parameters[14]=r.absolute_depth;
        }
        output.parameters[13]|=int(blend<<8);
        const bool rle=shape->HasCompression(r.frame);
        if (r.depth_mode==ShapeDepthMode::legacy &&
            !original_shape_mode(r,t,rle,output)) return DrawingStatus::unsupported;
        if (rle && !t.shape_z_state) {
            output.parameters[17]=1;
            output.parameters[18]=std::max(output.parameters[5],output.parameters[7]);
        }
        return DrawingStatus::drawn;
    } catch (...) { output={}; return DrawingStatus::backend_failure; }
}
DrawingStatus decode_type_shape(const ShapeDrawingRequest& r, std::vector<std::uint32_t>& output) noexcept {
    output.clear();
    try {
        auto* shape=shape_data(r.image);
        if (!shape) return DrawingStatus::unavailable;
        if (r.frame<0 || r.frame>=shape->Frames) return DrawingStatus::invalid_argument;
        const auto original=shape->GetFrameBounds(r.frame);
        auto b=original;Point2D offset;
        const auto aux=auxiliary_bounds(r,*shape,b,offset);
        if (aux!=DrawingStatus::drawn) return aux;
        if (b.Width<=0 || b.Height<=0 || b.Width>4096 || b.Height>4096) return DrawingStatus::invalid_argument;
        std::vector<std::uint8_t> bytes; std::string error;
        if (!shape->GetPixels(r.frame) || !decode_shp_pixels(shape->GetPixels(r.frame),original.Width,original.Height,
                shape->HasCompression(r.frame),bytes,error)) return DrawingStatus::invalid_argument;
        auto* z=shape->HasCompression(r.frame)?shape_data(r.depth_image):nullptr;
        const auto* deltas=z?z->GetPixels(r.depth_frame):nullptr;
        if(z&&!deltas)return DrawingStatus::unavailable;
        const int pitch=z?z->GetFrameBounds(r.depth_frame).Width:0;
        output.resize(std::size_t(b.Width)*b.Height);
        for(int y=0;y<b.Height;++y)for(int x=0;x<b.Width;++x){
            const auto value=bytes[std::size_t(y)*original.Width+x];
            const unsigned delta=deltas?deltas[std::size_t(y+offset.Y)*pitch+x+offset.X]:0;
            output[std::size_t(y)*b.Width+x]=(value?0x10000u|value:0)|(delta<<8);
        }
        if(z){
            // Original RLE prefix clipping advances destination/Z/alpha when
            // a transparent run crosses the clip, but holds the SHA pointer.
            // Preserve each run boundary so the GPU can reproduce that shift
            // without camera-dependent decoding or merging adjacent runs.
            const auto*row=shape->GetPixels(r.frame);
            for(int y=0;y<b.Height;++y){
                const auto*cursor=row+2;int x=0;
                while(x<original.Width){
                    if(*cursor++){++x;continue;}
                    const int run=*cursor++;
                    for(int j=1;j<run&&x+j<b.Width;++j)
                        output[std::size_t(y)*b.Width+x+j]|=unsigned(run-j)<<17;
                    x+=run;
                }
                row+=unsigned(row[0])|(unsigned(row[1])<<8);
            }
        }
        return DrawingStatus::drawn;
    } catch (...) { output.clear(); return DrawingStatus::backend_failure; }
}
DrawingStatus prepare_type_shape(const ShapeDrawingRequest& r,const TypeGpuTarget& t,TypeGpuPacket& p) noexcept {
    auto result=prepare_type_shape_parameters(r,t,p);
    if (result!=DrawingStatus::drawn) return result;
    result=decode_type_shape(r,p.texels);
    if (result!=DrawingStatus::drawn) p={};
    return result;
}
bool type_gpu_packet_visible(const TypeGpuPacket& packet) noexcept { return visible(packet); }
DrawingStatus prepare_type_tile_parameters(const TileDrawingRequest& r, const TypeGpuTarget& t,
        TypeGpuPacket& base, TypeGpuPacket& extra) noexcept {
    base={}; extra={};
    if (r.flat || r.flag16 || r.flag17) return DrawingStatus::unsupported;
    if (!r.resource || !r.image) return DrawingStatus::invalid_argument;
    if (r.resource->Width!=60 || r.resource->Height!=30) return DrawingStatus::unsupported;
    const auto* im=r.image;
    if (!setup(base,t,r.clip,r.position.X,r.position.Y,60,29,r.intensity,true)) return DrawingStatus::invalid_argument;
    const bool depth=r.use_depth && (im->Flags&2);
    const auto z=depth ? std::int64_t(std::uint16_t(std::int64_t(t.depth_bounds_y)+t.depth_max-r.position.Y-30))-15ll*r.level : 0;
    if (!integer(z) || !integer(z+255)) return DrawingStatus::invalid_argument;
    base.parameters[13]=depth; base.parameters[14]=int(z); base.parameters[16]=r.buffer_offset_y % t.height;
    if (im->Flags&1) {
        if (!setup(extra,t,r.clip,std::int64_t(r.position.X)+im->ExtraX-im->X,
                std::int64_t(r.position.Y)+im->ExtraY-im->Y,im->ExtraWidth,im->ExtraHeight,r.intensity,true))
            return DrawingStatus::invalid_argument;
        extra.parameters[13]=depth; extra.parameters[14]=int(z); extra.parameters[15]=1;
        extra.parameters[16]=r.buffer_offset_y % t.height;
    }
    return visible(base) || visible(extra) ? DrawingStatus::drawn : DrawingStatus::skipped;
}
DrawingStatus decode_type_tile(const TileDrawingRequest& r,std::vector<std::uint32_t>& base,
        std::vector<std::uint32_t>& extra) noexcept {
    base.clear(); extra.clear();
    if (!r.resource || !r.image) return DrawingStatus::invalid_argument;
    if (r.resource->Width!=60 || r.resource->Height!=30) return DrawingStatus::unsupported;
    try {
        const auto* im=r.image;
        const bool depth=r.use_depth && (im->Flags&2);
        base.assign(60*29,0);
        int source=0;
        for (int y=0;y<29;++y) {
            const int width=4*(y<15 ? y+1 : 29-y), left=(60-width)/2;
            for (int x=0;x<width;++x,++source) {
                const unsigned dz=depth ? im->At(im->ZOffset)[source] : 0;
                base[std::size_t(y)*60+left+x]=0x10000u|im->Pixels()[source]|(dz<<8);
            }
        }
        if (im->Flags&1) {
            if (im->ExtraWidth<1 || im->ExtraHeight<1 || im->ExtraWidth>4096 || im->ExtraHeight>4096)
                return DrawingStatus::invalid_argument;
            extra.resize(std::size_t(im->ExtraWidth)*im->ExtraHeight);
            for (std::size_t i=0;i<extra.size();++i) {
                const auto index=im->At(im->ExtraOffset)[i];
                const unsigned dz=depth ? im->At(im->ExtraZOffset)[i] : 0;
                extra[i]=index ? (0x10000u|index|(dz<<8)) : 0;
            }
        }
        return DrawingStatus::drawn;
    } catch (...) { base.clear(); extra.clear(); return DrawingStatus::backend_failure; }
}
DrawingStatus prepare_type_tile(const TileDrawingRequest& r, const TypeGpuTarget& t,
        TypeGpuPacket& base, TypeGpuPacket& extra) noexcept {
    auto status=prepare_type_tile_parameters(r,t,base,extra);
    if (status!=DrawingStatus::drawn) return status;
    status=decode_type_tile(r,base.texels,extra.texels);
    if (status!=DrawingStatus::drawn) { base={}; extra={}; return status; }
    if (!visible(base)) base.texels.clear();
    if (!visible(extra)) extra.texels.clear();
    return DrawingStatus::drawn;
}
}

namespace game {
DrawingStatus prepare_type_indexed_parameters(const IndexedDrawingRequest&r,const TypeGpuTarget&t,TypeGpuPacket&out) noexcept {
 out.parameters={};
 if(t.width<=0||t.height<=0||t.shade_count<1||t.shade_count>256||r.width<0||r.height<0||r.width>2048||r.height>2048||static_cast<unsigned>(r.depth_mode)>static_cast<unsigned>(ShapeDepthMode::read_write))return DrawingStatus::invalid_argument;
 if(!r.width||!r.height||r.clip.Width<=0||r.clip.Height<=0)return DrawingStatus::skipped;
 if(!r.pixels||std::uint64_t(r.width)*r.height>r.pixel_count)return DrawingStatus::invalid_argument;
 out.parameters={t.width,t.height,r.width,r.height,r.position.X,r.position.Y,r.clip.X,r.clip.Y,r.clip.Width,r.clip.Height,1,
 int(std::clamp(std::int64_t(261)*r.intensity/2048,std::int64_t(0),std::int64_t(254))),t.shade_count,
 (r.depth_mode==ShapeDepthMode::none?0:r.depth_mode==ShapeDepthMode::read?2:1)|(r.shadow?256:0),r.absolute_depth,1,0,0,0,3};
 if(r.depth_mode==ShapeDepthMode::legacy){
  // 0x00706640 -> 0x004AF2A0 -> 0x004373B0 uses the same plain
  // bitmap row dispatcher as an uncompressed SHP. Internal VXL Z stays local.
  ShapeDrawingRequest bitmap;bitmap.flags=r.flags|(r.shadow?1u:0u);bitmap.tint=int(r.tint);
  bitmap.gradient=2;bitmap.depth_adjustment=r.absolute_depth;
  if(!original_shape_mode(bitmap,t,false,out))return DrawingStatus::unsupported;
 }
 return type_gpu_packet_visible(out)?DrawingStatus::drawn:DrawingStatus::skipped;
}
}
