// YRpp 9402d7da; constructor 00726C80, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/TriggerTypeClass.h"
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

namespace { DynamicVectorClass<TriggerTypeClass*> types; }
DynamicVectorClass<TriggerTypeClass*>& TriggerTypeClass::Array = types;
TriggerTypeClass* YRPP_FASTCALL TriggerTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL TriggerTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

TriggerTypeClass::TriggerTypeClass(char const* pName)
    : AbstractTypeClass(pName),
      ArrayIndex{},
      Difficulty{},
      Enabled{},
      MustTransfer{},
      align_A1{},
      House{},
      NextTrigger{},
      FirstEvent{},
      FirstAction{} {
    ArrayIndex = -1;
    Difficulty[0] = true;
    Difficulty[1] = true;
    Difficulty[2] = true;
    Enabled = true;
    MustTransfer = false;
    House = nullptr;
    NextTrigger = nullptr;
    FirstEvent = nullptr;
    FirstAction = nullptr;
    ArrayIndex = Array.Count;
    Array.AddItem(this);
    TypeExpirationListeners.AddItem(this);
    TriggerExpirationListeners.AddItem(this);
}

TriggerTypeClass::~TriggerTypeClass() {
    NotifyTypeExpired();
    Array.Remove(this);
    auto* action = FirstAction; FirstAction = nullptr;
    while (action) { auto* next = action->NextAction; action->NextAction = nullptr; GameDelete(action); action = next; }
    auto* event = FirstEvent; FirstEvent = nullptr;
    while (event) { auto* next = event->NextEvent; event->NextEvent = nullptr; GameDelete(event); event = next; }
    TypeExpirationListeners.Remove(this);
    TriggerExpirationListeners.Remove(this);
}
int TriggerTypeClass::GetArrayIndex() const { return ArrayIndex; }
void TriggerTypeClass::PointerExpired(AbstractClass* object, bool removed) {
    (void)removed;
    if (NextTrigger == object) NextTrigger = NextTrigger->NextTrigger;
    for (auto* action = FirstAction; action; action = action->NextAction) action->PointerExpired(object, true);
    if (FirstAction == object) FirstAction = FirstAction->NextAction;
    for (auto* event = FirstEvent; event; event = event->NextEvent) event->PointerExpired(object, true);
    if (FirstEvent == object) FirstEvent = FirstEvent->NextEvent;
}
