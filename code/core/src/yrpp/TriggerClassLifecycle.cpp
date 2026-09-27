// Original lifecycle/primary vtable from the supplied gamemd exports.
#include "yrpp/TriggerClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TEventClass.h"
#include "yrpp/Memory.h"
#include <cstring>
#include <stdexcept>
namespace { DynamicVectorClass<TriggerClass*> instances; }
DynamicVectorClass<TriggerClass*>& TriggerClass::Array = instances;
AbstractType TriggerClass::WhatAmI() const { return AbsID; }
int TriggerClass::Size() const { return sizeof(*this); }
HRESULT YRPP_STDCALL TriggerClass::GetClassID(CLSID* dest) {
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = { 3224180112u, 298977834u, 1610655660u, 3042641160u };
    std::memcpy(dest, words, sizeof(words)); return 0;
}

TriggerClass::TriggerClass(TriggerTypeClass* type) : AbstractClass(), Type(type),
    NextTrigger(nullptr), House(nullptr), Destroyed(false), align_31{}, Timer{},
    OccuredEvents(0), Enabled(true), padding_45{} {
    Timer.Start(0);
    if (Type) {
        ResetTimers();
        if (!Type->Enabled) Enabled = false;
        else {
            if (!ScenarioClass::Instance) throw std::logic_error("Trigger requires a live Scenario for difficulty");
            const auto difficulty = ScenarioClass::Instance->Difficulty1;
            if (difficulty < 3 && !Type->Difficulty[difficulty]) Enabled = false;
        }
    }
    TypeExpirationListeners.AddItem(this);
    TriggerInstanceExpirationListeners.AddItem(this);
    Array.AddItem(this);
}
TriggerClass::~TriggerClass() {
    for (int i = 0; i < TriggerInstanceExpirationListeners.Count; ++i)
        if (auto* p = TriggerInstanceExpirationListeners[i]) p->PointerExpired(this, true);
    Array.Remove(this);
    TypeExpirationListeners.Remove(this);
    TriggerInstanceExpirationListeners.Remove(this);
}
void TriggerClass::PointerExpired(AbstractClass* object, bool) {
    if (Type == object) Type = nullptr;
    if (NextTrigger == object) NextTrigger = NextTrigger->NextTrigger;
    if (reinterpret_cast<AbstractClass*>(House) == object) House = nullptr;
}
void TriggerClass::ResetTimers() {
    // 726400: elapsed-time and random-delay events share this instance's timer.
    if (!Type) return;
    unsigned index = 0;
    for (auto* event = Type->FirstEvent; event; event = event->NextEvent, ++index) {
        const auto kind = static_cast<unsigned>(event->EventKind);
        if (kind != 13 && kind != 51) continue;
        int value = event->Value;
        if (kind == 51) {
            if (!ScenarioClass::Instance) throw std::logic_error("Random trigger delay requires Scenario random state");
            value = static_cast<int>(static_cast<unsigned>(ScenarioClass::Instance->Random.RandomRanged(0, value))
                + static_cast<unsigned>(value / 2));
        }
        Timer.Start(static_cast<int>(static_cast<unsigned>(value) * 15u));
        OccuredEvents &= ~(1u << (index & 31u));
    }
}
TriggerClass* YRPP_FASTCALL TriggerClass::GetInstance(TriggerTypeClass* type) {
    if (!type) return nullptr;
    for (auto* instance : Array) if (instance->Type == type) return instance;
    return GameCreate<TriggerClass>(type);
}
