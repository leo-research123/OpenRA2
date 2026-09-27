#pragma once
#include "bridge/ra2_core.hpp"
#include "bridge/map_renderer.hpp"
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <memory>
namespace game { class MapResources; }
class RA2MapView : public godot::Node2D {
    GDCLASS(RA2MapView,godot::Node2D)
protected:
    static void _bind_methods();
    void _notification(int what);
public:
    RA2MapView();
    ~RA2MapView() override;
    void configure(const godot::Ref<RA2Core>&);
    void open_map(const godot::String&);
    void close_map();
    godot::Dictionary get_render_status() const;
    godot::Dictionary handle_map_input(const godot::Ref<godot::InputEvent>&,const godot::Vector2& surface_size);
    void reset_map_input();
    void _process(double delta) override;
    void _draw() override;
private:
    godot::Ref<RA2Core> core_;
    godot::Ref<RA2MapRenderer> renderer_;
    godot::Ref<godot::Texture2DRD> texture_;
    godot::RID texture_rid_;
    std::unique_ptr<game::MapResources> resources_;
    bool focused_=true;
    bool control_click_=false;
    std::uint64_t revision_=0,last_clock_usec_=0;
    std::uint64_t camera_revision_=0,ui_revision_=0,presentation_revision_=0;
    int width_=0,height_=0;
    std::uint32_t drawn_=0;
    std::size_t packets_=0;
    godot::String error_;
};
