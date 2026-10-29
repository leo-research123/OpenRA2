#pragma once
#include <utility>

namespace ra2::test {
// Ordinary C++ scope guards for project contract fixtures.
template <class T>
struct ScopedValue {
    T& value;
    T previous;
    explicit ScopedValue(T& value) : value(value), previous(value) {}
    ScopedValue(T& value, const T& initial) : value(value), previous(std::exchange(value, initial)) {}
    ~ScopedValue() { value = previous; }
    ScopedValue(const ScopedValue&) = delete;
    ScopedValue& operator=(const ScopedValue&) = delete;
};
template <class F>
class ScopeExit {
public:
    explicit ScopeExit(F action) : action_(std::move(action)) {}
    ~ScopeExit() noexcept { action_(); }
    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
private:
    F action_;
};
template <class F>
[[nodiscard]] ScopeExit<F> scope_exit(F action) { return ScopeExit<F>(std::move(action)); }
}
