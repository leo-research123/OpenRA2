#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/Fundamentals.h"
#include <bit>
#include <concepts>
#include <cstddef>

template<typename T>
concept TimerType = std::convertible_to<T, int> && requires (T t)
{
    { t() }->std::convertible_to<long>;
    { t }->std::convertible_to<int>;
};

struct FrameTimer
{
    long operator()()const { return Unsorted::CurrentFrame; }
    operator long() const { return Unsorted::CurrentFrame; }
};

struct SystemTimer
{
    // Millisecond source for original timeGetTime consumers (e.g. Event
    // queue timestamps). GetTime applies the original four-bit tick shift.
    // This shared accessor has no separate original function entry.
    static DWORD GetMilliseconds() noexcept;
    /// VA: 0x006C8C40
    static DWORD GetTime();
    long operator()()const { return SystemTimer::GetTime(); }
    operator long() const { return SystemTimer::GetTime(); }
};

template<TimerType Clock>
struct TimerStruct
{
    int StartTime;
    Clock CurrentTime;
    int TimeLeft;

    constexpr TimerStruct() :StartTime { -1 }, TimeLeft { 0 } { };
    explicit TimerStruct(const noinit_t&){ }
    explicit TimerStruct(int duration) { this->Start(duration); }
    TimerStruct(const TimerStruct& other):StartTime{other.StartTime },TimeLeft{other.TimeLeft }{}

    void Start(int duration)
    {
        this->StartTime = this->CurrentTime;
        this->TimeLeft = duration;
    }

    void Stop()
    {
        this->StartTime = -1;
        this->TimeLeft = 0;
    }

    void Pause()
    {
        if (this->IsTicking())
        {
            this->TimeLeft = this->GetTimeLeft();
            this->StartTime = -1;
        }
    }

    void Resume()
    {
        if (!this->IsTicking())
        {
            this->StartTime = this->CurrentTime;
        }
    }

    int GetTimeLeft() const
    {
        if (!this->IsTicking())
        {
            return this->TimeLeft;
        }

        const auto passed = static_cast<DWORD>(static_cast<int>(this->CurrentTime)) - static_cast<DWORD>(this->StartTime);
        const int elapsed = std::bit_cast<std::int32_t>(passed);
        const int left = elapsed >= this->TimeLeft ? 0 :
            std::bit_cast<std::int32_t>(static_cast<DWORD>(this->TimeLeft) - passed);

        return (left <= 0) ? 0 : left;
    }

    // returns whether a ticking timer has finished counting down.
    bool Completed() const
    {
        return this->IsTicking() && !this->HasTimeLeft();
    }

    // Returns whether a delay is active or a timer is still counting down.
    // this is the 'opposite' of Completed() (meaning: incomplete / still busy)
    // and logically the same as !Expired() (meaning: blocked / delay in progress)
    bool InProgress() const
    {
        return this->IsTicking() && this->HasTimeLeft();
    }

    // Returns whether a delay is inactive. same as !InProgress().
    bool Expired() const
    {
        return !this->IsTicking() || !this->HasTimeLeft();
    }

    // Sometimes I want to know if the timer has ever started
    bool HasStarted() const
    {
        return this->IsTicking() || this->HasTimeLeft();
    }

    // Returns whether or not the timer is currently ticking (started but not stopped or paused),
    // regardless of if there is time left or not.
    bool IsTicking() const
    {
        return this->StartTime != -1;
    }

    // Returns whether or not the timer has time left.
    bool HasTimeLeft() const
    {
        return this->GetTimeLeft() > 0;
    }
};

// Timer that counts down from specified value towards zero, counted in frames.
using CDTimerClass = TimerStruct<FrameTimer>;
using SysTimerClass = TimerStruct<SystemTimer>;

static_assert(offsetof(CDTimerClass, TimeLeft) == 0x8);
static_assert(sizeof(SysTimerClass) == 0xC);

// The original Scenario elapsed timer counts up using the system clock.
// It has the same 12-byte storage as the countdown timer, but +8 is elapsed
// time accumulated while paused, not a remaining duration.
template<TimerType Clock>
struct ElapsedTimerStruct
{
    int StartTime;
    Clock CurrentTime;
    int TimeElapsed;

    constexpr ElapsedTimerStruct() : StartTime(-1), TimeElapsed(0) {}
    explicit ElapsedTimerStruct(const noinit_t&) {}
    explicit ElapsedTimerStruct(int initial) { Start(initial); }
    void Start(int initial = 0) {
        StartTime = static_cast<int>(CurrentTime);
        TimeElapsed = initial;
    }
    void Stop() { StartTime = -1; TimeElapsed = 0; }
    bool IsTicking() const { return StartTime != -1; }
    int GetTimeElapsed() const {
        const DWORD passed = IsTicking() ? static_cast<DWORD>(static_cast<int>(CurrentTime)) -
            static_cast<DWORD>(StartTime) : 0u;
        return std::bit_cast<std::int32_t>(static_cast<DWORD>(TimeElapsed) + passed);
    }
    void Pause() {
        if (IsTicking()) { TimeElapsed = GetTimeElapsed(); StartTime = -1; }
    }
    void Resume() { if (!IsTicking()) StartTime = static_cast<int>(CurrentTime); }
};
using SysElapsedTimerClass = ElapsedTimerStruct<SystemTimer>;
static_assert(sizeof(SysElapsedTimerClass) == 0xC);
static_assert(offsetof(SysElapsedTimerClass, TimeElapsed) == 0x8);

// Timer that counts down towards zero at specified rate, counted in frames.
class RateTimer : public CDTimerClass
{
public:
    int Rate { 0 };

    constexpr RateTimer() = default;
    RateTimer(int rate) { RateTimer::Start(rate); }

    void Start(int rate)
    {
        this->Rate = rate;
        this->CDTimerClass::Start(rate);
    }

    double GetRatePassed()
    {
        const int rate = this->Rate;
        return rate ? static_cast<double>(rate - this->GetTimeLeft()) / static_cast<double>(rate) : 1.0;
    }
};
