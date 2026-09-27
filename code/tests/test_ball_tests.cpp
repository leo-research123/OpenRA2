#include "support/test_support.hpp"
#include "experiments/test_ball.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using ra2::experiments::TestBall;



bool near(double a, double b) { return std::abs(a - b) < 1e-7; }

void bounces_and_overshoot() {
    TestBall ball;
    EXPECT_TRUE((ball.reset(560, 400))) << "reset";
    EXPECT_TRUE((ball.advance(1, 560, 400))) << "step";
    auto s = ball.state();
    EXPECT_TRUE((near(s.x, 540) && near(s.y, 380) && s.velocity_x < 0 && s.velocity_y < 0)) << "right/bottom corner must reflect both axes at the ball's outer edge";
    ball.advance(0.25, 560, 400);
    s = ball.state();
    EXPECT_TRUE((near(s.x, 475) && near(s.y, 335))) << "remaining distance after a bounce";
    ball.advance(2, 560, 400);
    s = ball.state();
    EXPECT_TRUE((near(s.x, 85) && near(s.y, 65) && s.velocity_x > 0 && s.velocity_y > 0)) << "left/top bounces must preserve overshoot";

    ball.reset(560, 600);
    ball.advance(1, 560, 600);
    s = ball.state();
    EXPECT_TRUE((near(s.x, 540) && near(s.y, 480) && s.velocity_x < 0 && s.velocity_y > 0)) << "hitting one wall must not reverse the other axis";

    ball.reset(560, 400);
    ball.advance(1'000'000.25, 560, 400);
    s = ball.state();
    EXPECT_TRUE((near(s.x, 345) && near(s.y, 245) && s.velocity_x > 0 && s.velocity_y > 0)) << "a long frame must handle multiple round trips without freezing or dropping time";
}

void frame_rate_independence() {
    TestBall reference;
    reference.reset(1280, 720);
    reference.advance(120, 1280, 720);
    for (int fps : {30, 60, 144}) {
        TestBall ball;
        ball.reset(1280, 720);
        for (int frame = 0; frame < 120 * fps; ++frame) ball.advance(1.0 / fps, 1280, 720);
        const auto& actual = ball.state();
        const auto& expected = reference.state();
        EXPECT_TRUE((near(actual.x, expected.x) && near(actual.y, expected.y) &&
            actual.velocity_x == expected.velocity_x && actual.velocity_y == expected.velocity_y)) << "equal elapsed time must give the same motion at 30/60/144 FPS";
        const double hue_distance = std::abs(actual.hue - expected.hue);
        EXPECT_TRUE((near(hue_distance, 0) || near(hue_distance, 1))) << "color cycle must be frame-rate independent";
    }
    reference.reset(1280, 720);
    reference.advance(3, 1280, 720);
    EXPECT_TRUE((near(reference.state().hue, 0.5))) << "half a color cycle after three seconds";
    reference.advance(3, 1280, 720);
    EXPECT_TRUE((near(reference.state().hue, 0))) << "full color cycle after six seconds";
}

void resizing_and_invalid_input() {
    TestBall ball;
    ball.reset(1280, 720);
    ball.advance(1, 1280, 720);
    ball.advance(0, 160, 90);
    auto s = ball.state();
    EXPECT_TRUE((near(s.x, 140) && near(s.y, 70) && s.velocity_x < 0 && s.velocity_y < 0)) << "shrinking viewport must immediately contain the ball and point it inward";
    ball.advance(1, 20, 12);
    s = ball.state();
    EXPECT_TRUE((near(s.radius, 6) && s.x >= 6 && s.x <= 14 && near(s.y, 6))) << "a viewport smaller than the ball must remain well-defined";
    ball.advance(1, 0, 0);
    EXPECT_TRUE((near(ball.state().radius, 0))) << "empty viewport";
    ball.advance(1, 1280, 720);
    s = ball.state();
    EXPECT_TRUE((near(s.radius, 20) && s.x >= 20 && s.x <= 1260 && s.y >= 20 && s.y <= 700)) << "restoring viewport must restore the radius and keep moving inside";

    EXPECT_TRUE((!ball.advance(-1, 1280, 720) &&
        !ball.advance(std::numeric_limits<double>::infinity(), 1280, 720) &&
        !ball.advance(1, -1, 720) &&
        !ball.reset(std::numeric_limits<double>::quiet_NaN(), 720))) << "invalid inputs must fail";
    EXPECT_TRUE((near(ball.state().x, s.x) && near(ball.state().y, s.y) && near(ball.state().hue, s.hue))) << "invalid input must leave state unchanged";
}

