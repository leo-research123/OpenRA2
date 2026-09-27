// YRpp 9402d7da; constructor 006E5B60, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/TagTypeClass.h"
#include "type_registry.hpp"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ParticleTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TEventClass.h"
#include "yrpp/TActionClass.h"
#include <cstring>
#include <new>

namespace { DynamicVectorClass<TagTypeClass*> types; }
DynamicVectorClass<TagTypeClass*>& TagTypeClass::Array = types;
TagTypeClass* YRPP_FASTCALL TagTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL TagTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }
TagTypeClass* YRPP_FASTCALL TagTypeClass::FindByNameOrID(const char* name) {
    if(name)for(auto* type:Array)if(!_strcmpi(type->ID,name)||!_strcmpi(type->Name,name))return type;
    return nullptr;
}

TagTypeClass::TagTypeClass(char const* pName)
    : AbstractTypeClass(pName),
      ArrayIndex{},
      Persistence{},
      FirstTrigger{} {
    ArrayIndex = -1;
    Persistence = static_cast<::TriggerPersistence>(0x0u);
    FirstTrigger = nullptr;
    ArrayIndex = Array.Count;
    Array.AddItem(this);
    TypeExpirationListeners.AddItem(this);
}

TagTypeClass::~TagTypeClass() {
    NotifyTypeExpired();
    auto* trigger = FirstTrigger; FirstTrigger = nullptr;
    while (trigger) { auto* next = trigger->NextTrigger; trigger->NextTrigger = nullptr; GameDelete(trigger); trigger = next; }
    DestroyTagInstances();
    TypeExpirationListeners.Remove(this);
    Array.Remove(this);
}
int TagTypeClass::GetArrayIndex() const { return ArrayIndex; }
void TagTypeClass::PointerExpired(AbstractClass* object, bool removed) {
    (void)removed;
    if (FirstTrigger == object) FirstTrigger = FirstTrigger->NextTrigger;
}

#include "yrpp/TagClass.h"
void TagTypeClass::DestroyTagInstances() {
    for (int i = 0; i < TagClass::Array.Count;) {
        auto* tag = TagClass::Array[i];
        if (tag && tag->Type == this) GameDelete(tag);
        else ++i;
    }
}
