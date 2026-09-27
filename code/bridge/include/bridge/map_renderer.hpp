#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/texture2drd.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <map>
#include <memory>
#include <mutex>
namespace game { struct MapFrameData; struct MapBufferData; }
class RA2MapRenderer : public godot::RefCounted {
    GDCLASS(RA2MapRenderer,godot::RefCounted)
protected:
    static void _bind_methods() {}
public:
    void submit(std::shared_ptr<game::MapFrameData>);
    godot::Ref<godot::Texture2DRD> texture();
    godot::String error() const;
    bool is_current() const;
    void shutdown();
private:
    void render_on_thread(const godot::Ref<godot::RefCounted>& keep_alive);
    void clear_on_thread(const godot::Ref<godot::RefCounted>& keep_alive);
    bool render(const game::MapFrameData&);
    bool create_pipeline(const char*,godot::RID&,godot::RID&);
    void free_states();
    void free_uniforms();
    godot::RID gpu_buffer(const game::MapBufferData&);
    godot::RID draw_uniform(const game::MapBufferData&,const game::MapBufferData&);
    godot::RID lighting_uniform(const game::MapBufferData&);
    mutable std::mutex mutex_;
    std::shared_ptr<game::MapFrameData> pending_;
    bool scheduled_=false,closed_=false;
    std::uint64_t requested_sequence_=0;
    std::uint64_t completed_sequence_=0;
    godot::String error_;
    godot::RID published_texture_;
    godot::Ref<godot::Texture2DRD> texture_;
    int requested_width_=0,requested_height_=0;
    // Only the RenderingServer thread touches device state below.
    godot::RenderingDevice* rd_=nullptr;
    godot::RID shader_,pipeline_,present_shader_,present_pipeline_,lighting_shader_,lighting_pipeline_;
    godot::RID colors_,depths_,lights_,target_,parameters_,present_uniform_;
    godot::RID lighting_batch_shader_,lighting_batch_pipeline_,lighting_batch_uniform_;
    godot::RID lighting_atlas_,lighting_bins_;
    godot::PackedByteArray lighting_atlas_bytes_;
    std::map<std::uint64_t,std::int32_t> lighting_offsets_;
    std::size_t lighting_atlas_capacity_=0,lighting_bins_capacity_=0;
    std::size_t lighting_atlas_uploaded_=0;
    std::map<std::uint64_t,godot::RID> buffers_;
    std::map<std::pair<std::uint64_t,std::uint64_t>,godot::RID> uniforms_;
    std::uint64_t generation_=0;
    std::size_t parameter_capacity_=0;
    int width_=0,height_=0;
};
