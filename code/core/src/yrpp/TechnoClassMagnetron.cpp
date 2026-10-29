// YR 0x70FEE0 / 0x710000: magnetron control and COM piggyback ownership.
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/TeamClass.h"
#include <cstdlib>

void TechnoClass::ReleaseLocomotor(bool setTarget) noexcept {
    try {
        if (!LocomotorTarget) return;
        LocomotorTarget->LocomotorSource = nullptr;
        if ((LocomotorTarget->AbstractFlags & ::AbstractFlags::Foot) != ::AbstractFlags::None) {
            if (LocomotorTarget->GetHeight() > 0) {
                LocomotorTarget->IsCrashing = true;
                LocomotorTarget->IsBeingManipulated = true;
                LocomotorTarget->BeingManipulatedBy = this;
                LocomotorTarget->ChronoWarpedByHouse = Owner;
                LocomotorTarget->Stun();
                LocomotorTarget->SetDestination(nullptr, true);
                LocomotorTarget = nullptr;
                if (setTarget) SetTarget(nullptr);
                return;
            }
            LocomotorTarget->SetDestination(nullptr, true);
            LocomotorTarget->IsLetGoByLocomotor = true;
            LocomotorTarget->SetDestination(nullptr, true);
        }
        LocomotorTarget = nullptr;
        if (setTarget) SetTarget(nullptr);
    } catch (...) {
        std::abort();
    }
}

void TechnoClass::ImbueLocomotor(FootClass* victim, CLSID clsid) noexcept {
    try {
        if (!victim) return;
        if (victim->SpawnManager) victim->SpawnManager->KillNodes();
        auto* parasite = victim->ParasiteEatingMe;
        if (parasite && parasite->ParasiteImUsing && parasite->GetTechnoType()->Naval)
            parasite->ParasiteImUsing->ExitUnit();
        const auto check = [](HRESULT result) {
            if (result < 0 && result != static_cast<HRESULT>(0x80004002u)) std::abort();
        };
        ILocomotion* previous = victim->Locomotor;
        IPiggyback* oldPiggy = nullptr;
        if (previous) check(LocomotionClass::TryPiggyback(&oldPiggy, &previous));
        if (victim->GetTechnoType()->ResourceGatherer && oldPiggy && !victim->LocomotorSource &&
            oldPiggy->Is_Piggybacking()) {
            if (!oldPiggy->Is_Ok_To_End()) {
                if (!previous) std::abort();
                previous->Stop_Moving();
                victim->ForceMission(Mission::None);
                victim->QueueMission(Mission::Stop, false);
            } else {
#if defined(_MSC_VER)
                victim->Locomotor = nullptr;
#else
                if (victim->Locomotor) victim->Locomotor->Release();
                victim->Locomotor = nullptr;
#endif
                ILocomotion* restored = nullptr;
                oldPiggy->End_Piggyback(&restored);
#if defined(_MSC_VER)
                victim->Locomotor.Attach(restored);
#else
                victim->Locomotor = restored;
#endif
            }
        } else {
            if (!previous) std::abort();
            previous->Mark_All_Occupation_Bits(MarkType::Up);
            previous->Force_Track(-1, CoordStruct::Empty);
            const auto at = victim->GetCoords();
            victim->UnmarkAllOccupationBits(at);
            victim->FrozenStill = true;
            victim->Guard();
            IPiggyback* piggy = nullptr;
            previous = victim->Locomotor;
            if (previous) check(LocomotionClass::TryPiggyback(&piggy, &previous));
            ILocomotion* next = nullptr;
            check(LocomotionClass::CreateInstance(&next, &clsid, nullptr, 7));
            if (!next) std::abort();
            next->Link_To_Object(victim);
            IPiggyback* nextPiggy = nullptr;
            check(LocomotionClass::TryPiggyback(&nextPiggy, &next));
            if (nextPiggy) nextPiggy->AddRef();
            if (piggy) piggy->Release();
            piggy = nextPiggy;
            if (nextPiggy) nextPiggy->Release();
            if (piggy) {
                if (LocomotorTarget) ReleaseLocomotor(false);
                if (victim->LocomotorTarget) victim->ReleaseLocomotor(true);
                piggy->Begin_Piggyback(victim->Locomotor);
#if defined(_MSC_VER)
                victim->Locomotor = next;
#else
                if (victim->Locomotor != next) {
                    auto* old = victim->Locomotor;
                    victim->Locomotor = next;
                    next->AddRef();
                    if (old) old->Release();
                }
#endif
                LocomotorTarget = victim;
                // Original sources are Foot objects; the stored ABI field has this type.
                victim->LocomotorSource = static_cast<FootClass*>(this);
                victim->SetDestination(this, true);
                victim->Deselect();
                if (victim->Team) victim->Team->LiberateMember(victim, -1, 0);
                victim->IsAttackedByLocomotor = true;
                auto* link = victim->RadioLinks.Capacity ? victim->GetNthLink() : nullptr;
                if (link && link->WhatAmI() == AbstractType::Building)
                    victim->SendToFirstLink(static_cast<RadioCommand>(3));
                victim->QueueMission(Mission::Guard, true);
                if (!victim->Owner->IsHumanPlayer && !victim->GetTechnoType()->ResourceGatherer &&
                    !victim->Owner->Type->MultiplayPassive)
                    victim->QueueMission(Mission::Hunt, false);
            }
            if (next) next->Release();
            if (piggy) piggy->Release();
        }
        if (oldPiggy) oldPiggy->Release();
    } catch (...) {
        std::abort();
    }
}
