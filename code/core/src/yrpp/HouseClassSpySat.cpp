// House satellite availability, YR 0x00508F60.
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/SessionClass.h"

void HouseClass::UpdateSpySat() noexcept {
    RecheckRadar = false;
    const bool campaign = SessionClass::Instance.GameMode == GameMode::Campaign;
    bool available = false;
    const int count = Buildings.Count;
    for (int i = 0; i < count; ++i) {
        auto* building = Buildings[i];
        if (!building || !building->Type->SpySat || building->InLimbo || !building->IsOnMap) continue;
        if (campaign && (IsHumanPlayer || IsInPlayerControl) && !building->DiscoveredByCurrentPlayer) continue;
        if (building->CurrentMission == Mission::Selling || building->QueuedMission == Mission::Selling) continue;
        // The original checks warp only after selecting the first candidate.
        // Power, EMP and radar outages do not gate the satellite effect.
        available = !building->IsBeingWarpedOut();
        break;
    }
    if (SpySatActive == available) return;
    if (available)
        MapClass::Instance.Reveal(this);
    else
        MapClass::Instance.Reshroud(this);
    // Map callbacks observe the previous satellite state.
    SpySatActive = available;
    if (this == CurrentPlayer)
        VocClass::PlayGlobal(available ? RulesClass::Instance->SpySatActivationSound
                                       : RulesClass::Instance->SpySatDeactivationSound,
                             0x2000, 1.0f, nullptr);
}
