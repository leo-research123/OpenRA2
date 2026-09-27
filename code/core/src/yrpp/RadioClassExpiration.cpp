// YR 0x0065AAC0: preserve radio slots, clear matching pointers only on removal.
#include "yrpp/RadioClass.h"
#include "yrpp/TechnoClass.h"
void RadioClass::PointerExpired(AbstractClass* object, bool removed) {
    ObjectClass::PointerExpired(object, removed);
    if (removed) for (int i = 0; i < RadioLinks.Capacity; ++i)
        if (RadioLinks[i] == object) RadioLinks[i] = nullptr;
}
