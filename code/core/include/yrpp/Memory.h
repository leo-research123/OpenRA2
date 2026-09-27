#pragma once

// Fixed YRpp 9402d7da Memory.h. Templates retain the upstream allocator model.
// Non-template operations are owned by core/src/yrpp/Memory.cpp in every target.
#include "yrpp/platform/ABI.h"
#include <cstddef>
#include <cstring>

#include <stdlib.h>

#include <memory>
#include <type_traits>
#include <utility>
#include <concepts>

/*
 * The memory (de)allocators have to match!
 * Do not allocate memory in the DLL and hand it to the game to deallocate, or vice versa.
 * Kapiche?

 * A simple |auto foo = new IngameClass();| allocates memory from the DLL's pool.
 * But |delete foo;| deallocates memory from the game's own pool. (assuming the class you're freeing has a virtual SDDTOR)

 * So use the macros to make sure game classes go to the game's pool.
 * The custom classes like ExtMap do not need this treatment so you can use the plain old new/delete on them.

 * For the ObjectClass derivates, if you use the game's built-in allocators like | Type->CreateObject() | ,
 * you can use plain | delete |;

 */

/*
 * OK, new plan - the game's operator new/delete has been hooked to redirect to the DLL's
 * so GAME_(DE)ALLOC is now just a wrapper. Don't remove it though, just in case this fails
 * and I need to run those allocations differently.
 */

/*
 * Newer plan - previous hook screwed performance, so going back
 */

/*
* Yet a newer plan - use variadic templates
*/

// Existing YRpp interface. No original address or inline assembly in public headers.
namespace YRMemory {
    using RecoverAllocation = bool (YRPP_CDECL *)(size_t requested) noexcept;
    using ReadHeapNewMode = bool (YRPP_CDECL *)() noexcept;
    // Configure each core instance during serial startup, before concurrent use.
    // Callbacks must remain valid for its lifetime. Identical registration is
    // idempotent; a different registration or a zero limit fails without changes.
    // No allocator can be supplied here. Defaults: no recovery, SIZE_MAX limit.
    bool ConfigureFailureRecovery(RecoverAllocation recover, ReadHeapNewMode heap_new_mode,
        size_t maximum_request) noexcept;
    // One CRT attempt; zero requests allocate one byte; failure returns null.
    // No recovery or policy limit.
    void* YRPP_CDECL AllocateOnce(size_t sz) noexcept;
    // New-style allocation: enforce the configured limit and retry while recovery
    // permits. The callback receives the original size, including zero.
    // Final failure returns null.
    void* YRPP_CDECL Allocate(size_t sz);
    // Malloc-style allocation: snapshot new-mode at entry (absent means off).
    void* YRPP_CDECL AllocateBytes(size_t sz) noexcept;
    void YRPP_CDECL Deallocate(const void* mem);
    // Complete Allocate's recovery first; final failure exits with 0x30000000 | sz.
    void* AllocateChecked(size_t sz);
    // One CRT attempt, byte count only. Original 32-bit calloc policy stays in compat.
    void* YRPP_CDECL AllocateZeroed(size_t sz) noexcept;
    // One CRT attempt: null uses AllocateOnce; non-null with size zero frees;
    // failure preserves the old block. Original realloc retry stays in compat.
    void* YRPP_CDECL Reallocate(void* mem, size_t sz) noexcept;
    // Requires a live allocation from this CRT. No pointer-validity probe is implied.
    // On failure size is zero; platforms without a CRT size query return false.
    bool YRPP_CDECL TryGetAllocationSize(const void* mem, size_t& size) noexcept;
}
template<typename T>
concept needs_vector_delete = !std::is_scalar_v<T> && !std::is_trivially_destructible_v<T>;

// this is a stateless basic allocator definition that manages memory using the
// game's operator new and operator delete methods. do not use it directly,
// though. use std::allocator_traits, which will fill in the blanks.

template<typename T>
concept CanUseGameAlloc = !requires { { T::GameCreateDisallowed } ->std::convertible_to<const bool>; } || T::GameCreateDisallowed == false;

template <CanUseGameAlloc T>
struct GameAllocator {
    using value_type = T;

    constexpr GameAllocator() noexcept = default;

    template <typename U>
    constexpr GameAllocator(const GameAllocator<U>&) noexcept { }

    constexpr bool operator == (const GameAllocator&) const noexcept { return true; }
    constexpr bool operator != (const GameAllocator&) const noexcept { return false; }

    T* allocate(const size_t count) const noexcept
    {
        return static_cast<T*>(YRMemory::AllocateChecked(count * sizeof(T)));
    }

