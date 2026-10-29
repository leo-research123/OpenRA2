#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/texture2drd.hpp>
#include <godot_cpp/variant/string.hpp>
#include <memory>
#include <mutex>
#include <vector>
#include "render/renderer.hpp"
namespace game { struct QueuedFrame; class GodotDevice; }
// Node glue owns frame queuing until the runtime module takes it over.
class GodotRenderHost : public godot::RefCounted {
    GDCLASS(GodotRenderHost, godot::RefCounted)
protected:
    static void _bind_methods() {}
public:
    GodotRenderHost();
    ~GodotRenderHost() override;
    void submit(std::shared_ptr<game::QueuedFrame>);
    godot::Ref<godot::Texture2DRD> texture();
    godot::String error() const;
    bool is_current() const;
    bool performance(std::uint64_t&, std::uint64_t&, double&) const noexcept;
    void shutdown();
private:
    void render_on_thread(const godot::Ref<godot::RefCounted>&);
    void clear_on_thread(const godot::Ref<godot::RefCounted>&);
    bool render(const game::QueuedFrame&);
    const char* failure_stage_ = "gpu_prepare";
    std::unique_ptr<game::GodotDevice> device_;
    render::RendererHandle* renderer_ = nullptr;
    render::Handle output_{};
    mutable std::mutex mutex_;
    std::shared_ptr<game::QueuedFrame> pending_;
    std::vector<std::shared_ptr<game::QueuedFrame>> pending_captures_;
    std::vector<std::shared_ptr<game::QueuedFrame>> retired_frames_;
    bool scheduled_ = false, closed_ = false;
    std::uint64_t requested_sequence_ = 0, completed_sequence_ = 0;
    std::uint64_t performance_generation_ = 0, performance_sequence_ = 0;
    double performance_cpu_ms_ = 0;
    godot::String error_;
    godot::RID published_texture_;
    godot::Ref<godot::Texture2DRD> texture_;
    int requested_width_ = 0, requested_height_ = 0;
};
