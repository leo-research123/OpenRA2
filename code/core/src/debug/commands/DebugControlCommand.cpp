#include "debug/debug_command.hpp"
#include "map_view.hpp"
#include "map_world_internal.hpp"
#include <cstdio>
namespace game {
class DebugControlCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.read_integer(arguments, 1, 3);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        int mode = 1;
        arguments.get_integer(0, mode);
        auto& view = context.world.impl->view;
        context.writes_started = true;
        view.debug.control = mode;
        ++view.ui_generation;
        context.result.candidates = context.result.applied = 1;
        std::snprintf(view.debug.feedback.message, sizeof(view.debug.feedback.message), "控制模式 %d：%s。", mode,
                      mode == 1   ? "默认"
                      : mode == 2 ? "全部非中立阵营对象"
                                  : "全部阵营及中立对象");
        return DebugCommandStatus::success;
    }

private:
    static constexpr DebugCommandDescriptor metadata{
        DebugCommandId::control,
        DebugArgumentKind::integer,
        1,
        3,
        "control",
        "control <1|2|3>",
        "Control: 1=default, 2=all non-neutral objects, 3=include neutral; selection is independent.",
        "Control mode updated."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugControlCommand);
}
