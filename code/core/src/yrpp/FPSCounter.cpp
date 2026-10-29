// Original Detail module in FPSCounter.h. 0x0055AF40 publishes the minimum
// effects frame rate used by radar movie entry/exit and post-load repair.
#include "yrpp/FPSCounter.h"
#if !defined(RA2_YRPP_GAME)
namespace {
unsigned minimum_frame_rate = 15, buffer_zone = 5, current_fps = 60, total_frames = 0, total_time = 0;
bool reduced = false;
}
unsigned& Detail::BufferZoneWidth = buffer_zone;
unsigned& FPSCounter::CurrentFrameRate = current_fps;
unsigned& FPSCounter::TotalFramesElapsed = total_frames;
unsigned& FPSCounter::TotalTimeElapsed = total_time;
bool& FPSCounter::ReducedEffects = reduced;
unsigned& Detail::MinFrameRate = minimum_frame_rate;
#endif
void Detail::SetMinFrameRate(unsigned value) noexcept { MinFrameRate = value; }

#if !defined(RA2_YRPP_GAME)
// 0x0055AF60: retain the original hysteresis when choosing expensive effects.
unsigned Detail::GetMinFrameRate() {
    if (FPSCounter::ReducedEffects) {
        const unsigned threshold = MinFrameRate + BufferZoneWidth;
        if (FPSCounter::CurrentFrameRate < threshold) return threshold;
        FPSCounter::ReducedEffects = false;
        return MinFrameRate;
    }
    if (FPSCounter::CurrentFrameRate < MinFrameRate) {
        FPSCounter::ReducedEffects = true;
        return MinFrameRate + BufferZoneWidth;
    }
    return MinFrameRate;
}

#endif
