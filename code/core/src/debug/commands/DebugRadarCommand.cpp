#include "debug/debug_command.hpp"
#include "map_view.hpp"
#include "map_world_internal.hpp"
#include "yrpp/HouseClass.h"
#include "yrpp/RadarClass.h"
#include <cstdio>

namespace game {
class DebugRadarCommand final : public DebugConsoleCommand {
public:
    const DebugCommandDescriptor& descriptor() const noexcept override { return metadata; }
    DebugCommandStatus parse(DebugArgumentReader& reader, DebugCommandArguments& arguments) const noexcept override {
        return reader.finish() == DebugCommandStatus::success ? DebugCommandStatus::success
                                                              : reader.read_integer(arguments, 0, 1);
    }
    DebugCommandStatus execute(DebugCommandContext& context, const DebugCommandArguments& arguments) const override {
        context.result.candidates = 1;
        if (!debug_current_player_available()) {
            context.result.skipped = 1;
            context.record_failure({}, DebugTargetFailure::unavailable_player);
            return DebugCommandStatus::no_targets;
        }
        auto& state = context.world.impl->view.debug;
        int value = !state.radar_forced;
        arguments.get_integer(0, value);
        context.writes_started = true;
        state.radar_forced = value != 0;
        HouseClass::CurrentPlayer->UpdateRadarAvailability();
        RadarClass::Instance.RedrawRadar(true);
        map_object_changed();
        context.result.value = value;
        context.result.applied = 1;
        std::snprintf(state.feedback.message, sizeof(state.feedback.message), "%s",
                      value ? "雷达地图已强制开启。" : "雷达地图已恢复正常建筑和电力规则。");
        return DebugCommandStatus::success;
    }

private:
    static constexpr DebugCommandDescriptor metadata{DebugCommandId::radar,
                                                     DebugArgumentKind::optional_integer,
                                                     0,
                                                     1,
                                                     "radar",
                                                     "radar [0|1]",
                                                     "Toggle forced radar availability; 0 restores normal rules, 1 enables radar.",
                                                     "Radar availability updated."};
};
GAME_REGISTER_DEBUG_COMMAND(DebugRadarCommand);
}
