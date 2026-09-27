// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tevent.cpp: Attaches_To, operator(), Is_Persistent.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR 0x71F680/0x71E940/0x71F950/0x71F9C0 calibrate event IDs and masks.
#include "yrpp/TEventClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/ObjectClass.h"

TriggerAttachType YRPP_FASTCALL TEventClass::GetAttachType(int kind) {
    unsigned flags=0;
    switch(kind){case 0:case 1:case 4:case 8:case 24:case 25:case 26:case 31:case 53:case 54:case 59:flags|=1;}
    switch(kind){case 0:case 1:case 2:case 4:case 6:case 7:case 8:case 29:case 33:case 34:case 35:case 38:case 39:case 40:case 41:case 42:case 43:case 44:case 48:case 49:flags|=2;}
    if(kind==8||kind==24)flags|=4;
    switch(kind){case 3:case 5:case 8:case 9:case 10:case 11:case 12:case 15:case 16:case 17:case 18:case 19:case 20:case 21:case 22:case 30:case 32:case 52:case 55:case 56:case 57:case 58:flags|=8;}
    switch(kind){case 8:case 13:case 14:case 23:case 27:case 28:case 36:case 37:case 45:case 46:case 47:case 50:case 51:case 60:case 61:flags|=16;}
    return static_cast<TriggerAttachType>(flags);
}
bool TEventClass::GetStateA() const {
    switch(static_cast<unsigned>(EventKind)){
    case 1:case 2:case 3:case 4:case 6:case 7:case 18:case 19:case 20:case 21:case 22:case 23:case 24:case 25:case 26:case 29:case 31:case 33:case 34:case 35:case 38:case 39:case 40:case 41:case 42:case 43:case 44:case 48:case 49:case 50:case 53:case 54:case 59:return true;
    default:return false;}
}
bool TEventClass::GetStateB() const {
    switch(static_cast<unsigned>(EventKind)){case 1:case 6:case 31:case 44:case 59:return false;default:return true;}
}
bool TEventClass::HasOccured(int eventKind,HouseClass*,ObjectClass* object,CDTimerClass* timer,bool* repeating,TechnoClass*) {
    const auto kind=static_cast<unsigned>(EventKind);
    auto* scenario=ScenarioClass::Instance;
    char value=0;
    switch(kind){
    case 1: { // EnteredBy: YR also remembers the entering house for action value 8997.
        if(eventKind!=int(kind)||Unsorted::ArmageddonMode||!object)return false;
        if(Value!=-1){
            auto* house=HouseClass::FindByCountryIndex(Value);
            if(!house||object->GetOwningHouseIndex()!=house->ArrayIndex)return false;
        }
        *repeating=true;House=object->GetOwningHouse();return true;
    }
    // YR 0x71E940 uses country lookup, and excludes aircraft from the
    // unit/all-destroyed totals. Limbo passengers still count as owned.
    case 9:case 10:case 11:{
        HouseClass* house=nullptr;for(auto* candidate:HouseClass::Array)
            if(candidate->Type&&candidate->Type->ArrayIndex==Value){house=candidate;break;}
        if(!house)return true;
        const bool ground=house->ActiveUnitTypes.GetTotal()<=0&&house->ActiveInfantryTypes.GetTotal()<=0;
        return kind==9?ground:kind==10?house->OwnedBuildings<=0:ground&&house->OwnedBuildings<=0;
    }
    case 13:case 51:return timer&&timer->GetTimeLeft()==0;
    case 14:return scenario&&scenario->MissionTimer.IsTicking()&&scenario->MissionTimer.GetTimeLeft()==0;
    case 27:case 28:
        if(!scenario||!scenario->GetGlobal(Value,&value))return false;
        return kind==27?value!=0:value==0;
    case 36:case 37:
        if(!scenario||!scenario->GetLocal(Value,&value))return false;
        return kind==36?value!=0:value==0;
    case 47:return Value<=Unsorted::CurrentFrame/15;
    case 8:return true;
    // Other events require their owning object/house producers; do not
    // interpret an unsupported predicate as a satisfied campaign event.
    default:return false;
    }
}
