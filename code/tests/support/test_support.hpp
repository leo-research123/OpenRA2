#pragma once

#include <gtest/gtest.h>
#include <optional>
#include <tuple>
#include <utility>

namespace ra2::test {

struct Arguments {
    int argc;
    char** argv;
};
struct WideArguments {
    int argc;
    wchar_t** argv;
};
Arguments arguments() noexcept;
WideArguments wide_arguments() noexcept;
bool gtest_requested() noexcept;

// Diagnostic commands return an exit code; nullopt continues to GoogleTest.
// Registration never executes the command during static initialization.
using CommandHandler = std::optional<int> (*)(int, char**);
class CommandRegistration {
public:
    explicit CommandRegistration(CommandHandler handler);
};

// Most old check() helpers threw immediately, including from non-void callbacks.
// A shared GoogleTest event listener preserves that unwind behavior. Suites
// which originally accumulated failures opt out with this scoped guard.
class ContinueAfterFailure {
public:
    ContinueAfterFailure() noexcept;
    ~ContinueAfterFailure();
    ContinueAfterFailure(const ContinueAfterFailure&) = delete;
    ContinueAfterFailure& operator=(const ContinueAfterFailure&) = delete;
private:
    bool previous_;
};

// Keep cleanup that used to live after a main-level catch active on both the
// success and failure paths. Copying is deliberately disabled.
template<class F>
class ScopeExit {
public:
    explicit ScopeExit(F action) : action_(std::move(action)) {}
    ~ScopeExit() noexcept { action_(); }
    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
private:
    F action_;
};
template<class F>
[[nodiscard]] ScopeExit<F> scope_exit(F action) {
    return ScopeExit<F>(std::move(action));
}

int run(int argc, char** argv);
int run(int argc, wchar_t** argv);

} // namespace ra2::test
