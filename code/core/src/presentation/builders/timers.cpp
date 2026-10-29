// Original 0x6D4B50 timer formatting and text submissions, without blink-state writes.
#include "presentation/builders/timers.hpp"
#include "presentation/context.hpp"
#include "tactical_timer_cycle.hpp"
#include "yrpp/BitFont.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/TacticalClass.h"
#include "game_ui_runtime.hpp"
#include "tactical_drawing.hpp"
#include <bit>
namespace game {
int build_tactical_timer(PresentationContext& ctx, int index, ColorScheme* scheme, int seconds, const wchar_t* text,
                          const LARGE_INTEGER* blinkTimer, const bool* blinkState) {
    TacticalTimerText output;
    const auto status = layout_tactical_timer(output, scheme, seconds, text);
    if (status != DrawingStatus::drawn) { record_tactical_drawing(ctx, status); return 0; }
    auto* font = BitFont::Instance;
    auto* numberScheme = scheme;
    if (!seconds && blinkTimer && blinkState) {
        if (*blinkState) numberScheme = ColorScheme::Array.GetItemOrDefault(5);
    }
    const int y =
        std::bit_cast<int>(unsigned(TacticalClass::ViewBounds.Height) - (unsigned(index) + 1u) * (unsigned(font->field_1C) + 2u));
    const int right = std::bit_cast<int>(unsigned(TacticalClass::ViewBounds.Width) - 3u);
    constexpr auto flags = static_cast<TextPrintType>(0x4618);
    game::record_tactical_drawing(ctx, 
        game::draw_tactical_text(output.label, {std::bit_cast<int>(unsigned(right) - unsigned(output.width)), y}, scheme, flags));
    game::record_tactical_drawing(ctx, game::draw_tactical_text(output.time, {right, y}, numberScheme, flags));
    return 0;
}
void build_tactical_timers(PresentationContext& context) {
    visit_tactical_timers(&context, [](void* value, int row, ColorScheme* scheme, int seconds, wchar_t* text,
                                      LARGE_INTEGER* timer, bool* state) {
        build_tactical_timer(*static_cast<PresentationContext*>(value), row, scheme, seconds, text, timer, state);
    });
}
}
