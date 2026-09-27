// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 factory.cpp; YR 0x4C98B0..0x4CA6B0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FactoryClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <cstdlib>

FactoryClass::FactoryClass() noexcept : AbstractClass(),Production{},QueuedObjects{},Object{},OnHold{},IsDifferent{},
    Balance{},OriginalBalance{},SpecialItem{-1},Owner{},IsSuspended{},IsManual{true}{
    Production.Start(0);if(!Array.AddItem(this))std::abort();
}
FactoryClass::~FactoryClass(){AbandonProduction();Array.Remove(this);}
void FactoryClass::Update(){
    if(IsSuspended||(!Object&&!SpecialItem)||IsDone()||!Production.Update())return;
    IsDifferent=true;const int cost=std::min(GetCostPerStep(),Balance);
    if(cost>Owner->Available_Money()){OnHold=true;--Production.Value;}
    else {Owner->TakeMoney(cost);OnHold=false;Balance-=cost;}
    if(Production.Value==54){IsSuspended=true;Production.Start(0);Owner->TakeMoney(Balance);Balance=0;}
}
bool FactoryClass::HasProgressChanged(){const bool changed=IsDifferent;IsDifferent=false;return changed;}
bool FactoryClass::DemandProduction(const TechnoTypeClass* type,HouseClass* owner,bool resume){
    if(type->WhatAmI()==AbstractType::BuildingType)AbandonProduction();
    if(type->WhatAmI()==AbstractType::BuildingType||((!Production.Rate||IsSuspended)&&!QueuedObjects.Count&&(!Object||!IsSuspended))||resume){
        IsDifferent=IsSuspended=true;Production.Start(0);Production.Value=0;
        Object=static_cast<TechnoClass*>(const_cast<TechnoTypeClass*>(type)->CreateObject(owner));
        if(!Object)return false;
        // Native constructors leave locomotor setup to the placement caller.
        bool ready=true;
        if(Object->WhatAmI()==AbstractType::Infantry)ready=static_cast<InfantryClass*>(Object)->InitializeLocomotor();
        else if(Object->WhatAmI()==AbstractType::Unit)ready=static_cast<UnitClass*>(Object)->InitializeLocomotor();
        if(!ready){GameDelete(Object);Object=nullptr;return false;}
        Owner=Object->Owner;Balance=type->GetActualCost(Owner);Object->Value=Balance;return true;
    }
    if(QueuedObjects.Count>=RulesClass::Instance->MaximumQueuedObjects||owner->HasReachedBuildLimit(const_cast<TechnoTypeClass*>(type)))return false;
    return QueuedObjects.AddItem(const_cast<TechnoTypeClass*>(type));
}
void FactoryClass::SetObject(TechnoClass* object){
    AbandonProduction();Object=object;Owner=object->Owner;Balance=0;Production.Value=54;IsSuspended=IsDifferent=true;
}
bool FactoryClass::Suspend(bool manual){
    if(IsSuspended)return false;IsManual=manual;IsSuspended=true;Production.Start(0);return true;
}
bool FactoryClass::Unsuspend(bool manual){
    if((!Object&&!SpecialItem)||!IsSuspended||IsDone())return false;
    IsSuspended=false;Production.Start(std::clamp((Object?Object->TimeToBuild():0)/54,1,255));
    if(Owner->Available_Money()<GetCostPerStep())return false;
    IsManual=true;if(manual)Suspend(true);return true;
}
int FactoryClass::GetBuildTimeFrames() const{return std::clamp((Object?Object->TimeToBuild():0)/54,1,255);}
int FactoryClass::GetProgress() const{return Production.Value;}
bool FactoryClass::IsDone() const{return (Object||SpecialItem!=-1)&&Production.Value==54;}
int FactoryClass::GetCostPerStep() const{return !Object?0:Production.Value==54?Balance:Balance/(54-Production.Value);}
bool FactoryClass::AbandonProduction(){
    if(!Object)return false;
    Owner->GiveMoney(Object->GetType()->GetActualCost(Object->Owner)-Balance);Balance=0;
    if(SpecialItem)SpecialItem=-1;
    Production.Start(0);Production.Value=0;IsSuspended=IsDifferent=true;
    ++Unsorted::ScenarioInit;GameDelete(Object);Object=nullptr;--Unsorted::ScenarioInit;return true;
}
bool FactoryClass::CompletedProduction(){
    if((Object||SpecialItem)&&Production.Value==54){
        if(Object)Object=nullptr;else SpecialItem=-1;
        IsSuspended=IsDifferent=true;Production.Value=0;Production.Start(0);return true;
    }
    return false;
}
void FactoryClass::StartProduction(){
    if(!QueuedObjects.Count||Object||(Production.Rate&&!IsSuspended))return;
    auto* type=QueuedObjects[0];QueuedObjects.RemoveItem(0);
    const int index=type->GetArrayIndex();if(index>=0)Owner->BeginProduction(type->WhatAmI(),index,type->Naval,true);
}
bool FactoryClass::RemoveOneFromQueue(const TechnoTypeClass* type){return QueuedObjects.Remove(const_cast<TechnoTypeClass*>(type));}
int FactoryClass::CountTotal(const TechnoTypeClass* type) const {
    int count=Object&&Object->GetTechnoType()==type;for(auto* queued:QueuedObjects)if(queued==type)++count;return count;
}
bool FactoryClass::IsQueued(const TechnoTypeClass* type) const {return QueuedObjects.FindItemIndex(const_cast<TechnoTypeClass*>(type))>=0;}
