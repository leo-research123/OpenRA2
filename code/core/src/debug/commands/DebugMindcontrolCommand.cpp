#include "debug/debug_command.hpp"
#include "debug/debug_state.hpp"
#include "yrpp/TechnoClass.h"

namespace game {
class DebugMindcontrolCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.finish() == DebugCommandStatus::success ? DebugCommandStatus::success
                                                              : reader.read_integer(arguments, 0, 1);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        return execute_selected_debug_objects(
            context, arguments, debug_live_techno_filter,
            [](DebugCommandContext& context, ObjectClass& object, const DebugCommandArguments& arguments) {
                // Toggle only the extra immunity, never the shared type or existing control relations.
                int allowed = debug_mind_control_immune(static_cast<TechnoClass*>(&object)) ? 1 : 0;
                arguments.get_integer(0, allowed);
                return set_debug_mind_control_allowed(context.world, object, allowed != 0)
                           ? DebugTargetFailure::none
                           : DebugTargetFailure::dead_object;
            },
            DebugTargetLifetime::scoped_pointer);
    }

private:
    static constexpr DebugCommandDescriptor metadata{
        DebugCommandId::mindcontrol,
        DebugArgumentKind::optional_integer,
        0,
        1,
        "mindcontrol",
        "mindcontrol [0|1]",
        "Toggle selected objects' extra mind-control immunity; 0 blocks capture, 1 restores original rules.",
        "Updated extra mind-control immunity for {count} selected objects; original restrictions remain."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugMindcontrolCommand);
}
