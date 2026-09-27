// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744 layer.cpp::Submit / Sorted_Add / Sort.
// YR 0x005519B0 / 0x00551A90 / 0x00551A30; EA Section 7: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/ObjectClass.h"
#include <cstdlib>
#include <limits>

bool LayerClass::AddObject(ObjectClass* object,bool sorted) {
    try {
        if(!sorted)return AddItem(object);
        if(!object)return false; // invalid receiver: original dereferences null
        // Grow before comparisons, without exposing a partially inserted object.
        if(Count>=Capacity) {
            if((!IsAllocated && Capacity) || CapacityIncrement<=0 ||
                Capacity>std::numeric_limits<int>::max()-CapacityIncrement ||
                !SetCapacity(Capacity+CapacityIncrement,nullptr))return false;
        }
        int index=0;
        for(;index<Count;++index) {
            // Original operator> (0x005F6220) calls incoming, then existing.
            const int incoming=object->GetYSort();
            if(Items[index]->GetYSort()>incoming)break;
        }
        for(int i=Count;i>index;--i)Items[i]=Items[i-1];
        Items[index]=object;++Count;return true;
    } catch(...) {return false;}
}
void LayerClass::Sort() {
    try {
        // One forward pass, live Count and stable equal keys, not a full sort.
        for(int i=0;i<Count-1;++i) {
            auto* next=Items[i+1];auto* current=Items[i];
            const int next_y=next->GetYSort();
            if(next_y<current->GetYSort()) {Items[i+1]=current;Items[i]=next;}
        }
    } catch(...) {std::abort();} // cannot return a partial sort across this void ABI
}
