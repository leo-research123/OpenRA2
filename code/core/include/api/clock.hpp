#pragma once
#include <cstdint>

namespace game {
// One authority for the lifetime of the standalone core, including static
// original-object construction. The host advances it between updates; reads
// never advance it. Zero pauses time. Negative/non-finite deltas are rejected.
// Serialize calls on the game thread. Advancement during a core update is
// rejected. Map/session replacement never resets this process-lifetime clock.
// Output has the original timeGetTime 32-bit wrap, before SystemTimer's >> 4.
std::uint32_t clock_milliseconds() noexcept;
bool advance_clock(double seconds) noexcept;

// Borrowed for one synchronous operation. The callback returns the same
// wrapping millisecond counter as Win32 timeGetTime, before the game's >> 4.
struct ClockServices {
    void* context = nullptr;
    std::uint32_t (*milliseconds)(void*) noexcept = nullptr;
    // Test-only progression. A read-only fixture accepts only a zero delta.
    bool (*advance)(void*, double seconds) noexcept = nullptr;
};

// Isolated test fixtures only: construct and destroy all clock consumers
// inside this scope. Production map/session operations never replace time.
// Invalid callbacks return false without running the operation. Nested calls
// restore the previous clock, including when the operation throws. The caller
// advances the original Unsorted::CurrentFrame independently of this clock.
bool with_clock(const ClockServices& services, void (*operation)(void*), void* context);
}
