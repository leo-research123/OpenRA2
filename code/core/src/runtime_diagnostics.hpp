#pragma once
#include <cstdint>
#include "api/runtime_diagnostics.hpp"
class TagClass;
class Randomizer;

namespace game {
bool runtime_log_enabled() noexcept;
const char* runtime_log_session_directory() noexcept;
void runtime_log_resource(const char* filename) noexcept;
void runtime_log_rng(const Randomizer& scenario, const Randomizer& global) noexcept;
void runtime_log_event(const char* event, const char* format, ...) noexcept;
void runtime_log_map(const char* filename, int mode, int difficulty, const char* player, std::uint64_t world) noexcept;
void runtime_log_tick(std::uint64_t world, std::uint64_t tick, bool completed) noexcept;
void runtime_log_tag_created(const TagClass& tag) noexcept;
// These calls only consult copied metadata; the pointer is never dereferenced.
void runtime_log_tag(const char* event, const TagClass* tag, int kind, int index = -1, int count = -1) noexcept;
void runtime_log_tag_deleted(const TagClass* tag) noexcept;
}
