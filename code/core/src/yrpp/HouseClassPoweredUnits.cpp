// YR 0x50E010 / 0x50E0E0 / 0x50E1B0: powered centers share the house count.
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/VoxClass.h"
#include <bit>

bool HouseClass::HasPoweredCenter(TechnoTypeClass*) const { return PoweredUnitCenters > 0; }
void HouseClass::RegisterPoweredCenter(TechnoTypeClass* type) {
    PoweredUnitCenters = std::bit_cast<int>(unsigned(PoweredUnitCenters) + 1u);
    if (PoweredUnitCenters != 1 || !type) return;
    bool changed = false;
    for (int i = TechnoClass::Array.Count - 1; i >= 0; --i) {
        auto* unit = TechnoClass::Array[i];
        if (unit->Owner == this && unit->GetTechnoType() == type && !unit->InLimbo) {
            unit->Reactivate();
            changed = true;
        }
    }
    if (changed && Game::IsActive && IsControlledByCurrentPlayer() && !Unsorted::ScenarioInit)
        VoxClass::Play("EVA_RobotTanksBackOnline");
}
void HouseClass::LostPoweredCenter(TechnoTypeClass* type) {
    PoweredUnitCenters = std::bit_cast<int>(unsigned(PoweredUnitCenters) - 1u);
    if (PoweredUnitCenters != 0 || !type) return;
    bool changed = false;
    for (int i = TechnoClass::Array.Count - 1; i >= 0; --i) {
        auto* unit = TechnoClass::Array[i];
        if (unit->Owner == this && unit->GetTechnoType() == type && !unit->InLimbo) {
            unit->Deactivate();
            changed = true;
        }
    }
    if (changed && Game::IsActive && IsControlledByCurrentPlayer() && !Unsorted::ScenarioInit)
        VoxClass::Play("EVA_RobotTanksOffline");
}
