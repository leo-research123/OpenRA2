#include "debug/debug_command.hpp"
#include "debug/debug_state.hpp"
#include <cstring>
namespace game {
class DebugAtkCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        auto copy = reader;
        const char* text = nullptr;
        std::uint32_t length = 0;
        DebugCommandArguments probe;
        if (copy.read_text(probe) == DebugCommandStatus::success && probe.get_text(0, text, length) && length == 5 &&
            !std::memcmp(text, "reset", 5)) {
            reader = copy;
            arguments.values[arguments.count++] = {DebugValueKind::integer, -1};
            return DebugCommandStatus::success;
        }
        return reader.read_integer(arguments, 1, 1000000);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        return execute_selected_debug_objects(
            context, arguments, debug_live_techno_filter,
            [](DebugCommandContext& context, ObjectClass& object, const DebugCommandArguments& values) {
                int damage = 0;
                values.get_integer(0, damage);
                return set_debug_attack(context.world, object, damage) ? DebugTargetFailure::none
                                                                       : DebugTargetFailure::unsupported_type;
            },
            DebugTargetLifetime::scoped_pointer);
    }

private:
    static constexpr DebugCommandDescriptor metadata{
        DebugCommandId::atk,
        DebugArgumentKind::integer_or_reset,
        1,
        1000000,
        "atk",
        "atk <1..1000000|reset>",
        "Override per-object outgoing damage before victim defenses; reset restores normal damage.",
        "Updated the attack of {count} selected objects."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugAtkCommand);
}
