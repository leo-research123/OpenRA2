#pragma once

#include "api/debug_commands.hpp"

class ObjectClass;
namespace game {
class MapWorld;

// Internal command arguments never cross the core/host ABI. Text is owned by
// this request; offsets remain valid when the argument bundle is copied.
enum class DebugValueKind { integer, boolean, text, real };
struct DebugArgumentValue {
    DebugValueKind kind = DebugValueKind::integer;
    std::int32_t integer = 0;
    std::uint32_t offset = 0, length = 0;
    double real = 0;
};
struct DebugCommandArguments {
    static constexpr std::uint32_t capacity = 16;
    DebugArgumentValue values[capacity]{};
    std::uint32_t count = 0, text_length = 0;
    char text[debug_command_max_bytes + 1]{};

    bool get_integer(std::uint32_t, std::int32_t& output) const noexcept;
    bool get_boolean(std::uint32_t, bool& output) const noexcept;
    bool get_text(std::uint32_t, const char*& output, std::uint32_t& length) const noexcept;
    bool get_real(std::uint32_t, double& output) const noexcept;
};

class DebugArgumentReader {
public:
    DebugArgumentReader(const char* text, std::uint32_t length) noexcept;
    DebugCommandStatus read_integer(DebugCommandArguments&, std::int32_t minimum, std::int32_t maximum) noexcept;
    DebugCommandStatus read_boolean(DebugCommandArguments&) noexcept;
    DebugCommandStatus read_text(DebugCommandArguments&) noexcept;
    DebugCommandStatus read_real(DebugCommandArguments&, double minimum, double maximum) noexcept;
    DebugCommandStatus finish() const noexcept;

private:
    bool token(char* output, std::uint32_t& length, bool& quoted) noexcept;
    const char* text_;
    std::uint32_t length_, position_ = 0;
};

struct DebugCommandContext {
    MapWorld& world;
    DebugCommandResult& result;
    bool writes_started = false;
    void record_failure(MapObjectId, DebugTargetFailure) noexcept;
};

// This interface is private to the standalone core. Existing CommandClass
// interfaces, virtual tables and original input semantics remain independent.
class DebugConsoleCommand {
public:
    virtual ~DebugConsoleCommand() = default;
    virtual const DebugCommandDescriptor& descriptor() const noexcept = 0;
    virtual DebugCommandStatus parse(DebugArgumentReader&, DebugCommandArguments&) const noexcept = 0;
    // Recoverable exceptions are contained by the synchronous core entry.
    virtual DebugCommandStatus execute(DebugCommandContext&, const DebugCommandArguments&) const = 0;
};

// Registrations have static storage in production and perform no allocation.
// Command objects must outlive their registration. Mutation after startup is
// reserved for serialized tests; command execution never changes the registry.
class DebugCommandRegistration {
public:
    explicit DebugCommandRegistration(const DebugConsoleCommand&) noexcept;
    ~DebugCommandRegistration();
    DebugCommandRegistration(const DebugCommandRegistration&) = delete;
    DebugCommandRegistration& operator=(const DebugCommandRegistration&) = delete;
    const DebugConsoleCommand& command;
    DebugCommandRegistration* next = nullptr;
};

bool debug_command_registry_valid() noexcept;
const DebugConsoleCommand* find_debug_command(const char*, std::uint32_t) noexcept;

enum class DebugTargetLifetime { scoped_pointer, resolve_each_target };
using DebugTargetFilter = DebugTargetFailure (*)(ObjectClass&);
using DebugTargetApply = DebugTargetFailure (*)(DebugCommandContext&, ObjectClass&, const DebugCommandArguments&);
DebugTargetFailure debug_live_unit_filter(ObjectClass&);
DebugTargetFailure debug_live_techno_filter(ObjectClass&);
DebugTargetFailure debug_live_health_filter(ObjectClass&);
DebugCommandStatus execute_selected_debug_objects(DebugCommandContext&, const DebugCommandArguments&, DebugTargetFilter,
                                                  DebugTargetApply, DebugTargetLifetime);
bool debug_current_player_available() noexcept;

// Put this macro after the complete command class, inside namespace game.
#define GAME_REGISTER_DEBUG_COMMAND(Type)                                                                              \
    static const Type registered_##Type;                                                                               \
    static ::game::DebugCommandRegistration registration_##Type(registered_##Type)
}
