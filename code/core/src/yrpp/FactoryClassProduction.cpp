// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 factory.cpp; YR 0x4C98B0..0x4CA6B0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FactoryClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <cstdlib>

FactoryClass::FactoryClass() noexcept
    : AbstractClass(), Production{}, QueuedObjects{}, Object{}, OnHold{}, IsDifferent{}, Balance{}, OriginalBalance{},
      SpecialItem{-1}, Owner{}, IsSuspended{}, IsManual{true} {
    Production.Start(0);
    if (!Array.AddItem(this) || !AbstractClass::Array.AddItem(this)) std::abort();
}
FactoryClass::~FactoryClass() {
    NotifyObjectExpired(true);
    Array.Remove(this);
    if (Game::IsActive) AbandonProduction();
    AbstractClass::Array.Remove(this);
}
void FactoryClass::PointerExpired(AbstractClass* object, bool) {
    if (object == Object) Object = nullptr;
}
void FactoryClass::Update() {
    if (IsSuspended || (!Object && !SpecialItem) || IsDone() || !Production.Update()) return;
    IsDifferent = true;
    const int cost = std::min(GetCostPerStep(), Balance);
    if (cost > Owner->Available_Money()) {
        OnHold = true;
        --Production.Value;
    } else {
        Owner->TakeMoney(cost);
        OnHold = false;
        Balance -= cost;
    }
    if (Production.Value == 54) {
        IsSuspended = true;
        Production.Start(0);
        Owner->TakeMoney(Balance);
        Balance = 0;
    }
}
bool FactoryClass::HasProgressChanged() {
    const bool changed = IsDifferent;
    IsDifferent = false;
    return changed;
}
bool FactoryClass::DemandProduction(const TechnoTypeClass* type, HouseClass* owner, bool resume) {
    if (type->WhatAmI() == AbstractType::BuildingType) AbandonProduction();
    if (type->WhatAmI() == AbstractType::BuildingType ||
        ((!Production.Rate || IsSuspended) && !QueuedObjects.Count && (!Object || !IsSuspended)) || resume) {
        IsDifferent = IsSuspended = true;
        Production.Start(0);
        Production.Value = 0;
        Object = static_cast<TechnoClass*>(const_cast<TechnoTypeClass*>(type)->CreateObject(owner));
        if (!owner->IsControlledByHuman() && Object && Object->WhatAmI() == AbstractType::Building)
            static_cast<BuildingClass*>(Object)->BeingProduced = true;
        if (!Object) return false;
        // Mobile constructors own locomotor creation before CreateObject returns.
        Owner = Object->Owner;
        Balance = type->GetActualCost(Owner);
        Object->Value = Balance;
        return true;
    }
    if (QueuedObjects.Count >= RulesClass::Instance->MaximumQueuedObjects ||
        owner->HasReachedBuildLimit(const_cast<TechnoTypeClass*>(type))) {
        if (owner->IsControlledByCurrentPlayer()) VocClass::PlayGlobal(RulesClass::Instance->ScoldSound, 0x2000, 1.0f);
        return false;
    }
    // The original accepts this request even when queue storage cannot grow.
    QueuedObjects.AddItem(const_cast<TechnoTypeClass*>(type));
    return true;
}
void FactoryClass::SetObject(TechnoClass* object) {
    AbandonProduction();
    Object = object;
    Owner = object->Owner;
    Balance = 0;
    Production.Start(0);
    Production.Value = 54;
    IsSuspended = IsDifferent = true;
}
bool FactoryClass::Suspend(bool manual) {
    if (IsSuspended) return false;
    IsManual = manual;
    IsSuspended = true;
    Production.Start(0);
    return true;
}
bool FactoryClass::Unsuspend(bool manual) {
    if ((!Object && !SpecialItem) || !IsSuspended || IsDone()) return false;
    IsSuspended = false;
    Production.Start(std::clamp((Object ? Object->TimeToBuild() : 0) / 54, 1, 255));
    if (Owner->Available_Money() < GetCostPerStep()) return false;
    IsManual = true;
    if (manual) Suspend(true);
    return true;
}
int FactoryClass::GetBuildTimeFrames() const { return std::clamp((Object ? Object->TimeToBuild() : 0) / 54, 1, 255); }
int FactoryClass::GetProgress() const { return Production.Value; }
bool FactoryClass::IsDone() const { return (Object || SpecialItem != -1) && Production.Value == 54; }
int FactoryClass::GetCostPerStep() const {
    return !Object ? 0 : Production.Value == 54 ? Balance : Balance / (54 - Production.Value);
}
bool FactoryClass::AbandonProduction() {
    if (!Object) return false;
    // 0x4C9FF0 performs a discarded type query before the refund query.
    // Keep the virtual call and reload Object after its possible callback.
    (void)Object->GetType();
    Owner->GiveMoney(Object->GetType()->GetActualCost(Object->Owner) - Balance);
    Balance = 0;
    if (SpecialItem) SpecialItem = -1;
    Production.Start(0);
    Production.Value = 0;
    IsSuspended = IsDifferent = true;
    if (!Owner->IsControlledByHuman()) {
        if (Object->WhatAmI() == AbstractType::Infantry) Owner->ProducingInfantryTypeIndex = -1;
        if (Object->WhatAmI() == AbstractType::Unit) Owner->ProducingUnitTypeIndex = -1;
        if (Object->WhatAmI() == AbstractType::Aircraft) Owner->ProducingAircraftTypeIndex = -1;
        if (Object->WhatAmI() == AbstractType::Building) Owner->ProducingBuildingTypeIndex = -1;
    }
    ++Unsorted::ScenarioInit;
    delete Object;
    Object = nullptr;
    --Unsorted::ScenarioInit;
    return true;
}
bool FactoryClass::CompletedProduction() {
    if ((Object || SpecialItem) && Production.Value == 54) {
        if (Object)
            Object = nullptr;
        else
            SpecialItem = -1;
        IsSuspended = IsDifferent = true;
        Production.Value = 0;
        Production.Start(0);
        return true;
    }
    return false;
}
void FactoryClass::StartProduction() {
    if (!QueuedObjects.Count || Object || (Production.Rate && !IsSuspended)) return;
    auto* type = QueuedObjects[0];
    QueuedObjects.RemoveItem(0);
    const int index = type->GetArrayIndex();
    if (index >= 0) Owner->BeginProduction(type->WhatAmI(), index, type->Naval, true);
}
bool FactoryClass::RemoveOneFromQueue(const TechnoTypeClass* type) {
    return QueuedObjects.Remove(const_cast<TechnoTypeClass*>(type));
}
int FactoryClass::CountTotal(const TechnoTypeClass* type) const {
    int count = Object && Object->GetTechnoType() == type;
    for (auto* queued : QueuedObjects)
        if (queued == type) ++count;
    return count;
}
bool FactoryClass::IsQueued(const TechnoTypeClass* type) const {
    for (int i = 0; i < QueuedObjects.Count; ++i)
        if (QueuedObjects[i] == type) return true;
    return false;
}
