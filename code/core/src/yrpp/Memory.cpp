// Existing YRpp 9402d7da Memory.h non-template implementation.
#include "yrpp/Memory.h"
#include <cerrno>
#include <cstdlib>
#include <limits>
#if defined(_WIN32)
#include <malloc.h>
#elif defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__GLIBC__)
#include <malloc.h>
#endif

#ifdef RA2_MEMORY_TESTING
#include "memory_testing.hpp"
#include <cstdint>
#include <unordered_set>

namespace {
std::size_t attempts = 0, fail_at = 0;
std::unordered_set<std::uintptr_t> allocations;
bool fail_allocation() {
    if (++attempts != fail_at) return false;
    errno = ENOMEM;
    return true;
}
void record_allocation(void* memory) {
    if (memory && !allocations.insert(reinterpret_cast<std::uintptr_t>(memory)).second)
        std::abort();
}
void forget_allocation(std::uintptr_t address) {
    if (address && allocations.erase(address) != 1) std::abort();
}
}
void game::fail_memory_allocation_after(std::size_t count) noexcept {
    fail_at = count ? attempts + count : 0;
}
std::size_t game::memory_allocation_attempts() noexcept { return attempts; }
std::size_t game::outstanding_memory_allocations() noexcept { return allocations.size(); }
#endif

namespace {
YRMemory::RecoverAllocation recover_allocation = nullptr;
YRMemory::ReadHeapNewMode read_heap_new_mode = nullptr;
std::size_t maximum_request = std::numeric_limits<std::size_t>::max();
bool recovery_configured = false;

void* allocate_with_recovery(std::size_t size, bool retry) noexcept {
    if (size > maximum_request) return nullptr;
    do {
        if (void* memory = YRMemory::AllocateOnce(size)) return memory;
    } while (retry && recover_allocation && recover_allocation(size));
    return nullptr;
}
}

bool YRMemory::ConfigureFailureRecovery(RecoverAllocation recover, ReadHeapNewMode heap_new_mode,
        size_t limit) noexcept {
    if (!limit) return false;
    if (recovery_configured)
        return recover_allocation == recover && read_heap_new_mode == heap_new_mode && maximum_request == limit;
    recover_allocation = recover;
    read_heap_new_mode = heap_new_mode;
    maximum_request = limit;
    recovery_configured = true;
    return true;
}

void* YRPP_CDECL YRMemory::Allocate(size_t size) {
    return allocate_with_recovery(size, true);
}

void* YRPP_CDECL YRMemory::AllocateBytes(size_t size) noexcept {
    return allocate_with_recovery(size, read_heap_new_mode && read_heap_new_mode());
}

void* YRPP_CDECL YRMemory::AllocateOnce(size_t size) noexcept {
#ifdef RA2_MEMORY_TESTING
    if (fail_allocation()) return nullptr;
#endif
    // Resolve directly to the linked CRT; never enter a hooked EXE address.
    void* memory = std::malloc(size ? size : 1);
#ifdef RA2_MEMORY_TESTING
    record_allocation(memory);
#endif
    return memory;
}

void YRPP_CDECL YRMemory::Deallocate(const void* memory) {
#ifdef RA2_MEMORY_TESTING
    forget_allocation(reinterpret_cast<std::uintptr_t>(memory));
#endif
    std::free(const_cast<void*>(memory));
}

void* YRPP_CDECL YRMemory::AllocateZeroed(size_t size) noexcept {
#ifdef RA2_MEMORY_TESTING
    if (fail_allocation()) return nullptr;
#endif
    void* memory = std::calloc(1, size ? size : 1);
#ifdef RA2_MEMORY_TESTING
    record_allocation(memory);
#endif
    return memory;
}

void* YRPP_CDECL YRMemory::Reallocate(void* memory, size_t size) noexcept {
    if (!memory) return AllocateOnce(size);
    if (!size) { Deallocate(memory); return nullptr; }
#ifdef RA2_MEMORY_TESTING
    if (fail_allocation()) return nullptr;
    const auto address = reinterpret_cast<std::uintptr_t>(memory);
#endif
    void* resized = std::realloc(memory, size);
#ifdef RA2_MEMORY_TESTING
    if (resized) { forget_allocation(address); record_allocation(resized); }
#endif
    return resized;
}

bool YRPP_CDECL YRMemory::TryGetAllocationSize(const void* memory, size_t& size) noexcept {
    size = 0;
#if defined(_WIN32)
    // Preserve _msize's invalid-parameter handling, including null, on Windows.
    const auto result = _msize(const_cast<void*>(memory));
    if (result == static_cast<std::size_t>(-1)) return false;
    size = result;
#elif defined(__APPLE__)
    if (!memory) return false;
    size = malloc_size(memory);
#elif defined(__GLIBC__)
    if (!memory) return false;
    size = malloc_usable_size(const_cast<void*>(memory));
#else
    (void)memory;
    return false;
#endif
    return true;
}

#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
void* YRMemory::AllocateChecked(size_t size) {
    if (auto* memory = YRMemory::Allocate(size)) return memory;
    std::exit(static_cast<int>(0x30000000u | size));
}
