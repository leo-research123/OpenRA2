#include "bridge/map_renderer.hpp"
#include "map_resources.hpp"
#include "lighting_batch.hpp"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/rd_shader_file.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <algorithm>
#include <cstring>
#include <exception>
#include <set>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
using namespace godot;
namespace {
PackedByteArray bytes(const void* data,std::size_t size) {
    PackedByteArray result; result.resize(static_cast<std::int64_t>(size));
    if (size) std::memcpy(result.ptrw(),data,size);
    return result;
}
Ref<RDUniform> uniform(int binding,RenderingDevice::UniformType type,RID rid) {
    Ref<RDUniform> u; u.instantiate(); u->set_binding(binding); u->set_uniform_type(type); u->add_id(rid); return u;
}
}
void RA2MapRenderer::submit(std::shared_ptr<game::MapFrameData> frame) {
    bool schedule=false;
    const bool resized=requested_width_!=frame->width || requested_height_!=frame->height;
    if (resized && texture_.is_valid()) texture_->set_texture_rd_rid({});
    requested_width_=frame->width; requested_height_=frame->height;
    {
        const std::lock_guard lock(mutex_);
        if (closed_) return;
        if (resized) published_texture_={};
        ++requested_sequence_;
        pending_=std::move(frame);
        if (!scheduled_) { scheduled_=true; schedule=true; }
    }
    // Binding a strong self reference keeps queued work alive after node exit.
    if (schedule) RenderingServer::get_singleton()->call_on_render_thread(
        callable_mp(this,&RA2MapRenderer::render_on_thread).bind(Ref<RefCounted>(this)));
}
Ref<Texture2DRD> RA2MapRenderer::texture() {
    RID rid;
    { const std::lock_guard lock(mutex_); rid=published_texture_; }
    if (!rid.is_valid()) return {};
    if (texture_.is_null()) texture_.instantiate();
    if (texture_->get_texture_rd_rid()!=rid) texture_->set_texture_rd_rid(rid);
    return texture_;
}
String RA2MapRenderer::error() const { const std::lock_guard lock(mutex_); return error_; }
bool RA2MapRenderer::is_current() const {
    const std::lock_guard lock(mutex_);
    return !closed_ && requested_sequence_ && requested_sequence_==completed_sequence_;
}
void RA2MapRenderer::shutdown() {
    if (texture_.is_valid()) { texture_->set_texture_rd_rid({}); texture_.unref(); }
    {
        const std::lock_guard lock(mutex_);
        if (closed_) return;
        closed_=true; pending_.reset(); published_texture_={};
    }
    RenderingServer::get_singleton()->call_on_render_thread(
        callable_mp(this,&RA2MapRenderer::clear_on_thread).bind(Ref<RefCounted>(this)));
}
void RA2MapRenderer::render_on_thread(const Ref<RefCounted>&) {
    std::shared_ptr<game::MapFrameData> frame;
    std::uint64_t sequence=0;
    {
        const std::lock_guard lock(mutex_);
        scheduled_=false;
        if (closed_) return;
        frame=std::move(pending_);
        sequence=requested_sequence_;
    }
    if (!frame) return;
    try {
        const bool ok=render(*frame);
        const std::lock_guard lock(mutex_);
        if (!closed_ && ok && sequence==requested_sequence_) {
            published_texture_=target_; completed_sequence_=sequence;
        }
        else if (!closed_ && !ok && error_.is_empty()) error_="Map GPU resource or dispatch creation failed";
    } catch (const std::exception& e) {
        const std::lock_guard lock(mutex_); error_=String::utf8(e.what());
    } catch (...) { const std::lock_guard lock(mutex_); error_="Map GPU submission failed"; }
}
void RA2MapRenderer::free_uniforms() {
    if (!rd_) return;
    for (auto [key,rid] : uniforms_) if (rd_->uniform_set_is_valid(rid)) rd_->free_rid(rid);
    uniforms_.clear();
    if (lighting_batch_uniform_.is_valid() && rd_->uniform_set_is_valid(lighting_batch_uniform_))
        rd_->free_rid(lighting_batch_uniform_);
    lighting_batch_uniform_={};
}
void RA2MapRenderer::free_states() {
    if (!rd_) return;
    free_uniforms();
    if (present_uniform_.is_valid() && rd_->uniform_set_is_valid(present_uniform_)) rd_->free_rid(present_uniform_);
    present_uniform_={};
    for (auto [key,rid] : buffers_) rd_->free_rid(rid);
    buffers_.clear();
    for (RID* rid : {&colors_,&depths_,&lights_,&target_,&parameters_,&lighting_atlas_,&lighting_bins_}) {
        if (rid->is_valid()) rd_->free_rid(*rid);
        *rid={};
    }
    width_=height_=0; parameter_capacity_=0;
    lighting_atlas_bytes_.clear(); lighting_offsets_.clear();
    lighting_atlas_capacity_=lighting_bins_capacity_=0;
    lighting_atlas_uploaded_=0;
}
void RA2MapRenderer::clear_on_thread(const Ref<RefCounted>&) {
    free_states();
    if (rd_) for (RID* rid : {&pipeline_,&shader_,&present_pipeline_,&present_shader_,&lighting_pipeline_,&lighting_shader_,
        &lighting_batch_pipeline_,&lighting_batch_shader_}) {
        if (rid->is_valid()) rd_->free_rid(*rid);
        *rid={};
    }
    rd_=nullptr;
}
bool RA2MapRenderer::create_pipeline(const char* path,RID& shader,RID& pipeline) {
    // Exported GLSL is an imported RDShaderFile, not a loose text file. Use the
    // same imported SPIR-V in development and export; no runtime source variant.
    Ref<RDShaderFile> source=ResourceLoader::get_singleton()->load(path);
    if (source.is_null()) {
        const std::lock_guard lock(mutex_); error_=String("Cannot load map shader: ")+path; return false;
    }
    auto spirv=source->get_spirv();
    if (spirv.is_null()) return false;
    const auto error=spirv->get_stage_compile_error(RenderingDevice::SHADER_STAGE_COMPUTE);
    if (!error.is_empty()) { const std::lock_guard lock(mutex_); error_=error; return false; }
    shader=rd_->shader_create_from_spirv(spirv);
    if (shader.is_valid()) pipeline=rd_->compute_pipeline_create(shader);
    return pipeline.is_valid();
}
RID RA2MapRenderer::gpu_buffer(const game::MapBufferData& data) {
    auto found=buffers_.find(data.id);
    if (found!=buffers_.end()) return found->second;
    auto rid=rd_->storage_buffer_create(static_cast<std::uint32_t>(data.bytes.size()),data.bytes);
    if (rid.is_valid()) buffers_.emplace(data.id,rid);
    return rid;
}
RID RA2MapRenderer::draw_uniform(const game::MapBufferData& source,const game::MapBufferData& palette) {
    const auto key=std::make_pair(source.id,palette.id);
    if (auto found=uniforms_.find(key);found!=uniforms_.end()) return found->second;
    const auto src=gpu_buffer(source),pal=gpu_buffer(palette);
    if (!src.is_valid() || !pal.is_valid()) return {};
    TypedArray<RDUniform> values;
    const RID buffers[]{parameters_,src,colors_,depths_,lights_,pal};
    for (int i=0;i<6;++i) values.append(uniform(i,RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER,buffers[i]));
    const auto rid=rd_->uniform_set_create(values,shader_,0);
    if (rid.is_valid()) uniforms_.emplace(key,rid);
    return rid;
}
RID RA2MapRenderer::lighting_uniform(const game::MapBufferData& source) {
    // Buffer identities start at 1; palette id 0 reserves the lighting layout.
    const auto key=std::make_pair(source.id,std::uint64_t(0));
    if(auto it=uniforms_.find(key);it!=uniforms_.end())return it->second;
    const auto src=gpu_buffer(source);if(!src.is_valid())return {};
    TypedArray<RDUniform> values;
    const RID buffers[]{parameters_,src,lights_};
    for(int i=0;i<3;++i)values.append(uniform(i,RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER,buffers[i]));
    const auto rid=rd_->uniform_set_create(values,lighting_shader_,0);
    if(rid.is_valid())uniforms_.emplace(key,rid);return rid;
}
bool RA2MapRenderer::render(const game::MapFrameData& frame) {
    const auto profile_start=std::chrono::steady_clock::now();
    if (!rd_) rd_=RenderingServer::get_singleton()->get_rendering_device();
    if (!rd_) { const std::lock_guard lock(mutex_); error_="RenderingDevice unavailable; select Forward+ or Mobile"; return false; }
    if (!pipeline_.is_valid() && !create_pipeline("res://shaders/map_draw.glsl",shader_,pipeline_)) return false;
    if (!present_pipeline_.is_valid() && !create_pipeline("res://shaders/map_present.glsl",present_shader_,present_pipeline_)) return false;
    if (!lighting_pipeline_.is_valid() && !create_pipeline("res://shaders/map_abuffer.glsl",lighting_shader_,lighting_pipeline_)) return false;
    if (!lighting_batch_pipeline_.is_valid() && !create_pipeline("res://shaders/map_abuffer_batch.glsl",lighting_batch_shader_,lighting_batch_pipeline_)) return false;
    if (generation_!=frame.generation || width_!=frame.width || height_!=frame.height) {
        free_states(); generation_=frame.generation; width_=frame.width; height_=frame.height;
        const auto size=std::uint32_t(width_)*height_*4;
        colors_=rd_->storage_buffer_create(size); depths_=rd_->storage_buffer_create(size); lights_=rd_->storage_buffer_create(size);
        Ref<RDTextureFormat> format; format.instantiate(); format->set_width(width_); format->set_height(height_);
        format->set_format(RenderingDevice::DATA_FORMAT_R8G8B8A8_UNORM);
        format->set_usage_bits(RenderingDevice::TEXTURE_USAGE_STORAGE_BIT|RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT|RenderingDevice::TEXTURE_USAGE_CAN_COPY_FROM_BIT);
        Ref<RDTextureView> view; view.instantiate(); target_=rd_->texture_create(format,view);
        if (!colors_.is_valid() || !depths_.is_valid() || !lights_.is_valid() || !target_.is_valid()) return false;
        TypedArray<RDUniform> u;
        u.append(uniform(0,RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER,colors_));
        u.append(uniform(1,RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER,depths_));
        u.append(uniform(2,RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER,lights_));
        u.append(uniform(3,RenderingDevice::UNIFORM_TYPE_IMAGE,target_));
        present_uniform_=rd_->uniform_set_create(u,present_shader_,0);
        if (!present_uniform_.is_valid()) return false;
    }
    // Raster frames own transient immutable uploads. Retire identities not
    // referenced by this frame; RenderingDevice defers their GPU destruction.
    // Uniforms must be freed before buffers to keep the cache free of dead RIDs.
    std::set<std::uint64_t> live;
    for (const auto& packet : frame.packets) if (!packet.cached_lighting) {
        live.insert(packet.source->id); live.insert(packet.palette->id);
    }
    for (auto it=uniforms_.begin();it!=uniforms_.end();) {
        if (!live.contains(it->first.first) || (it->first.second&&!live.contains(it->first.second))) {
            if (rd_->uniform_set_is_valid(it->second)) rd_->free_rid(it->second);
            it=uniforms_.erase(it);
        } else ++it;
    }
    for (auto it=buffers_.begin();it!=buffers_.end();) {
        if (!live.contains(it->first)) { rd_->free_rid(it->second); it=buffers_.erase(it); }
        else ++it;
    }
    const auto needed=std::max<std::size_t>(frame.packets.size()*80,80);
    if (parameter_capacity_<needed) {
        free_uniforms();
        if (parameters_.is_valid()) rd_->free_rid(parameters_);
        parameter_capacity_=std::max(needed,parameter_capacity_*2);
        parameters_=rd_->storage_buffer_create(static_cast<std::uint32_t>(parameter_capacity_));
        if (!parameters_.is_valid()) return false;
    }
    struct LightingBatch { std::size_t first,end; int bin_offset; };
    std::vector<LightingBatch> batches;
    std::vector<game::LightingParameters> words(frame.packets.size());
    std::vector<std::int32_t> bins;
    std::size_t lighting_packets=0;
    for (std::size_t i=0;i<frame.packets.size();++i) {
        const auto& packet=frame.packets[i];
        words[i]=packet.parameters;
        if (!packet.cached_lighting) continue;
        ++lighting_packets;
        const auto& source=*packet.source;
        auto found=lighting_offsets_.find(source.id);
        if (found==lighting_offsets_.end()) {
            const auto offset=lighting_atlas_bytes_.size();
            if (offset+source.bytes.size()>INT32_MAX || source.bytes.size()%4)
                throw std::length_error("lighting atlas");
            lighting_atlas_bytes_.append_array(source.bytes);
            found=lighting_offsets_.emplace(source.id,std::int32_t(offset/4)).first;
        }
        words[i][17]=found->second;
    }
    // Only consecutive lighting producers share a dispatch. A color/Z consumer
    // or a transient raster request is a boundary, preserving original order.
    for (std::size_t i=0;i<frame.packets.size();) {
        if (!frame.packets[i].cached_lighting) { ++i; continue; }
        const auto first=i;
        while (i<frame.packets.size() && frame.packets[i].cached_lighting) ++i;
        const auto offset=game::append_lighting_bins(words.data(),int(first),int(i-first),width_,height_,bins);
        batches.push_back({first,i,offset});
    }
    std::size_t atlas_upload_bytes=0;
    if (!batches.empty()) {
        const auto grow=[&](RID& rid,std::size_t& capacity,std::size_t size) {
            if (capacity>=size && rid.is_valid()) return false;
            free_uniforms();
            if (rid.is_valid()) rd_->free_rid(rid);
            capacity=std::max(size,capacity*2);
            rid=rd_->storage_buffer_create(static_cast<std::uint32_t>(capacity));
            return true;
        };
        const bool atlas_grew=grow(lighting_atlas_,lighting_atlas_capacity_,lighting_atlas_bytes_.size());
        if (atlas_grew) lighting_atlas_uploaded_=0;
        grow(lighting_bins_,lighting_bins_capacity_,bins.size()*sizeof(std::int32_t));
        if (!lighting_atlas_.is_valid() || !lighting_bins_.is_valid()) return false;
        const auto upload_start=lighting_atlas_uploaded_;
        atlas_upload_bytes=lighting_atlas_bytes_.size()-upload_start;
        if (atlas_upload_bytes && rd_->buffer_update(lighting_atlas_,upload_start,atlas_upload_bytes,
            bytes(lighting_atlas_bytes_.ptr()+upload_start,atlas_upload_bytes))!=OK) return false;
        lighting_atlas_uploaded_=lighting_atlas_bytes_.size();
        if (rd_->buffer_update(lighting_bins_,0,bins.size()*sizeof(std::int32_t),
            bytes(bins.data(),bins.size()*sizeof(std::int32_t)))!=OK) return false;
        if (!lighting_batch_uniform_.is_valid()) {
            TypedArray<RDUniform> values;
            const RID buffers[]{parameters_,lighting_atlas_,lights_,lighting_bins_};
            for (int i=0;i<4;++i) values.append(uniform(i,RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER,buffers[i]));
            lighting_batch_uniform_=rd_->uniform_set_create(values,lighting_batch_shader_,0);
            if (!lighting_batch_uniform_.is_valid()) return false;
        }
    }
    PackedByteArray params; params.resize(needed);
    std::vector<RID> sets(frame.packets.size());
    for (std::size_t i=0;i<frame.packets.size();++i) {
        const auto& packet=frame.packets[i];
        std::memcpy(params.ptrw()+i*80,words[i].data(),80);
        if (packet.cached_lighting) continue;
        sets[i]=packet.lighting?lighting_uniform(*packet.source):draw_uniform(*packet.source,*packet.palette);
        if (!sets[i].is_valid()) return false;
    }
    if (rd_->buffer_update(parameters_,0,static_cast<std::uint32_t>(params.size()),params)!=OK) return false;
    const auto profile_prepared=std::chrono::steady_clock::now();
    const auto commands=rd_->compute_list_begin();
    if (commands<0) return false;
    const auto present=[&](int clear) {
        const std::int32_t push[]{width_,height_,clear,0};
        rd_->compute_list_bind_compute_pipeline(commands,present_pipeline_);
        rd_->compute_list_bind_uniform_set(commands,present_uniform_,0);
        rd_->compute_list_set_push_constant(commands,bytes(push,sizeof(push)),sizeof(push));
        rd_->compute_list_dispatch(commands,(width_+7)/8,(height_+7)/8,1);
    };
    present(1); rd_->compute_list_add_barrier(commands);
    rd_->compute_list_bind_compute_pipeline(commands,pipeline_);
    std::size_t next_batch=0;
    for (std::size_t i=0;i<frame.packets.size();++i) {
        if (next_batch<batches.size() && batches[next_batch].first==i) {
            const auto& batch=batches[next_batch++];
            const std::int32_t push[]{width_,height_,(width_+game::LightingBinSize-1)/game::LightingBinSize,batch.bin_offset};
            rd_->compute_list_bind_compute_pipeline(commands,lighting_batch_pipeline_);
            rd_->compute_list_bind_uniform_set(commands,lighting_batch_uniform_,0);
            rd_->compute_list_set_push_constant(commands,bytes(push,sizeof(push)),sizeof(push));
            rd_->compute_list_dispatch(commands,(width_+7)/8,(height_+7)/8,1);
            rd_->compute_list_add_barrier(commands); // Completed ABuffer before its next consumer.
            i=batch.end-1;
            continue;
        }
        const auto& p=frame.packets[i].parameters; const auto index=static_cast<std::int32_t>(i);
        rd_->compute_list_bind_compute_pipeline(commands,frame.packets[i].lighting?lighting_pipeline_:pipeline_);
        rd_->compute_list_bind_uniform_set(commands,sets[i],0);
        rd_->compute_list_set_push_constant(commands,bytes(&index,sizeof(index)),sizeof(index));
        rd_->compute_list_dispatch(commands,(p[2]+7)/8,(p[3]+7)/8,1);
        rd_->compute_list_add_barrier(commands); // Preserve original base/extra and tile ordering.
    }
    present(0); rd_->compute_list_end();
    if (std::getenv("RA2_MAP_PROFILE")) std::fprintf(stderr,"MAP_PROFILE packets=%zu lighting_packets=%zu lighting_batches=%zu atlas_upload_bytes=%zu prepare_us=%lld dispatch_us=%lld\n",
        frame.packets.size(),lighting_packets,batches.size(),atlas_upload_bytes,
        static_cast<long long>(std::chrono::duration_cast<std::chrono::microseconds>(profile_prepared-profile_start).count()),
        static_cast<long long>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-profile_prepared).count()));
    // Global RenderingDevice submission belongs to Godot. No submit/sync or readback here.
    return true;
}