    void deallocate(T* const ptr, size_t count) const noexcept
    {
        YRMemory::Deallocate(ptr);
    }
};

// construct or destroy objects using an allocator.
class Memory {
public:
    // construct scalars
    template <typename T, typename TAlloc, typename... TArgs>
    static inline T* Create(TAlloc& alloc, TArgs&&... args)
    {
        auto const ptr = std::allocator_traits<TAlloc>::allocate(alloc, 1);
        std::allocator_traits<TAlloc>::construct(alloc, ptr, std::forward<TArgs>(args)...);
        return ptr;
    };

    // destruct scalars
    template<typename T, typename TAlloc>
    static inline void Delete(TAlloc& alloc, T* ptr)
    {
        if (ptr)
        {
            std::allocator_traits<TAlloc>::destroy(alloc, ptr);
            std::allocator_traits<TAlloc>::deallocate(alloc, ptr, 1);
        }
    };

    // construct vectors
    template <typename T, typename TAlloc, typename... TArgs>
    static inline T* CreateArray(TAlloc& alloc, size_t capacity, TArgs&&... args)
    {
        auto const ptr = std::allocator_traits<TAlloc>::allocate(alloc, capacity);
        if (capacity && !sizeof...(args) && std::is_scalar<T>::value)
        {
            // set to 0
            std::memset(ptr, 0, capacity * sizeof(T));
        }
        else
        {
            for (size_t i = 0; i < capacity; ++i)
            {
                // use args... here. can't move args, because we need to reuse them
                std::allocator_traits<TAlloc>::construct(alloc, &ptr[i], args...);
            }
        }
        return ptr;
    }

    // destruct vectors
    template<typename T, typename TAlloc>
    static inline void DeleteArray(TAlloc& alloc, T* ptr, size_t capacity)
    {
        if (ptr)
        {
            // call the destructor if required
            if (capacity && !std::is_trivially_destructible<T>::value)
            {
                for (size_t i = 0; i < capacity; ++i)
                    std::allocator_traits<TAlloc>::destroy(alloc, &ptr[i]);
            }

            std::allocator_traits<TAlloc>::deallocate(alloc, ptr, capacity);
        }
    };
};

// helper methods as free functions.

template <typename T, typename... TArgs>
requires std::constructible_from<T, TArgs...>
static inline T* GameCreate(TArgs&&... args)
{

    GameAllocator<T> alloc;
    return Memory::Create<T>(alloc, std::forward<TArgs>(args)...);
}

template<typename T>
static inline void GameDelete(T* ptr)
{
    GameAllocator<T> alloc;
    Memory::Delete(alloc, ptr);
}

template <typename T, typename... TArgs>
requires std::constructible_from<T, TArgs...>
static inline T* GameCreateArray(size_t capacity, TArgs&&... args)
{

    GameAllocator<T> alloc;
    return Memory::CreateArray<T>(alloc, capacity, std::forward<TArgs>(args)...);
}

template<typename T>
static inline void GameDeleteArray(T* ptr, size_t capacity)
{
    GameAllocator<T> alloc;
    Memory::DeleteArray(alloc, ptr, capacity);
}

template <typename T, typename... TArgs>
requires std::constructible_from<T, TArgs...>
static inline T* DLLCreate(TArgs&&... args)
{

    std::allocator<T> alloc;
    return Memory::Create<T>(alloc, std::forward<TArgs>(args)...);
}

template<typename T>
static inline void DLLDelete(T* ptr)
{
    std::allocator<T> alloc;
    Memory::Delete(alloc, ptr);
}

template <typename T, typename... TArgs>
requires std::constructible_from<T, TArgs...>
static inline T* DLLCreateArray(size_t capacity, TArgs&&... args)
{
    std::allocator<T> alloc;
    return Memory::CreateArray<T>(alloc, capacity, std::forward<TArgs>(args)...);
}

template<typename T>
static inline void DLLDeleteArray(T* ptr, size_t capacity)
{
    std::allocator<T> alloc;
    Memory::DeleteArray(alloc, ptr, capacity);
}

struct GameDeleter
{
    template <typename T>
    void operator ()(T* ptr) noexcept
    {
        if (ptr)
            GameDelete(ptr);
    }
};

//#define GAME_ALLOC(TT, var, ...) \
//	var = GameCreate<TT>(__VA_ARGS__);
//
//#define GAME_DEALLOC(var) \
//	GameDelete(var);
//
//#define GAME_ALLOC_ARR(TT, Capacity, var) \
//	var = GameCreateArray<TT>(Capacity);
