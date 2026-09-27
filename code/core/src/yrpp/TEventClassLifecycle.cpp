// Original 71E6A0/71FA80/71F800; see supplied primary vtable evidence.
#include "yrpp/TEventClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include <cstring>
namespace { DynamicVectorClass<TEventClass*> nodes; }
DynamicVectorClass<TEventClass*>& TEventClass::Array = nodes;
TEventClass::TEventClass() : AbstractClass(), ArrayIndex(-1), NextEvent(nullptr), EventKind{}, TeamType(nullptr), Value(0), String{}, House(nullptr) {
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
    TypeExpirationListeners.AddItem(this);
    TriggerExpirationListeners.AddItem(this);
}
TEventClass::~TEventClass() {
    NotifyTriggerNodeExpired();
    TriggerExpirationListeners.Remove(this); TypeExpirationListeners.Remove(this);
    Array.Remove(this);
}
void TEventClass::PointerExpired(AbstractClass* object, bool) {
    if (NextEvent == object) NextEvent = NextEvent->NextEvent;
    if (TeamType == object) TeamType = nullptr;
}
int TEventClass::GetArrayIndex() const { return ArrayIndex; }
AbstractType TEventClass::WhatAmI() const { return AbsID; }
int TEventClass::Size() const { return sizeof(*this); }
HRESULT YRPP_STDCALL TEventClass::GetClassID(CLSID* dest) {
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = { 1326367635u, 298977877u, 1610655660u, 3042641160u };
    std::memcpy(dest, words, sizeof(words));
    return 0;
}
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(TEventClass) == 88);
#endif
