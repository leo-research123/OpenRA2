#pragma once
#include <vector>
#include "sprite_draw_batch.hpp"

namespace ra2::experiments {

struct BallState {
    double x = 0.0;
    double y = 0.0;
    double velocity_x = 260.0;
    double velocity_y = 180.0;
    double radius = 20.0;
    double hue = 0.0;
};

// Display-only experiment. Time is measured in seconds, coordinates in viewport pixels.
class TestBall {
public:
    bool reset(double width, double height, int count = 1, double radius = 20.0);
    bool advance(double seconds, double width, double height);
    void prepare_sprites(std::vector<ra2::experiments::SpriteDrawInstance>& output) const;
    const BallState& state() const { return states_.front(); }
    const std::vector<BallState>& states() const { return states_; }

private:
    std::vector<BallState> states_{1};
    double radius_ = 20.0;
};

}
