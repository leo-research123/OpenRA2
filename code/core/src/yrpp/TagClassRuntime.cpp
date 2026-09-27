// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tag.cpp Spring/Find_Or_Make; YR 0x6E53A0/0x6E52A0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TagClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/TEventClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"

TagClass* YRPP_FASTCALL TagClass::GetInstance(TagTypeClass* type) {
    for(auto* tag:Array)if(tag->Type==type)return tag;
    return GameCreate<TagClass>(type,CellStruct::Empty);
}
void TagClass::Destroy(){Destroyed=true;PendingDeletes.AddItem(this);}
bool TagClass::ContainsTrigger(TriggerClass* trigger) const {
    for(auto* candidate=FirstTrigger;candidate;candidate=candidate->NextTrigger)
        if(candidate==trigger)return true;
    return false;
}
bool TagClass::RaiseEvent(TriggerEvent kind,ObjectClass* object,CellStruct location,bool forced,TechnoClass* source) {
    if(Unsorted::ArmageddonMode||IsExecuting||Destroyed||!Type)return false;
    bool fired=false,remove=false,detach=false;
    IsExecuting=true;
    for(auto* trigger=FirstTrigger;trigger;trigger=trigger->NextTrigger){
        const auto persistence=Type->Persistence;
        if(!trigger->RegisterEvent(kind,object,forced,persistence==TriggerPersistence::Persistent,source))continue;
        switch(persistence){
        case TriggerPersistence::Volatile:
            trigger->FireActions(object,location);trigger->Destroy();fired=remove=detach=true;break;
        case TriggerPersistence::SemiPersistant:
            if(InstanceCount==1){trigger->FireActions(object,location);trigger->Destroy();fired=remove=true;}
            else detach=true;
            break;
        case TriggerPersistence::Persistent:trigger->FireActions(object,location);fired=true;break;
        }
    }
    IsExecuting=false;
    if(detach){
        if(object&&object->AttachedTag==this){object->AttachedTag=nullptr;--InstanceCount;}
        if(location!=CellStruct::Empty){auto* cell=MapClass::Instance.TryGetCellAt(location);if(cell&&cell->AttachedTag){--cell->AttachedTag->InstanceCount;cell->AttachedTag=nullptr;}}
    }
    if(remove){
        // Original 0x7258D0 detaches the worklist before deferred deletion.
        NotifyObjectExpired(true);
        MapClass::Instance.PointerGotInvalid(this,true);
        LogicClass::Instance.PointerGotInvalid(this,true);
        for(auto* receiver:TagExpirationListeners)if(receiver)receiver->PointerExpired(this,true);
        PendingDeletes.AddItem(this);
    }
    return fired;
}
void TagClass::GlobalChanged(int index){for(auto* trigger=FirstTrigger;trigger;trigger=trigger->NextTrigger)trigger->NotifyGlobalChanged(index);}
void TagClass::LocalChanged(int index){for(auto* trigger=FirstTrigger;trigger;trigger=trigger->NextTrigger)trigger->NotifyLocalChanged(index);}
void YRPP_FASTCALL TagClass::NotifyGlobalChanged(int index){for(auto* tag:Array)tag->GlobalChanged(index);}
void YRPP_FASTCALL TagClass::NotifyLocalChanged(int index){for(auto* tag:Array)tag->LocalChanged(index);}
