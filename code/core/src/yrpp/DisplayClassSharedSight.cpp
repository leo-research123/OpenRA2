// DisplayClass shared sight around alliance changes, YR 0x004ADEE0 and
// 0x004ADCD0. Both run on the display but read only the techno registry.
#include "yrpp/DisplayClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TechnoTypeClass.h"
#include "visibility_grid.hpp"

namespace {
bool skipped(const TechnoClass* techno, bool skipBuildings) {
    return !techno || techno->InLimbo || (techno->WhatAmI() == AbstractType::Building && skipBuildings);
}
bool allied_reveal(const TechnoClass& techno) {
    return techno.WhatAmI() == AbstractType::Building && RulesClass::Instance->AllyReveal &&
           techno.Owner->IsAlliedWith(HouseClass::CurrentPlayer);
}
}

void DisplayClass::WithdrawSharedSight(bool skipBuildings, int sightValue) {
    const int count = TechnoClass::Array.Count;
    for (int i = 0; i < count; ++i) {
        auto* techno = TechnoClass::Array[i];
        if (skipped(techno, skipBuildings)) continue;
        if (techno->Owner->IsControlledByCurrentPlayer()) {
            if (!techno->DiscoveredByCurrentPlayer) continue;
            techno->vt_entry_48C(false, 0, false, nullptr);
            if (techno->GetTechnoType()->GapGenerator) techno->DestroyGap();
        } else if (allied_reveal(*techno)) {
            techno->vt_entry_48C(false, sightValue, false, nullptr);
        } else if (techno->GetTechnoType()->GapGenerator) {
            techno->DestroyGap();
        }
    }
    game::visibility_flush();
}

void DisplayClass::RestoreSharedSight(bool skipBuildings, int sightValue) {
    const int count = TechnoClass::Array.Count;
    const auto reveal = [](TechnoClass& techno) {
        CoordStruct at = techno.Location;
        MapClass::Instance.RevealArea3(&at, 0, techno.LastSightRange + 3, false);
    };
    for (int i = 0; i < count; ++i) {
        auto* techno = TechnoClass::Array[i];
        if (skipped(techno, skipBuildings)) continue;
        if (techno->Owner->IsControlledByCurrentPlayer()) {
            if (!techno->DiscoveredByCurrentPlayer) continue;
            techno->UpdateSight(false, 0, false, nullptr, 0);
            reveal(*techno);
            if (techno->GetTechnoType()->GapGenerator) techno->CreateGap();
        } else if (allied_reveal(*techno)) {
            techno->UpdateSight(false, sightValue, false, nullptr, 0);
            reveal(*techno);
        } else if (techno->WhatAmI() == AbstractType::Building && techno->GetTechnoType()->RevealToAll &&
                   !techno->Owner->IsControlledByCurrentPlayer()) {
            techno->UpdateSight(false, 0, true, HouseClass::CurrentPlayer, 0);
            reveal(*techno);
        } else if (techno->GetTechnoType()->GapGenerator) {
            techno->CreateGap();
        }
    }
    game::visibility_flush();
}
