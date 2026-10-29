#include "tactical_timer_cycle.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/StringTable.h"
#include "yrpp/SessionClass.h"
#include "yrpp/BitFont.h"
#include "yrpp/Audio.h"
#include <bit>
#include <cwchar>
namespace game {
DrawingStatus layout_tactical_timer(TacticalTimerText& output, ColorScheme* scheme, int seconds, const wchar_t* text) noexcept {
    auto* font = BitFont::Instance;
    if (!font || !scheme || !text) {
        return DrawingStatus::unavailable;
    }
    auto& label = output.label;
    auto& time = output.time;
    // Copy wide titles directly: BSD wide printf can reject localized text in
    // the C locale even though the original Windows formatter accepts it.
    unsigned length = 0;
    while (length < 253 && text[length]) {
        label[length] = text[length];
        ++length;
    }
    if (text[length]) {
        return DrawingStatus::invalid_argument;
    }
    label[length] = L' ';
    label[length + 1] = L' ';
    label[length + 2] = 0;
    const int hours = seconds / 60 / 60;
    const int minutes = seconds / 60 % 60;
    const int remainder = seconds % 60;
    if ((hours ? std::swprintf(time, 256, L"%d:%02d:%02d", hours, minutes, remainder)
               : std::swprintf(time, 256, L"%02d:%02d", minutes, remainder)) < 0) {
        return DrawingStatus::invalid_argument;
    }
    auto& width = output.width;
    width = 0;
    if (!font->GetTextDimension(time, &width, nullptr, TacticalClass::ViewBounds.Width)) {
        return DrawingStatus::unavailable;
    }
    return DrawingStatus::drawn;
}
void visit_tactical_timers(void* context, TacticalTimerVisitor visitor) {
    int row = 0;
    auto* scenario = ScenarioClass::Instance;
    if (scenario && scenario->MissionTimer.IsTicking()) {
        auto* player = HouseClass::CurrentPlayer;
        auto* scheme = player ? ColorScheme::Array.GetItemOrDefault(player->ColorSchemeIndex) : nullptr;
        visitor(context, row++, scheme, scenario->MissionTimer.GetTimeLeft() / 15,
                                 scenario->MissionTimerTextCSF, nullptr, nullptr);
    }
    const int supers = SuperClass::ShowTimers.Count;
    for (int i = 0; i < supers; ++i) {
        auto* super = SuperClass::ShowTimers[i];
        if (super->IsSuspended && SessionClass::IsCampaign() &&
            super->RechargeTimer.GetTimeLeft() == super->GetRechargeTime())
            continue;
        auto* scheme = ColorScheme::Array.GetItemOrDefault(super->Owner->ColorSchemeIndex);
        visitor(context, row++, scheme, super->RechargeTimer.GetTimeLeft() / 15,
                                 const_cast<wchar_t*>(super->Type->UIName),
                                 &super->BlinkTimer, &super->BlinkState);
    }
    const int houses = HouseClass::Array.Count;
    for (int i = 0; i < houses; ++i) {
        auto* house = HouseClass::Array[i];
        const int left = house->PowerBlackoutTimer.GetTimeLeft();
        if (left > 0) {
            auto* scheme = ColorScheme::Array.GetItemOrDefault(house->ColorSchemeIndex);
            auto* title = const_cast<wchar_t*>(
                StringTable::LoadString("MSG:BlackoutTimer", nullptr, "D:\\ra2mdpost\\Tactical.CPP", 452));
            visitor(context, row++, scheme, left / 15, title, nullptr, nullptr);
        }
    }
}
void advance_tactical_timer(ColorScheme* scheme, int seconds, const wchar_t* text,
                           LARGE_INTEGER* blink_timer, bool* blink_state) noexcept {
    if (seconds || !blink_timer || !blink_state) return;
    TacticalTimerText output;
    if (layout_tactical_timer(output, scheme, seconds, text) != DrawingStatus::drawn) return;
    if (static_cast<unsigned long long>(blink_timer->QuadPart) <= Audio::GetTime()) {
        blink_timer->QuadPart = std::bit_cast<long long>(Audio::GetTime() + 1000ull);
        *blink_state = !*blink_state;
    }
}
void advance_tactical_timers() noexcept {
    try {
        visit_tactical_timers(nullptr, [](void*, int, ColorScheme* scheme, int seconds, wchar_t* text,
                                      LARGE_INTEGER* blink_timer, bool* blink_state) {
            advance_tactical_timer(scheme, seconds, text, blink_timer, blink_state);
        });
    } catch (...) { /* The render-phase boundary must contain resource failures. */ }
}
}
