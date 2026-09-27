#include "type_drawing.hpp"
#include "yrpp/ObjectClass.h"
#include "yrpp/ObjectTypeClass.h"
#include "yrpp/FileFormats/SHP.h"
#include <cstdio>
#include <vector>

namespace game {
namespace {
struct Scope {
    const TypeDrawingContext* context;
    DrawingStatus status = DrawingStatus::skipped;
};
thread_local Scope* active = nullptr;
thread_local const DrawingResources* resources = nullptr;
thread_local char failure[1024]{};
DrawingStatus checked_status(DrawingStatus value) noexcept {
    switch (value) {
    case DrawingStatus::drawn: case DrawingStatus::skipped: case DrawingStatus::unavailable:
    case DrawingStatus::unsupported: case DrawingStatus::invalid_argument: case DrawingStatus::backend_failure:
        return value;
    }
    return DrawingStatus::backend_failure;
}
bool valid(const TypeDrawingContext& context) noexcept {
    return context.backend.version == 12 &&
        context.backend.struct_size == sizeof(TypeDrawingBackend);
}
}
bool drawing_completed(DrawingStatus status) noexcept {
    return status == DrawingStatus::drawn || status == DrawingStatus::skipped;
}
const DrawingResources* active_drawing_resources() noexcept { return resources; }
DrawingStatus with_drawing_resources(const DrawingResources& value, const TypeDrawingContext& context,
        void (*operation)(void*), void* argument) noexcept {
    const auto* previous=resources;
    resources=&value;
    const auto status=with_type_drawing(context,operation,argument);
    resources=previous;
    return status;
}
bool resolve_drawing_palette(DrawingPaletteKind kind,int index,const DrawingPaletteHandle*& result) noexcept {
    result=nullptr;
    auto status=resources&&resources->palette
        ? resources->palette(resources->context,kind,index,result) : DrawingStatus::unavailable;
    if(status==DrawingStatus::drawn&&!result)status=DrawingStatus::unavailable;
    if(!drawing_completed(status))record_type_drawing_result(status);
    return status==DrawingStatus::drawn;
}
void clear_drawing_failure() noexcept { failure[0]=0; }
const char* drawing_failure() noexcept { return failure; }
DrawingStatus record_drawing_failure(DrawingStatus status,const char* stage,
        const ObjectClass* object,const ShapeDrawingRequest* shape) noexcept {
    if(drawing_completed(status)||failure[0])return status;
    const auto* type=object?object->GetType():nullptr;
    const auto location=object?object->Location:CoordStruct{};
    const auto* image=shape&&shape->image?shape->image->AsReference():nullptr;
    const auto* depth=shape&&shape->depth_image?shape->depth_image->AsReference():nullptr;
    std::snprintf(failure,sizeof(failure),
        "%s: %s; object=%s id=%u kind=%d world=(%d,%d,%d) image=%s frame=%d flags=0x%X depth_image=%s",
        stage,drawing_status_name(status),type?type->ID:"<none>",object?unsigned(object->UniqueID):0,
        object?int(object->WhatAmI()):-1,location.X,location.Y,location.Z,
        image?image->Filename:"<none/raw>",shape?shape->frame:-1,shape?shape->flags:0,
        depth?depth->Filename:"<none/raw>");
    return status;
}
const TypeDrawingContext* active_type_drawing() noexcept { return active ? active->context : nullptr; }
void record_type_drawing_result(DrawingStatus status) noexcept {
    if (!active || !drawing_completed(active->status)) return;
    if (status != DrawingStatus::skipped) active->status = status;
}
DrawingStatus submit_type_shape(const TypeDrawingContext& context, const ShapeDrawingRequest& request) noexcept {
    if (!valid(context)) return DrawingStatus::unsupported;
    if (!context.backend.shape) return DrawingStatus::unavailable;
    if (!request.target || !request.palette || request.frame < 0) return DrawingStatus::invalid_argument;
    if (!request.image || request.clip.Width <= 0 || request.clip.Height <= 0) return DrawingStatus::skipped;
    try { return checked_status(context.backend.shape(context.backend_context, request)); }
    catch (...) { return DrawingStatus::backend_failure; }
}
DrawingStatus submit_type_tile(const TypeDrawingContext& context, const TileDrawingRequest& request) noexcept {
    if (!valid(context)) return DrawingStatus::unsupported;
    if (!context.backend.tile) return DrawingStatus::unavailable;
    if (!request.target || !request.palette || request.sub_tile < 0) return DrawingStatus::invalid_argument;
    if (!request.resource || !request.image || request.clip.Width <= 0 || request.clip.Height <= 0)
        return DrawingStatus::skipped;
    try { return checked_status(context.backend.tile(context.backend_context, request)); }
    catch (...) { return DrawingStatus::backend_failure; }
}
DrawingStatus with_type_drawing(const TypeDrawingContext& context, void (*operation)(void*), void* argument) noexcept {
    if (!operation) return DrawingStatus::invalid_argument;
    if (!valid(context)) return DrawingStatus::unsupported;
    Scope scope{&context};
    Scope* previous = active;
    active = &scope;
    try { operation(argument); }
    catch (...) { scope.status = DrawingStatus::backend_failure; }
    active = previous;
    return scope.status;
}
DrawingStatus submit_type_raster(const TypeDrawingContext& context,const RasterDrawingRequest& request) noexcept {
    if (!valid(context)) return DrawingStatus::unsupported;
    if (!context.backend.raster) return DrawingStatus::unavailable;
    if(request.blend_mode!=RasterBlendMode::copy&&request.blend_mode!=RasterBlendMode::spotlight&&request.blend_mode!=RasterBlendMode::depth_glow&&request.blend_mode!=RasterBlendMode::depth_alpha&&request.blend_mode!=RasterBlendMode::particle&&request.blend_mode!=RasterBlendMode::shroud&&request.blend_mode!=RasterBlendMode::fog&&request.blend_mode!=RasterBlendMode::alpha_shape)
        return DrawingStatus::invalid_argument;
    if(request.original_line&&request.blend_mode!=RasterBlendMode::copy)return DrawingStatus::invalid_argument;
    if(request.blend_mode!=RasterBlendMode::copy&&request.spotlight_flags>15)return DrawingStatus::invalid_argument;
    if(request.blend_mode==RasterBlendMode::spotlight&&!request.pixels&&request.color>255)return DrawingStatus::invalid_argument;
    if(request.blend_mode==RasterBlendMode::depth_alpha&&(request.line_rgb>0xFFFFFFu||request.line_opacity<0||request.line_opacity>255))return DrawingStatus::invalid_argument;
    if(request.blend_mode==RasterBlendMode::particle&&request.line_rgb>0xFFFFFFu)return DrawingStatus::invalid_argument;

    if (!request.target || request.width<0 || request.height<0 || request.width>8192 || request.height>8192)
        return DrawingStatus::invalid_argument;
    if (!request.width || !request.height || request.clip.Width<=0 || request.clip.Height<=0)
        return DrawingStatus::skipped;
    if((request.original_line || request.blend_mode==RasterBlendMode::depth_glow || request.blend_mode==RasterBlendMode::depth_alpha || request.blend_mode==RasterBlendMode::particle) && (request.width!=1 || request.height!=1 || request.pixels))
        return DrawingStatus::invalid_argument;
    if (request.pixels && (request.pitch<request.width ||
        std::uint64_t(request.height-1)*request.pitch+request.width>request.pixel_count))
        return DrawingStatus::invalid_argument;
    try { return checked_status(context.backend.raster(context.backend_context,request)); }
    catch (...) { return DrawingStatus::backend_failure; }
}
DrawingStatus submit_type_lighting_shape(const TypeDrawingContext& c,const LightingShapeDrawingRequest& r) noexcept {
    if(!valid(c))return DrawingStatus::unsupported;
    if(!r.target||r.frame<0||(r.operation!=RasterBlendMode::shroud&&r.operation!=RasterBlendMode::fog&&r.operation!=RasterBlendMode::alpha_shape))
        return DrawingStatus::invalid_argument;
    if(!r.image)return DrawingStatus::unavailable;
    if(r.clip.Width<=0||r.clip.Height<=0)return DrawingStatus::skipped;
    try {
        if(c.backend.lighting_shape)return checked_status(c.backend.lighting_shape(c.backend_context,r));
        if(!c.backend.raster)return DrawingStatus::unavailable;
        auto* image=r.image;
        if(auto* ref=image->AsReference()){ref->Load();image=ref->Data;}
        if(!image)return DrawingStatus::unavailable;
        if(r.frame>=image->Frames)return DrawingStatus::invalid_argument;
        const auto rect=image->GetFrameBounds(r.frame);
        if(rect.Width<=0||rect.Height<=0)return DrawingStatus::skipped;
        if(rect.Width>8192||rect.Height>8192)return DrawingStatus::invalid_argument;
        const auto* pixels=image->GetPixels(r.frame);if(!pixels)return DrawingStatus::unavailable;
        // Same raw lighting-frame contract as the original shroud/alpha loop.
        std::vector<std::uint16_t> samples(std::size_t(rect.Width)*rect.Height);
        for(std::size_t i=0;i<samples.size();++i)samples[i]=pixels[i];
        const auto x=std::int64_t(r.position.X)+rect.X,y=std::int64_t(r.position.Y)+rect.Y;
        if(x<INT32_MIN||x>INT32_MAX||y<INT32_MIN||y>INT32_MAX)return DrawingStatus::invalid_argument;
        RasterDrawingRequest raster;raster.target=r.target;raster.position={int(x),int(y)};raster.clip=r.clip;
        raster.width=rect.Width;raster.height=rect.Height;raster.pitch=rect.Width;
        raster.pixels=samples.data();raster.pixel_count=unsigned(samples.size());raster.blend_mode=r.operation;
        return submit_type_raster(c,raster);
    }catch(...){return DrawingStatus::backend_failure;}
}
DrawingStatus submit_type_indexed(const TypeDrawingContext& c,const IndexedDrawingRequest& r) noexcept {
    if(!valid(c))return DrawingStatus::unsupported;
    if(!c.backend.indexed)return DrawingStatus::unavailable;
    if(!r.target||!r.palette||r.width<0||r.height<0||r.width>2048||r.height>2048)
        return DrawingStatus::invalid_argument;
    if(!r.width||!r.height||r.clip.Width<=0||r.clip.Height<=0)return DrawingStatus::skipped;
    if(!r.pixels||std::uint64_t(r.width)*r.height>r.pixel_count||
        static_cast<unsigned>(r.depth_mode)>static_cast<unsigned>(ShapeDepthMode::read_write))
        return DrawingStatus::invalid_argument;
    try{return checked_status(c.backend.indexed(c.backend_context,r));}
    catch(...){return DrawingStatus::backend_failure;}
}
const char* drawing_status_name(DrawingStatus status) noexcept {
    switch (status) {
        case DrawingStatus::drawn: return "drawn";
        case DrawingStatus::skipped: return "skipped";
        case DrawingStatus::unavailable: return "unavailable";
        case DrawingStatus::unsupported: return "unsupported";
        case DrawingStatus::invalid_argument: return "invalid_argument";
        case DrawingStatus::backend_failure: return "backend_failure";
    }
    return "invalid_status";
}
}
