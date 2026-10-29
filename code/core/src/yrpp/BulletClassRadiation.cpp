// Original YR radiation entry 0x0046ADE0 and Detonate's matching inline branch.
#include "yrpp/BulletClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RadSiteClass.h"
#include <cstdlib>

void YRPP_STDCALL BulletClass::ApplyRadiationToCell(CellStruct cell, int radius, int amount) noexcept {
    try {
        auto* target = MapClass::Instance.GetCellAt(cell);
        if (auto* site = target->GetRadSite()) {
            site->Add(amount);
            return;
        }
        auto* site = GameCreate<RadSiteClass>();
        site->SetBaseCell(&cell);
        site->SetSpread(radius);
        site->SetRadLevel(amount);
        site->Activate();
        target->SetRadSite(site);
    } catch (...) {
        // A partially applied radiation field cannot be reported as success.
        std::abort();
    }
}
