// YR 0x4A2600 / 0x6D0680: credit counter and command-panel update order.
#include "yrpp/MouseClass.h"

#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Surface.h"
#include <algorithm>
#include <bit>

#if !defined(RA2_YRPP_GAME)
void TabClass::RefreshMissionTimer() noexcept {
    MissionTimerPinged = true;
    MarkNeedsRedraw(0);
}
namespace {
int credits_frame{};
bool credits_redraw{};
}
int& TabDataClass::LastUpdateFrame = credits_frame;
bool& TabDataClass::CreditsNeedsRedraw = credits_redraw;
void TabDataClass::Update(bool force) {
    LastUpdateFrame = Unsorted::CurrentFrame;
    const auto redraw = [&] {
        NeedsRedraw = true;
        GScreenClass::Instance.GScreenClass::MarkNeedsRedraw(0);
        CreditsNeedsRedraw = true;
    };
    if (HouseClass::CurrentPlayer == HouseClass::Observer) {
        TargetValue = ScenarioClass::Instance->ElapsedTimer.GetTimeElapsed() / 60;
        if (TargetValue != LastValue || force) {
            LastValue = TargetValue;
            redraw();
        }
        return;
    }
    TargetValue = std::max(0, std::bit_cast<int>(static_cast<unsigned>(HouseClass::CurrentPlayer->Available_Money())));
    const auto& timer = ScenarioClass::Instance->MissionTimer;
    if (timer.IsTicking() || timer.GetTimeLeft()) {
        NeedsRedraw = true;
        GScreenClass::Instance.GScreenClass::MarkNeedsRedraw(0);
    }
    if (LastValue != TargetValue) {
        if (force) {
            ValueChanged = false;
            LastValue = TargetValue;
        } else {
            if (ValueDelta) --ValueDelta;
            const auto difference = unsigned(TargetValue) - unsigned(LastValue);
            const auto signed_difference = std::bit_cast<int>(difference);
            ValueDelta = signed_difference > 0 ? 1 : 3;
            const int magnitude = std::bit_cast<int>(signed_difference < 0 ? 0u - difference : difference);
            int step = std::clamp(magnitude >> 3, 1, 143);
            if (LastValue > TargetValue) step = -step;
            LastValue = std::bit_cast<int>(unsigned(LastValue) + unsigned(step));
            ValueChanged = true;
            ValueIncreased = step > 0;
        }
        redraw();
    }
}
void TabClass::Update(const int& key, const Point2D& point) {
    if (InsufficientFundsBlinkTimer.GetTimeLeft() == 1) {
        MissionTimerPinged = true;
        MarkNeedsRedraw(0);
    }
    const auto bar = GetCommandBarBounds();
    if (point.X >= bar.X && point.X < bar.X + DSurface::ViewBounds.Width && point.Y >= bar.Y && point.Y < bar.Y + 30)
        MouseClass::Instance.MouseClass::UpdateCursor(MouseCursorType::Default, false);
    if ((key >= 0x8000 + 214 && key < 0x8000 + 239) || key == 0x8000 + 240 || key == 0x8000 + 241 ||
        (key >= 0xC000 + 214 && key < 0xC000 + 239))
        ProcessButtonKey(static_cast<DWORD>(key));
    TabData.Update(false);
    SidebarClass::Update(key, point);
}
#endif
