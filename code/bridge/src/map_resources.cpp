#include "map_resources.hpp"
#include "lighting_batch.hpp"
#include "yrpp/ConvertClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/FileFormats/SHP.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
namespace game {
namespace {
std::uint32_t rgba565(WORD word) {
    const unsigned r=(word>>11)&31,g=(word>>5)&63,b=word&31;
    // Exact normalized RGB565 -> RGB8, rounded to nearest. Bit replication
    // differs by one for some channels (for example red 3 becomes 24, not 25).
    return ((r*255u+15u)/31u)|(((g*255u+31u)/63u)<<8)|(((b*255u+15u)/31u)<<16)|0xff000000u;
}
}
std::shared_ptr<MapBufferData> MapResources::buffer(const std::vector<std::uint32_t>& words) {
    auto data=std::make_shared<MapBufferData>(); data->id=next_id_++;
    data->bytes.resize(static_cast<std::int64_t>(words.size())*4);
    if (!words.empty()) std::memcpy(data->bytes.ptrw(),words.data(),words.size()*4);
    return data;
}
void MapResources::clear() { frame_=nullptr; tiles_.clear(); palettes_.clear(); shapes_.clear(); fills_.clear();lighting_images_.clear(); }
MapDrawingContext MapResources::begin_frame(MapFrameData& frame) {
    frame_=&frame;
    error_[0]=0;
    MapDrawingContext context;
    context.types.backend_context=this;
    context.types.target=reinterpret_cast<DrawingTargetHandle*>(this);
    context.types.backend.tile=tile;
    context.types.backend.shape=shape;
    context.types.backend.raster=raster;
    context.types.backend.indexed=indexed;
    context.types.backend.lighting_shape=lighting_shape;
    context.terrain_palette=palette;
    context.shape_palette=shape_palette;
    context.color_scheme_palette=color_scheme_palette;
    context.plain_palette=[](void* p,const BytePalette& source,const DrawingPaletteHandle*& out) noexcept {
        return shape_palette(p,source,1,out);
    };
    return context;
}
DrawingStatus MapResources::palette(void* pointer,const BytePalette& source,int r,int g,int b,int shades,
    const DrawingPaletteHandle*& output) noexcept {
    return prepare_palette(pointer,source,r,g,b,shades,PaletteKind::terrain,output);
}
DrawingStatus MapResources::shape_palette(void* pointer,const BytePalette& source,int shades,
    const DrawingPaletteHandle*& output) noexcept {
    return prepare_palette(pointer,source,1000,1000,1000,shades,PaletteKind::shape,output);
}
DrawingStatus MapResources::color_scheme_palette(void* pointer,const BytePalette& source,int shades,
    const DrawingPaletteHandle*& output) noexcept {
    return prepare_palette(pointer,source,1000,1000,1000,shades,PaletteKind::color_scheme,output);
}
DrawingStatus MapResources::prepare_palette(void* pointer,const BytePalette& source,int r,int g,int b,int shades,
    PaletteKind kind,const DrawingPaletteHandle*& output) noexcept {
    auto& self=*static_cast<MapResources*>(pointer);
    try {
        const auto key=std::make_pair(&source,std::array<int,5>{r,g,b,shades,int(kind)});
        auto found=self.palettes_.find(key);
        if (found==self.palettes_.end()) {
            std::vector<WORD> words(std::size_t(shades+1)*256);
            if (!ConvertClass::BuildColorTable(source,words.data(),256,1,2) ||
                !(kind==PaletteKind::shape ? ConvertClass::BuildColorTable(source,words.data()+256,words.size()-256,shades,2)
                  : kind==PaletteKind::color_scheme ? ColorScheme::BuildColorTable(source,words.data()+256,words.size()-256,shades,2,false)
                  : LightConvertClass::BuildColorTable(source,words.data()+256,words.size()-256,shades,r,g,b,nullptr,0,2,false)))
                return DrawingStatus::backend_failure;
            std::vector<std::uint32_t> rgba(words.size());
            for (std::size_t i=0;i<words.size();++i) rgba[i]=rgba565(words[i]);
            found=self.palettes_.emplace(key,MapPaletteData{self.buffer(rgba),shades}).first;
        }
        output=reinterpret_cast<const DrawingPaletteHandle*>(&found->second);
        return DrawingStatus::drawn;
    } catch (...) { return DrawingStatus::backend_failure; }
}
DrawingStatus MapResources::tile(void* pointer,const TileDrawingRequest& request) {
    auto& self=*static_cast<MapResources*>(pointer);
    if (!self.frame_ || request.target!=reinterpret_cast<DrawingTargetHandle*>(&self) || !request.palette)
        return DrawingStatus::invalid_argument;
    const auto& palette=*reinterpret_cast<const MapPaletteData*>(request.palette);
    TypeGpuTarget target{self.frame_->width,self.frame_->height,palette.shades,0,0x8000,true};
    TypeGpuPacket base,extra;
    auto result=prepare_type_tile_parameters(request,target,base,extra);
    if (result!=DrawingStatus::drawn) return result;
    const auto key=std::make_pair(request.image,request.use_depth);
    auto found=self.tiles_.find(key);
    if (found==self.tiles_.end()) {
        std::vector<std::uint32_t> base_pixels,extra_pixels;
        result=decode_type_tile(request,base_pixels,extra_pixels);
        if (result!=DrawingStatus::drawn) return result;
        MapTileData data{self.buffer(base_pixels),{}};
        if (!extra_pixels.empty()) data.extra=self.buffer(extra_pixels);
        found=self.tiles_.emplace(key,std::move(data)).first;
    }
    if (type_gpu_packet_visible(base)) self.frame_->packets.push_back({base.parameters,found->second.base,palette.buffer});
    if (type_gpu_packet_visible(extra) && found->second.extra) self.frame_->packets.push_back({extra.parameters,found->second.extra,palette.buffer});
    return DrawingStatus::drawn;
}
DrawingStatus MapResources::shape(void* pointer,const ShapeDrawingRequest& request) {
    auto& self=*static_cast<MapResources*>(pointer);
    const auto checked=[&](DrawingStatus status,const char* stage) {
        if(status!=DrawingStatus::drawn&&status!=DrawingStatus::skipped&&!self.error_[0]) {
            const auto* image=request.image?request.image->AsReference():nullptr;
            const auto* depth=request.depth_image?request.depth_image->AsReference():nullptr;
            std::snprintf(self.error_,sizeof(self.error_),
                "SHP %s: %s; image=%s frame=%d flags=0x%X position=(%d,%d) clip=(%d,%d,%d,%d) depth_mode=%u gradient=%d depth_image=%s depth_frame=%d depth_offset=(%d,%d)",
                stage,drawing_status_name(status),image?image->Filename:"<raw>",request.frame,request.flags,
                request.position.X,request.position.Y,request.clip.X,request.clip.Y,request.clip.Width,request.clip.Height,
                unsigned(request.depth_mode),request.gradient,depth?depth->Filename:"<none/raw>",request.depth_frame,
                request.depth_offset.X,request.depth_offset.Y);
        }
        return status;
    };
    if (!self.frame_ || request.target!=reinterpret_cast<DrawingTargetHandle*>(&self) || !request.palette)
        return checked(DrawingStatus::invalid_argument,"target/palette");
    const auto& palette=*reinterpret_cast<const MapPaletteData*>(request.palette);
    TypeGpuPacket packet;
    const auto status=prepare_type_shape_parameters(request,{self.frame_->width,self.frame_->height,palette.shades,0,0x8000,true},packet);
    if (status!=DrawingStatus::drawn) return checked(status,"parameters");
    auto& source=self.shapes_[{request.image,request.frame,request.depth_image,
        request.depth_frame,request.depth_offset.X,request.depth_offset.Y}];
    if (!source) {
        const auto decoded=decode_type_shape(request,packet.texels);
        if (decoded!=DrawingStatus::drawn) return checked(decoded,"decode");
        source=self.buffer(packet.texels);
    }
    self.frame_->packets.push_back({packet.parameters,source,palette.buffer});
    return DrawingStatus::drawn;
}
DrawingStatus MapResources::lighting_shape(void* pointer,const LightingShapeDrawingRequest& request) {
    auto& self=*static_cast<MapResources*>(pointer);
    if(!self.frame_||request.target!=reinterpret_cast<DrawingTargetHandle*>(&self))return DrawingStatus::invalid_argument;
    const auto key=std::make_pair(static_cast<const SHPStruct*>(request.image),request.frame);
    auto found=self.lighting_images_.find(key);
    if(found==self.lighting_images_.end()){
        LightingImageData decoded;
        const auto status=decode_lighting_image(request.image,request.frame,decoded);
        if(status!=DrawingStatus::drawn)return status;
        found=self.lighting_images_.emplace(key,MapLightingImage{decoded.bounds,self.buffer(decoded.texels)}).first;
    }
    MapDrawPacket packet;
    const auto status=prepare_lighting_parameters(request,found->second.bounds,self.frame_->width,self.frame_->height,packet.parameters);
    if(status!=DrawingStatus::drawn)return status;
    packet.lighting=packet.cached_lighting=true;
    packet.source=packet.palette=found->second.source;
    self.frame_->packets.push_back(std::move(packet));
    return DrawingStatus::drawn;
}
DrawingStatus MapResources::raster(void* pointer,const RasterDrawingRequest& request) {
    auto& self=*static_cast<MapResources*>(pointer);
    if (!self.frame_ || request.target!=reinterpret_cast<DrawingTargetHandle*>(&self)) return DrawingStatus::invalid_argument;
    const int w=self.frame_->width,h=self.frame_->height;
    const auto left=std::max({std::int64_t(0),std::int64_t(request.position.X),std::int64_t(request.clip.X)});
    const auto top=std::max({std::int64_t(0),std::int64_t(request.position.Y),std::int64_t(request.clip.Y)});
    const auto right=std::min({std::int64_t(w),std::int64_t(request.position.X)+request.width,
        std::int64_t(request.clip.X)+request.clip.Width});
    const auto bottom=std::min({std::int64_t(h),std::int64_t(request.position.Y)+request.height,
        std::int64_t(request.clip.Y)+request.clip.Height});
    if (right<=left || bottom<=top) return DrawingStatus::skipped;
    if(request.blend_mode==RasterBlendMode::shroud||request.blend_mode==RasterBlendMode::fog||request.blend_mode==RasterBlendMode::alpha_shape){
        std::vector<std::uint32_t> pixels(std::size_t(right-left)*std::size_t(bottom-top));
        for(auto y=top;y<bottom;++y)for(auto x=left;x<right;++x){
            const auto value=request.pixels?request.pixels[(y-request.position.Y)*request.pitch+x-request.position.X]:request.color;
            pixels[(y-top)*(right-left)+x-left]=value<=255?(0x10000u|value):0;
        }
        MapDrawPacket packet;packet.lighting=true;
        packet.parameters={w,h,int(right-left),int(bottom-top),int(left),int(top),int(left),int(top),int(right-left),int(bottom-top)};
        packet.parameters[19]=request.blend_mode==RasterBlendMode::shroud?1:request.blend_mode==RasterBlendMode::fog?2:3;
        packet.source=self.buffer(pixels);packet.palette=packet.source;
        self.frame_->packets.push_back(std::move(packet));return DrawingStatus::drawn;
    }
    std::shared_ptr<MapBufferData> source;
    auto& dummy=self.fills_[0];
    if (!dummy) dummy=self.buffer({rgba565(0)});
    if (request.pixels) {
        std::vector<std::uint32_t> pixels(std::size_t(right-left)*std::size_t(bottom-top));
        for (auto y=top;y<bottom;++y) for (auto x=left;x<right;++x)
            pixels[(y-top)*(right-left)+x-left]=(request.blend_mode==RasterBlendMode::copy?rgba565(request.pixels[(y-request.position.Y)*request.pitch+x-request.position.X]):request.pixels[(y-request.position.Y)*request.pitch+x-request.position.X]);
        source=self.buffer(pixels);
    } else {
        auto& cached=self.fills_[request.color];
        if (!cached) cached=self.buffer({rgba565(request.color)});
        source=cached;
    }
    MapDrawPacket packet;
    packet.parameters={w,h,int(right-left),int(bottom-top),int(left),int(top),int(left),int(top),int(right-left),int(bottom-top)};
    packet.parameters[19]=request.pixels ? 1 : 2;
    if(request.original_line){packet.parameters[19]=4;
        packet.parameters[13]=2;
        packet.parameters[14]=std::uint16_t(0x8000u-std::uint32_t(request.position.Y)-std::uint32_t(request.clip.Y)+std::uint32_t(request.line_z));}
    if(request.blend_mode==RasterBlendMode::particle){
        TypeGpuPacket encoded;
        const auto status=prepare_type_particle_parameters(request,{w,h,0,0,0x8000,true},encoded);
        if(status!=DrawingStatus::drawn)return status;packet.parameters=encoded.parameters;
    }else if(request.blend_mode==RasterBlendMode::depth_alpha){
        if(request.line_opacity<8)return DrawingStatus::skipped;
        packet.parameters[19]=7;packet.parameters[11]=int(request.line_rgb);packet.parameters[12]=request.line_opacity;
        packet.parameters[14]=std::uint16_t(0x8000u-std::uint32_t(request.position.Y)-std::uint32_t(request.clip.Y)+std::uint32_t(request.line_z));
    }else if(request.blend_mode!=RasterBlendMode::copy){
        packet.parameters[19]=request.blend_mode==RasterBlendMode::spotlight?5:6;
        packet.parameters[11]=int(request.spotlight_flags);
        packet.parameters[12]=request.blend_mode==RasterBlendMode::depth_glow?request.light_strength:request.color;
        packet.parameters[10]=request.pixels?1:0;
        packet.parameters[14]=std::uint16_t(0x8000u-std::uint32_t(request.position.Y)-std::uint32_t(request.clip.Y)+std::uint32_t(request.line_z));
    }
    packet.source=std::move(source); packet.palette=dummy;
    self.frame_->packets.push_back(std::move(packet));
    return DrawingStatus::drawn;
}
DrawingStatus MapResources::indexed(void* pointer,const IndexedDrawingRequest& r) {
    auto& self=*static_cast<MapResources*>(pointer);
    if(!self.frame_||r.target!=reinterpret_cast<DrawingTargetHandle*>(&self)||!r.palette)
        return DrawingStatus::invalid_argument;
    const auto& palette=*reinterpret_cast<const MapPaletteData*>(r.palette);
    MapDrawPacket packet;
    TypeGpuPacket prepared;
    const auto result=prepare_type_indexed_parameters(r,{self.frame_->width,self.frame_->height,palette.shades,0,0x8000,true},prepared);
    if(result!=DrawingStatus::drawn)return result;
    packet.parameters=prepared.parameters;
    // A new immutable buffer is intentionally owned by the queued frame. Never
    // retain the core's temporary vector pointer or reuse a pointer-keyed cache.
    packet.source=self.buffer(std::vector<std::uint32_t>(r.pixels,r.pixels+std::size_t(r.width)*r.height));
    packet.palette=palette.buffer;self.frame_->packets.push_back(std::move(packet));
    return DrawingStatus::drawn;
}
}
