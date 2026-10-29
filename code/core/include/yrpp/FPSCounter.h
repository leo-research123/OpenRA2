#pragma once
#include "yrpp/YRPPCore.h"
#include "yrpp/Helpers/CompileTime.h"

class FPSCounter {
public:
    //!< The number of frames processed in the last second.
    /// Global VA: 0x00ABCD44.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(unsigned int, CurrentFrameRate, 0xABCD44u)
#else
    static unsigned int& CurrentFrameRate;
#endif

    //!< The total number of frames elapsed.
    /// Global VA: 0x00ABCD48.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(unsigned int, TotalFramesElapsed, 0xABCD48u)
#else
    static unsigned int& TotalFramesElapsed;
#endif

    //!< The time it took to process TotalFramesElapsed frames.
    /// Global VA: 0x00ABCD4C.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(unsigned int, TotalTimeElapsed, 0xABCD4Cu)
#else
    static unsigned int& TotalTimeElapsed;
#endif

    //!< Whether the current fps is considered too low.
    /// Global VA: 0x00ABCD50.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, ReducedEffects, 0xABCD50u)
#else
    static bool& ReducedEffects;
#endif

    //!< The average frame rate for all frames processed.
    static inline double GetAverageFrameRate() {
        if (TotalTimeElapsed) {
            return static_cast<double>(TotalFramesElapsed) / static_cast<double>(TotalTimeElapsed);
        }

        return 0.0;
    }
};

class Detail {
public:
    //!< What is considered the minimum acceptable FPS.
    /// Global VA: 0x00829FF4.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(unsigned int, MinFrameRate, 0x829FF4u)
#else
    static unsigned int& MinFrameRate;
#endif
    /// VA: 0x0055AF40
    static void SetMinFrameRate(unsigned int value) noexcept;

    //!< The zone that needs to be left to change
    /// Global VA: 0x00829FF8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(unsigned int, BufferZoneWidth, 0x829FF8u)
#else
    static unsigned int& BufferZoneWidth;
#endif

    //!< The minimum frame rate considering the buffer zone.
    /// VA: 0x0055AF60
    static unsigned int GetMinFrameRate()
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x55AF60);
    }
#else
        ;
#endif

    //!< Whether effects should be reduced.
    static inline bool ReduceEffects() { return FPSCounter::CurrentFrameRate < GetMinFrameRate(); }
};
