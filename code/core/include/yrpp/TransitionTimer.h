#pragma once

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/Timer.h"

struct TransitionTimer
{
public:
    // Constructor, Destructor
    /// VA: 0x004A50F0.
#if defined(RA2_YRPP_GAME)
    TransitionTimer() { JMP_THIS(0x4A50F0); }
#else
    TransitionTimer() noexcept : Rate1(0), ActionTimer(0), Rate2(0), State1(false), State2(false) {}
#endif

    ~TransitionTimer() = default;

    bool AreStates11() // 0x4A5110
        { return this->State1 && this->State2; }

    bool AreStates10() // 0x4A5130
        { return this->State1 && !this->State2; }

    bool AreStates01() // 0x4A51B0
        { return !this->State1 && this->State2 == 1; }

    bool AreStates00() // 0x4A51D0
        { return !this->State1 && !this->State2; }

    /// VA: 0x004A5150.
#if defined(RA2_YRPP_GAME)
    bool IsTimerFinished()
        { JMP_THIS(0x4A5150); }
#else
    bool IsTimerFinished();
#endif

    /// VA: 0x004A51F0.
#if defined(RA2_YRPP_GAME)
    void StartTimer11(double time)
        { JMP_THIS(0x4A51F0); }
#else
    void StartTimer11(double time);
#endif

    /// VA: 0x004A5240.
#if defined(RA2_YRPP_GAME)
    void StartTimer10(double time)
        { JMP_THIS(0x4A5240); }
#else
    void StartTimer10(double time);
#endif

    /// VA: 0x004A5290.
#if defined(RA2_YRPP_GAME)
    void Update()
        { JMP_THIS(0x4A5290); }
#else
    void Update(); // Reverses an active transition; this is not a per-frame tick.
#endif

    /// VA: 0x004A52F0.
#if defined(RA2_YRPP_GAME)
    double PercentageDone()
        { JMP_THIS(0x4A52F0); }
#else
    double PercentageDone();
#endif

    /// VA: 0x004A5360.
#if defined(RA2_YRPP_GAME)
    void SetToDone()
        { JMP_THIS(0x4A5360); }
#else
    void SetToDone();
#endif

    // Properties

public:

    double      Rate1;
    CDTimerClass ActionTimer;
    DWORD       Rate2;
    bool        State1;
    bool        State2;
};
