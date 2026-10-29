#pragma once
#include "yrpp/Timer.h"
#include <bit>
namespace game {
inline int checksum_timer_remaining(const CDTimerClass& timer) noexcept {
    int left = timer.TimeLeft;
    if (timer.StartTime != -1) {
        const int elapsed = std::bit_cast<int>(unsigned(Unsorted::CurrentFrame) - unsigned(timer.StartTime));
        left = elapsed >= left ? 0 : std::bit_cast<int>(unsigned(left) - unsigned(elapsed));
    }
    return left;
}
}
