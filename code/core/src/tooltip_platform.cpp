#include "tooltip_platform.hpp"
#include "platform_input_codes.hpp"
#include "clock.hpp"
#include "map_world_internal.hpp"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include <algorithm>

namespace game {
namespace {
thread_local ToolTipPlatform* active_tooltip = nullptr;
}
ToolTipScope::ToolTipScope(ToolTipPlatform& platform, CCToolTip& tip) noexcept
    : previous_(active_tooltip), previous_instance_(CCToolTip::Instance) {
    active_tooltip = &platform;
    CCToolTip::Instance = &tip;
}
ToolTipScope::~ToolTipScope() {
    active_tooltip = previous_;
    CCToolTip::Instance = previous_instance_;
}
bool tooltip_pointer(Point2D& point) noexcept {
    if (!active_tooltip || !active_tooltip->pointer || !active_tooltip->pointer_visible ||
        !*active_tooltip->pointer_visible)
        return false;
    point = *active_tooltip->pointer;
    return true;
}
void tooltip_set_timer(ToolTipManager& owner, int milliseconds) noexcept {
    if (!active_tooltip) return;
    active_tooltip->timer_owner = &owner;
    active_tooltip->timer_start = clock_milliseconds();
    // Win32 USER_TIMER_MINIMUM / USER_TIMER_MAXIMUM, not simulation ticks.
    active_tooltip->timer_interval = std::clamp(std::uint32_t(milliseconds), 10u, 0x7FFFFFFFu);
}
void tooltip_kill_timer(ToolTipManager& owner) noexcept {
    if (active_tooltip && active_tooltip->timer_owner == &owner) active_tooltip->timer_owner = nullptr;
}
void tooltip_poll_timer() noexcept {
    if (!active_tooltip || !active_tooltip->timer_owner) return;
    if (clock_milliseconds() - active_tooltip->timer_start < active_tooltip->timer_interval) return;
    auto* owner = active_tooltip->timer_owner;
    MSG message{};
    message.message = Win32MessageTimer;
    message.wParam = ToolTipTimerId;
    owner->ProcessMessage(&message);
}
void tooltip_input(const GameInputEvent& event) noexcept {
    auto* tip = CCToolTip::Instance;
    if (!tip) return;
    if (event.kind == GameInputKind::focus_lost || event.kind == GameInputKind::pointer_leave) {
        tooltip_kill_timer(*tip);
        tip->Hide();
        return;
    }
    MSG message{};
    if (event.kind == GameInputKind::pointer_move)
        message.message = Win32MessageMouseMove;
    else if (event.kind == GameInputKind::pointer_button) {
        if (event.code == 1)
            message.message = event.pressed ? Win32MessageLeftButtonDown : Win32MessageLeftButtonUp;
        else if (event.code == 2)
            message.message = event.pressed ? Win32MessageRightButtonDown : Win32MessageRightButtonUp;
        else if (event.code == 3)
            message.message = event.pressed ? Win32MessageMiddleButtonDown : Win32MessageMiddleButtonUp;
    }
    tip->ProcessMessage(&message);
}
bool tooltip_pick(const Point2D& point, CellStruct& cell, CoordStruct& coords, ObjectClass*& object) noexcept {
    auto* tactical = TacticalClass::Instance;
    if (!tactical) return false;
    const auto& bounds = DSurface::ViewBounds.Width > 0 ? DSurface::ViewBounds : TacticalClass::ViewBounds;
    if (!tactical->PickTerrainCell(point, bounds, cell)) return false;
    auto* tile = MapClass::Instance.TryGetCellAt(cell);
    if (!tile) return false;
    tile->GetCellCoords(&coords);
    try {
        if (auto* world = current_map_world())
            object = pick_world_object(*world, point);
        else
            object = tactical->GetSelectableObject({point.X - bounds.X, point.Y - bounds.Y});
        return true;
    } catch (...) {
        object = nullptr;
        return false;
    }
}
}
