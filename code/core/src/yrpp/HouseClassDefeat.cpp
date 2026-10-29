// YR 0x004FC0B0: defeat notification and surviving alliance resolution.
#include "yrpp/HouseClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/MPGameModeClass.h"
#include "yrpp/VoxClass.h"
#include "type_resources.hpp"

void HouseClass::AcceptDefeat() {
    Defeated = true;
    const bool campaign = SessionClass::Instance.GameMode == GameMode::Campaign;
    if (IQLevel2 == RulesClass::Instance->MaxIQLevels && !IsControlledByHuman() && RulesClass::Instance->Paranoid &&
        !campaign)
        AllyAIHouses();
    if (ScenarioClass::Instance->SpecialFlags.CTFMode) {
        if (OurFlagCarrier)
            UpdateFlagCoords(OurFlagCarrier, 1);
        else if (OurFlagCoords != CellStruct::Empty)
            UpdateFlagCoords(reinterpret_cast<UnitClass*>(MapClass::Instance.GetCellAt(OurFlagCoords)), 1);
    }
    if (!campaign && ScenarioClass::Instance->SpecialFlags.HarvesterImmune)
        for (int i = 0; i < UnitClass::Array.Count; ++i) {
            auto* unit = UnitClass::Array[i];
            if (unit->Owner == this && unit->IsAlive) unit->UnInit();
        }
    if (this != CurrentPlayer && !Type->MultiplayPassive) MapIsClear = true;
    if (this == CurrentPlayer) Unsorted::MuteSWLaunches = 1;
    // The host presents the result; the original Win32 observer dialog and
    // message pump are not allowed to block a simulation update.
    if (this != Observer && (this == CurrentPlayer || !Type->MultiplayPassive) &&
        !game::type_resources().audio_unavailable)
        VoxClass::Play(this == CurrentPlayer ? "EVA_YouHaveLost" : "EVA_PlayerDefeated");
    int survivors = 0, humans = 0;
    bool mutuallyAllied = true;
    for (auto* house : Array) {
        if (!house || house->Defeated || house->Type->MultiplayPassive) continue;
        ++survivors;
        if (house->IsControlledByHuman()) ++humans;
        for (auto* other : Array) {
            if (!other || other == house || other->Defeated || other->Type->MultiplayPassive) continue;
            if (!house->IsAlliedWith(other) || !other->IsAlliedWith(house)) mutuallyAllied = false;
        }
    }
    if (mutuallyAllied) SessionClass::Instance.SawCompletion = true;
    if (!CurrentPlayer || (!mutuallyAllied && survivors != 1 && humans != 0)) return;
    if (!CurrentPlayer->Defeated) {
        CurrentPlayer->Win(false);
        return;
    }
    auto* mode = SessionClass::Instance.MPGameMode;
    const auto session = SessionClass::Instance.GameMode;
    const bool teamVictory = (session == GameMode::LAN || session == GameMode::Internet) && mode &&
                             mode->vt_entry_04() && mode->vt_entry_08();
    if (mutuallyAllied) survivors = 1;
    // 0x4FC62C counts any surviving human here, including a passive House;
    // the earlier active-contender count deliberately excludes passive Houses.
    bool survivingHuman = false;
    if (survivors == 1 && teamVictory)
        for (auto* house : Array)
            if (house && !house->Defeated && house->IsControlledByHuman()) survivingHuman = true;
    if (survivingHuman)
        CurrentPlayer->Win(false);
    else
        CurrentPlayer->Lose(false);
}
