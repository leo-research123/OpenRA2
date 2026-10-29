#include "debug/debug_command.hpp"
#include "debug/debug_state.hpp"
#include "yrpp/RulesClass.h"
#include "map_world_internal.hpp"
#include "map_view.hpp"
#include <cstdio>
namespace game {
class DebugXpCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.read_real(arguments, 0, 1000000);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        double value = 0;
        arguments.get_real(0, value);
        if (!RulesClass::Instance || value > RulesClass::Instance->VeteranCap) {
            auto& feedback = context.world.impl->view.debug.feedback;
            std::snprintf(feedback.message, sizeof(feedback.message), "经验超过当前规则上限（VeteranCap=%g）。",
                          RulesClass::Instance ? RulesClass::Instance->VeteranCap : 0);
            return DebugCommandStatus::invalid_argument;
        }
        return execute_selected_debug_objects(
            context, arguments, debug_live_techno_filter,
            [](DebugCommandContext&, ObjectClass& object, const DebugCommandArguments& values) {
                double xp = 0;
                values.get_real(0, xp);
                return set_debug_xp(object, xp) ? DebugTargetFailure::none : DebugTargetFailure::value_out_of_range;
            },
            DebugTargetLifetime::scoped_pointer);
    }

private:
    static constexpr DebugCommandDescriptor metadata{
        DebugCommandId::xp,
        DebugArgumentKind::real,
        0,
        1000000,
        "xp",
        "xp <0..VeteranCap> (1=veteran, 2=elite)",
        "Set absolute original veterancy, including buildings; fractional values are accepted.",
        "Updated the XP of {count} selected objects."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugXpCommand);
}
