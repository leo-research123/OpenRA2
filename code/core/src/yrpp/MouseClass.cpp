// Existing YRpp 9402d7da hierarchy; field initialization calibrated to fixed
// YR 7b8a0685.
#include "yrpp/MouseClass.h"
#include "map_runtime.hpp"
TabClass::TabClass()
    : SidebarClass(), TabData{}, unknown_timer_552C{}, InsufficientFundsBlinkTimer{}, ThumbActive{},
      MissionTimerPinged{}, unknown_byte_5546{} {
    unknown_timer_552C.Start(0);
    InsufficientFundsBlinkTimer.Start(0);
    ThumbActive = MissionTimerPinged = true;
    unknown_byte_5546 = 1;
}
bool YRPP_STDCALL TabClass::INoticeSink_Unknown(DWORD code) {
    const auto notify = game::map_runtime().tab_notice;
    return notify ? notify(this, code) : false;
}
ScrollClass::ScrollClass()
    : TabClass(), unknown_int_5548{}, unknown_byte_554C{}, unknown_int_5550{}, unknown_int_5554{},
      unknown_byte_5548{}, unknown_byte_5549{}, unknown_byte_554A{} { unknown_byte_5549 = 1; }
MouseClass::MouseClass()
    : ScrollClass(), MouseCursorIsMini{}, MouseCursorIndex{}, MouseCursorLastIndex{}, MouseCursorCurrentFrame{} {}
MouseClass::~MouseClass() = default;

