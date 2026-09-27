// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 trigger.cpp Should_Spring/Spring; YR 0x7264C0/0x7265C0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TriggerClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/TEventClass.h"
#include "yrpp/TActionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"

namespace {
HouseClass* owner(const TriggerTypeClass* type) {
    if(type&&type->House)for(auto* house:HouseClass::Array)
        if(house->Type==type->House)return house;
    return nullptr;
}
}
bool TriggerClass::RegisterEvent(TriggerEvent kind,ObjectClass* object,bool forced,bool persistent,TechnoClass* source) {
    if(!Enabled||Destroyed||!Type)return false;
    bool all=true;
    if(!forced){
        unsigned index=0;
        for(auto* event=Type->FirstEvent;event;event=event->NextEvent,++index){
            const unsigned mask=1u<<(index&31u);
            if((OccuredEvents&mask)||event->HasOccured(static_cast<int>(kind),owner(Type),object,&Timer,&persistent,source)){
                if(event->House)House=event->House;
                if(persistent&&event->GetStateA()&&event->GetStateB())OccuredEvents|=mask;
            }else all=false;
        }
    }
    if(all&&persistent)ResetTimers();
    return all;
}
bool TriggerClass::FireActions(ObjectClass* object,CellStruct location) {
    if(!Enabled||Destroyed||!Type)return false;
    bool fired=false;
    for(auto* action=Type->FirstAction;action;action=action->NextAction)
        if(action->Execute(owner(Type),object,this,location))fired=true;
    return fired;
}
void TriggerClass::Destroy() {
    Destroyed=true;
    PendingDeletes.AddItem(this);
}
void TriggerClass::NotifyGlobalChanged(int index) {
    if(Type)for(auto* event=Type->FirstEvent;event;event=event->NextEvent)
        if((event->EventKind==TriggerEvent::GlobalSet||event->EventKind==TriggerEvent::GlobalCleared)&&event->Value==index){ResetTimers();break;}
}
void TriggerClass::NotifyLocalChanged(int index) {
    if(Type)for(auto* event=Type->FirstEvent;event;event=event->NextEvent)
        if((static_cast<unsigned>(event->EventKind)==36||static_cast<unsigned>(event->EventKind)==37)&&event->Value==index){ResetTimers();break;}
}
