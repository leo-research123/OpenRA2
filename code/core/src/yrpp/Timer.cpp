#include "yrpp/Timer.h"
#include "clock.hpp"
DWORD SystemTimer::GetMilliseconds() noexcept { return game::clock_milliseconds(); }
DWORD SystemTimer::GetTime() { return GetMilliseconds() >> 4; }
