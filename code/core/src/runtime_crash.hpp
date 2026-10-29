#pragma once
#include "api/runtime_diagnostics.hpp"
namespace game {
bool prepare_runtime_crash_session(const char* directory) noexcept;
void runtime_crash_sequence(std::uint64_t sequence, std::uint64_t world, std::uint64_t tick) noexcept;
}
