// Base-defense team suspension, YR 0x6EC250.
#include "yrpp/TeamClass.h"
#include "yrpp/RulesClass.h"
void YRPP_FASTCALL TeamClass::SuspendTeams(int priority, HouseClass* owner) {
    for (int i = 0; i < Array.Count; ++i) {
        auto* team = Array[i];
        if (!team || team->Owner != owner || team->Type->Priority >= priority) continue;
        while (team->FirstUnit)
            team->LiberateMember(team->FirstUnit, -1, 0);
        team->JustDisappeared = true;
        team->NeedsToDisappear = true;
        team->IsSuspended = true;
        team->SuspendTimer.Start(static_cast<int>(RulesClass::Instance->SuspendDelay * 900.0));
    }
}
