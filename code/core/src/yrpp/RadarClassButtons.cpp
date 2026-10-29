// YR 0x00653850 key branch, integrated with the existing Radar/Sidebar input.
#include "yrpp/RadarClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/MPGameModeClass.h"
#include "yrpp/Unsorted.h"

void RadarClass::ProcessButtonKey(DWORD key) noexcept {
    if (key == 0x80F3) {
        Game::SpecialDialog = 1;
        return;
    }
    if (key != 0x80F2) return;
    const auto* session = &SessionClass::Instance;
    try {
        const auto mode = session->GameMode;
        auto* multiplayer = session->MPGameMode;
        const bool briefing =
            mode == GameMode::Campaign || ((mode == GameMode::LAN || mode == GameMode::Internet) && multiplayer &&
                                           multiplayer->vt_entry_08() && multiplayer->vt_entry_04());
        Game::SpecialDialog = briefing ? 9 : 8;
    } catch (...) {
    }  // Host-provided game-mode overrides cannot unwind through input.
}
