#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

// defines a compile time pointer to a known memory address
template <typename T, unsigned int Address>
struct [[deprecated("Use the DEFINE_POINTER macro.")]] constant_ptr {
    using value_type = T*;

    constexpr constant_ptr() noexcept = default;
    constant_ptr(constant_ptr&) = delete;

private:
    value_type get() const noexcept {
        return reinterpret_cast<value_type>(Address);
    }
public:
    operator value_type() const noexcept {
        return get();
    }

    value_type operator()() const noexcept {
        return get();
    }

    value_type operator->() const noexcept {
        return get();
    }

    T& operator*() const noexcept {
        return *get();
    }
};

// defines a compile time reference to a known memory address
template <typename T, unsigned int Address, size_t Count = 0>
struct [[deprecated("Use the DEFINE_REFERENCE macro.")]] reference {
    using value_type = T[Count];

    static const auto Size = Count;
    static_assert(!std::is_reference_v<T> && (std::is_const_v<T> || (Address > 0x812000)));
    constexpr reference() noexcept = default;
    reference(reference&) = delete;

private:
    value_type& get() const noexcept {
        // fixes" C2101: '&' on constant
        static auto const address = Address;
        return *reinterpret_cast<value_type*>(address);
    }
public:
    operator value_type&() const noexcept {
        return get();
    }

    value_type& operator()() const noexcept {
        return get();
    }

    decltype(auto) operator&() const noexcept {
        return &get();
    }

    decltype(auto) operator*() const noexcept {
        return *get();
    }

    T& operator[](int index) const noexcept {
        return get()[index];
    }

    value_type& data() const noexcept {
        return get();
    }

    size_t size() const noexcept {
        return Size;
    }

    T* begin() const noexcept {
        return data();
    }

    T* end() const noexcept {
        return begin() + Size;
    }
};

// specializations for non-array references
template <typename T, unsigned int Address>
struct [[deprecated("Use the DEFINE_REFERENCE macro.")]] reference<T, Address, 0> {
    using value_type = T;

    constexpr reference() noexcept = default;
    reference(reference&) = delete;
    static_assert(!std::is_reference_v<T> && (std::is_const_v<T> || (Address > 0x812000)));
private:
    value_type& get() const noexcept {
        return *reinterpret_cast<value_type*>(Address);
    }
public:
    template <typename T2>	requires std::assignable_from<T&,T2>
        T& operator=(T2&& rhs) const
    {
        return get() = std::forward<T2>(rhs);
    }

    operator T&() const noexcept {
        return get();
    }

    T& operator()() const noexcept {
        return get();
    }

    decltype(auto) operator&() const noexcept {
        return &get();
    }

    decltype(auto) operator->() const noexcept {
        if constexpr (std::is_pointer_v<T>)
            return get();
        else
            return &get();
    }

    decltype(auto) operator*() const noexcept {
        return *get();
    }

    decltype(auto) operator[](int index) const noexcept {
        return get()[index];
    }
};

/*
 *	This set of macros is necessary because a type argument can contain commas, which would split it into multiple arguments.
 *	To circumvent this, we can enclose the argument in parentheses.
 *	However, parentheses are significant, so we then need a macro to remove them so as
 *	to not affect our final result.
 *	Source: https://groups.google.com/a/isocpp.org/g/std-proposals/c/Ngl_vTAdddA
 */
#define UNPAREN( ... ) UNPAREN __VA_ARGS__
#define DONE_UNPAREN

#define CAT_LIT(A, ...) A ## __VA_ARGS__
#define CAT(A, ...) CAT_LIT(A, __VA_ARGS__)
#define UIP(...) CAT( DONE_, UNPAREN __VA_ARGS__  )

/*
 *	Use these macros to define a reference to an address in the game's memory.
 */
#define DEFINE_NONSTATIC_REFERENCE(type, name, address) UIP(type) (&name) = *reinterpret_cast<UIP(type)*>(address);
#define DEFINE_REFERENCE(type, name, address) static inline DEFINE_NONSTATIC_REFERENCE(type, name, address);
#define DEFINE_NONSTATIC_ARRAY_REFERENCE(type, dimensions, name, address) UIP(type) (&name)dimensions = *reinterpret_cast<UIP(type) (*)dimensions>(address);
#define DEFINE_ARRAY_REFERENCE(type, dimensions, name, address) static inline DEFINE_NONSTATIC_ARRAY_REFERENCE(type, dimensions, name, address);

/*
 *	Use these macros to define a pointer to an address in the game's memory.
 *	Pretty much only useful for strings that exist in the executable,
 *	for everything else, prefer references.
 */
#define DEFINE_NONSTATIC_POINTER(type, name, address) UIP(type)* const (name) = reinterpret_cast<UIP(type)*>(address);
#define DEFINE_POINTER(type, name, address) static inline DEFINE_NONSTATIC_POINTER(type, name, address);
