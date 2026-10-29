#include "debug/debug_command.hpp"
#include "debug/debug_state.hpp"
#include "map_view.hpp"
#include "map_world_internal.hpp"
#include <cstdio>
namespace game {
class DebugPerfCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.finish() == DebugCommandStatus::success ? DebugCommandStatus::success
                                                              : reader.read_integer(arguments, 0, 1);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        auto& state = context.world.impl->view.debug;
        int value = !state.performance_visible;
        arguments.get_integer(0, value);
        context.writes_started = true;
        state.performance_visible = value != 0;
        context.result.value = value;
        std::snprintf(state.feedback.message, sizeof(state.feedback.message), "性能面板已%s。",
                      value ? "开启" : "关闭");
        return DebugCommandStatus::success;
    }

private:
    static constexpr DebugCommandDescriptor metadata{DebugCommandId::perf,
                                                     DebugArgumentKind::optional_integer,
                                                     0,
                                                     1,
                                                     "perf",
                                                     "perf [0|1]",
                                                     "Toggle the live performance overlay; 0 hides, 1 shows.",
                                                     "Performance overlay updated."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugPerfCommand);
}
