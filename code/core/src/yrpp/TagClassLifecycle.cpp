// Original lifecycle/primary vtable from the supplied gamemd exports.
#include "yrpp/TagClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TEventClass.h"
#include "yrpp/Memory.h"
#include <cstring>
#include <stdexcept>
namespace { DynamicVectorClass<TagClass*> instances; }
DynamicVectorClass<TagClass*>& TagClass::Array = instances;
AbstractType TagClass::WhatAmI() const { return AbsID; }
int TagClass::Size() const { return sizeof(*this); }
HRESULT YRPP_STDCALL TagClass::GetClassID(CLSID* dest) {
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = { 1425466418u, 298977773u, 1610655148u, 3042641160u };
    std::memcpy(dest, words, sizeof(words)); return 0;
}

void (*TagClass::NativeWorldExpiration)(TagClass*, bool) noexcept = nullptr;
TagClass::TagClass(TagTypeClass* type, const CellStruct& coords) : AbstractClass(),
    Type(type), FirstTrigger(nullptr), InstanceCount(0), DefaultCoords(coords),
    Destroyed(false), IsExecuting(false), padding_36{} {
    Array.AddItem(this);
    TypeExpirationListeners.AddItem(this);
    TriggerInstanceExpirationListeners.AddItem(this);
    try {
        if (type) for (auto* t = type->FirstTrigger; t; t = t->NextTrigger) {
            auto* trigger = GameCreate<TriggerClass>(t);
            if (!trigger) throw std::bad_alloc();
            trigger->NextTrigger = FirstTrigger;
            FirstTrigger = trigger;
        }
    } catch (...) {
        while (FirstTrigger) { auto* t = FirstTrigger; FirstTrigger = t->NextTrigger; t->NextTrigger = nullptr; GameDelete(t); }
        TypeExpirationListeners.Remove(this); TriggerInstanceExpirationListeners.Remove(this); Array.Remove(this);
        throw;
    }
}
TagClass::~TagClass() {
    for (int i = 0; i < TagExpirationListeners.Count; ++i)
        if (auto* p = TagExpirationListeners[i]) p->PointerExpired(this, true);
    // World/session bookkeeping of 6E4F60 is explicitly supplied by the host;
    // no-world local construction does not fabricate a map, player or UI.
    if (NativeWorldExpiration) NativeWorldExpiration(this, false);
    for (auto* trigger = FirstTrigger; trigger; trigger = trigger->NextTrigger) PendingDeletes.AddItem(trigger);
    TypeExpirationListeners.Remove(this);
    TriggerInstanceExpirationListeners.Remove(this);
    Array.Remove(this);
    for (int i = 0; i < TagExpirationListeners.Count; ++i)
        if (auto* p = TagExpirationListeners[i]) p->PointerExpired(this, true);
    if (NativeWorldExpiration) NativeWorldExpiration(this, true);
}
void TagClass::PointerExpired(AbstractClass* object, bool) {
    if (Type == object) Type = nullptr;
    if (FirstTrigger == object) FirstTrigger = FirstTrigger->NextTrigger;
    if (!Type) { Destroyed = true; PendingDeletes.AddItem(this); }
}
