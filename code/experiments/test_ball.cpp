#include "test_ball.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ra2::experiments {
namespace {
bool valid_bounds(double width, double height) {
    return std::isfinite(width) && std::isfinite(height) && width >= 0.0 && height >= 0.0;
}

double fitted_radius(double width, double height, double radius) {
    return std::min({radius, width * 0.5, height * 0.5});
}

void advance_axis(double& position, double& velocity, double extent, double radius, double seconds) {
    const double span = extent - 2.0 * radius;
    if (span <= 0.0) {
        position = extent * 0.5;
        return;
    }

    // A resize can put the old position outside the new bounds.
    position = std::clamp(position, radius, extent - radius);
    const double speed = std::abs(velocity);
    const double period = 2.0 * span;
    double phase = position - radius;
    if (velocity < 0.0) phase = period - phase;

    // Fold the travelled distance into a round trip. Preserve overshoot, including
    // multiple bounces in a long frame, without a delta cap or collision loop.
    phase = std::fmod(phase + std::fmod(seconds, period / speed) * speed, period);
    if (phase < span) {
        position = radius + phase;
        velocity = speed;
    } else {
        position = radius + period - phase;
        velocity = -speed;
    }
}
}

bool TestBall::reset(double width, double height, int count, double radius) {
    if (!valid_bounds(width, height) || count < 1 || count > 100'000 ||
        !std::isfinite(radius) || radius <= 0.0) return false;
    states_.resize(static_cast<std::size_t>(count));
    radius_ = radius;
    const double fitted = fitted_radius(width, height, radius_);
    // Fixed seed and explicit integer-to-fraction mapping make reruns reproducible.
    std::uint32_t seed = 0x42414c4c;
    const auto random = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<double>(seed >> 8) / 16777216.0;
    };
    for (auto& state : states_) {
        state = BallState{};
        state.radius = fitted;
        state.x = width * 0.5;
        state.y = height * 0.5;
        if (count == 1) continue;
        state.x = fitted + random() * (width - 2.0 * fitted);
        state.y = fitted + random() * (height - 2.0 * fitted);
        state.velocity_x = 100.0 + random() * 200.0;
        state.velocity_y = 100.0 + random() * 200.0;
        if (random() < 0.5) state.velocity_x = -state.velocity_x;
        if (random() < 0.5) state.velocity_y = -state.velocity_y;
        state.hue = random();
    }
    return true;
}

bool TestBall::advance(double seconds, double width, double height) {
    if (!std::isfinite(seconds) || seconds < 0.0 || !valid_bounds(width, height)) return false;
    const double radius = fitted_radius(width, height, radius_);
    const double hue_step = std::fmod(seconds, 6.0) / 6.0;
    for (auto& state : states_) {
        state.radius = radius;
        advance_axis(state.x, state.velocity_x, width, radius, seconds);
        advance_axis(state.y, state.velocity_y, height, radius, seconds);
        state.hue = std::fmod(state.hue + hue_step, 1.0);
    }
    return true;
}

void TestBall::prepare_sprites(std::vector<ra2::experiments::SpriteDrawInstance>& output) const {
    output.resize(states_.size());
    for (std::size_t i = 0; i < states_.size(); ++i) {
        const auto& ball = states_[i];
        const float hue = static_cast<float>(ball.hue) * 6.0f;
        const int sector = static_cast<int>(std::floor(hue));
        const float fraction = hue - static_cast<float>(sector);
        const float p = 1.0f - 0.8f, q = 1.0f - 0.8f * fraction, t = 1.0f - 0.8f * (1.0f - fraction);
        float r, g, b;
        switch (sector % 6) {
            case 0: r=1; g=t; b=p; break;
            case 1: r=q; g=1; b=p; break;
            case 2: r=p; g=1; b=t; break;
            case 3: r=p; g=q; b=1; break;
            case 4: r=t; g=p; b=1; break;
            default: r=1; g=p; b=q; break;
        }
        output[i] = {static_cast<float>(ball.x - ball.radius), static_cast<float>(ball.y - ball.radius),
            static_cast<float>(ball.radius * 2), static_cast<float>(ball.radius * 2), 0, 0, 1, 1, r, g, b, 1};
    }
}

}
