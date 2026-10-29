#include "debug/debug_command.hpp"
#include "debug/debug_state.hpp"
#include "map_view.hpp"
#include "map_world_internal.hpp"
#include <cstdio>
namespace game {
class DebugSelectCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.read_integer(arguments, 1, 3);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        int mode = 1;
        arguments.get_integer(0, mode);
        context.writes_started = true;
        set_debug_selection(context.world, mode);
        context.result.candidates = context.result.applied = 1;
        auto& feedback = context.world.impl->view.debug.feedback;
        std::snprintf(feedback.message, sizeof(feedback.message), "选择模式 %d：%s；框选不包含建筑。", mode,
                      mode == 1   ? "默认"
                      : mode == 2 ? "全部非中立阵营单位"
                                  : "全部阵营及中立单位");
        return DebugCommandStatus::success;
    }

private:
    static constexpr DebugCommandDescriptor metadata{
        DebugCommandId::select,
        DebugArgumentKind::integer,
        1,
        3,
        "select",
        "select <1|2|3>",
        "Selection: 1=default, 2=all non-neutral units, 3=include neutral; no building bandbox.",
        "Selection mode updated."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugSelectCommand);
}
