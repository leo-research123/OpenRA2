#include "presentation/frame_compiler.hpp"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/IsometricTileTypeClass.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace game {
namespace {
std::vector<std::uint8_t> copy_frame(SHPStruct* source,int index) {
    if (!source) return {};
    source->Load();
    const auto* file=source->GetData();
    if (!file || index<0 || index>=file->Frames) throw std::invalid_argument("reference shape frame");
    auto header=file->GetFrameHeader(index);
    const auto* pixels=file->GetPixels(index);
    std::size_t size=0;
    if (header.Flags&2) {
        for (int y=0;y<header.Height;++y) {
            std::uint16_t row=0; std::memcpy(&row,pixels+size,2);
            if (row<2 || size+row>64*1024*1024) throw std::invalid_argument("reference RLE row");
            size+=row;
        }
    } else size=std::size_t(std::max<int>(header.Width,0))*std::max<int>(header.Height,0);
    std::vector<std::uint8_t> bytes(sizeof(SHPStruct)+sizeof(SHPFrame)+size);
    const std::int16_t fields[]{0,file->Width,file->Height,1};
    std::memcpy(bytes.data(),fields,sizeof(fields));
    header.Offset=sizeof(SHPStruct)+sizeof(SHPFrame);
    std::memcpy(bytes.data()+sizeof(SHPStruct),&header,sizeof(header));
    if (size) std::memcpy(bytes.data()+header.Offset,pixels,size);
    return bytes;
}
}
void FrameCompiler::retain_reference_palette(render::ReferenceDraw& draw,const DrawingPaletteHandle* identity) {
    const auto& palette=*reinterpret_cast<const MapPaletteData*>(identity);
    draw.shades=palette.shades;
    const auto& bytes=palette.buffer->bytes;
    draw.palette.resize(palette.buffer->byte_count/4);
    for (std::size_t i=0;i<draw.palette.size();++i) {
        std::uint32_t rgba=0;std::memcpy(&rgba,bytes.get()+i*4,4);
        draw.palette[i]=std::uint16_t(((rgba&0xFF)>>3)<<11 | (((rgba>>8)&0xFF)>>2)<<5 | ((rgba>>16)&0xFF)>>3);
    }
}
void FrameCompiler::retain_reference(const ShapeDrawingRequest& q) {
    render::ReferenceDraw d; d.kind=render::ReferenceKind::shape; d.position=q.position; d.clip=q.clip;
    d.words={int(q.flags),q.depth_adjustment,q.gradient,q.intensity,q.tint,int(q.depth_mode),int(q.blend_mode),
        q.absolute_depth,q.depth_offset.X,q.depth_offset.Y};
    d.image=copy_frame(q.image,q.frame); d.auxiliary=copy_frame(q.depth_image,q.depth_frame);
    retain_reference_palette(d,q.palette); frame_->reference->draws.push_back(std::move(d));
}
void FrameCompiler::retain_reference(const TileDrawingRequest& q) {
    render::ReferenceDraw d; d.kind=render::ReferenceKind::tile; d.position=q.position; d.clip=q.clip;
    d.words={q.level,q.intensity,q.use_depth,q.flat,q.flag16,q.flag17,q.color,q.buffer_offset_y,q.translucent};
    d.tile.resize(sizeof(TMPStruct));std::memcpy(d.tile.data(),q.resource,sizeof(TMPStruct));
    std::size_t size=sizeof(TMPImage)+900;
    const auto include=[&](int offset,std::size_t count) {
        if (offset<0 || std::uint64_t(offset)+count>64*1024*1024) throw std::invalid_argument("reference TMP plane");
        size=std::max(size,std::size_t(offset)+count);
    };
    if (q.image->Flags&2) include(q.image->ZOffset,900);
    if (q.image->Flags&1) {
        const auto extra=std::size_t(std::max(q.image->ExtraWidth,0))*std::max(q.image->ExtraHeight,0);
        include(q.image->ExtraOffset,extra);if (q.image->Flags&2) include(q.image->ExtraZOffset,extra);
    }
    d.image.resize(size);std::memcpy(d.image.data(),q.image,size);
    retain_reference_palette(d,q.palette); frame_->reference->draws.push_back(std::move(d));
}
void FrameCompiler::retain_reference(const RasterDrawingRequest& q) {
    render::ReferenceDraw d;d.kind=render::ReferenceKind::raster; d.position=q.position;d.clip=q.clip;
    d.words={q.width,q.height,q.pitch,q.color,q.original_line,int(q.blend_mode),int(q.spotlight_flags),q.light_strength,
        q.line_z,int(q.line_rgb),q.line_opacity,q.line_neighbor.X,q.line_neighbor.Y,q.line_weight,q.line_neighbor_weight,
        int(q.line_channels),q.line_light_x,q.line_sample_x,q.line_red,q.line_green,q.line_blue};
    if (q.pixels && q.height>0) d.pixels.assign(q.pixels,q.pixels+q.pixel_count);
    for (std::uint32_t i=0;i<q.wave_pixel_count;++i) d.wave.push_back({q.wave_pixels[i].destination,q.wave_pixels[i].sample,q.wave_pixels[i].strength});
    frame_->reference->draws.push_back(std::move(d));
}
void FrameCompiler::retain_reference(const IndexedDrawingRequest& q) {
    if (q.voxel) throw std::invalid_argument("software reference requires generated voxel pixels");
    render::ReferenceDraw d;d.kind=render::ReferenceKind::indexed; d.position=q.position;d.clip=q.clip;
    d.words={q.width,q.height,q.intensity,q.absolute_depth,int(q.depth_mode),q.shadow,int(q.flags),int(q.tint),q.gradient};
    if (q.pixels) d.indexed.assign(q.pixels,q.pixels+q.pixel_count);
    retain_reference_palette(d,q.palette); frame_->reference->draws.push_back(std::move(d));
}
}
