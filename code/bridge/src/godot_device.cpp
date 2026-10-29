#include "godot_device.hpp"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>
#include <godot_cpp/classes/rd_sampler_state.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/classes/rd_pipeline_rasterization_state.hpp>
#include <godot_cpp/classes/rd_pipeline_multisample_state.hpp>
#include <godot_cpp/classes/rd_pipeline_depth_stencil_state.hpp>
#include <godot_cpp/classes/rd_pipeline_color_blend_state.hpp>
#include <godot_cpp/classes/rd_pipeline_color_blend_state_attachment.hpp>
#include <cstring>
#include <cstdio>
#include <stdexcept>
using namespace godot;
namespace game {
namespace {
using RD=RenderingDevice;
using Status=render::Status;
template<class F> Status boundary(F&& operation) noexcept {
    try { return operation(); } catch (...) { return Status::backend_failure; }
}
PackedByteArray bytes(const void* source,std::uint64_t size) {
    PackedByteArray data; data.resize(size); if(size) std::memcpy(data.ptrw(),source,size); return data;
}
RD::DataFormat format(render::Format value) {
    switch(value) {
        case render::Format::rgba8: return RD::DATA_FORMAT_R8G8B8A8_UNORM;
        case render::Format::depth32: return RD::DATA_FORMAT_D32_SFLOAT;
        case render::Format::r32u: return RD::DATA_FORMAT_R32_UINT;
        case render::Format::rg32u: return RD::DATA_FORMAT_R32G32_UINT;
        case render::Format::r8: return RD::DATA_FORMAT_R8_UNORM;
        case render::Format::r8u: return RD::DATA_FORMAT_R8_UINT;
    }
    throw std::invalid_argument("texture format");
}
std::uint64_t usage(std::uint32_t flags) {
    std::uint64_t value=0;
    if(flags&render::sampled) value|=RD::TEXTURE_USAGE_SAMPLING_BIT;
    if(flags&render::storage) value|=RD::TEXTURE_USAGE_STORAGE_BIT;
    if(flags&render::color_attachment) value|=RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
    if(flags&render::depth_attachment) value|=RD::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if(flags&render::copy_source) value|=RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
    if(flags&render::copy_destination) value|=RD::TEXTURE_USAGE_CAN_COPY_TO_BIT;
    if(flags&render::storage_atomic) value|=RD::TEXTURE_USAGE_STORAGE_ATOMIC_BIT;
    return value;
}
RD::BlendFactor factor(render::BlendFactor f) {
    switch(f) {
        case render::BlendFactor::zero: return RD::BLEND_FACTOR_ZERO;
        case render::BlendFactor::one: return RD::BLEND_FACTOR_ONE;
        case render::BlendFactor::constant_color: return RD::BLEND_FACTOR_CONSTANT_COLOR;
    }
    throw std::invalid_argument("blend factor");
}
RD::CompareOperator compare(render::Compare c) {
    switch(c) {
        case render::Compare::less: return RD::COMPARE_OP_LESS;
        case render::Compare::less_equal: return RD::COMPARE_OP_LESS_OR_EQUAL;
        case render::Compare::always: return RD::COMPARE_OP_ALWAYS;
    }
    throw std::invalid_argument("depth comparison");
}
Status status(Error e) { return e==OK?Status::ok:Status::backend_failure; }
Vector3 vector(render::Extent3 e) { return {float(e.x),float(e.y),float(e.z)}; }
}
RID GodotDevice::rid(render::Handle h) const {
    if(!h.is_valid()) return {};
    const auto found=resources_.find(h.value);
    if(found==resources_.end()) throw std::invalid_argument("unknown device resource");
    return found->second.rid;
}
Status GodotDevice::retain(RID resource,render::Handle& h,ResourceKind kind) {
    h={}; if(!resource.is_valid()) return Status::backend_failure;
    try { resources_.emplace(std::uint64_t(resource.get_id()),Resource{resource,kind}); }
    catch(...) { rd_->free_rid(resource); throw; }
    h.value=std::uint64_t(resource.get_id()); return Status::ok;
}
render::Device GodotDevice::interface() noexcept {
    rd_=RenderingServer::get_singleton()->get_rendering_device();
    if(!rd_) return {};
    render::Device d; d.context=this;
    d.create_buffer=[](void* p,std::uint64_t size,const void* data,std::uint64_t count,render::Handle& out) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); return s.retain(s.rd_->storage_buffer_create(size,bytes(data,count)),out);});
    };
    d.update_buffer=[](void* p,render::Handle h,std::uint64_t offset,const void* data,std::uint64_t size) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); return status(s.rd_->buffer_update(s.rid(h),offset,size,bytes(data,size)));});
    };
    d.copy_buffer=[](void* p,render::Handle a,render::Handle b,std::uint64_t src,std::uint64_t dst,std::uint64_t size) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); return status(s.rd_->buffer_copy(s.rid(a),s.rid(b),src,dst,size));});
    };
    d.clear_buffer=[](void* p,render::Handle h,std::uint64_t offset,std::uint64_t size) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); return status(s.rd_->buffer_clear(s.rid(h),offset,size));});
    };
    d.create_texture=[](void* p,const render::TextureDesc& desc,render::Handle& out) noexcept {
        return boundary([&]{
            auto& s=*static_cast<GodotDevice*>(p); Ref<RDTextureFormat> f; f.instantiate();
            f->set_width(desc.width); f->set_height(desc.height); f->set_array_layers(desc.layers);
            f->set_format(format(desc.format)); f->set_usage_bits(usage(desc.usage));
            if(desc.array) f->set_texture_type(RD::TEXTURE_TYPE_2D_ARRAY);
            Ref<RDTextureView> view; view.instantiate(); return s.retain(s.rd_->texture_create(f,view),out,ResourceKind::texture);
        });
    };
    d.supports_texture=[](void* p,render::Format f,std::uint32_t u) noexcept {
        try {auto& s=*static_cast<GodotDevice*>(p); return s.rd_->texture_is_format_supported_for_usage(format(f),usage(u));} catch(...) {return false;}
    };
    d.texture_slice=[](void* p,render::Handle base,std::uint32_t layer,render::Handle& out) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); Ref<RDTextureView> view; view.instantiate();
            return s.retain(s.rd_->texture_create_shared_from_slice(view,s.rid(base),layer,0,1,RD::TEXTURE_SLICE_2D),out,ResourceKind::texture);});
    };
    d.copy_texture=[](void* p,render::Handle a,render::Handle b,render::Extent3 src,render::Extent3 dst,
                      render::Extent3 extent,std::uint32_t sl,std::uint32_t dl) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); return status(s.rd_->texture_copy(s.rid(a),s.rid(b),vector(src),vector(dst),vector(extent),0,0,sl,dl));});
    };
    d.read_texture=[](void* p,render::Handle h,std::uint32_t layer,void* output,std::uint64_t capacity,std::uint64_t& size) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); const auto data=s.rd_->texture_get_data(s.rid(h),layer);
            size=data.size(); if(output) {if(capacity<size) return Status::invalid_argument; std::memcpy(output,data.ptr(),size);} return Status::ok;});
    };
    d.max_texture_layers=[](void* p) noexcept {return std::uint32_t(static_cast<GodotDevice*>(p)->rd_->limit_get(RD::LIMIT_MAX_TEXTURE_ARRAY_LAYERS));};
    d.create_framebuffer=[](void* p,const render::Handle* values,std::uint32_t count,render::Handle& out) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); TypedArray<RID> attachments;
            for(std::uint32_t i=0;i<count;++i) attachments.append(s.rid(values[i])); return s.retain(s.rd_->framebuffer_create(attachments),out,ResourceKind::framebuffer);});
    };
    d.framebuffer_format=[](void* p,render::Handle h) noexcept -> std::uint64_t {
        try {auto& s=*static_cast<GodotDevice*>(p); return s.rd_->framebuffer_get_format(s.rid(h));} catch(...) {return 0;}
    };
    d.create_shader=[](void* p,const render::ShaderProgram& program,render::Handle& out) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); Ref<RDShaderSPIRV> spirv; spirv.instantiate();
            const auto stage=[&](RD::ShaderStage stage,const render::Spirv& data){
                if(data.count) spirv->set_stage_bytecode(stage,bytes(data.words,std::uint64_t(data.count)*4));
            };
            stage(RD::SHADER_STAGE_COMPUTE,program.compute); stage(RD::SHADER_STAGE_VERTEX,program.vertex);
            stage(RD::SHADER_STAGE_FRAGMENT,program.fragment); return s.retain(s.rd_->shader_create_from_spirv(spirv),out);});
    };
    d.create_pipeline=[](void* p,const render::PipelineDesc& desc,render::Handle& out) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p);
            Ref<RDPipelineRasterizationState> raster; raster.instantiate(); Ref<RDPipelineMultisampleState> ms; ms.instantiate();
            Ref<RDPipelineDepthStencilState> depth; depth.instantiate(); depth->set_enable_depth_test(desc.depth.test);
            depth->set_enable_depth_write(desc.depth.write); depth->set_depth_compare_operator(compare(desc.depth.compare));
            TypedArray<RDPipelineColorBlendStateAttachment> attachments;
            for(std::uint32_t i=0;i<desc.attachment_count;++i) {
                const auto& a=desc.attachments[i]; Ref<RDPipelineColorBlendStateAttachment> b; b.instantiate();
                b->set_enable_blend(a.enabled); b->set_write_r(a.red); b->set_write_g(a.green); b->set_write_b(a.blue); b->set_write_a(a.alpha);
                b->set_src_color_blend_factor(factor(a.source)); b->set_dst_color_blend_factor(factor(a.destination));
                b->set_src_alpha_blend_factor(factor(a.source_alpha)); b->set_dst_alpha_blend_factor(factor(a.destination_alpha));
                b->set_color_blend_op(RD::BLEND_OP_ADD); b->set_alpha_blend_op(RD::BLEND_OP_ADD); attachments.append(b);
            }
            Ref<RDPipelineColorBlendState> blend; blend.instantiate(); blend->set_attachments(attachments);
            return s.retain(s.rd_->render_pipeline_create(s.rid(desc.shader),desc.framebuffer_format,RD::INVALID_FORMAT_ID,
                RD::RENDER_PRIMITIVE_TRIANGLES,raster,ms,depth,blend,desc.dynamic_blend?RD::DYNAMIC_STATE_BLEND_CONSTANTS:0),out);
        });
    };
    d.create_compute_pipeline=[](void* p,render::Handle shader,render::Handle& out) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); return s.retain(s.rd_->compute_pipeline_create(s.rid(shader)),out);});
    };
    d.create_sampler=[](void* p,render::Handle& out) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); Ref<RDSamplerState> sampler; sampler.instantiate(); return s.retain(s.rd_->sampler_create(sampler),out);});
    };
    d.create_bindings=[](void* p,render::Handle shader,const render::Binding* values,std::uint32_t count,render::Handle& out) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); TypedArray<RDUniform> uniforms;
            for(std::uint32_t i=0;i<count;++i) {
                const auto& b=values[i]; Ref<RDUniform> u; u.instantiate(); u->set_binding(b.index);
                u->set_uniform_type(b.kind==render::BindingKind::buffer?RD::UNIFORM_TYPE_STORAGE_BUFFER:
                    b.kind==render::BindingKind::image?RD::UNIFORM_TYPE_IMAGE:RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
                u->add_id(s.rid(b.resource)); if(b.texture.is_valid()) u->add_id(s.rid(b.texture)); uniforms.append(u);
            }
            return s.retain(s.rd_->uniform_set_create(uniforms,s.rid(shader),0),out,ResourceKind::bindings);
        });
    };
    d.bindings_valid=[](void* p,render::Handle h) noexcept {
        try {auto& s=*static_cast<GodotDevice*>(p); return s.rd_->uniform_set_is_valid(s.rid(h));} catch(...) {return false;}
    };
    d.texture_valid=[](void* p,render::Handle h) noexcept {
        try {auto& s=*static_cast<GodotDevice*>(p); return s.rd_->texture_is_valid(s.rid(h));} catch(...) {return false;}
    };
    d.framebuffer_valid=[](void* p,render::Handle h) noexcept {
        try {auto& s=*static_cast<GodotDevice*>(p); return s.rd_->framebuffer_is_valid(s.rid(h));} catch(...) {return false;}
    };
    d.begin_compute=[](void* p,std::int64_t& list) noexcept {
        return boundary([&]{list=static_cast<GodotDevice*>(p)->rd_->compute_list_begin(); return list>=0?Status::ok:Status::backend_failure;});
    };
    d.begin_draw=[](void* p,const render::DrawListDesc& desc,std::int64_t& list) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); PackedColorArray colors;
            for(std::uint32_t i=0;i<desc.color_count;++i) {const auto c=desc.colors[i]; colors.push_back(Color(c.r,c.g,c.b,c.a));}
            std::uint32_t flags=0; if(desc.clear_mask&0x80000000u) flags|=RD::DRAW_CLEAR_DEPTH;
            const auto mask=desc.clear_mask&0x7FFFFFFFu;
            if(mask==0x7FFFFFFFu) flags|=RD::DRAW_CLEAR_COLOR_ALL; else if(mask&1) flags|=RD::DRAW_CLEAR_COLOR_0;
            list=s.rd_->draw_list_begin(s.rid(desc.framebuffer),RD::DrawFlags(flags),colors,desc.depth);
            return list>=0?Status::ok:Status::backend_failure;
        });
    };
    d.record=[](void* p,const render::DeviceCommand& c) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); using K=render::CommandKind;
            switch(c.kind) {
                case K::end_compute:s.rd_->compute_list_end();break;
                case K::end_draw:s.rd_->draw_list_end();break;
                case K::compute_pipeline:s.rd_->compute_list_bind_compute_pipeline(c.list,s.rid(c.resource));break;
                case K::draw_pipeline:s.rd_->draw_list_bind_render_pipeline(c.list,s.rid(c.resource));break;
                case K::compute_binding:s.rd_->compute_list_bind_uniform_set(c.list,s.rid(c.resource),c.x);break;
                case K::draw_binding:s.rd_->draw_list_bind_uniform_set(c.list,s.rid(c.resource),c.x);break;
                case K::compute_push:s.rd_->compute_list_set_push_constant(c.list,bytes(c.data,c.size),c.size);break;
                case K::draw_push:s.rd_->draw_list_set_push_constant(c.list,bytes(c.data,c.size),c.size);break;
                case K::dispatch:s.rd_->compute_list_dispatch(c.list,c.x,c.y,c.z);break;
                case K::draw:s.rd_->draw_list_draw(c.list,false,c.x,c.y);break;
                case K::barrier:s.rd_->compute_list_add_barrier(c.list);break;
                case K::blend_constant:s.rd_->draw_list_set_blend_constants(c.list,Color(c.color.r,c.color.g,c.color.b,c.color.a));break;
                case K::timestamp:s.rd_->capture_timestamp(String::utf8(static_cast<const char*>(c.data)));break;
                default:return Status::invalid_argument;
            }
            return Status::ok;
        });
    };
    d.release=[](void* p,render::Handle h) noexcept {
        try {auto& s=*static_cast<GodotDevice*>(p); const auto found=s.resources_.find(h.value);
            if(found!=s.resources_.end()) {
                const Resource resource=found->second;
                // Dependencies may already have invalidated these Godot objects.
                // Always retire the adapter handle, even when no RID remains to free.
                const bool valid=resource.kind==ResourceKind::bindings ? s.rd_->uniform_set_is_valid(resource.rid) :
                    resource.kind==ResourceKind::framebuffer ? s.rd_->framebuffer_is_valid(resource.rid) :
                    resource.kind==ResourceKind::texture ? s.rd_->texture_is_valid(resource.rid) : true;
                s.resources_.erase(found);
                if(valid) s.rd_->free_rid(resource.rid);
            }} catch(...) {}
    };
    d.timestamp_frame=[](void* p) noexcept {return static_cast<GodotDevice*>(p)->rd_->get_captured_timestamps_frame();};
    d.timestamp_count=[](void* p) noexcept {return static_cast<GodotDevice*>(p)->rd_->get_captured_timestamps_count();};
    d.timestamp=[](void* p,std::uint32_t index,render::Timestamp& t) noexcept {
        return boundary([&]{auto& s=*static_cast<GodotDevice*>(p); t.gpu_time=s.rd_->get_captured_timestamp_gpu_time(index);
            const auto name=s.rd_->get_captured_timestamp_name(index).utf8(); std::snprintf(t.name,sizeof(t.name),"%s",name.get_data()); return Status::ok;});
    };
    return d;
}
}
