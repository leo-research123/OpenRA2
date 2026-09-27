// Object-list arm of YR 0x007258D0, original mixed list 0x00B0F720.
// Do not substitute Techno::Array or selected objects: non-Techno receivers
// (for example an attached animation) must see the same notification.
#include "yrpp/AbstractClass.h"
void AbstractClass::NotifyObjectExpired(bool removed) {
    for (int i = 0; i < Array.Count; ++i)
        if (auto* receiver = Array[i]) receiver->PointerExpired(this, removed);
}
