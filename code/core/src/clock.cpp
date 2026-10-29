#include "clock.hpp"
#include <cmath>
#include <cfenv>
namespace game {
namespace {
// Constant initialization precedes every original global constructor.
constinit double virtual_milliseconds = 0;
thread_local const ClockServices* active_clock = nullptr;
thread_local unsigned read_scope_depth = 0;
}
void enter_clock_read_scope() noexcept { ++read_scope_depth; }
void leave_clock_read_scope() noexcept { --read_scope_depth; }
std::uint32_t clock_milliseconds() noexcept {
    if (active_clock) return active_clock->milliseconds(active_clock->context);
#if defined(RA2_YRPP_GAME)
    // In-process replacements share the EXE's timeGetTime authority. Installing
    // a virtual clock there requires replacing the EXE driver and reader too.
    return default_clock_milliseconds();
#else
    return static_cast<std::uint32_t>(virtual_milliseconds);
#endif
}
bool advance_clock(double seconds) noexcept {
    if (read_scope_depth || !std::isfinite(seconds) || seconds < 0) return false;
    // Time conversion is a host boundary, including an injected clock.
    struct RestoreRounding {
        int previous = std::fegetround();
        RestoreRounding() {
            if (previous != -1) std::fesetround(FE_TONEAREST);
        }
        ~RestoreRounding() {
            if (previous != -1) std::fesetround(previous);
        }
    } rounding;
    if (active_clock)
        return active_clock->advance ? active_clock->advance(active_clock->context, seconds) : seconds == 0;
#if defined(RA2_YRPP_GAME)
    return seconds == 0;
#else
    // Preserve sub-millisecond fractions and reduce before multiplication so
    // even a finite, very large delta cannot overflow the accumulator.
    // Host wall time must not inherit the rounding mode changed by _ftol.
    virtual_milliseconds = std::fmod(virtual_milliseconds + std::fmod(seconds, 4294967.296) * 1000.0, 4294967296.0);
    return true;
#endif
}
bool with_clock(const ClockServices& services, void (*operation)(void*), void* context) {
    if (!services.milliseconds || !operation) return false;
    struct Restore {
        const ClockServices* previous;
        ~Restore() { active_clock = previous; }
    } restore{active_clock};
    active_clock = &services;
    operation(context);
    return true;
}
}
