// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 scenario.cpp tag distribution during scenario finishing.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "scenario_loading.hpp"
#include "yrpp/TagClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include <new>
namespace game {
void load_scenario_tag_instances() {
    for(auto* type:TagTypeClass::Array){
        const auto flags=type->GetFlags();
        if(!(flags&0x1C))continue;
        auto* tag=TagClass::GetInstance(type);
        if(!tag)throw std::bad_alloc();
        if((flags&4)&&MapClass::PendingTags.FindItemIndex(tag)<0)MapClass::PendingTags.AddItem(tag);
        if((flags&16)&&LogicClass::PendingTags.FindItemIndex(tag)<0)LogicClass::PendingTags.AddItem(tag);
        if((flags&8)&&type->FirstTrigger)for(auto* house:HouseClass::Array)
            if(house->Type==type->FirstTrigger->House&&house->RelatedTags.FindItemIndex(tag)<0)house->RelatedTags.AddItem(tag);
    }
}
}
