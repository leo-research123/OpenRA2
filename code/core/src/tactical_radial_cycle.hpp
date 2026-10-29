#pragma once
namespace game {
struct TacticalRadialPulse {
    int value = 0, direction = 1;
    void step() noexcept {
        value += 10 * direction;
        if (value >= 100) direction = -1;
        else if (value <= 0) direction = 1;
    }
};
struct TacticalRadialState { TacticalRadialPulse building, superweapon; };
const TacticalRadialState& tactical_radial_state() noexcept;
void advance_tactical_radials() noexcept;
}
