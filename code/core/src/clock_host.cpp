#include "clock.hpp"
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif
namespace game {
std::uint32_t default_clock_milliseconds() noexcept {
#ifdef _WIN32
    return timeGetTime();
#else
    return static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}
}
