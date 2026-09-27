// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 event.cpp constructors.
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md. YR 0x004C65E0..0x004C6B60 calibration.
#include "yrpp/EventClass.h"
#include <new>

#if !defined(RA2_YRPP_GAME)
// These original constructors intentionally do not touch IsExecuted or unused
// payload bytes. No allocation or exception crosses the construction boundary.
// A native caller wanting deterministic unused bytes may prepare EventClass()
// storage first, then construct its typed event in that storage.
EventClass::EventClass(int house,EventType type,int id,int rtti) noexcept {
    HouseIndex=static_cast<char>(house<0?-1:house);
    Type=house<0?EventType::Empty:type;
    if(house>=0) {
        ::new(static_cast<void*>(&Repair)) REPAIR;
        Repair.Whom.m_ID=id;
        Repair.Whom.m_RTTI=static_cast<char>(rtti);
    }
    Frame=static_cast<unsigned>(Unsorted::CurrentFrame);
}
EventClass::EventClass(int house,EventType type,const CellStruct& cell) noexcept {
    HouseIndex=static_cast<char>(house<0?-1:house);
    Type=house<0?EventType::Empty:type;
    if(house>=0)::new(static_cast<void*>(&SellCell)) SELLCELL{cell};
    Frame=static_cast<unsigned>(Unsorted::CurrentFrame);
}
EventClass::EventClass(int house,EventType type,AbstractType rtti,const CellStruct& cell) noexcept {
    HouseIndex=static_cast<char>(house<0?-1:house);
    Type=house<0?EventType::Empty:type;
    if(house>=0)::new(static_cast<void*>(&Place)) PLACE{rtti,-1,0,cell};
    Frame=static_cast<unsigned>(Unsorted::CurrentFrame);
}
EventClass::EventClass(int house,EventType type,AbstractType rtti,int heap,int naval,const CellStruct& cell) noexcept {
    HouseIndex=static_cast<char>(house<0?-1:house);
    Type=house<0?EventType::Empty:type;
    if(house>=0)::new(static_cast<void*>(&Place)) PLACE{rtti,heap,naval,cell};
    Frame=static_cast<unsigned>(Unsorted::CurrentFrame);
}
EventClass::EventClass(int house,EventType type,AbstractType rtti,int heap,const CellStruct& cell) noexcept
    : EventClass(house,type,rtti,heap,0,cell) {}
EventClass::EventClass(int house,EventType type,int id,const CellStruct& cell) noexcept {
    HouseIndex=static_cast<char>(house<0?-1:house);
    Type=house<0?EventType::Empty:type;
    if(house>=0)::new(static_cast<void*>(&SpecialPlace)) SPECIAL_PLACE{id,cell};
    Frame=static_cast<unsigned>(Unsorted::CurrentFrame);
}
EventClass::EventClass(int house,TargetClass source,Mission mission,TargetClass target,TargetClass destination,TargetClass follow) noexcept {
    HouseIndex=static_cast<char>(house<0?-1:house);
    Type=house<0?EventType::Empty:EventType::MegaMission;
    if(house>=0) {
        ::new(static_cast<void*>(&MegaMission)) MEGAMISSION;
        MegaMission.IsPlanningEvent=false;
        MegaMission.Whom=source;MegaMission.Mission=static_cast<unsigned char>(mission);
        MegaMission.Target=target;MegaMission.Destination=destination;MegaMission.Follow=follow;
    }
    Frame=static_cast<unsigned>(Unsorted::CurrentFrame);
}
#endif
