// Original 6DD000/6E4660/6DD2C0; see supplied primary vtable evidence.
#include "yrpp/TActionClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include <cstring>
namespace { DynamicVectorClass<TActionClass*> nodes; }
DynamicVectorClass<TActionClass*>& TActionClass::Array = nodes;
TActionClass::TActionClass() : AbstractClass(), ArrayIndex(-1), NextAction(nullptr), ActionKind{}, TeamType(nullptr), Bounds{}, Waypoint(0), Value2(0), TagType(nullptr), TriggerType(nullptr), TechnoID{}, Text{}, align_8D{}, Value(0) {
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
    TypeExpirationListeners.AddItem(this);
    TriggerExpirationListeners.AddItem(this);
}
TActionClass::~TActionClass() {
    NotifyTriggerNodeExpired();
    TypeExpirationListeners.Remove(this); TriggerExpirationListeners.Remove(this);
    Array.Remove(this);
}
void TActionClass::PointerExpired(AbstractClass* object, bool) {
    if (NextAction == object) NextAction = NextAction->NextAction;
    if (TeamType == object) TeamType = nullptr;
    if (TriggerType == object) TriggerType = nullptr;
    if (TagType == object) TagType = nullptr;
}
int TActionClass::GetArrayIndex() const { return ArrayIndex; }
AbstractType TActionClass::WhatAmI() const { return AbsID; }
int TActionClass::Size() const { return sizeof(*this); }
HRESULT YRPP_STDCALL TActionClass::GetClassID(CLSID* dest) {
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = { 1326367634u, 298977877u, 1610655660u, 3042641160u };
    std::memcpy(dest, words, sizeof(words));
    return 0;
}
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(TActionClass) == 148);
#endif
