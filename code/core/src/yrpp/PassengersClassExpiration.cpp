// Original 0x004734B0. Removing cargo does not clear the removed node's link.
#include "yrpp/FootClass.h"
#include "yrpp/TeamClass.h"
#include "RulesClassReaders.hpp"

// OpenTS CargoClass::Attach; YR 0x4733A0 preserves a linked group and
// recomputes its length. The original passenger object is placed in limbo.
void PassengersClass::AddPassenger(FootClass* passenger){
 if(!passenger)return;
 passenger->Limbo();auto* tail=passenger;
 while(tail->NextObject&&(tail->NextObject->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None)
  tail=static_cast<FootClass*>(tail->NextObject);
 tail->NextObject=FirstPassenger;FirstPassenger=passenger;NumPassengers=0;
 for(auto* node=static_cast<ObjectClass*>(FirstPassenger);node&&(node->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None;node=node->NextObject)++NumPassengers;
}
int PassengersClass::GetTotalSize() const {
 int size=0;auto* node=FirstPassenger;
 for(int i=0;node&&i<NumPassengers;++i){size=rule_integer(double(size)+node->GetTechnoType()->Size);node=static_cast<FootClass*>(node->NextObject);}
 return size;
}
int PassengersClass::IndexOf(FootClass* passenger) const {
 int index=0;
 for(auto* node=static_cast<ObjectClass*>(FirstPassenger);node&&(node->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None;node=node->NextObject){++index;if(node==passenger)return index;}
 return 0;
}

FootClass* PassengersClass::RemoveFirstPassenger() {
    auto* result=FirstPassenger;
    if(result) {
        FirstPassenger=static_cast<FootClass*>(result->NextObject);
        result->NextObject=nullptr;
        --NumPassengers;
    }
    return result;
}
void PassengersClass::RemovePassenger(FootClass* passenger) {
    if (!passenger) return;
    ObjectClass* previous = nullptr;
    for (ObjectClass* node = FirstPassenger; node; node = node->NextObject) {
        if (node != passenger) { previous = node; continue; }
        if (previous) previous->NextObject = node->NextObject;
        else FirstPassenger = static_cast<FootClass*>(node->NextObject);
        --NumPassengers;
        return;
    }
}

// OpenTS Kill_Cargo, YR 0x707CB0. Pop before recursive destruction because
// pointer-expiry notifications can mutate both the cargo and team chains.
void TechnoClass::KillPassengers(TechnoClass* source) {
    while(Passengers.GetFirstPassenger()) {
        auto* passenger=Passengers.GetFirstPassenger();
        if(passenger->Team)passenger->Team->LiberateMember(passenger,-1,0);
        passenger=Passengers.RemoveFirstPassenger();
        if(passenger) {
            passenger->KillPassengers(source);
            passenger->RegisterDestruction(source);
            passenger->UnInit();
        }
    }
}
