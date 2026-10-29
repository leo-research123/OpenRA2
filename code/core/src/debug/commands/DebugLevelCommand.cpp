#include "debug/debug_command.hpp"
#include "map_debug_commands.hpp"

namespace game {
class DebugLevelCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.read_integer(arguments, debug_level_minimum, debug_level_maximum);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        return execute_selected_debug_objects(
            context, arguments, debug_live_techno_filter,
            [](DebugCommandContext&, ObjectClass& object, const DebugCommandArguments& values) {
                std::int32_t level = 0;
                values.get_integer(0, level);
                return apply_debug_level(object, level) ? DebugTargetFailure::none
                                                        : DebugTargetFailure::unsupported_type;
            },
            DebugTargetLifetime::scoped_pointer);
    }

private:
    static constexpr DebugCommandDescriptor metadata{
        DebugCommandId::level,
        DebugArgumentKind::integer,
        debug_level_minimum,
        debug_level_maximum,
        "level",
        "level <0..2>",
        "Set selected objects, including buildings, to rookie (0), veteran (1) or elite (2).",
        "Updated the rank of {count} selected objects."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugLevelCommand);
}
