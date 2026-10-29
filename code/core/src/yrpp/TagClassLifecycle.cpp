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
#include "runtime_diagnostics.hpp"
namespace {
DynamicVectorClass<TagClass*> instances;
}
DynamicVectorClass<TagClass*>& TagClass::Array = instances;
AbstractType TagClass::WhatAmI() const { return AbsID; }
int TagClass::Size() const { return sizeof(*this); }
HRESULT YRPP_STDCALL TagClass::GetClassID(CLSID* dest) {
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {1425466418u, 298977773u, 1610655148u, 3042641160u};
    std::memcpy(dest, words, sizeof(words));
    return 0;
}

TagClass::TagClass(TagTypeClass* type) noexcept : TagClass(type, CellStruct::Empty) {}

TagClass::TagClass(TagTypeClass* type, const CellStruct& coords)
    : AbstractClass(), Type(type), FirstTrigger(nullptr), InstanceCount(0), DefaultCoords(coords), Destroyed(false),
      IsExecuting(false), padding_36{} {
    Array.AddItem(this);
    TypeExpirationListeners.AddItem(this);
    TriggerInstanceExpirationListeners.AddItem(this);
    try {
        if (type)
            for (auto* t = type->FirstTrigger; t; t = t->NextTrigger) {
                auto* trigger = GameCreate<TriggerClass>(t);
                if (!trigger) throw std::bad_alloc();
                trigger->NextTrigger = FirstTrigger;
                FirstTrigger = trigger;
            }
    } catch (...) {
        while (FirstTrigger) {
            auto* t = FirstTrigger;
            FirstTrigger = t->NextTrigger;
            t->NextTrigger = nullptr;
            GameDelete(t);
        }
        TypeExpirationListeners.Remove(this);
        TriggerInstanceExpirationListeners.Remove(this);
        Array.Remove(this);
        throw;
    }
    game::runtime_log_tag_created(*this);
}
TagClass::~TagClass() {
    game::runtime_log_tag("tag_destructor", this, -1);
    // 0x6E4F60 announces the Tag through 0x7258D0 before and after unlinking it.
    NotifyObjectExpired(true);
    for (auto* trigger = FirstTrigger; trigger; trigger = trigger->NextTrigger)
        PendingDeletes.AddItem(trigger);
    TypeExpirationListeners.Remove(this);
    TriggerInstanceExpirationListeners.Remove(this);
    Array.Remove(this);
    NotifyObjectExpired(true);
    game::runtime_log_tag_deleted(this);
}
void TagClass::PointerExpired(AbstractClass* object, bool) {
    if (Type == object) Type = nullptr;
    if (FirstTrigger == object) FirstTrigger = FirstTrigger->NextTrigger;
    if (!Type) {
        Destroyed = true;
        PendingDeletes.AddItem(this);
    }
}
#if !defined(RA2_YRPP_GAME)
void YRPP_STDCALL TagClass::DeleteAll() {
    while (Array.Count > 0) {
        if (auto* tag = Array[0])
            delete tag;
        else
            Array.RemoveItem(0);
    }
}
#endif
