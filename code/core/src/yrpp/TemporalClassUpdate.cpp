// YR 0x71A760: victim-driven progress and original retirement handoffs.
#include "yrpp/TemporalClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/AnimClass.h"
#include "x87_integer.hpp"
#include "yrpp/YRMath.h"
#include <cstdlib>
#include <bit>
#if !defined(RA2_YRPP_GAME)
void TemporalClass::Update() try {
    if (Target && Target->TemporalTargetingMe == this && NextTemporal) {
        Target->TemporalTargetingMe = nullptr;
        Target->BeingWarpedOut = false;
        Detach();
        return;
    }
    if (Owner && Owner->InOpenToppedTransport && Target) {
        CoordStruct targetBuffer, ownerBuffer;
        const auto* target = Target->GetCoords(&targetBuffer);
        const auto* owner = Owner->GetCoords(&ownerBuffer);
        const double x = std::bit_cast<int>(unsigned(owner->X) - unsigned(target->X));
        const double y = std::bit_cast<int>(unsigned(owner->Y) - unsigned(target->Y));
        const double z = std::bit_cast<int>(unsigned(owner->Z) - unsigned(target->Z));
        // This entry uses raw 3D separation, without building foundation reduction.
        if (game::x87_integer(Math::sqrt(x * x + y * y + z * z)) >
            std::bit_cast<int>(unsigned(RulesClass::Instance->OpenToppedWarpDistance) << 8)) {
            LetGo();
            return;
        }
    }
    const int helpers = PrevTemporal ? PrevTemporal->GetWarpPerStep(1) : 0;
    WarpPerStep = Owner->GetWeapon(Owner->SelectWeapon(nullptr))->WeaponType->Damage;
    WarpRemaining = std::bit_cast<int>(unsigned(WarpRemaining) - unsigned(helpers) - unsigned(WarpPerStep));
    if (WarpRemaining > 0) return;
    if (Target) {
        if (auto* memory = YRMemory::Allocate(sizeof(AnimClass)))
            ::new (memory) AnimClass(RulesClass::Instance->WarpAway, Target->Location, 0, 1, 0x600, 0, false);
    } else {
        Target = nullptr;
        PrevTemporal = NextTemporal = nullptr;
        SourceSW = nullptr;
        unknown_pointer_38 = nullptr;
        if (Owner) Owner->EnterIdleMode(false, true);
    }
    if (Target) {
        if (Owner->GetTechnoType()->Trainable) {
            auto* victimType = Target->GetTechnoType();
            auto* ownerType = Owner->GetTechnoType();
            const int victimCost = victimType->GetActualCost(Target->Owner);
            const int ownerCost = ownerType->GetActualCost(Owner->Owner);
            Owner->Veterancy.Add(ownerCost, victimCost);
        }
        if (Target->WhatAmI() == AbstractType::Building) {
            auto* building = static_cast<BuildingClass*>(Target);
            if (building->GetOccupantCount() > 0) building->KillOccupants(nullptr);
            if (!Unsorted::ScenarioInit)
                while (building->Passengers.NumPassengers) {
                    GameDelete(building->Passengers.RemoveFirstPassenger());
                }
            if (SourceSW) {
                SourceSW->SetOnHold(false);
                SourceSW = nullptr;
            }
            if (building->BunkerLinkedItem) building->UnloadBunker();
            if (Target->SlaveManager) Target->SlaveManager->Killed(Owner, nullptr);
            Target->Destroyed(Owner);
            Target->RegisterDestruction(Owner);
            Target->UnInit();
            building->Owner->RecheckTechTree = true;
        } else {
            if (Target->BunkerLinkedItem && Target->BunkerLinkedItem->WhatAmI() == AbstractType::Building)
                static_cast<BuildingClass*>(Target->BunkerLinkedItem)->ClearBunker();
            if (Target->SlaveManager) Target->SlaveManager->Killed(Owner, nullptr);
            Target->Destroyed(Owner);
            Target->RegisterDestruction(Owner);
            Target->UnInit();
        }
        if (Owner) Owner->EnterIdleMode(false, true);
    }
    Target = nullptr;
    PrevTemporal = NextTemporal = nullptr;
    SourceSW = nullptr;
    unknown_pointer_38 = nullptr;
    if (Owner) Owner->EnterIdleMode(false, true);
} catch (...) {
    std::abort();
}
#endif
