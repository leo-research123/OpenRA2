// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp::Detach; YR 0x004D9960 adds capture/Mega/parasite rules.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"

void FootClass::PointerExpired(AbstractClass* object, bool removed) {
    if (!object) return;
    TechnoClass::PointerExpired(object, removed);
    if (Team == object) Team = nullptr;
    if (ParasiteEatingMe == object && (!ParasiteEatingMe->Health || !Unsorted::ScenarioStarted))
        ParasiteEatingMe = nullptr;
    if (ParasiteEatingMe && ParasiteEatingMe->Health > 0 && ParasiteEatingMe->ParasiteImUsing)
        ParasiteEatingMe->ParasiteImUsing->PointerExpired(object, true);
    if (object == this) ParasiteEatingMe = nullptr;
    if (NextTeamMember == object && removed) NextTeamMember = NextTeamMember->NextTeamMember;
    if (ArchiveTarget == object) SetArchiveTarget(nullptr);
    if (LastDestination == object) LastDestination = nullptr;
    if (Destination == object) {
        bool clear = true;
        if (!removed && (object->AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None && Owner) {
            auto* cell = MapClass::Instance.TryGetCellAt(object->GetCoords());
            const int index = Owner->ArrayIndex;
            if (cell && index >= 0 && index < 0x18 && cell->Sensors_InclHouse(index)) clear = false;
        }
        bool capture = false;
        if (GetCurrentMission() == Mission::Capture && WhatAmI() == AbstractType::Infantry &&
            static_cast<InfantryClass*>(this)->Type && static_cast<InfantryClass*>(this)->Type->Occupier &&
            (object->AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None) {
            auto* target = static_cast<ObjectClass*>(object);
            capture = target->IsAlive && target->Health > 0 && target->GetCurrentMission() != Mission::Selling;
        }
        if (clear && !capture) { unknown_5A0 = nullptr; Destination = nullptr; }
    }
    if (MegaTarget == object) {
        CoordStruct coords = object->GetCoords();
        MegaDestination = MapClass::Instance.GetCellAt(CellStruct{short(coords.X / 256), short(coords.Y / 256)});
        MegaTarget = nullptr;
    }
    if (MegaDestination == object) MegaDestination = nullptr;
    while (NavQueue.Remove(object)) {}
    while (unknown_abstract_array_588.Remove(object)) {}
}