void ten_thousand_balls() {
    TestBall field, reference;
    EXPECT_TRUE((field.reset(1920, 1080, 10'000, 3))) << "initialize stress load";
    EXPECT_TRUE((reference.reset(1920, 1080, 10'000, 3))) << "repeat stress load";
    const auto initial = field.states();
    EXPECT_TRUE((initial.size() == 10'000)) << "stress load must contain exactly ten thousand balls";
    for (std::size_t i = 0; i < initial.size(); ++i) {
        const auto& state = initial[i];
        const auto& repeated = reference.states()[i];
        EXPECT_TRUE((state.x == repeated.x && state.y == repeated.y && state.hue == repeated.hue)) << "initial load must be reproducible";
        EXPECT_TRUE((state.x >= 3 && state.x <= 1917 && state.y >= 3 && state.y <= 1077)) << "every ball must start inside the viewport";
    }
    field.advance(1.0 / 60, 1920, 1080);
    for (std::size_t i = 0; i < initial.size(); ++i) {
        const auto& state = field.states()[i];
        EXPECT_TRUE(((state.x != initial[i].x || state.y != initial[i].y) && state.hue != initial[i].hue)) << "every ball must move and change color";
    }
    for (int frame = 1; frame < 60; ++frame) field.advance(1.0 / 60, 1920, 1080);
    reference.advance(1, 1920, 1080);
    for (std::size_t i = 0; i < initial.size(); ++i)
        EXPECT_TRUE((near(field.states()[i].x, reference.states()[i].x) && near(field.states()[i].y, reference.states()[i].y))) << "stress motion must also be frame-rate independent";
    field.advance(1'000'000.25, 240, 160);
    for (const auto& state : field.states())
        EXPECT_TRUE((state.x >= 3 && state.x <= 237 && state.y >= 3 && state.y <= 157)) << "resize and long frames must keep all ten thousand balls inside";
    EXPECT_TRUE((!field.reset(1920, 1080, 0, 3) && !field.reset(1920, 1080, 100'001, 3) &&
        !field.reset(1920, 1080, 10'000, 0) && field.states().size() == 10'000)) << "invalid stress configuration must preserve the current load";
}

void portable_sprite_output() {
    TestBall ball;
    std::vector<ra2::experiments::SpriteDrawInstance> sprites;
    ball.reset(560, 400);
    ball.prepare_sprites(sprites);
    EXPECT_TRUE((sprites.size() == 1 && near(sprites[0].x, 260) && near(sprites[0].y, 180) &&
        near(sprites[0].width, 40) && near(sprites[0].height, 40))) << "sprite output must contain the final screen rectangle";
    EXPECT_TRUE((near(sprites[0].u, 0) && near(sprites[0].v, 0) && near(sprites[0].uv_width, 1) &&
        near(sprites[0].uv_height, 1) && near(sprites[0].red, 1) && near(sprites[0].alpha, 1))) << "sprite output must include atlas selection and color without a Godot dependency";
    ball.advance(3, 560, 400);
    ball.prepare_sprites(sprites);
    EXPECT_TRUE((near(sprites[0].red, 0.2) && near(sprites[0].green, 1) && near(sprites[0].blue, 1))) << "half-cycle sprite color";
    ball.reset(1920, 1080, 10'000, 3);
    ball.prepare_sprites(sprites);
    EXPECT_TRUE((sprites.size() == 10'000)) << "sprite output must contain the complete stress load";
    for (const auto& sprite : sprites)
        EXPECT_TRUE((near(sprite.width, 6) && near(sprite.height, 6))) << "stress sprites must retain the requested diameter";
}
}


TEST(TestBall, Contracts) {
    bounces_and_overshoot();
    frame_rate_independence();
    resizing_and_invalid_input();
    ten_thousand_balls();
    portable_sprite_output();
}
