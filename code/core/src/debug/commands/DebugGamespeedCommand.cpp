#include "debug/debug_command.hpp"
#include "map_view.hpp"
#include "map_world_internal.hpp"
#include <cstdio>
namespace game {
class DebugGamespeedCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.read_integer(arguments, 1, 9);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        int level = 0;
        arguments.get_integer(0, level);
        auto& view = context.world.impl->view;
        const int previous_speed = view.loop.speed_level;
        if (!set_game_loop_speed(view.loop, level)) return DebugCommandStatus::execution_failure;
        if (!view.debug.original_speed) view.debug.original_speed = previous_speed;
        context.writes_started = true;
        ++view.ui_generation;
        context.result.candidates = context.result.applied = 1;
        std::snprintf(view.debug.feedback.message, sizeof(view.debug.feedback.message), "游戏速度已设为 %d 档%s。",
                      level,
                      level == 7   ? "（目标 120 逻辑帧/秒）"
                      : level == 8 ? "（目标 240 逻辑帧/秒）"
                      : level == 9 ? "（不限目标帧率）"
                                   : "");
        return DebugCommandStatus::success;
    }

private:
    static constexpr DebugCommandDescriptor metadata{DebugCommandId::gamespeed,
                                                     DebugArgumentKind::integer,
                                                     1,
                                                     9,
                                                     "gamespeed",
                                                     "gamespeed <1..9>",
                                                     "Set live speed: 1..6 timed, 7=120, 8=240, 9=unlimited logic FPS.",
                                                     "Game speed updated."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugGamespeedCommand);
}
