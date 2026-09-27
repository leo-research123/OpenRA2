#pragma once

#include "../../../experiments/test_ball.hpp"
#include "bridge/sprite_batch_renderer.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

class RA2TestBall : public godot::RefCounted {
    GDCLASS(RA2TestBall, godot::RefCounted)

protected:
    static void _bind_methods();

public:
    bool initialize(const godot::RID& parent_canvas_item, const godot::RID& texture, const godot::Vector2& size, int count, double radius);
    godot::Vector3 advance(double seconds, const godot::Vector2& size);
    bool is_release_build() const;
    void shutdown();
    ~RA2TestBall() override;

private:
    ra2::experiments::TestBall ball_;
    std::vector<ra2::experiments::SpriteDrawInstance> sprites_;
    SpriteBatchRenderer renderer_;
    bool active_ = false;
};
