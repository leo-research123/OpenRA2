#pragma once
#include "type_drawing_context.hpp"
class ColorScheme;
namespace game {
struct TacticalTimerText { wchar_t label[256]{}, time[256]{}; int width = 0; };
DrawingStatus layout_tactical_timer(TacticalTimerText&, ColorScheme*, int seconds, const wchar_t*) noexcept;
using TacticalTimerVisitor = void (*)(void*, int row, ColorScheme*, int seconds, wchar_t*, LARGE_INTEGER*, bool*);
void visit_tactical_timers(void*, TacticalTimerVisitor);
void advance_tactical_timer(ColorScheme*, int seconds, const wchar_t*, LARGE_INTEGER*, bool*) noexcept;
void advance_tactical_timers() noexcept;
}
