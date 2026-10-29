// Object-list arm of YR 0x007258D0, original mixed list 0x00B0F720.
// Do not substitute Techno::Array or selected objects: non-Techno receivers
// (for example an attached animation) must see the same notification.
#include "yrpp/AbstractClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/ObjectTypeClass.h"
#include "yrpp/AbstractTypeClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Kamikaze.h"
#include "yrpp/BombListClass.h"
#include "yrpp/DiskLaserClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/Notifications.h"
#include "runtime_diagnostics.hpp"
void AbstractClass::NotifyObjectExpired(bool removed) {
    const auto kind = WhatAmI();
    // 0x7258D0 clears the active placement identities before dispatching.
    auto& display = DisplayClass::Instance;
    if (display.CurrentBuilding == this) display.CurrentBuilding = nullptr;
    if (display.CurrentBuildingType == this) {
        display.CurrentBuildingType = nullptr;
        display.SetActiveFoundation(nullptr);
    }
    // 0x725917: clear the singleton before either mixed receiver slot runs.
    if (kind == AbstractType::ParticleSystem && ParticleSystemClass::DefaultSystem == this)
        ParticleSystemClass::DefaultSystem = nullptr;
    // 0x7259D1: House uses its own live receivers, then Logic. It never
    // enters the mixed Object arm or the Factory fallback.
    if (kind == AbstractType::House) {
        auto& receivers = PointerExpiredNotification::NotifyInvalidHouse.Array;
        for (int i = 0; i < receivers.Count; ++i)
            receivers[i]->PointerExpired(this, removed);
        LogicClass::Instance.LogicClass::PointerGotInvalid(this, removed);
        return;
    }
    // Team retirement visits only the live Foot receiver vector.
    if (kind == AbstractType::Team) {
        auto& receivers = PointerExpiredNotification::NotifyInvalidTeam.Array;
        for (int i = 0; i < receivers.Count; ++i)
            receivers[i]->PointerExpired(this, removed);
        return;
    }
    // 0x725AB8: Tags use their dedicated live receiver table, followed by Map
    // and Logic in that order. They must not also enter the mixed object arm.
    if (kind == AbstractType::Tag) {
        game::runtime_log_tag("tag_expired", static_cast<TagClass*>(this), -1, -1, TagExpirationListeners.Count);
        for (int i = 0; i < TagExpirationListeners.Count; ++i)
            if (auto* receiver = TagExpirationListeners[i]) receiver->PointerExpired(this, removed);
        MapClass::Instance.MapClass::PointerGotInvalid(this, true);
        LogicClass::Instance.LogicClass::PointerGotInvalid(this, removed);
        return;
    }
    // 0x725A1F: shared Building/Bullet/Super/Parasite receivers. Count is
    // read after each callback; animation expiry does not broadcast objects.
    if (kind == AbstractType::Anim) {
        for (int i = 0; i < AnimExpirationListeners.Count; ++i)
            if (auto* receiver = AnimExpirationListeners[i]) receiver->PointerExpired(this, removed);
        return;
    }
    const bool object = (AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None;
    if (object) {
        for (int i = 0; i < Array.Count; ++i)
            if (auto* receiver = Array[i]) receiver->PointerExpired(this, removed);
        // 0x725967: bomb owners expire after receivers, before missile targets.
        BombListClass::Instance.PointerGotInvalid(this);
        Kamikaze::Instance.Remove(this);
        for (int i = DiskLaserClass::Array.Count - 1; i >= 0; --i)
            DiskLaserClass::Array[i]->PointerGotInvalid(this);
        Game::PointerGotInvalid(this);
        // Tactical always receives removed=true, followed by Logic with the
        // caller's original flag. Neither is a mixed receiver slot.
        if (TacticalClass::Instance) TacticalClass::Instance->PointerExpired(this, true);
        LogicClass::Instance.LogicClass::PointerGotInvalid(this, removed);
        return;
    }
    if (auto* type = dynamic_cast<AbstractTypeClass*>(this)) {
        type->NotifyTypeExpired(removed);
        return;
    }
    switch (kind) {
        case AbstractType::Factory: {
            auto& receivers = PointerExpiredNotification::NotifyInvalidFactory.Array;
            for (int i = 0; i < receivers.Count; ++i)
                receivers[i]->PointerExpired(this, removed);
            MapClass::Instance.MapClass::PointerGotInvalid(this, true);
            break;
        }
        case AbstractType::Trigger:
            for (int i = 0; i < TriggerInstanceExpirationListeners.Count; ++i)
                TriggerInstanceExpirationListeners[i]->PointerExpired(this, removed);
            break;
        case AbstractType::Action:
        case AbstractType::Event:
            NotifyTriggerNodeExpired(removed);
            break;
        case AbstractType::Waypoint: {
            auto& receivers = PointerExpiredNotification::NotifyInvalidWaypoint.Array;
            for (int i = 0; i < receivers.Count; ++i)
                receivers[i]->PointerExpired(this, removed);
            break;
        }
        case AbstractType::Neuron:
            // Legacy native name: 0xB0F6F0 belongs to Neuron, not Script.
            for (int i = 0; i < ScriptExpirationListeners.Count; ++i)
                ScriptExpirationListeners[i]->PointerExpired(this, removed);
            break;
        default:
            // Script, RadSite and ordinary Cell expiration have no receiver arm.
            break;
    }
}
