#pragma once
#include "render/device.hpp"
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <span>
namespace render {
// Implementation conveniences never cross Device's POD boundary.
using Bytes = std::vector<std::uint8_t>;
using ByteView = std::span<const std::uint8_t>;
using Colors = std::vector<Color>;
inline constexpr int OK = 0;
struct TextureSpec : TextureDesc {
    TextureSpec* operator->() { return this; }
    void set_width(std::uint32_t v) { width=v; }
    void set_height(std::uint32_t v) { height=v; }
    void set_array_layers(std::uint32_t v) { layers=v; }
    void set_format(Format v) { format=v; }
    void set_usage_bits(std::uint32_t v) { usage=v; }
    void set_texture_type(int) { array=true; }
};
struct DepthSpec : DepthDesc {
    DepthSpec* operator->() { return this; }
    void set_enable_depth_test(bool v) { test=v; }
    void set_enable_depth_write(bool v) { write=v; }
    void set_depth_compare_operator(Compare v) { compare=v; }
};
struct AttachmentSpec : BlendAttachment {
    AttachmentSpec* operator->() { return this; }
    void set_enable_blend(bool v) { enabled=v; }
    void set_write_r(bool v) { red=v; }
    void set_write_g(bool v) { green=v; }
    void set_write_b(bool v) { blue=v; }
    void set_write_a(bool v) { alpha=v; }
    void set_src_color_blend_factor(BlendFactor v) { source=v; }
    void set_dst_color_blend_factor(BlendFactor v) { destination=v; }
    void set_src_alpha_blend_factor(BlendFactor v) { source_alpha=v; }
    void set_dst_alpha_blend_factor(BlendFactor v) { destination_alpha=v; }
    void set_color_blend_op(int) {}
    void set_alpha_blend_op(int) {}
};
struct BlendSpec {
    std::vector<BlendAttachment> attachments;
    BlendSpec* operator->() { return this; }
    void set_attachments(const std::vector<AttachmentSpec>& v) { attachments.assign(v.begin(),v.end()); }
};
class DeviceOperations {
public:
    explicit DeviceOperations(const Device& device) : device_(device) {}
    static constexpr auto DATA_FORMAT_R8G8B8A8_UNORM=Format::rgba8, DATA_FORMAT_D32_SFLOAT=Format::depth32,
        DATA_FORMAT_R32_UINT=Format::r32u, DATA_FORMAT_R32G32_UINT=Format::rg32u,
        DATA_FORMAT_R8_UNORM=Format::r8, DATA_FORMAT_R8_UINT=Format::r8u;
    using DataFormat=Format;
    using DrawFlags=std::uint32_t;
    static constexpr auto TEXTURE_USAGE_SAMPLING_BIT=sampled, TEXTURE_USAGE_STORAGE_BIT=storage,
        TEXTURE_USAGE_COLOR_ATTACHMENT_BIT=color_attachment, TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT=depth_attachment,
        TEXTURE_USAGE_CAN_COPY_FROM_BIT=copy_source, TEXTURE_USAGE_CAN_COPY_TO_BIT=copy_destination,
        TEXTURE_USAGE_STORAGE_ATOMIC_BIT=storage_atomic;
    static constexpr auto COMPARE_OP_LESS=Compare::less, COMPARE_OP_LESS_OR_EQUAL=Compare::less_equal,
        COMPARE_OP_ALWAYS=Compare::always;
    static constexpr auto BLEND_FACTOR_ZERO=BlendFactor::zero, BLEND_FACTOR_ONE=BlendFactor::one,
        BLEND_FACTOR_CONSTANT_COLOR=BlendFactor::constant_color;
    static constexpr int BLEND_OP_ADD=0, TEXTURE_TYPE_2D_ARRAY=0, LIMIT_MAX_TEXTURE_ARRAY_LAYERS=0,
        TEXTURE_SLICE_2D=0;
    static constexpr std::uint32_t DRAW_CLEAR_DEPTH=0x80000000u, DRAW_CLEAR_COLOR_0=1, DRAW_CLEAR_COLOR_ALL=0x7FFFFFFFu;
    static constexpr bool DYNAMIC_STATE_BLEND_CONSTANTS=true;
    static constexpr auto UNIFORM_TYPE_STORAGE_BUFFER=BindingKind::buffer, UNIFORM_TYPE_IMAGE=BindingKind::image,
        UNIFORM_TYPE_SAMPLER_WITH_TEXTURE=BindingKind::sampled_texture;
    using UniformType=BindingKind;
    const Device& device() const { return device_; }
    Handle storage_buffer_create(std::uint64_t size, ByteView data={}) {
        Handle h; check(device_.create_buffer(device_.context,size,data.data(),data.size(),h)); return h;
    }
    int buffer_update(Handle h,std::uint64_t offset,std::uint64_t size,ByteView data) {
        if(size>data.size()) throw std::invalid_argument("buffer update size");
        check(device_.update_buffer(device_.context,h,offset,data.data(),size)); return OK;
    }
    int buffer_copy(Handle a,Handle b,std::uint64_t src,std::uint64_t dst,std::uint64_t size) {
        check(device_.copy_buffer(device_.context,a,b,src,dst,size)); return OK;
    }
    int buffer_clear(Handle h,std::uint64_t offset,std::uint64_t size) {
        check(device_.clear_buffer(device_.context,h,offset,size)); return OK;
    }
    Handle texture_create(const TextureDesc& desc) {
        Handle h; check(device_.create_texture(device_.context,desc,h)); return h;
    }
    bool texture_is_format_supported_for_usage(Format f,std::uint32_t usage) {
        return device_.supports_texture(device_.context,f,usage);
    }
    Handle texture_create_shared_from_slice(Handle base,std::uint32_t layer) {
        Handle h; check(device_.texture_slice(device_.context,base,layer,h)); return h;
    }
    int texture_copy(Handle a,Handle b,Extent3 src,Extent3 dst,Extent3 extent,int,int,std::uint32_t sl,std::uint32_t dl) {
        check(device_.copy_texture(device_.context,a,b,src,dst,extent,sl,dl)); return OK;
    }
    Bytes texture_get_data(Handle h,std::uint32_t layer) {
        std::uint64_t size=0; check(device_.read_texture(device_.context,h,layer,nullptr,0,size));
        Bytes data(size); check(device_.read_texture(device_.context,h,layer,data.data(),data.size(),size)); return data;
    }
    std::uint32_t limit_get(int) { return device_.max_texture_layers(device_.context); }
    Handle framebuffer_create(const std::vector<Handle>& attachments) {
        Handle h; check(device_.create_framebuffer(device_.context,attachments.data(),attachments.size(),h)); return h;
    }
    std::uint64_t framebuffer_get_format(Handle h) { return device_.framebuffer_format(device_.context,h); }
    Handle shader_create_from_spirv(const ShaderProgram& program) {
        Handle h; check(device_.create_shader(device_.context,program,h)); return h;
    }
    Handle render_pipeline_create(Handle shader,std::uint64_t format,const DepthSpec& depth,
                                   const BlendSpec& blend,bool dynamic=false) {
        Handle h; const PipelineDesc desc{shader,format,depth,blend.attachments.data(),
            std::uint32_t(blend.attachments.size()),dynamic};
        check(device_.create_pipeline(device_.context,desc,h)); return h;
    }
    Handle compute_pipeline_create(Handle shader) {
        Handle h; check(device_.create_compute_pipeline(device_.context,shader,h)); return h;
    }
    Handle sampler_create() { Handle h; check(device_.create_sampler(device_.context,h)); return h; }
    Handle binding_set(Handle shader,const std::vector<Binding>& bindings) {
        Handle h; check(device_.create_bindings(device_.context,shader,bindings.data(),bindings.size(),h)); return h;
    }
    bool uniform_set_is_valid(Handle h) { return device_.bindings_valid(device_.context,h); }
    bool framebuffer_is_valid(Handle h) { return device_.framebuffer_valid(device_.context,h); }
    bool texture_is_valid(Handle h) { return device_.texture_valid(device_.context,h); }
    std::int64_t compute_list_begin() {
        std::int64_t list=-1; check(device_.begin_compute(device_.context,list)); return list;
    }
    std::int64_t draw_list_begin(Handle fb,std::uint32_t mask,const Colors& colors,float depth) {
        std::int64_t list=-1; const DrawListDesc desc{fb,mask,colors.data(),std::uint32_t(colors.size()),depth};
        check(device_.begin_draw(device_.context,desc,list)); return list;
    }
    void compute_list_end() { record({CommandKind::end_compute}); }
    void draw_list_end() { record({CommandKind::end_draw}); }
    void compute_list_bind_compute_pipeline(std::int64_t list,Handle h) { record({CommandKind::compute_pipeline,list,h}); }
    void draw_list_bind_render_pipeline(std::int64_t list,Handle h) { record({CommandKind::draw_pipeline,list,h}); }
    void compute_list_bind_uniform_set(std::int64_t list,Handle h,std::uint32_t set) { record({CommandKind::compute_binding,list,h,set}); }
    void draw_list_bind_uniform_set(std::int64_t list,Handle h,std::uint32_t set) { record({CommandKind::draw_binding,list,h,set}); }
    void compute_list_set_push_constant(std::int64_t list,ByteView bytes,std::uint32_t size) { push(CommandKind::compute_push,list,bytes,size); }
    void draw_list_set_push_constant(std::int64_t list,ByteView bytes,std::uint32_t size) { push(CommandKind::draw_push,list,bytes,size); }
    void compute_list_dispatch(std::int64_t list,std::uint32_t x,std::uint32_t y,std::uint32_t z) { record({CommandKind::dispatch,list,{},x,y,z}); }
    void draw_list_draw(std::int64_t list,bool,std::uint32_t instances,std::uint32_t vertices) { record({CommandKind::draw,list,{},instances,vertices}); }
    void compute_list_add_barrier(std::int64_t list) { record({CommandKind::barrier,list}); }
    void draw_list_set_blend_constants(std::int64_t list,Color color) { DeviceCommand c{CommandKind::blend_constant,list}; c.color=color; record(c); }
    void capture_timestamp(const char* name) { DeviceCommand c{CommandKind::timestamp}; c.data=name; record(c); }
    std::uint64_t get_captured_timestamps_frame() { return device_.timestamp_frame(device_.context); }
    std::uint32_t get_captured_timestamps_count() { return device_.timestamp_count(device_.context); }
    std::uint64_t get_captured_timestamp_gpu_time(std::uint32_t index) { Timestamp t; check(device_.timestamp(device_.context,index,t)); return t.gpu_time; }
    std::string get_captured_timestamp_name(std::uint32_t index) { Timestamp t; check(device_.timestamp(device_.context,index,t)); return t.name; }
    void free_rid(Handle h) { device_.release(device_.context,h); }
private:
    Device device_;
    static void check(Status status) { if(status!=Status::ok) throw std::runtime_error("render device operation failed"); }
    void record(const DeviceCommand& c) { check(device_.record(device_.context,c)); }
    void push(CommandKind kind,std::int64_t list,ByteView data,std::uint32_t size) {
        if(size>data.size()) throw std::invalid_argument("push constant size");
        DeviceCommand c{kind,list}; c.data=data.data(); c.size=size; record(c);
    }
};
}
