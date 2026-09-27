#include "bridge/test_ball.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <chrono>

void RA2TestBall::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("initialize", "parent_canvas_item", "texture", "size", "count", "radius"), &RA2TestBall::initialize);
    godot::ClassDB::bind_method(godot::D_METHOD("advance", "seconds", "size"), &RA2TestBall::advance);
    godot::ClassDB::bind_method(godot::D_METHOD("is_release_build"), &RA2TestBall::is_release_build);
    godot::ClassDB::bind_method(godot::D_METHOD("shutdown"), &RA2TestBall::shutdown);
}

bool RA2TestBall::initialize(const godot::RID& parent_canvas_item, const godot::RID& texture, const godot::Vector2& size, int count, double radius) {
    shutdown();
    if (!ball_.reset(size.x, size.y, count, radius) || !renderer_.initialize(parent_canvas_item, texture, count)) return false;
    ball_.prepare_sprites(sprites_);
    active_ = renderer_.submit({sprites_});
    return active_;
}

godot::Vector3 RA2TestBall::advance(double seconds, const godot::Vector2& size) {
    if (!active_) return {};
    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    if (!ball_.advance(seconds, size.x, size.y)) return {};
    ball_.prepare_sprites(sprites_);
    const auto updated = Clock::now();
    const int submitted = renderer_.submit({sprites_}) ? static_cast<int>(sprites_.size()) : 0;
    const auto drawn = Clock::now();
    return godot::Vector3(
        static_cast<float>(std::chrono::duration<double, std::milli>(updated - start).count()),
        static_cast<float>(std::chrono::duration<double, std::milli>(drawn - updated).count()),
        static_cast<float>(submitted));
}

bool RA2TestBall::is_release_build() const {
#ifdef NDEBUG
    return true;
#else
    return false;
#endif
}

void RA2TestBall::shutdown() {
    active_ = false;
    renderer_.shutdown();
}

RA2TestBall::~RA2TestBall() { shutdown(); }
