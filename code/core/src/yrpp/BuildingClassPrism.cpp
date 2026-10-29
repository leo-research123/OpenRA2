// YR 0x004503F0 / 0x0044ABD0: delayed building fire and prism support beams.
#include "yrpp/BuildingClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/LaserDrawClass.h"
#include <bit>

#if !defined(RA2_YRPP_GAME)

void BuildingClass::UpdateDelayedFire() {
    if (PrismStage == PrismChargeState::Idle) return;
    DelayBeforeFiring -= 1u;
    if (std::bit_cast<int>(DelayBeforeFiring) > 0) return;
    DelayBeforeFiring = 0;
    if (PrismStage == PrismChargeState::Slave)
        FireLaser(PrismTargetCoords);
    else if (PrismStage == PrismChargeState::Master && Target) {
        const int weapon = PrismTargetCoords.X;
        if (GetFireError(Target, weapon, true) == FireError::OK) {
            if (auto* bullet = Fire(Target, weapon); bullet && SupportingPrisms) {
                const unsigned percentage =
                    SupportingPrisms * unsigned(RulesClass::Instance->PrismSupportModifier) + 100u;
                bullet->DamageMultiplier = std::bit_cast<int>((percentage << 8) / 100u);
                SupportingPrisms = 0;
            }
        }
    }
    PrismStage = PrismChargeState::Idle;
}

void BuildingClass::FireLaser(CoordStruct coords) {
    auto* memory = YRMemory::Allocate(sizeof(LaserDrawClass));
    if (memory) {
        CoordStruct source;
        GetFLH(&source, 0, CoordStruct::Empty);
        auto* laser = ::new (memory) LaserDrawClass(source, coords, 0, 1, Owner->LaserColor, ColorStruct{},
                                                    ColorStruct{}, RulesClass::Instance->PrismSupportDuration);
        laser->IsHouseColor = true;
        laser->Thickness = 3;
    }
    SupportingPrisms = 0;
    RearmTimer.Start(RulesClass::Instance->PrismSupportDelay);
}

#endif
