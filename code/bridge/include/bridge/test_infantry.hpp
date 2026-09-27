#pragma once
#include "bridge/sprite_batch_renderer.hpp"
#include "yrpp/BasicStructures.h"
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <chrono>
#include <vector>

namespace godot { class Label; }

// test1 is a host graphics experiment, not a game simulation or a unit model.
// All Godot objects, atlas creation and test motion remain in bridge.
class RA2TestInfantry : public godot::Node2D {
    GDCLASS(RA2TestInfantry, godot::Node2D)
public:
    static void _bind_methods();
    void _ready() override;
    void _process(double delta) override;
    void _draw() override;
    void _exit_tree() override;
    void _unhandled_key_input(const godot::Ref<godot::InputEvent>& event) override;
    void advance(double seconds);
    godot::Dictionary get_display_state() const;
    godot::Dictionary get_instance_state(int index) const;
    void set_instance_count(int count);
    int get_instance_count() const { return instance_count_; }
    void set_game_data_path(const godot::String& path) { game_data_path_ = path; }
    godot::String get_game_data_path() const { return game_data_path_; }
    void set_show_info(bool value) { show_info_ = value; }
    bool get_show_info() const { return show_info_; }
    void set_resize_window(bool value) { resize_window_ = value; }
    bool get_resize_window() const { return resize_window_; }
    void set_unlimited_fps(bool value) { unlimited_fps_ = value; }
    bool get_unlimited_fps() const { return unlimited_fps_; }
    ~RA2TestInfantry() override;
private:
    struct Frame {
        int index;
        RectangleStruct bounds;
        float u, v, uv_width, uv_height;
    };
    struct Walker {
        double x, y, velocity_x, velocity_y, animation;
        int direction = 0, phase = 0;
    };
    godot::String resource_directory() const;
    void load_graphics();
    void load_graphics_in_context();
    void release_graphics();
    void reset_walkers();
    void update_display(double seconds);
    void update_info();
    void configure_window();
    void restore_window();
    void observe_benchmark(double frame_ms);
    void save_benchmark();
    struct Sample {
        double frame_ms, update_ms, submit_ms;
        int submitted;
        bool focused, paused;
    };
    godot::String benchmark_report_;
    std::vector<Sample> benchmark_samples_;
    std::chrono::steady_clock::time_point benchmark_origin_;
    bool benchmark_recording_ = false, benchmark_resized_ = false;
    godot::Vector2 benchmark_viewport_;
    double benchmark_measured_seconds_ = 0;
    SpriteBatchRenderer renderer_;
    godot::Ref<godot::ImageTexture> atlas_, palette_;
    std::vector<Frame> frames_;
    std::vector<Walker> walkers_;
    std::vector<size_t> draw_order_;
    std::vector<ra2::experiments::SpriteDrawInstance> sprites_;
    godot::String game_data_path_, error_;
    godot::Label* info_ = nullptr;
    godot::Vector2 viewport_size_;
    godot::Rect2 frame_envelope_;
    int canvas_width_ = 0, canvas_height_ = 0;
    int walk_start_ = 0, walk_count_ = 0, walk_stride_ = 0;
    int instance_count_ = 10'000, submitted_ = 0;
    uint64_t submission_count_ = 0;
    double update_ms_ = 0, submit_ms_ = 0;
    std::chrono::steady_clock::time_point last_frame_;
    double hud_elapsed_ = 0.0;
    double hud_update_ms_ = 0, hud_submit_ms_ = 0, hud_max_ms_ = 0;
    double shown_fps_ = 0, shown_update_ms_ = 0, shown_submit_ms_ = 0, shown_max_ms_ = 0;
    int hud_frames_ = 0;
    bool ready_ = false, paused_ = false, show_info_ = true;
    static constexpr float display_scale_ = 1.0f;
    bool resize_window_ = true, unlimited_fps_ = true, window_configured_ = false;
    godot::String previous_title_;
    godot::Vector2i previous_size_, previous_position_, previous_scale_size_;
    godot::Window::ContentScaleMode previous_scale_mode_;
    godot::DisplayServer::VSyncMode previous_vsync_;
    int previous_max_fps_ = 0;
};
