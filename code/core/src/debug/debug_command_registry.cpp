#include "debug/debug_command.hpp"
#include "runtime_diagnostics.hpp"
#include <cstring>

namespace game {
struct DebugRegistryState {
    DebugCommandRegistration* first = nullptr;
    std::uint64_t revision = 1, validated_revision = 0;
    std::uint32_t count = 0;
    bool valid = false;
};

static DebugRegistryState& registry() noexcept {
    static DebugRegistryState state;
    return state;
}

DebugCommandRegistration::DebugCommandRegistration(const DebugConsoleCommand& item) noexcept : command(item) {
    auto& state = registry();
    auto** position = &state.first;
    const auto id = static_cast<std::uint32_t>(command.descriptor().id);
    while (*position && static_cast<std::uint32_t>((*position)->command.descriptor().id) < id)
        position = &(*position)->next;
    next = *position;
    *position = this;
    ++state.count;
    ++state.revision;
}

DebugCommandRegistration::~DebugCommandRegistration() {
    auto& state = registry();
    auto** position = &state.first;
    while (*position && *position != this)
        position = &(*position)->next;
    if (*position) {
        *position = next;
        --state.count;
        ++state.revision;
    }
}

static bool valid_descriptor(const DebugCommandDescriptor& descriptor) noexcept {
    if (descriptor.id == DebugCommandId::none || !descriptor.name[0] ||
        !std::memchr(descriptor.name, 0, sizeof(descriptor.name)) ||
        !std::memchr(descriptor.usage, 0, sizeof(descriptor.usage)) ||
        !std::memchr(descriptor.description, 0, sizeof(descriptor.description)) ||
        !std::memchr(descriptor.success_message, 0, sizeof(descriptor.success_message)))
        return false;
    for (const char* character = descriptor.name; *character; ++character) {
        if (!(*character >= 'a' && *character <= 'z') && !(*character >= '0' && *character <= '9') &&
            *character != '_' && *character != '-' && *character != '.')
            return false;
    }
    return std::strcmp(descriptor.name, "help") && std::strcmp(descriptor.name, "clear") &&
           (descriptor.argument == DebugArgumentKind::none ||
            (descriptor.argument >= DebugArgumentKind::integer &&
             descriptor.argument <= DebugArgumentKind::integer_or_reset && descriptor.minimum <= descriptor.maximum));
}

bool debug_command_registry_valid() noexcept {
    auto& state = registry();
    if (state.validated_revision == state.revision) return state.valid;
    state.validated_revision = state.revision;
    state.valid = false;
    for (auto* entry = state.first; entry; entry = entry->next) {
        const auto& descriptor = entry->command.descriptor();
        if (!valid_descriptor(descriptor)) {
            runtime_log_event("command_registry_failure", "reason=invalid_descriptor id=%u name=%.*s",
                              static_cast<std::uint32_t>(descriptor.id), static_cast<int>(sizeof(descriptor.name)),
                              descriptor.name);
            return false;
        }
        for (auto* previous = state.first; previous != entry; previous = previous->next) {
            const auto& other = previous->command.descriptor();
            if (descriptor.id == other.id || !std::strcmp(descriptor.name, other.name)) {
                runtime_log_event("command_registry_failure", "reason=%s id=%u name=%s other=%s",
                                  descriptor.id == other.id ? "duplicate_id" : "duplicate_name",
                                  static_cast<std::uint32_t>(descriptor.id), descriptor.name, other.name);
                return false;
            }
        }
    }
    state.valid = true;
    return true;
}

std::uint32_t debug_command_count() noexcept { return debug_command_registry_valid() ? registry().count : 0; }

bool get_debug_command_descriptor(std::uint32_t index, DebugCommandDescriptor& output) noexcept {
    if (!debug_command_registry_valid()) return false;
    auto* entry = registry().first;
    for (; entry && index; --index)
        entry = entry->next;
    if (!entry) return false;
    output = entry->command.descriptor();
    return true;
}

const DebugConsoleCommand* find_debug_command(const char* text, std::uint32_t length) noexcept {
    if (!text || !debug_command_registry_valid()) return nullptr;
    for (auto* entry = registry().first; entry; entry = entry->next) {
        const char* name = entry->command.descriptor().name;
        if (std::strlen(name) != length) continue;
        std::uint32_t index = 0;
        for (; index < length; ++index) {
            const char lower = text[index] >= 'A' && text[index] <= 'Z' ? text[index] + ('a' - 'A') : text[index];
            if (lower != name[index]) break;
        }
        if (index == length) return &entry->command;
    }
    return nullptr;
}
}
