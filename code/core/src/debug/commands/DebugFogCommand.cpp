#include "debug/debug_command.hpp"
#include "map_view.hpp"
#include "map_world_internal.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/RadarClass.h"
#include <cstdio>
namespace game {
class DebugFogCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.finish() == DebugCommandStatus::success ? DebugCommandStatus::success
                                                              : reader.read_integer(arguments, 0, 1);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        auto& state = context.world.impl->view.debug;
        int value = !state.fog_visible;
        arguments.get_integer(0, value);
        context.writes_started = true;
        state.fog_visible = value != 0;
        context.result.value = value;
        context.result.candidates = context.result.applied = 1;
        MapClass::Instance.MarkNeedsRedraw(2);
        RadarClass::Instance.unknown_bool_14D9 = true;
        map_object_changed();
        std::snprintf(state.feedback.message, sizeof(state.feedback.message), "迷雾显示已%s。",
                      value ? "开启" : "关闭");
        return DebugCommandStatus::success;
    }

private:
    static constexpr DebugCommandDescriptor metadata{DebugCommandId::fog,
                                                     DebugArgumentKind::optional_integer,
                                                     0,
                                                     1,
                                                     "fog",
                                                     "fog [0|1]",
                                                     "Toggle shroud/fog display without changing explored cells.",
                                                     "Fog display updated."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugFogCommand);
}
