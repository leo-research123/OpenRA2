// Boris airstrike manager, YR 0x41D380..0x41DDDD.
#include "yrpp/AirstrikeClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/Unsorted.h"
#include <bit>

AirstrikeClass::AirstrikeClass(TechnoClass* owner) noexcept
    : AbstractClass(), AirstrikeTeamTypeIndex(0), EliteAirstrikeTeamTypeIndex(0), unknown_34(0), unknown_38(0),
      IsOnMission(false), unknown_bool_3D(false), TeamDissolveFrame(0), Owner(owner), Target(nullptr),
      FirstObject(nullptr) {
    // The original leaves counters and the pre-first-flight timestamp unspecified.
    // Native construction initializes them rather than reading allocator contents.
    Array.AddItem(this);
    AirstrikeTeam = Owner->GetTechnoType()->AirstrikeTeam;
    EliteAirstrikeTeam = Owner->GetTechnoType()->EliteAirstrikeTeam;
    AirstrikeRechargeTime = Owner->GetTechnoType()->AirstrikeRechargeTime;
    EliteAirstrikeRechargeTime = Owner->GetTechnoType()->EliteAirstrikeRechargeTime;
    AirstrikeTeamType = Owner->GetTechnoType()->AirstrikeTeamType;
    EliteAirstrikeTeamType = Owner->GetTechnoType()->EliteAirstrikeTeamType;
}
AirstrikeClass::~AirstrikeClass() { Array.Remove(this); }
bool AirstrikeClass::CanTarget(ObjectClass* target) const {
    const int recharge = Owner->Veterancy.IsElite() ? EliteAirstrikeRechargeTime : AirstrikeRechargeTime;
    const int ready = std::bit_cast<int>(unsigned(recharge) + TeamDissolveFrame);
    return (!IsOnMission && ready <= Unsorted::CurrentFrame) || target != Target;
}
void AirstrikeClass::StartMission(ObjectClass* target) {
    if (!IsOnMission)
        CreateMission(target);
    else if (target != Target)
        ResetTarget(target);
}
void AirstrikeClass::CreateMission(ObjectClass* target) {
    IsOnMission = true;
    unknown_bool_3D = true;
    unknown_34 = 0;
    int plane = AirstrikeTeamType->ArrayIndex;
    const int passenger = RulesClass::Instance->SovParaDropInf[0]->ArrayIndex;
    int count = AirstrikeTeam;
    if (Owner->Veterancy.IsElite()) {
        count = EliteAirstrikeTeam;
        plane = EliteAirstrikeTeamType->ArrayIndex;
    }
    const auto where = target->GetCoords();
    const CellStruct cell{short(where.X / 256), short(where.Y / 256)};
    const int created = Owner->GetOwningHouse()->SendAirstrikePlanes(
        plane, count, Mission::Attack, MapClass::Instance.GetCellAt(cell), nullptr, passenger, 0, this);
    AirstrikeTeamTypeIndex = EliteAirstrikeTeamTypeIndex = created;
    if (!created) return;
    if (FirstObject) VocClass::PlayAt(RulesClass::Instance->AirstrikeAttackVoice, FirstObject->Location, 0);
    Target = target;
    if (target && target->WhatAmI() == AbstractType::Building) {
        auto* building = static_cast<BuildingClass*>(target);
        building->Airstrike = this;
        building->StartAirstrikeTimer(100000);
        building->ColorAdd = RulesClass::Instance->ColorAdd[RulesClass::Instance->LaserTargetColor];
        building->unknown_int_6FC = 1;
        building->Mark(MarkType::Change);
    }
}
void AirstrikeClass::ResetTarget(ObjectClass* target) {
    if (FirstObject)
        for (auto* p = FirstObject->NextTeamMember; p; p = p->NextTeamMember)
            if (p->Ammo != FirstObject->Ammo) {
                ClearTarget();
                return;
            }
    if (Target && Target->WhatAmI() == AbstractType::Building) {
        auto* old = static_cast<TechnoClass*>(Target);
        old->Airstrike = nullptr;
        old->StopAirstrikeTimer();
        old->Mark(MarkType::Change);
    }
    Target = nullptr;
    if (target && target->WhatAmI() == AbstractType::Building) {
        auto* next = static_cast<TechnoClass*>(target);
        next->Airstrike = this;
        next->StartAirstrikeTimer(100000);
        next->Mark(MarkType::Change);
    }
    Target = target;
    auto* plane = FirstObject;
    for (int i = 0; i < EliteAirstrikeTeamTypeIndex; ++i)
        if (plane) {
            if (plane->Ammo > 0) {
                plane->SetTarget(target);
                plane->QueueMission(Mission::Attack, false);
                unknown_bool_3D = true;
            }
            plane = plane->NextTeamMember;
        }
}
void AirstrikeClass::ClearTarget() {
    if (!Game::IsActive) return;
    for (auto* p = FirstObject; p; p = p->NextTeamMember)
        if (!p->Ammo) {
            for (auto* q = FirstObject; q; q = q->NextTeamMember)
                q->Ammo = 0;
            break;
        }
    int count = 0;
    for (auto* p = FirstObject; p && count <= 100; p = p->NextTeamMember, ++count) {
        p->QueueMission(Mission::Retreat, false);
        p->SetTarget(nullptr);
        p->SetDestination(nullptr, true);
    }
    unknown_bool_3D = false;
    if (Target && Target->WhatAmI() == AbstractType::Building) {
        auto* building = static_cast<TechnoClass*>(Target);
        if (building->Airstrike == this) {
            AirstrikeClass* replacement = nullptr;
            for (auto* other : Array)
                if (other != this && other->Target == Target) replacement = other;
            building->Airstrike = replacement;
            if (!replacement && Game::IsActive) building->Mark(MarkType::Change);
            if (building->Airstrike == this) building->Airstrike = nullptr;
        }
    }
    Target = nullptr;
}
void AirstrikeClass::RemoveAircraft(FootClass* aircraft) {
    if (FirstObject == aircraft) {
        FirstObject = aircraft->NextTeamMember;
        aircraft->NextTeamMember = nullptr;
        --EliteAirstrikeTeamTypeIndex;
        aircraft->Airstrike = nullptr;
    } else {
        int count = 0;
        for (auto* p = FirstObject; p && count <= 100; ++count) {
            if (p->NextTeamMember == aircraft) {
                p->NextTeamMember = aircraft->NextTeamMember;
                aircraft->NextTeamMember = nullptr;
                --EliteAirstrikeTeamTypeIndex;
                aircraft->Airstrike = nullptr;
                break;
            }
            p = p->NextTeamMember;
        }
    }
    if (EliteAirstrikeTeamTypeIndex <= 0) {
        IsOnMission = false;
        TeamDissolveFrame = Unsorted::CurrentFrame;
        unknown_bool_3D = false;
    }
}
void AirstrikeClass::InvalidatePointer(void* ptr) {
    auto* object = static_cast<AbstractClass*>(ptr);
    auto* foot = object && (object->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None
                     ? static_cast<FootClass*>(object)
                     : nullptr;
    if (!Game::IsActive) {
        if (ptr == Owner)
            for (auto* p = FirstObject; p; p = p->NextTeamMember)
                p->Airstrike = nullptr;
        else if (foot && foot->Airstrike == this)
            RemoveAircraft(foot);
        return;
    }
    auto* building =
        object && object->WhatAmI() == AbstractType::Building ? static_cast<BuildingClass*>(object) : nullptr;
    if (ptr == Owner) {
        if (unknown_bool_3D && Owner && FirstObject && Owner->Owner->IsControlledByCurrentPlayer())
            VocClass::PlayAt(RulesClass::Instance->AirstrikeAbortSound, FirstObject->Location, 0);
        ClearTarget();
        while (FirstObject)
            RemoveAircraft(FirstObject);
        if (Target && (Target->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None)
            static_cast<TechnoClass*>(Target)->Airstrike = nullptr;
        Target = nullptr;
    } else if (foot && foot->Airstrike == this)
        RemoveAircraft(foot);
    else if (building && building->Airstrike == this && (building->Health <= 0 || !building->IsAlive))
        ClearTarget();
}
