#include "debug/debug_command.hpp"
#include "map_debug_commands.hpp"

namespace game {
class DebugHpCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.read_integer(arguments, debug_health_minimum, debug_health_maximum);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        std::int32_t health = 0;
        arguments.get_integer(0, health);
        return execute_selected_debug_objects(
            context, arguments, debug_live_health_filter,
            [](DebugCommandContext& context, ObjectClass& object, const DebugCommandArguments& values) {
                std::int32_t health = 0;
                values.get_integer(0, health);
                return apply_debug_unit_health(context.world, object, health) ? DebugTargetFailure::none
                                                                              : DebugTargetFailure::dead_object;
            },
            health == 0 ? DebugTargetLifetime::resolve_each_target : DebugTargetLifetime::scoped_pointer);
    }

private:
    static constexpr DebugCommandDescriptor metadata{
        DebugCommandId::hp,
        DebugArgumentKind::integer,
        debug_health_minimum,
        debug_health_maximum,
        "hp",
        "hp <0..1000000>",
        "Set selected object health, including buildings; zero retires objects.",
        "Updated the health of {count} selected objects."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugHpCommand);
}
