#include "debug/debug_command.hpp"
#include "map_view.hpp"
#include "map_world_internal.hpp"
#include <cstdio>
namespace game {
class DebugTpCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.finish() == DebugCommandStatus::success ? DebugCommandStatus::success
                                                              : reader.read_integer(arguments, 0, 1);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        auto& state = context.world.impl->view.debug;
        int value = !state.teleport;
        arguments.get_integer(0, value);
        context.writes_started = true;
        state.teleport = value != 0;
        context.result.value = value;
        context.result.candidates = context.result.applied = 1;
        ++context.world.impl->view.ui_generation;
        std::snprintf(state.feedback.message, sizeof(state.feedback.message), "瞬移移动已%s。",
                      value ? "开启" : "关闭");
        return DebugCommandStatus::success;
    }

private:
    static constexpr DebugCommandDescriptor metadata{DebugCommandId::tp,
                                                     DebugArgumentKind::optional_integer,
                                                     0,
                                                     1,
                                                     "tp",
                                                     "tp [0|1]",
                                                     "Toggle instant relocation for subsequent player movement clicks.",
                                                     "Teleport movement updated."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugTpCommand);
}
