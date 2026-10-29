#pragma once
#include "bridge/ra2_core.hpp"
#include "bridge/godot_render_host.hpp"
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <memory>
namespace game {
struct PresentationHandle;
}
class RA2MapView : public godot::Node2D {
    GDCLASS(RA2MapView, godot::Node2D)
protected:
    static void _bind_methods();
    void _notification(int what);

public:
    RA2MapView();
    ~RA2MapView() override;
    void configure(const godot::Ref<RA2Core>&);
    void open_map(const godot::String&);
    void open_campaign(const godot::String&, int difficulty);
    void open_skirmish(const godot::String&, const godot::Dictionary&);
    void close_map();
    bool continue_mission(std::int64_t id);
    godot::Dictionary get_render_status() const;
    godot::Dictionary handle_map_input(const godot::Ref<godot::InputEvent>&, const godot::Vector2& surface_size);
    void reset_map_input();
    void _process(double delta) override;
    void _draw() override;

private:
    void clear_presentation();
    godot::Ref<RA2Core> core_;
    godot::Ref<GodotRenderHost> renderer_;
    godot::Ref<godot::Texture2DRD> texture_;
    godot::Ref<godot::ImageTexture> cursor_texture_;
    // RGB565 display of the frame color (canvas item material).
    godot::RID display_shader_, display_material_;
    game::GameCursorFrame cursor_{};
    void update_cursor();
    bool owns_mouse_visibility_ = false;
    godot::RID texture_rid_;
    game::PresentationHandle* resources_ = nullptr;
    bool focused_ = true;
    bool control_click_ = false;
    std::uint64_t revision_ = 0, last_clock_usec_ = 0;
    std::uint64_t camera_revision_ = 0, ui_revision_ = 0, presentation_revision_ = 0;
    int width_ = 0, height_ = 0;
    std::uint32_t drawn_ = 0;
    std::size_t packets_ = 0;
    godot::String error_;
};
