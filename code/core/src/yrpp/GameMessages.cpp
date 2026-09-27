// OpenTS global Messages/TickCount ownership; YR 0x00730A90 and 0x0040EBD0.
#include "yrpp/Unsorted.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/HouseClass.h"
#if !defined(RA2_YRPP_GAME)
namespace {
// Native authority begins at zero before global constructors. This original
// elapsed timer preserves its independent pause/accumulation state, not a
// second source of time. No lazy first-message epoch or per-map reset.
constinit SysElapsedTimerClass ticks=[]() constexpr {SysElapsedTimerClass t;t.StartTime=0;return t;}();
}
SysElapsedTimerClass& Game::TickCount=ticks;
void YRPP_FASTCALL Game::ShowMessage(const wchar_t* text,int duration) noexcept {
    if(duration==-1)duration=240;
    const auto* house=HouseClass::CurrentPlayer;
    MessageListClass::Instance.AddMessage(nullptr,0,text,house?house->ColorSchemeIndex:3,
        TextPrintType(0x4046),duration,true);
}
#endif
