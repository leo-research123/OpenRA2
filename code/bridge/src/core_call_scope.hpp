#pragma once
#include <cfenv>

namespace game {
// Original _ftol-compatible algorithms change the rounding mode. Preserve
// those changes within the call, but never leak them into Godot or the CLR.
class CoreCallScope {
    std::fenv_t environment_{};
    bool saved_ = std::fegetenv(&environment_) == 0;

public:
    CoreCallScope() noexcept = default;
    ~CoreCallScope() {
        if (saved_) std::fesetenv(&environment_);
    }
    CoreCallScope(const CoreCallScope&) = delete;
    CoreCallScope& operator=(const CoreCallScope&) = delete;
};
}
