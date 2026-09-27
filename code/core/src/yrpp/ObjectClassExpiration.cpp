// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 object.cpp::Detach, YR 0x005F5230 adds parachute expiry.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ObjectClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/AnimClass.h"
// OpenTS Attach_Trigger; YR 0x005F5B50 shares tags and balances the attachment
// count even when replacing a tag with itself. A null tag detaches and fails.
bool ObjectClass::AttachTrigger(TagClass* tag) {
    if(AttachedTag){--AttachedTag->InstanceCount;AttachedTag=nullptr;}
    if(!tag)return false;
    AttachedTag=tag;++tag->InstanceCount;return true;
}
void ObjectClass::PointerExpired(AbstractClass* object, bool removed) {
    if (AttachedTag == object && AttachedTag) {
        --AttachedTag->InstanceCount;
        AttachedTag = nullptr;
    }
    if (removed && object && NextObject == object) NextObject = NextObject->NextObject;
    if (Parachute == object) Parachute = nullptr;
}
