#include "debug/debug_command.hpp"
#include "map_world_internal.hpp"
#include "map_view.hpp"
#include "yrpp/HouseClass.h"
#include <climits>
#include <cstdio>
namespace game {
class DebugMoneyCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.finish() == DebugCommandStatus::success ? DebugCommandStatus::success
                                                              : reader.read_integer(arguments, 1, INT_MAX);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        int amount = 10000;
        arguments.get_integer(0, amount);
        context.result.value = amount;
        context.result.candidates = 1;
        auto* player = HouseClass::CurrentPlayer;
        if (!player) {
            context.result.skipped = 1;
            context.record_failure({}, DebugTargetFailure::unavailable_player);
            return DebugCommandStatus::no_targets;
        }
        if (std::int64_t(player->Balance) + amount > INT_MAX ||
            std::int64_t(player->Available_Money()) + amount > INT_MAX) {
            context.result.skipped = 1;
            context.record_failure({}, DebugTargetFailure::value_out_of_range);
            auto& feedback = context.world.impl->view.debug.feedback;
            std::snprintf(feedback.message, sizeof(feedback.message), "增加资金会超过余额或可用资金上限。未修改资金。");
            return DebugCommandStatus::invalid_argument;
        }
        context.writes_started = true;
        player->GiveMoney(amount);
        context.result.applied = 1;
        ++context.world.impl->view.ui_generation;
        auto& feedback = context.world.impl->view.debug.feedback;
        std::snprintf(feedback.message, sizeof(feedback.message), "已增加 %d 资金，当前可用资金 %ld。", amount,
                      player->Available_Money());
        return DebugCommandStatus::success;
    }

private:
    static constexpr DebugCommandDescriptor metadata{DebugCommandId::money,
                                                     DebugArgumentKind::optional_integer,
                                                     1,
                                                     INT_MAX,
                                                     "money",
                                                     "money [amount] (default 10000)",
                                                     "Add cash to the current player; default 10000.",
                                                     "Money added."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugMoneyCommand);
}
