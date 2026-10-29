#include "bridge/godot_render_host.hpp"
#include "queued_frame.hpp"
#include "godot_device.hpp"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
using namespace godot;
void GodotRenderHost::submit(std::shared_ptr<game::QueuedFrame> frame) {
    bool schedule = false;
    const bool resized = requested_width_ != frame->input.width || requested_height_ != frame->input.height;
    if (resized && texture_.is_valid()) texture_->set_texture_rd_rid({});
    requested_width_ = frame->input.width;
    requested_height_ = frame->input.height;
    {
        const std::lock_guard lock(mutex_);
        if (closed_ || !error_.is_empty()) return;
        if (resized) published_texture_ = {};
        ++requested_sequence_;
        frame->diagnostic.enqueue = 1;
        frame->diagnostic.render_sequence = requested_sequence_;
        // A capture pass is a one-shot core command and must survive frame coalescing.
        if (pending_ && pending_->input.capture)
            pending_captures_.push_back(std::move(pending_));
        if (pending_) retired_frames_.push_back(std::move(pending_));
        pending_ = std::move(frame);
        if (!scheduled_) {
            scheduled_ = true;
            schedule = true;
        }
    }
    // Binding a strong self reference keeps queued work alive after node exit.
    if (schedule)
        RenderingServer::get_singleton()->call_on_render_thread(
            callable_mp(this, &GodotRenderHost::render_on_thread).bind(Ref<RefCounted>(this)));
}
Ref<Texture2DRD> GodotRenderHost::texture() {
    RID rid;
    {
        const std::lock_guard lock(mutex_);
        rid = published_texture_;
    }
    if (!rid.is_valid()) return {};
    if (texture_.is_null()) texture_.instantiate();
    if (texture_->get_texture_rd_rid() != rid) texture_->set_texture_rd_rid(rid);
    return texture_;
}
String GodotRenderHost::error() const {
    const std::lock_guard lock(mutex_);
    return error_;
}
bool GodotRenderHost::is_current() const {
    const std::lock_guard lock(mutex_);
    return !closed_ && error_.is_empty() && requested_sequence_ && requested_sequence_ == completed_sequence_;
}
bool GodotRenderHost::performance(std::uint64_t& generation, std::uint64_t& sequence, double& cpu_ms) const noexcept {
    try {
        const std::lock_guard lock(mutex_);
        if (!performance_sequence_ || closed_) return false;
        generation = performance_generation_;
        sequence = performance_sequence_;
        cpu_ms = performance_cpu_ms_;
        return true;
    } catch (...) {
        return false;
    }
}
void GodotRenderHost::shutdown() {
    if (texture_.is_valid()) {
        texture_->set_texture_rd_rid({});
        texture_.unref();
    }
    {
        const std::lock_guard lock(mutex_);
        if (closed_) return;
        closed_ = true;
        if (pending_) retired_frames_.push_back(std::move(pending_));
        for (auto& capture : pending_captures_) retired_frames_.push_back(std::move(capture));
        pending_captures_.clear();
        published_texture_ = {};
    }
    RenderingServer::get_singleton()->call_on_render_thread(
        callable_mp(this, &GodotRenderHost::clear_on_thread).bind(Ref<RefCounted>(this)));
}
void GodotRenderHost::render_on_thread(const Ref<RefCounted>&) {
    std::shared_ptr<game::QueuedFrame> frame;
    std::uint64_t sequence = 0;
    std::vector<std::shared_ptr<game::QueuedFrame>> captures, retired;
    {
        const std::lock_guard lock(mutex_);
        scheduled_ = false;
        retired.swap(retired_frames_);
        if (closed_ || !error_.is_empty()) {
            pending_.reset();
            pending_captures_.clear();
            return;
        }
        frame = std::move(pending_);
        captures.swap(pending_captures_);
        sequence = requested_sequence_;
    }
    if (!frame) return;
    game::begin_drawing_diagnostics(frame->diagnostic.frame);
    const game::QueuedFrame* failed_frame = frame.get();
    try {
        const auto started = std::chrono::steady_clock::now();
        bool ok = true;
        for (const auto& capture : captures) {
            failed_frame = capture.get();
            if (!render(*capture)) {
                ok = false;
                break;
            }
        }
        if (ok) {
            failed_frame = frame.get();
            ok = render(*frame);
        }
        const double cpu_ms =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
        const std::lock_guard lock(mutex_);
        if (!closed_ && ok) {
            performance_generation_ = frame->input.generation;
            performance_sequence_ = sequence;
            performance_cpu_ms_ = cpu_ms;
        }
        if (!closed_ && ok && sequence == requested_sequence_) {
            published_texture_ = device_->rid(output_);
            completed_sequence_ = sequence;
        } else if (!closed_ && !ok && error_.is_empty())
            error_ = "Map GPU resource or dispatch creation failed";
    } catch (const std::exception& e) {
        const std::lock_guard lock(mutex_);
        error_ = String::utf8(e.what());
    } catch (...) {
        const std::lock_guard lock(mutex_);
        error_ = "Map GPU submission failed";
    }
    try {
        const auto message = error();
        if (!message.is_empty()) {
            const auto utf8 = message.utf8();
            game::record_drawing_diagnostic(game::DrawingStatus::backend_failure, failure_stage_,
                                            "device_operation_failed", utf8.get_data(), __FILE__, __LINE__);
            game::DrawingFailureInfo failure;
            game::get_drawing_failure(failure);
            failure.frame = failed_frame->diagnostic.frame;
            auto context = failed_frame->diagnostic;
            std::snprintf(context.stage, sizeof(context.stage), "%s", failure_stage_);
            std::snprintf(context.bridge_error, sizeof(context.bridge_error), "%s", utf8.get_data());
            context.gpu_prepare = std::strcmp(failure_stage_, "gpu_prepare") == 0 ? 2 : 1;
            context.gpu_submit = context.gpu_prepare == 2 ? 0 : 2;
            context.commands = static_cast<std::uint32_t>(failed_frame->input.command_count);
            context.resources = static_cast<std::uint32_t>(failed_frame->input.resource_count);
            char path[4096];
            game::write_render_failure(context, failure, path, sizeof(path));
            const std::lock_guard lock(mutex_);
            error_ += String("\nfailure=") + String::num_uint64(failure.id) + " stage=" + failure_stage_ +
                      "\ndiagnostics=" + (path[0] ? String::utf8(path) : String("snapshot unavailable"));
        }
    } catch (...) {
        // Keep the original device failure if optional diagnostics cannot allocate.
    }
}
GodotRenderHost::GodotRenderHost() = default;
GodotRenderHost::~GodotRenderHost() = default;
bool GodotRenderHost::render(const game::QueuedFrame& frame) {
    if (!device_) device_ = std::make_unique<game::GodotDevice>();
    if (!renderer_ && render::create(device_->interface(), renderer_) != render::Status::ok) {
        const std::lock_guard lock(mutex_);
        error_ = "Rendering device unavailable";
        return false;
    }
    if (render::submit(*renderer_, frame.input, output_) == render::Status::ok) return true;
    failure_stage_ = render::failure_stage(*renderer_);
    const std::lock_guard lock(mutex_);
    error_ = String::utf8(render::error(*renderer_));
    return false;
}
void GodotRenderHost::clear_on_thread(const Ref<RefCounted>&) {
    std::vector<std::shared_ptr<game::QueuedFrame>> retired;
    {
        const std::lock_guard lock(mutex_);
        retired.swap(retired_frames_);
    }
    render::destroy(renderer_);
    renderer_ = nullptr;
    device_.reset();
}
