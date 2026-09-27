// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 taction.cpp operator(); YR 0x6DD8B0/0x6E2AF0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TActionClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TagClass.h"
#include "scenario_loading.hpp"

// OpenTS TAction_CHANGE_HOUSE, with YR 0x006E0AA0's trigger requirement,
// house selectors and silent transfer. The source is the entering engineer;
// the recipients are all live, placed technos whose tag contains this trigger.
// ALL07's allied Soviet base relies on this action before engineer repair.
bool TActionClass::SwitchAttachedObjectsToHouse(HouseClass*,ObjectClass*,TriggerClass* trigger,const CellStruct&) {
    if(!trigger||Value==-1)return false;
    auto* house=Value==8997?trigger->GetHouse():HouseClass::Index_IsMP(Value)
        ?HouseClass::FindByIndex(Value):HouseClass::FindByCountryIndex(Value);
    if(!house)return false;
    bool changed=false;
    for(int i=0;i<TechnoClass::Array.Count;++i){
        auto* techno=TechnoClass::Array[i];
        if(techno->IsAlive&&techno->IsOnMap&&!techno->InLimbo&&techno->AttachedTag&&techno->AttachedTag->ContainsTrigger(trigger)){
            techno->SetOwningHouse(house,false);changed=true;
        }
    }
    return changed;
}

bool TActionClass::Execute(HouseClass* house,ObjectClass* object,TriggerClass* trigger,const CellStruct& location) {
    auto* scenario=ScenarioClass::Instance;
    switch(ActionKind){
    case TriggerAction::ChangeHouse:return SwitchAttachedObjectsToHouse(house,object,trigger,location);
    case TriggerAction::ReinforcementAt:return TeamType&&Waypoint!=-1&&game::reinforce_team(*TeamType,Waypoint);
    case TriggerAction::Reinforcement:return TeamType&&game::reinforce_team(*TeamType,-1);
    case TriggerAction::EnableTrigger:
        for(auto* trigger:TriggerClass::Array)if(trigger->Type==TriggerType&&TriggerType){
            const auto difficulty=scenario->Difficulty1;
            if(difficulty>=3||TriggerType->Difficulty[difficulty])trigger->Enable();
        }
        return true;
    case TriggerAction::DisableTrigger:
        for(auto* trigger:TriggerClass::Array)if(trigger->Type==TriggerType&&TriggerType)trigger->Disable();
        return true;
    case TriggerAction::GlobalSet:scenario->SetGlobal(Value,1);return true;
    case TriggerAction::GlobalClear:scenario->SetGlobal(Value,0);return true;
    case TriggerAction::LocalSet:scenario->SetLocal(Value,1);return true;
    case TriggerAction::LocalClear:scenario->SetLocal(Value,0);return true;
    default:return false;
    }
}
