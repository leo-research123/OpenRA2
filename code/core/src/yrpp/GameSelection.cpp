// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 init.cpp selection commands; YR-specific UICommands helpers
// calibrated to 0x00732600/0x007327D0 and
// 0x00732770. These use the original Techno and Tactical selection arrays.
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md.
#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/HouseClass.h"
#include "scenario_runtime.hpp"
#include <new>

void YRPP_FASTCALL Game::SetSelectionCommandMode(int mode) noexcept {
    SelectionCommandMode=mode;
    AttackMoveMode=false;
}
#if !defined(RA2_YRPP_GAME)
bool Game::IsTypeSelecting() noexcept {return TypeSelectionActive;}
#endif
namespace {
bool matches(TechnoClass* unit,const char* name) {
    if(!unit || !unit->IsAlive)return false;
    const auto& runtime=game::scenario_runtime();
    const bool controlled=runtime.session_mode(runtime.context)!=0
        ? unit->Owner->IsControlledByCurrentPlayer():unit->Owner->IsInPlayerControl;
    return controlled && !_strcmpi(name,unit->GetType()->ID);
}
DynamicVectorClass<TechnoClass*>* collect(const char* name) {
    // The original dereferences this allocation even when it fails. Native
    // allocation failure therefore remains fatal at the noexcept entry rather
    // than propagating an exception or reporting a successful empty selection.
    auto* found=new DynamicVectorClass<TechnoClass*>;
    const auto append=[&](TechnoClass* unit) {
        try {found->AddItem(unit);}
        // Original vector growth returns false and retains its old storage;
        // restore the flag that SetCapacity clears before a native allocation.
        catch(const std::bad_alloc&) {found->IsInitialized=true;}
    };
    if(Game::TypeSelectionIncludesMap) {
        const int count=TechnoClass::Array.Count;
        for(int i=0;i<count;++i) {
            auto* unit=TechnoClass::Array[i];
            if(matches(unit,name))append(unit);
        }
    } else {
        const int count=TacticalClass::Instance->SelectableCount;
        for(int i=0;i<count;++i) {
            auto* object=TacticalClass::SelectableObjects[i].Object;
            if(object && (object->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None
                && matches(static_cast<TechnoClass*>(object),name))append(static_cast<TechnoClass*>(object));
        }
    }
    return found;
}
}
#if !defined(RA2_YRPP_GAME)
void YRPP_FASTCALL Game::UICommands_TypeSelect_7327D0(const char* name) noexcept {
    const bool voice=Unsorted::MoveFeedback;
    Unsorted::MoveFeedback=false;
    auto* found=collect(name);
    const int count=found->Count;
    for(int i=0;i<count;++i)found->Items[i]->Select();
    delete found;
    Unsorted::MoveFeedback=voice;
}
#endif
void YRPP_FASTCALL Game::UICommands_TypeDeselect(const char* name) noexcept {
    auto* found=collect(name);
    const int count=found->Count;
    for(int i=0;i<count;++i)found->Items[i]->Deselect();
    delete found;
}
