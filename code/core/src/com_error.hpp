#pragma once

#include <cstdint>
#include <cstdlib>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <comdef.h>
#endif

namespace game {

// Internal failure transport only. Resource/host entry points catch this
// before returning a failure status; it must not cross a module boundary.
#if defined(_WIN32)
using ComError = _com_error;

[[noreturn]] inline void raise_com_error(std::int32_t code) { _com_issue_error(static_cast<HRESULT>(code)); }
#else
class ComError final {
public:
    explicit ComError(std::int32_t code) noexcept : code_(code) {}
    std::int32_t Error() const noexcept { return code_; }

private:
    std::int32_t code_;
};

[[noreturn]] inline void raise_com_error(std::int32_t code) { throw ComError(code); }
#endif

// Constructors retain their noexcept boundary. Preserve the COM error inside
// the core, then terminate locally rather than leaking it into a host module.
[[noreturn]] inline void terminate_com_error(std::int32_t code) noexcept {
    try {
        raise_com_error(code);
    } catch (...) {
        std::abort();
    }
}

}
