// YR 0x00653850 key branch, integrated with the existing Radar/Sidebar input.
#include "yrpp/RadarClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/MPGameModeClass.h"
#include "yrpp/Unsorted.h"
#include "scenario_runtime.hpp"

void RadarClass::ProcessButtonKey(DWORD key) noexcept {
    if(key==0x80F3){Game::SpecialDialog=1;return;}
    if(key!=0x80F2)return;
#if defined(RA2_YRPP_GAME)
    const auto* session=&SessionClass::Instance;
#else
    const auto* houses=game::scenario_runtime().houses;
    const auto* session=houses ? houses->session : nullptr;
    if(!session)return;
#endif
    try {
        const auto mode=session->GameMode;
        auto* multiplayer=session->MPGameMode;
        const bool briefing=mode==GameMode::Campaign ||
            ((mode==GameMode::LAN || mode==GameMode::Internet) && multiplayer &&
                multiplayer->vt_entry_08() && multiplayer->vt_entry_04());
        Game::SpecialDialog=briefing ? 9 : 8;
    } catch(...) {} // Host-provided game-mode overrides cannot unwind through input.
}

