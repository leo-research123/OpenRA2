#pragma once
#include "api/clock.hpp"
namespace game {
std::uint32_t clock_milliseconds() noexcept;
std::uint32_t default_clock_milliseconds() noexcept;
// Host operations serialize access to original global objects. While an
// operation is executing, a callback must not advance the authority again.
void enter_clock_read_scope() noexcept;
void leave_clock_read_scope() noexcept;
struct ClockReadScope {
    ClockReadScope() noexcept { enter_clock_read_scope(); }
    ~ClockReadScope() { leave_clock_read_scope(); }
    ClockReadScope(const ClockReadScope&) = delete;
    ClockReadScope& operator=(const ClockReadScope&) = delete;
};
}
