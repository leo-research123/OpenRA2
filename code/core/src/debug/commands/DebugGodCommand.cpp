#include "debug/debug_command.hpp"
#include "debug/debug_state.hpp"
#include "map_debug_commands.hpp"
namespace game {
class DebugGodCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments&) const noexcept override {
        return reader.finish();
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        return execute_selected_debug_objects(
            context, arguments, debug_live_techno_filter,
            [](DebugCommandContext& context, ObjectClass& object, const DebugCommandArguments&) {
                if (!set_debug_god_overrides(context.world, object)) return DebugTargetFailure::unsupported_type;
                if (!apply_debug_level(object, 2) || !apply_debug_unit_health(context.world, object, 1000000))
                    return DebugTargetFailure::dead_object;
                return DebugTargetFailure::none;
            },
            DebugTargetLifetime::scoped_pointer);
    }

private:
    static constexpr DebugCommandDescriptor metadata{
        DebugCommandId::god,
        DebugArgumentKind::none,
        0,
        0,
        "god",
        "god",
        "Set selected combat objects to level 2, HP 1000000, attack 1000000 and mind-control immunity.",
        "Applied god attributes to {count} selected objects."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugGodCommand);
}
