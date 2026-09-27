// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tagtype.cpp Attaches_To; YR 0x6E61F0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TagTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/TEventClass.h"
TagTypeClass::Flags TagTypeClass::GetFlags() const {
    Flags flags=0;
    for(auto* trigger=FirstTrigger;trigger;trigger=trigger->NextTrigger)
        for(auto* event=trigger->FirstEvent;event;event=event->NextEvent)
            flags|=static_cast<Flags>(TEventClass::GetAttachType(static_cast<int>(event->EventKind)));
    return flags;
}
