// Existing YRpp 9402d7da hierarchy; field initialization calibrated to fixed
// YR 7b8a0685.
#include "yrpp/MouseClass.h"
#include "map_runtime.hpp"
#include "yrpp/Unsorted.h"
TabClass::TabClass()
    : SidebarClass(), TabData{}, unknown_timer_552C{}, InsufficientFundsBlinkTimer{}, ThumbActive{},
      MissionTimerPinged{}, unknown_byte_5546{} {
    Game::PlanningManager_SetNoticeSink(this);
    unknown_timer_552C.Start(0);
    InsufficientFundsBlinkTimer.Start(0);
    ThumbActive = MissionTimerPinged = true;
    unknown_byte_5546 = 1;
}
bool TabClass::INoticeSink_Unknown(DWORD code, DWORD) {
    if (code == 0x11C7 || code == 0x11C8) {
        const int slot = CommandPositions[9];
        if (slot >= 0 && slot < 25) {
            auto& button = CommandButtons[slot];
            if (code == 0x11C7 && !button.IsOn)
                button.TurnOn();
            else if (code == 0x11C8 && button.IsOn)
                button.TurnOff();
        }
        return true;
    }
    const auto notify = game::map_runtime().tab_notice;
    return notify ? notify(this, code) : false;
}
ScrollClass::ScrollClass()
    : TabClass(), unknown_int_5548{}, unknown_byte_554C{}, unknown_int_5550{}, unknown_int_5554{}, unknown_byte_5548{},
      unknown_byte_5549{}, unknown_byte_554A{} {
    unknown_byte_5549 = 1;
}
MouseClass::MouseClass()
    : ScrollClass(), MouseCursorIsMini{}, MouseCursorIndex{}, MouseCursorLastIndex{}, MouseCursorCurrentFrame{} {}
MouseClass::~MouseClass() = default;
