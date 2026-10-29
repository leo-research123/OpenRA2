#include "render/renderer.hpp"
#include "renderer_impl.hpp"
#include "frame_plan.hpp"
#include <cstdio>
#include <exception>
namespace render {
struct RendererHandle {
    explicit RendererHandle(const Device& device) : renderer(device) {}
    RendererImpl renderer;
    char failure[1024]{};
    MapFrameData plan;
};
Status create(const Device& d, RendererHandle*& output) noexcept {
    if (output || d.version!=1 || d.struct_size!=sizeof(Device) || !d.create_buffer || !d.update_buffer ||
        !d.copy_buffer || !d.clear_buffer || !d.create_texture || !d.supports_texture || !d.texture_slice ||
        !d.copy_texture || !d.read_texture || !d.max_texture_layers || !d.create_framebuffer ||
        !d.framebuffer_format || !d.create_shader || !d.create_pipeline || !d.create_compute_pipeline ||
        !d.create_sampler || !d.create_bindings || !d.bindings_valid || !d.texture_valid || !d.framebuffer_valid || !d.begin_compute || !d.begin_draw ||
        !d.record || !d.release || !d.timestamp_frame || !d.timestamp_count || !d.timestamp)
        return Status::invalid_argument;
    try { output=new RendererHandle(d); return Status::ok; } catch (...) { return Status::backend_failure; }
}
Status submit(RendererHandle& handle, const Frame& frame, Handle& texture) noexcept {
    texture={};
    const Status imported = FrameCompiler::import(frame, handle.plan);
    if (imported != Status::ok) return imported;
    const auto& data = handle.plan;
    try {
        if (handle.failure[0]) return Status::backend_failure;
        if (!handle.renderer.render(data)) {
            std::snprintf(handle.failure,sizeof(handle.failure),"%s",handle.renderer.error());
            if (!handle.failure[0]) std::snprintf(handle.failure,sizeof(handle.failure),"Render submission failed");
            return Status::backend_failure;
        }
        handle.renderer.verify(data);
        texture=handle.renderer.output();
        return Status::ok;
    } catch (const std::exception& e) {
        std::snprintf(handle.failure,sizeof(handle.failure),"%s",e.what());
    } catch (...) { std::snprintf(handle.failure,sizeof(handle.failure),"Render submission failed"); }
    return Status::backend_failure;
}
void destroy(RendererHandle* handle) noexcept {
    if (!handle) return;
    try { handle->renderer.clear(); } catch (...) {}
    delete handle;
}
const char* error(const RendererHandle& handle) noexcept { return handle.failure; }
const char* failure_stage(const RendererHandle& handle) noexcept { return handle.renderer.failure_stage(); }
}
