#include "bridge/test_infantry.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <numbers>
#include <numeric>
#include <stdexcept>


namespace {
// Same folded reflection as test-ball: retain overshoot through any number of
// bounces, and clamp positions to the new bounds after a window resize.
void advance_axis(double& position, double& velocity, double low, double high, double seconds) {
    const double span = high - low;
    if (span <= 0) { position = (low + high) * 0.5; return; }
    position = std::clamp(position, low, high);
    const double speed = std::abs(velocity), period = 2.0 * span;
    double phase = position - low;
    if (velocity < 0) phase = period - phase;
    phase = std::fmod(phase + std::fmod(seconds, period / speed) * speed, period);
    if (phase < span) { position = low + phase; velocity = speed; }
    else { position = low + period - phase; velocity = -speed; }
}
}

void RA2TestInfantry::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("advance", "seconds"), &RA2TestInfantry::advance);
    godot::ClassDB::bind_method(godot::D_METHOD("get_display_state"), &RA2TestInfantry::get_display_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_instance_state", "index"), &RA2TestInfantry::get_instance_state);
    godot::ClassDB::bind_method(godot::D_METHOD("set_instance_count", "count"), &RA2TestInfantry::set_instance_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_instance_count"), &RA2TestInfantry::get_instance_count);
    godot::ClassDB::bind_method(godot::D_METHOD("set_game_data_path", "path"), &RA2TestInfantry::set_game_data_path);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_data_path"), &RA2TestInfantry::get_game_data_path);
    godot::ClassDB::bind_method(godot::D_METHOD("set_show_info", "value"), &RA2TestInfantry::set_show_info);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_info"), &RA2TestInfantry::get_show_info);
    godot::ClassDB::bind_method(godot::D_METHOD("set_resize_window", "value"), &RA2TestInfantry::set_resize_window);
    godot::ClassDB::bind_method(godot::D_METHOD("get_resize_window"), &RA2TestInfantry::get_resize_window);
    godot::ClassDB::bind_method(godot::D_METHOD("set_unlimited_fps", "value"), &RA2TestInfantry::set_unlimited_fps);
    godot::ClassDB::bind_method(godot::D_METHOD("get_unlimited_fps"), &RA2TestInfantry::get_unlimited_fps);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "game_data_path", godot::PROPERTY_HINT_DIR),
        "set_game_data_path", "get_game_data_path");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_info"), "set_show_info", "get_show_info");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "instance_count", godot::PROPERTY_HINT_RANGE,
        "1,100000,1"), "set_instance_count", "get_instance_count");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "resize_window"), "set_resize_window", "get_resize_window");
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "unlimited_fps"), "set_unlimited_fps", "get_unlimited_fps");
}

