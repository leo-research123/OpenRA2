// YR 0x70FBE0 / 0x70FC90: robot activation changes the original locomotor.
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/BuildingClass.h"
#include <cstdlib>

void TechnoClass::Reactivate() {
    Guard();
    if ((AbstractFlags & ::AbstractFlags::Foot) != ::AbstractFlags::None) {
        auto& loco = static_cast<FootClass*>(this)->Locomotor;
        if (!loco) std::abort();
        loco->Power_On();
    }
    if (Deactivated && GetTechnoType()->ActivateSound != -1) {
        const auto at = Location;
        VocClass::PlayAt(GetTechnoType()->ActivateSound, at, nullptr);
    }
    Deactivated = false;
}
void TechnoClass::Deactivate() {
    if (IsTether && GetNthLink() && GetCell()->GetBuilding() == GetNthLink()) return;
    Guard();
    Deselect();
    if ((AbstractFlags & ::AbstractFlags::Foot) != ::AbstractFlags::None) {
        auto& loco = static_cast<FootClass*>(this)->Locomotor;
        if (!loco) std::abort();
        loco->Power_Off();
    }
    if (!Deactivated && GetTechnoType()->DeactivateSound != -1) {
        const auto at = Location;
        VocClass::PlayAt(GetTechnoType()->DeactivateSound, at, nullptr);
    }
    Deactivated = true;
}
