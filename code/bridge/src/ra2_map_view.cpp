#include "api/runtime_diagnostics.hpp"
#include "frame_diagnostics.hpp"
#include <cstdio>
#include "bridge/ra2_map_view.hpp"
#include "queued_frame.hpp"
#include "map_input.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <chrono>
#include <array>
#include <exception>
#include <new>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
using namespace godot;
void RA2MapView::_bind_methods() {
    ClassDB::bind_method(D_METHOD("configure", "core"), &RA2MapView::configure);
    ClassDB::bind_method(D_METHOD("open_map", "filename"), &RA2MapView::open_map);
    ClassDB::bind_method(D_METHOD("open_campaign", "filename", "difficulty"), &RA2MapView::open_campaign);
    ClassDB::bind_method(D_METHOD("open_skirmish", "filename", "options"), &RA2MapView::open_skirmish);
    ClassDB::bind_method(D_METHOD("close_map"), &RA2MapView::close_map);
    ClassDB::bind_method(D_METHOD("continue_mission", "id"), &RA2MapView::continue_mission);
    ClassDB::bind_method(D_METHOD("get_render_status"), &RA2MapView::get_render_status);
    ClassDB::bind_method(D_METHOD("handle_map_input", "event", "surface_size"), &RA2MapView::handle_map_input);
    ClassDB::bind_method(D_METHOD("reset_map_input"), &RA2MapView::reset_map_input);
}
RA2MapView::RA2MapView() {
    game::create_presentation(resources_);
    if (!resources_) error_ = "Map resource/input allocation failed";
    set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
    // The frame holds expanded RGB565 and 8-bit blend results; the original
    // surface was RGB565, so the display truncates every channel to 5/6/5 bits
    // and expands it as the renderer expands palette colors.
    if (auto* server = RenderingServer::get_singleton()) {
        display_shader_ = server->shader_create();
        server->shader_set_code(display_shader_,
                                "shader_type canvas_item;\n"
                                "void fragment() {\n"
                                "    vec4 color = texture(TEXTURE, UV);\n"
                                "    uvec3 bits = uvec3(round(color.rgb * 255.0)) >> uvec3(3u, 2u, 3u);\n"
                                "    uvec3 maximum = uvec3(31u, 63u, 31u);\n"
                                "    COLOR = vec4(vec3((bits * 255u + maximum / 2u) / maximum) / 255.0, color.a);\n"
                                "}\n");
        display_material_ = server->material_create();
        server->material_set_shader(display_material_, display_shader_);
        server->canvas_item_set_material(get_canvas_item(), display_material_);
    }
    set_process(true);
}
RA2MapView::~RA2MapView() {
    close_map();
    game::destroy_presentation(resources_);
    if (auto* server = RenderingServer::get_singleton())
        for (RID* rid : {&display_material_, &display_shader_})
            if (rid->is_valid()) server->free_rid(*rid);
}
void RA2MapView::_notification(int what) {
    if (what == NOTIFICATION_EXIT_TREE) close_map();
    if (what == NOTIFICATION_WM_WINDOW_FOCUS_IN) focused_ = true;
    if (what == NOTIFICATION_WM_WINDOW_FOCUS_OUT) focused_ = false;
    if (core_.is_valid() && (what == NOTIFICATION_PAUSED || what == NOTIFICATION_UNPAUSED ||
                             what == NOTIFICATION_WM_WINDOW_FOCUS_IN || what == NOTIFICATION_WM_WINDOW_FOCUS_OUT))
        core_->set_activity(focused_, !can_process());
    if (what == NOTIFICATION_PAUSED || what == NOTIFICATION_WM_WINDOW_FOCUS_OUT || what == NOTIFICATION_WM_MOUSE_EXIT)
        reset_map_input();
}
Dictionary RA2MapView::handle_map_input(const Ref<InputEvent>& event, const Vector2& size) {
    Dictionary device;
    if (core_.is_null() || !focused_ || !can_process() || !error_.is_empty() ||
        (renderer_.is_valid() && !renderer_->error().is_empty()))
        return device;
    game::GameInputEvent input;
    game::GameInputResult result;
    const auto canvas = get_viewport_rect().size;
    if (game::encode_canvas_input(event, size, canvas, input, control_click_) && core_->submit_input(input, result)) {
        device["consumed"] = result.consumed;
        if (result.warp_pointer) device["warp_pointer"] = Vector2(result.pointer_x, result.pointer_y) * size / canvas;
        update_cursor();
    }
    return device;
}
void RA2MapView::reset_map_input() {
    control_click_ = false;
    if (core_.is_valid()) {
        game::GameInputEvent event;
        event.kind = game::GameInputKind::pointer_leave;
        game::GameInputResult result;
        core_->submit_input(event, result);
    }
    cursor_.visible = false;
    if (owns_mouse_visibility_) {
        if (Input::get_singleton()->get_mouse_mode() == Input::MOUSE_MODE_HIDDEN)
            Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
        owns_mouse_visibility_ = false;
    }
    queue_redraw();
}
void RA2MapView::configure(const Ref<RA2Core>& core) {
    close_map();
    core_ = core;
}
void RA2MapView::open_map(const String& filename) {
    if (core_.is_null()) {
        error_ = "Map core is not configured";
        return;
    }
    close_map();
    renderer_.instantiate();
    core_->begin_map_loading(filename);
}
void RA2MapView::open_campaign(const String& filename, int difficulty) {
    if (core_.is_null()) {
        error_ = "Map core is not configured";
        return;
    }
    close_map();
    renderer_.instantiate();
    core_->begin_campaign_loading(filename, difficulty);
}
void RA2MapView::open_skirmish(const String& filename, const Dictionary& options) {
    if (core_.is_null()) {
        error_ = "Map core is not configured";
        return;
    }
    close_map();
    renderer_.instantiate();
    core_->begin_skirmish_loading(filename, options);
}
void RA2MapView::close_map() {
    clear_presentation();
    if (core_.is_valid()) core_->close_map();
}
bool RA2MapView::continue_mission(std::int64_t id) {
    if (core_.is_null()) return false;
    const auto status = core_->get_map_status();
    const int transition = status.get("mission_transition", 0);
    if (std::int64_t(status.get("mission_id", 0)) != id || (transition != 1 && transition != 2 && transition != 6))
        return false;
    // Drain queued rendering before the loader can retire any world resources.
    clear_presentation();
    renderer_.instantiate();
    return core_->begin_mission_transition(id);
}
void RA2MapView::clear_presentation() {
    reset_map_input();
    cursor_texture_.unref();
    cursor_ = {};
    texture_.unref();
    texture_rid_ = {};
    if (renderer_.is_valid()) {
        renderer_->shutdown();
        renderer_.unref();
    }
    if (resources_) game::clear_presentation(*resources_);
    last_clock_usec_ = 0;
    width_ = height_ = 0;
    revision_ = camera_revision_ = ui_revision_ = presentation_revision_ = 0;
    drawn_ = 0;
    packets_ = 0;
    error_ = resources_ ? "" : "Map resource/input allocation failed";
    queue_redraw();
}
Dictionary RA2MapView::get_render_status() const {
    auto result = core_.is_valid() ? core_->get_map_status() : Dictionary();
    if (!result.has("state")) result["state"] = "empty";
    auto error = error_;
    if (error.is_empty() && renderer_.is_valid()) error = renderer_->error();
    result["failure_stage"] = "loading";
    if (!error.is_empty()) {
        result["state"] = "failed";
        result["error"] = error;
        result["failure_stage"] = "rendering";
    } else if (String(result["state"]) == "ready")
        result["state"] =
            texture_.is_valid() && renderer_.is_valid() && renderer_->is_current() ? "drawn" : "rendering";
    result["drawn_cells"] = drawn_;
    result["packets"] = static_cast<std::int64_t>(packets_);
    result["viewport_width"] = width_;
    result["viewport_height"] = height_;
    result["map_input_enabled"] = focused_ && can_process() && error.is_empty();
    result["cursor_visible"] = cursor_.visible;
    result["cursor_index"] = cursor_.index;
    result["cursor_frame"] = cursor_.frame;
    game::PresentationStatistics presentation_stats{};
    if (resources_) game::presentation_statistics(*resources_, presentation_stats);
    result["decoded_tiles"] = static_cast<std::int64_t>(presentation_stats.decoded_tiles);
    result["decoded_lighting"] = static_cast<std::int64_t>(presentation_stats.decoded_lighting);
    result["palettes"] = static_cast<std::int64_t>(presentation_stats.prepared_palettes);
    return result;
}
void RA2MapView::_process(double) {
    if (core_.is_null() || renderer_.is_null() || !error_.is_empty()) return;
    error_ = renderer_->error();
    if (!error_.is_empty()) {
        reset_map_input();
        texture_.unref();
        texture_rid_ = {};
        renderer_->shutdown();
        queue_redraw();
        return;
    }
    game::begin_drawing_diagnostics();
    game::RenderFailureContext exception_context;
    const char* phase = "bridge_frame";
    char exception_snapshot[4096]{};
    const auto record_exception = [&](const char* message) noexcept {
        game::record_drawing_diagnostic(game::DrawingStatus::backend_failure, phase, "frame_preparation_exception",
                                         message, __FILE__, __LINE__);
        game::DrawingFailureInfo failure;
        game::get_drawing_failure(failure);
        std::snprintf(exception_context.stage, sizeof(exception_context.stage), "%s", phase);
        std::snprintf(exception_context.bridge_error, sizeof(exception_context.bridge_error), "%s", message);
        game::write_render_failure(exception_context, failure, exception_snapshot, sizeof(exception_snapshot));
    };
    try {
        auto status = core_->get_map_status();
        if (String(status.get("state", "empty")) != "ready") return;
        // Device wall clock, unaffected by Engine.time_scale or delta clamping.
        const auto now = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count());
        const double seconds = last_clock_usec_ ? double(now - last_clock_usec_) / 1000000.0 : 0.0;
        last_clock_usec_ = now;
        core_->set_activity(focused_, !can_process());
        game::GameInputEvent focus;
        game::GameInputResult ignored;
        focus.kind = focused_ ? game::GameInputKind::focus_gained : game::GameInputKind::focus_lost;
        core_->submit_input(focus, ignored);
        const auto presentation_revision =
            static_cast<std::uint64_t>(std::int64_t(status.get("presentation_revision", 0)));
        const auto ui_revision = static_cast<std::uint64_t>(std::int64_t(status.get("ui_revision", 0)));
        const auto revision = static_cast<std::uint64_t>(std::int64_t(status.get("revision", 0)));
        const auto camera_revision = static_cast<std::uint64_t>(std::int64_t(status.get("camera_revision", 0)));
        const auto size = get_viewport_rect().size;
        const int width = static_cast<int>(size.x), height = static_cast<int>(size.y);
        if (width < 640 || height < 480) return;
        {
            if (revision_ != revision || width_ != width || height_ != height) {
                texture_.unref();
                texture_rid_ = {};
                queue_redraw();
            }
            if (revision_ != revision) game::clear_presentation(*resources_);
            exception_context.generation = revision;
            exception_context.world = core_->get_debug_command_world();
            exception_context.frame = std::int64_t(status.get("current_frame", 0));
            exception_context.tick = std::int64_t(status.get("simulation_tick", 0));
            exception_context.width = width;
            exception_context.height = height;
            exception_context.camera_x = std::int64_t(status.get("camera_x", 0));
            exception_context.camera_y = std::int64_t(status.get("camera_y", 0));
            game::capture_render_failure_context(exception_context);
            auto frame = std::make_shared<game::QueuedFrame>();
            frame->diagnostic = exception_context;
            if (!game::begin_presentation_frame(*resources_, revision, width, height, frame->owner))
                throw std::runtime_error("Frame preparation allocation failed");
            bool rendered = false;
            const auto profile_start = std::chrono::steady_clock::now();
            phase = "core_draw";
            const auto result = core_->advance_map(seconds, *frame->owner, frame->stats, rendered);
            if (rendered && std::getenv("RA2_MAP_PROFILE"))
                std::fprintf(stderr, "MAP_PROFILE core_us=%lld\n",
                             static_cast<long long>(std::chrono::duration_cast<std::chrono::microseconds>(
                                                        std::chrono::steady_clock::now() - profile_start)
                                                        .count()));
            const bool core_ok = result == game::DrawingStatus::drawn || result == game::DrawingStatus::skipped;
            game::record_core_frame_result(result);
            exception_context.core_status = result;
            exception_context.core = core_ok ? 1 : 2;
            exception_context.frame = game::drawing_diagnostic_frame();
            phase = "bridge_end_frame";
            const bool prepared = game::end_presentation_frame(*resources_, *frame->owner, frame->input);
            exception_context.end_frame = prepared ? 1 : 2;
            game::DrawingFailureInfo failure;
            if (!game::finish_frame_diagnostics(result, prepared, game::presentation_error(*resources_), frame->diagnostic, failure)) {
                // The failure is copied before queries, input reset and resource retirement.
                const auto failed = core_->get_map_status();
                auto& context = frame->diagnostic;
                context.generation = revision;
                context.width = width;
                context.height = height;
                context.frame = game::drawing_diagnostic_frame();
                context.world = core_->get_debug_command_world();
                context.tick = std::int64_t(failed.get("simulation_tick", 0));
                context.camera_x = std::int64_t(failed.get("camera_x", 0));
                context.camera_y = std::int64_t(failed.get("camera_y", 0));
                context.core_status = result;
                context.core = core_ok ? 1 : 2;
                context.end_frame = prepared ? 1 : 2;
                context.commands = static_cast<std::uint32_t>(frame->input.command_count);
                context.resources = static_cast<std::uint32_t>(frame->input.resource_count);
                std::snprintf(context.stage, sizeof(context.stage), "%s",
                              failure.stage[0] ? failure.stage : "core_draw");
                const auto core_error = String(failed.get("error", "")).utf8();
                std::snprintf(context.core_error, sizeof(context.core_error), "%s", core_error.get_data());
                std::snprintf(context.bridge_error, sizeof(context.bridge_error), "%s", game::presentation_error(*resources_));
                char snapshot[4096];
                game::write_render_failure(context, failure, snapshot, sizeof(snapshot));
                error_ = String("Frame preparation: ") + game::drawing_status_name(result) +
                         "\ncore=" + (core_ok ? "succeeded" : "failed") +
                         " end_frame=" + (prepared ? "succeeded" : "failed") +
                         "\nfailure=" + String::num_uint64(failure.id) + " stage=" + failure.stage +
                         "\nrejected=" + failure.condition + "\n" + failure.detail + "\n" + failure.object +
                         "\nsource=" + failure.source + ":" + String::num_uint64(failure.line) + "\n" +
                         context.core_error + "\n" + context.bridge_error +
                         "\nframe=" + String::num_int64(context.frame) + " camera=(" +
                         String::num_int64(context.camera_x) + "," + String::num_int64(context.camera_y) + ")" +
                         "\ndiagnostics=" +
                         (snapshot[0] ? String::utf8(snapshot) : String("snapshot unavailable; see runtime events"));
                reset_map_input();
                return;
            }
            if (rendered) {
                const auto current = core_->get_map_status();
                frame->diagnostic.generation = revision;
                frame->diagnostic.world = core_->get_debug_command_world();
                frame->diagnostic.tick = std::int64_t(current.get("simulation_tick", 0));
                frame->diagnostic.frame = game::drawing_diagnostic_frame();
                frame->diagnostic.camera_x = std::int64_t(current.get("camera_x", 0));
                frame->diagnostic.camera_y = std::int64_t(current.get("camera_y", 0));
                frame->diagnostic.width = width;
                frame->diagnostic.height = height;
                frame->diagnostic.core_status = result;
                frame->diagnostic.core = frame->diagnostic.end_frame = 1;
                drawn_ = frame->stats.drawn;
                packets_ = frame->input.instance_count + frame->input.compute_count;
                phase = "render_enqueue";
                exception_context.enqueue = 2;
                renderer_->submit(std::move(frame));
                exception_context.enqueue = 1;
                phase = "bridge_present";
                width_ = width;
                height_ = height;
                revision_ = revision;
                camera_revision_ =
                    static_cast<std::uint64_t>(std::int64_t(current.get("camera_revision", 0)));
                ui_revision_ = ui_revision;
                presentation_revision_ = presentation_revision;
            }
        }
        auto texture = renderer_->texture();
        game::DebugHostPerformance performance;
        performance.frame_seconds = seconds;
        std::uint64_t performance_generation = 0;
        if (!renderer_->performance(performance_generation, performance.sequence, performance.render_cpu_ms) ||
            performance_generation != revision)
            performance.sequence = 0;
        core_->record_host_performance(static_cast<std::uint64_t>(core_->get_debug_command_world()), performance);
        const auto rid = texture.is_valid() ? texture->get_texture_rd_rid() : RID();
        // Texture2DRD keeps its Ref identity when its GPU target is recreated.
        // Canvas draw commands must be rebuilt for the new RID after a resize.
        if (texture != texture_ || rid != texture_rid_) {
            texture_ = texture;
            texture_rid_ = rid;
            queue_redraw();
        }
        update_cursor();
    } catch (const std::exception& e) {
        record_exception(e.what());
        error_ = String::utf8(e.what()) + "\ndiagnostics=" +
                 (exception_snapshot[0] ? String::utf8(exception_snapshot) : String("snapshot unavailable"));
        reset_map_input();
    } catch (...) {
        record_exception("Map frame preparation failed");
        error_ = String("Map frame preparation failed\ndiagnostics=") +
                 (exception_snapshot[0] ? String::utf8(exception_snapshot) : String("snapshot unavailable"));
        reset_map_input();
    }
}
void RA2MapView::update_cursor() {
    game::GameCursorFrame frame;
    if (core_.is_null() || !core_->read_cursor(frame)) frame.visible = false;
    if (frame.visible && (cursor_texture_.is_null() || frame.revision != cursor_.revision)) {
        PackedByteArray pixels;
        pixels.resize(std::int64_t(frame.width) * frame.height * 4);
        if (!core_->read_cursor(frame, pixels.ptrw(), std::uint32_t(pixels.size())))
            frame.visible = false;
        else {
            auto image = Image::create_from_data(frame.width, frame.height, false, Image::FORMAT_RGBA8, pixels);
            cursor_texture_ = ImageTexture::create_from_image(image);
        }
    }
    frame.visible = frame.visible && focused_ && can_process() && cursor_texture_.is_valid();
    auto* input = Input::get_singleton();
    if (frame.visible && input->get_mouse_mode() == Input::MOUSE_MODE_VISIBLE) {
        input->set_mouse_mode(Input::MOUSE_MODE_HIDDEN);
        owns_mouse_visibility_ = true;
    } else if (!frame.visible && owns_mouse_visibility_) {
        if (input->get_mouse_mode() == Input::MOUSE_MODE_HIDDEN) input->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
        owns_mouse_visibility_ = false;
    }
    if (frame.visible != cursor_.visible || frame.x != cursor_.x || frame.y != cursor_.y ||
        frame.revision != cursor_.revision || frame.hotspot_x != cursor_.hotspot_x ||
        frame.hotspot_y != cursor_.hotspot_y)
        queue_redraw();
    cursor_ = frame;
}
void RA2MapView::_draw() {
    if (texture_.is_valid()) draw_texture(texture_, Vector2());
    if (cursor_.visible && cursor_texture_.is_valid())
        draw_texture(cursor_texture_, Vector2(cursor_.x - cursor_.hotspot_x, cursor_.y - cursor_.hotspot_y));
}