void RA2TestInfantry::set_instance_count(int count) {
    ERR_FAIL_COND_MSG(is_inside_tree(), "Set instance_count before adding test1 to the scene tree.");
    instance_count_ = std::clamp(count, 1, 100'000);
}

void RA2TestInfantry::configure_window() {
    auto* window = get_window();
    auto* display = godot::DisplayServer::get_singleton();
    previous_title_ = window->get_title();
    previous_scale_mode_ = window->get_content_scale_mode();
    previous_scale_size_ = window->get_content_scale_size();
    previous_size_ = window->get_size(); previous_position_ = window->get_position();
    previous_max_fps_ = godot::Engine::get_singleton()->get_max_fps();
    previous_vsync_ = display->window_get_vsync_mode(window->get_window_id());
    window_configured_ = true;
    window->set_title("test1");
    // One SHP pixel maps to one viewport pixel, including after resizing.
    window->set_content_scale_mode(godot::Window::CONTENT_SCALE_MODE_DISABLED);
    window->set_content_scale_size({});
    if (resize_window_) {
        const auto usable = display->screen_get_usable_rect(window->get_current_screen());
        const godot::Vector2i size(std::min(1920, std::max(160, usable.size.x - 32)),
            std::min(1080, std::max(120, usable.size.y - 64)));
        window->set_size(size);
        window->set_position(usable.position + (usable.size - size) / 2);
    }
    // Keep the engine's existing limit, including --max-fps. Its default 0 is
    // already unlimited; the fixed benchmark protocol explicitly selects 60.
    if (!benchmark_report_.is_empty()) godot::Engine::get_singleton()->set_max_fps(60);
    if (unlimited_fps_ || !benchmark_report_.is_empty()) {
        display->window_set_vsync_mode(godot::DisplayServer::VSYNC_DISABLED, window->get_window_id());
    }
}

void RA2TestInfantry::restore_window() {
    if (!window_configured_) return;
    auto* window = get_window();
    window->set_title(previous_title_);
    window->set_content_scale_mode(previous_scale_mode_);
    window->set_content_scale_size(previous_scale_size_);
    if (resize_window_) { window->set_size(previous_size_); window->set_position(previous_position_); }
    godot::Engine::get_singleton()->set_max_fps(previous_max_fps_);
    godot::DisplayServer::get_singleton()->window_set_vsync_mode(previous_vsync_, window->get_window_id());
    window_configured_ = false;
}

void RA2TestInfantry::_ready() {
    hud_elapsed_ = hud_update_ms_ = hud_submit_ms_ = hud_max_ms_ = 0;
    shown_fps_ = shown_update_ms_ = shown_submit_ms_ = shown_max_ms_ = 0;
    hud_frames_ = submitted_ = 0; submission_count_ = 0; paused_ = false; error_ = "";
    for (const auto& argument : godot::OS::get_singleton()->get_cmdline_user_args())
        if (argument.begins_with("--benchmark-report=")) benchmark_report_ = argument.substr(19);
    configure_window();
    if (show_info_) {
        auto* panel = memnew(godot::PanelContainer);
        panel->set_position({16, 16}); panel->set_z_index(10);
        panel->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
        godot::Ref<godot::StyleBoxFlat> style;
        style.instantiate();
        style->set_bg_color({0.02f, 0.03f, 0.05f, 0.94f});
        for (auto side : {godot::SIDE_LEFT, godot::SIDE_TOP, godot::SIDE_RIGHT, godot::SIDE_BOTTOM})
            style->set_content_margin(side, 12);
        panel->add_theme_stylebox_override("panel", style);
        info_ = memnew(godot::Label);
        info_->add_theme_font_size_override("font_size", 18);
        panel->add_child(info_);
        add_child(panel);
    }
    bool parsing_arguments = true;
    try {
        for (const auto& argument : godot::OS::get_singleton()->get_cmdline_user_args()) {
            if (argument == "--instance-count")
                throw std::runtime_error("Use --instance-count=N, where N is an integer from 1 to 100000");
            if (!argument.begins_with("--instance-count=")) continue;
            const auto value = argument.substr(17).utf8();
            int count = 0;
            const auto parsed = std::from_chars(value.get_data(), value.get_data() + value.length(), count);
            if (parsed.ec != std::errc{} || parsed.ptr != value.get_data() + value.length() ||
                count < 1 || count > 100'000)
                throw std::runtime_error("--instance-count must be an integer from 1 to 100000");
            instance_count_ = count;
        }
        parsing_arguments = false;
        if (!benchmark_report_.is_empty()) {
#ifndef NDEBUG
            throw std::runtime_error("Benchmark requires a C++ Release build");
#endif
            auto* os = godot::OS::get_singleton();
            if (os->has_feature("editor") || !os->has_feature("release"))
                throw std::runtime_error("Benchmark requires a Godot Release export");
            benchmark_samples_.reserve(1200); // 5 seconds at 60 FPS, with spare capacity
        }
        load_graphics();
        reset_walkers();
        ready_ = true;
        update_display(0);
        godot::UtilityFunctions::print("test1: ", instance_count_, " GI, 1x; random movement and reflection; ",
            "8 directions x ", walk_count_, " frames; viewport=", viewport_size_,
            "; max_fps=", godot::Engine::get_singleton()->get_max_fps(), "; Esc exits.");
    } catch (const std::exception& e) {
        release_graphics();
        error_ = godot::String::utf8(e.what());
        ERR_PRINT(godot::String("test1: ") + error_);
        if (parsing_arguments || !benchmark_report_.is_empty()) get_tree()->quit(1);
    }
    update_info();
    last_frame_ = std::chrono::steady_clock::now();
    benchmark_origin_ = last_frame_;
    queue_redraw();
}

void RA2TestInfantry::reset_walkers() {
    viewport_size_ = get_viewport_rect().size;
    walkers_.resize(size_t(instance_count_)); sprites_.resize(walkers_.size());
    draw_order_.resize(walkers_.size());
    std::iota(draw_order_.begin(), draw_order_.end(), size_t(0));
    uint32_t seed = 0x4749574b;
    const auto random = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return double(seed >> 8) / 16777216.0;
    };
    const auto low = -frame_envelope_.position;
    const auto high = viewport_size_ - frame_envelope_.get_end();
    for (auto& walker : walkers_) {
        walker = {(low.x + high.x) * 0.5, (low.y + high.y) * 0.5, 260, 180, 0};
        if (instance_count_ == 1) continue; // repeatable isolated pixel check, like test-ball
        walker.x = low.x + random() * (high.x - low.x);
        walker.y = low.y + random() * (high.y - low.y);
        walker.velocity_x = 100 + random() * 200;
        walker.velocity_y = 100 + random() * 200;
        if (random() < 0.5) walker.velocity_x = -walker.velocity_x;
        if (random() < 0.5) walker.velocity_y = -walker.velocity_y;
        walker.animation = random() * walk_count_;
    }
}

void RA2TestInfantry::update_display(double seconds) {
    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    viewport_size_ = get_viewport_rect().size;
    const auto low = -frame_envelope_.position;
    const auto high = viewport_size_ - frame_envelope_.get_end();
    const double animation_step = std::fmod(seconds, walk_count_ / 8.0) * 8.0;
    for (auto& walker : walkers_) {
        advance_axis(walker.x, walker.velocity_x, low.x, high.x, seconds);
        advance_axis(walker.y, walker.velocity_y, low.y, high.y, seconds);
        // SHP directions: N, NW, W, SW, S, SE, E, NE in screen space.
        walker.direction = (int(std::lround(std::atan2(-walker.velocity_x, -walker.velocity_y)
            * 4.0 / std::numbers::pi)) + 8) % 8;
        walker.animation = std::fmod(walker.animation + animation_step, double(walk_count_));
        walker.phase = int(walker.animation);
    }
    // Keep instance identity stable while submitting from back to front.
    std::sort(draw_order_.begin(), draw_order_.end(), [this](size_t a, size_t b) {
        return walkers_[a].y == walkers_[b].y ? a < b : walkers_[a].y < walkers_[b].y;
    });
    for (size_t i = 0; i < walkers_.size(); ++i) {
        const auto& walker = walkers_[draw_order_[i]];
        const auto& frame = frames_[size_t(walker.direction) * walk_count_ + walker.phase];
        sprites_[i] = {
            float(walker.x) + frame.bounds.X - canvas_width_ / 2,
            float(walker.y) + frame.bounds.Y - canvas_height_ / 2,
            float(frame.bounds.Width), float(frame.bounds.Height),
            frame.u, frame.v, frame.uv_width, frame.uv_height, 1, 1, 1, 1};
    }
    const auto prepared = Clock::now();
    submitted_ = renderer_.submit({sprites_}) ? int(sprites_.size()) : 0;
    if (submitted_) ++submission_count_;
    const auto submitted = Clock::now();
    update_ms_ = std::chrono::duration<double, std::milli>(prepared - start).count();
    submit_ms_ = std::chrono::duration<double, std::milli>(submitted - prepared).count();
}

void RA2TestInfantry::advance(double seconds) {
    if (!ready_ || !std::isfinite(seconds) || seconds < 0) return;
    update_display(paused_ ? 0 : seconds);
}

void RA2TestInfantry::_process(double delta) {
    const auto now = std::chrono::steady_clock::now();
    const double frame_ms = std::chrono::duration<double, std::milli>(now - last_frame_).count();
    last_frame_ = now;
    if (get_viewport_rect().size != viewport_size_) queue_redraw();
    advance(delta);
    hud_elapsed_ += frame_ms / 1000; ++hud_frames_;
    hud_update_ms_ += update_ms_; hud_submit_ms_ += submit_ms_;
    hud_max_ms_ = std::max(hud_max_ms_, frame_ms);
    if (hud_elapsed_ >= 0.5) {
        shown_fps_ = hud_frames_ / hud_elapsed_;
        shown_update_ms_ = hud_update_ms_ / hud_frames_;
        shown_submit_ms_ = hud_submit_ms_ / hud_frames_;
        shown_max_ms_ = hud_max_ms_;
        update_info();
        hud_elapsed_ = hud_update_ms_ = hud_submit_ms_ = hud_max_ms_ = 0;
        hud_frames_ = 0;
    }
    if (ready_ && !benchmark_report_.is_empty()) observe_benchmark(frame_ms);
}

void RA2TestInfantry::_draw() {
    draw_rect(get_viewport_rect(), godot::Color(0.065f, 0.085f, 0.105f), true);
}

void RA2TestInfantry::update_info() {
    if (!info_) return;
    if (!error_.is_empty()) {
        info_->set_text(godot::String::utf8("test1 · 资源加载失败\n") + error_
            + godot::String::utf8("\n请使用 --game-data=目录 或 RA2_GAME_DATA 指定游戏资源。"));
        return;
    }
#ifdef NDEBUG
    const char* build = "C++ Release";
#else
    const char* build = "C++ Debug";
#endif
    info_->set_text(godot::String::utf8("test1 · ") + godot::String::num_int64(submitted_)
        + godot::String::utf8(" 个小兵 · 1× · ") + godot::String::num_int64(int(viewport_size_.x))
        + godot::String::utf8(" × ") + godot::String::num_int64(int(viewport_size_.y))
        + "\n" + godot::String::num(shown_fps_, 1) + godot::String::utf8(" FPS · max ") + godot::String::num(shown_max_ms_, 2) + " ms"
        + "\n" + build + godot::String::utf8(" · update ") + godot::String::num(shown_update_ms_, 2)
        + godot::String::utf8(" ms · submit ") + godot::String::num(shown_submit_ms_, 2) + " ms"
        + (benchmark_report_.is_empty() ? godot::String() : godot::String::utf8("\nRelease · 60 FPS cap · 5s warmup + 5s capture"))
        + godot::String::utf8("\n空格：暂停 / 继续    R：重置    Esc：退出")
        + (paused_ ? godot::String::utf8("  [已暂停]") : godot::String()));
}

void RA2TestInfantry::_unhandled_key_input(const godot::Ref<godot::InputEvent>& event) {
    const godot::Ref<godot::InputEventKey> key = event;
    if (key.is_null() || !key->is_pressed() || key->is_echo()) return;
    if (key->get_keycode() == godot::KEY_ESCAPE) get_tree()->quit();
    else if (key->get_keycode() == godot::KEY_SPACE) { paused_ = !paused_; update_info(); }
    else if (key->get_keycode() == godot::KEY_R) {
        paused_ = false;
        if (ready_) { reset_walkers(); update_display(0); }
        update_info();
    }
}

godot::Dictionary RA2TestInfantry::get_display_state() const {
    godot::Dictionary result = get_instance_state(0);
    result["ready"] = ready_; result["error"] = error_; result["paused"] = paused_;
    result["count"] = int64_t(walkers_.size());
    result["walk_start"] = walk_start_; result["walk_count"] = walk_count_;
    result["walk_stride"] = walk_stride_; result["atlas_frames"] = int64_t(frames_.size());
    result["submitted"] = int64_t(submitted_); result["scale"] = display_scale_;
    result["submission_count"] = int64_t(submission_count_);
    result["update_ms"] = update_ms_; result["submit_ms"] = submit_ms_;
    result["fps"] = shown_fps_;
    result["viewport_size"] = viewport_size_; result["frame_envelope"] = frame_envelope_;
    result["canvas_size"] = godot::Vector2(canvas_width_, canvas_height_);
    return result;
}

godot::Dictionary RA2TestInfantry::get_instance_state(int index) const {
    godot::Dictionary result;
    if (ready_ && index >= 0 && size_t(index) < walkers_.size()) {
        const auto& walker = walkers_[size_t(index)];
        const godot::Vector2 position(float(walker.x), float(walker.y));
        const auto& frame = frames_[size_t(walker.direction) * walk_count_ + walker.phase];
        result["position"] = position;
        result["velocity"] = godot::Vector2(float(walker.velocity_x), float(walker.velocity_y));
        result["direction"] = walker.direction; result["phase"] = walker.phase;
        result["frame"] = frame.index;
        result["frame_size"] = godot::Vector2(frame.bounds.Width, frame.bounds.Height);
        result["draw_rect"] = godot::Rect2(
            position + godot::Vector2(frame.bounds.X - canvas_width_ / 2,
                frame.bounds.Y - canvas_height_ / 2),
            godot::Vector2(frame.bounds.Width, frame.bounds.Height));
    }
    return result;
}

void RA2TestInfantry::release_graphics() {
    ready_ = false;
    renderer_.shutdown();
    atlas_.unref(); palette_.unref(); frames_.clear();
    walkers_.clear(); draw_order_.clear(); sprites_.clear(); submitted_ = 0;
}
void RA2TestInfantry::_exit_tree() { release_graphics(); restore_window(); }
RA2TestInfantry::~RA2TestInfantry() { release_graphics(); }
