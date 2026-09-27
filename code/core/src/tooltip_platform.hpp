#pragma once
#include "yrpp/CCToolTip.h"
#include "api/map_view.hpp"
#include <cstdint>

class ObjectClass;

namespace game {
// Replaces only Win32 cursor queries and SetTimer/KillTimer. Tooltip regions,
// content and visibility remain in the original ToolTipManager object.
struct ToolTipPlatform {
    const Point2D* pointer=nullptr;
    const bool* pointer_visible=nullptr;
    ToolTipManager* timer_owner=nullptr;
    std::uint32_t timer_start=0, timer_interval=0;
};
class ToolTipScope {
    ToolTipPlatform* previous_;
    CCToolTip* previous_instance_;
public:
    ToolTipScope(ToolTipPlatform&,CCToolTip&) noexcept;
    ~ToolTipScope();
};
bool tooltip_pointer(Point2D&) noexcept;
void tooltip_set_timer(ToolTipManager&,int milliseconds) noexcept;
void tooltip_kill_timer(ToolTipManager&) noexcept;
void tooltip_poll_timer() noexcept;
void tooltip_input(const GameInputEvent&) noexcept;
// The existing native ProcessClickCoords adapter uses original selectable
// objects and terrain picking; it does not maintain another hover-object list.
bool tooltip_pick(const Point2D&,CellStruct&,CoordStruct&,ObjectClass*&) noexcept;
}
