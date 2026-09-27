// YR 0x00731BF0, shared by cursor conversion and player mission dispatch.
#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/InputManagerClass.h"

bool Game::IsAttackMoveMode() noexcept {
    if(AttackMoveMode)return true;
    auto* input=InputManagerClass::Instance;
    if(!input)return false;
    const bool fire=input->IsForceFireKeyPressed();
    const bool select=input->IsForceSelectKeyPressed();
    if(!fire || !select)return false;
    // Original snapshots the selection count before invoking virtual methods.
    const int count=ObjectClass::CurrentObjects.Count;
    for(int i=0;i<count;++i) {
        auto* object=ObjectClass::CurrentObjects[i];
        if(object && (object->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None
            && !static_cast<TechnoClass*>(object)->CanAttackOnTheMove())return false;
    }
    return true;
}
