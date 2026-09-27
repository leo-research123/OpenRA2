#pragma once

#include "yrpp/ArrayClasses.h"
#include "yrpp/Helpers/CompileTime.h"

class AbstractClass;

// encapsulates a bunch of vectors that are used for announcing invalid pointers.
// if an AbstractClass is contained in an list it will be notified through
// PointerExpired whenever an object of that type expires.
class PointerExpiredNotification {
public:
    /// Global VA: 0x00B0F720.
    DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidObject, 0xB0F720u) // Object class hierarchy
    /// Global VA: 0x00B0F670.
    DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidType, 0xB0F670u) // AbstractType class hierarchy
    /// Global VA: 0x00B0F5B8.
    DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidAnim, 0xB0F5B8u) // AnimClass
    /// Global VA: 0x00B0F6C8.
    DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidHouse, 0xB0F6C8u) // HouseClass
    /// Global VA: 0x00B0F618.
    DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidTag, 0xB0F618u) // TagClass
    /// Global VA: 0x00B0F708.
    DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidTrigger, 0xB0F708u) // TriggerClass
    /// Global VA: 0x00B0F640.
    DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidFactory, 0xB0F640u) // FactoryClass
    /// Global VA: 0x00B0F5F0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidWaypoint, 0xB0F5F0u) // WaypointClass
#else
    static PointerExpiredNotification& NotifyInvalidWaypoint;
#endif
    /// Global VA: 0x00B0F5D8.
    DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidTeam, 0xB0F5D8u) // TeamClass
    /// Global VA: 0x00B0F6F0.
    DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidNeuron, 0xB0F6F0u) // NeuronClass
    /// Global VA: 0x00B0F658.
    DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidActionOrEvent, 0xB0F658u) // ActionClass and EventClass

    inline bool Add(AbstractClass* object) {
        // add only if doesn't exist
        return this->Array.AddUnique(object);
    }

    inline bool Remove(AbstractClass* object) {
        return this->Array.Remove(object);
    }

    DynamicVectorClass<AbstractClass*> Array;
};
