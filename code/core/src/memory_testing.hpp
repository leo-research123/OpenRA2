#pragma once
#ifndef RA2_MEMORY_TESTING
#error "Memory fault injection is available only in explicit test builds"
#endif
#include <cstddef>

// Private instrumentation of Memory.h's real CRT implementation, not an
// allocator provider. No production target defines RA2_MEMORY_TESTING.
namespace game {
// Fail exactly the nth upcoming allocation attempt; zero disables injection.
void fail_memory_allocation_after(std::size_t count) noexcept;
std::size_t memory_allocation_attempts() noexcept;
// Test builds also abort on a foreign or duplicate release.
std::size_t outstanding_memory_allocations() noexcept;
}
