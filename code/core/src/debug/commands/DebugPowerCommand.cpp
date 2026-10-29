#include "debug/debug_command.hpp"
#include "map_debug_commands.hpp"

namespace game {
class DebugPowerCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments&) const noexcept override {
        return reader.finish();
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments&) const override {
        auto& result = context.result;
        result.candidates = 1;
        if (!debug_current_player_available()) {
            result.skipped = 1;
            context.record_failure({}, DebugTargetFailure::unavailable_player);
            return DebugCommandStatus::no_targets;
        }
        context.writes_started = true;
        if (apply_debug_power(context.world)) {
            result.applied = 1;
            return DebugCommandStatus::success;
        }
        result.skipped = 1;
        context.record_failure({}, DebugTargetFailure::unavailable_player);
        return DebugCommandStatus::no_targets;
    }

private:
    static constexpr DebugCommandDescriptor metadata{DebugCommandId::power,
                                                     DebugArgumentKind::none,
                                                     0,
                                                     0,
                                                     "power",
                                                     "power",
                                                     "Set a persistent 1000000 power bonus for the current player.",
                                                     "Current player granted a persistent 1000000 power bonus."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugPowerCommand);
}
