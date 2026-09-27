// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 walk.cpp, calibrated against YR 0x75AA90..0x75CBC0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/WalkLocomotionClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include <cstring>
#include <cstdlib>

WalkLocomotionClass::WalkLocomotionClass() noexcept
    : DestinationCoord{}, HeadToCoord{}, IsMoving(false), InProcessing(false),
      IsReallyMoving(false), Piggybackee(nullptr) {}

WalkLocomotionClass::~WalkLocomotionClass() {
    if (Piggybackee) Piggybackee->Release();
}

HRESULT YRPP_STDCALL WalkLocomotionClass::QueryInterface(REFIID iid, void** output) {
    const HRESULT result = LocomotionClass::QueryInterface(iid, output);
    if (result != static_cast<HRESULT>(0x80004002u)) return result;
    constexpr GUID piggyback{0x92FEA800,0xA184,0x11D1,{0xB7,0x0A,0,0xA0,0x24,0xDD,0xAF,0xD1}};
    if (std::memcmp(&iid,&piggyback,sizeof(iid))) return result;
    *output = static_cast<IPiggyback*>(this);
    try { AddRef(); return 0; }
    catch (...) { *output = nullptr; return static_cast<HRESULT>(0x80004005u); }
}

HRESULT YRPP_STDCALL WalkLocomotionClass::GetClassID(CLSID* id) {
    if (!id) return static_cast<HRESULT>(0x80004003u);
    *id = CLSIDs::Walk;
    return 0;
}

HRESULT YRPP_STDCALL WalkLocomotionClass::Begin_Piggyback(ILocomotion* pointer) {
    if (!pointer) return static_cast<HRESULT>(0x80004003u);
    if (Piggybackee) return static_cast<HRESULT>(0x80004005u);
    Piggybackee = pointer;
    try { pointer->AddRef(); return 0; }
    catch (...) { Piggybackee = nullptr; return static_cast<HRESULT>(0x80004005u); }
}

HRESULT YRPP_STDCALL WalkLocomotionClass::End_Piggyback(ILocomotion** pointer) {
    if (!pointer) return static_cast<HRESULT>(0x80004003u);
    if (!Piggybackee) return 1; // S_FALSE leaves the caller's pointer untouched.
    *pointer = Piggybackee;
    Piggybackee = nullptr; // Transfer the held reference; do not AddRef/Release.
    return 0;
}

bool YRPP_STDCALL WalkLocomotionClass::Is_Ok_To_End() {
    return !Is_Moving() && Piggybackee && !InProcessing && !LinkedTo->IsAttackedByLocomotor;
}

HRESULT YRPP_STDCALL WalkLocomotionClass::Piggyback_CLSID(GUID* id) {
    if (!id) return static_cast<HRESULT>(0x80004003u);
    constexpr GUID persist_iid{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};
    IPersist* persist = nullptr;
    try {
        ILocomotion* object = Piggybackee ? Piggybackee : static_cast<ILocomotion*>(this);
        const HRESULT queried = object->QueryInterface(persist_iid, reinterpret_cast<void**>(&persist));
        if (queried < 0 || !persist) return static_cast<HRESULT>(0x80004005u);
        const HRESULT result = persist->GetClassID(id);
        auto* held = persist; persist = nullptr;
        held->Release();
        return result;
    } catch (...) {
        if (persist) { try { persist->Release(); } catch (...) {} }
        return static_cast<HRESULT>(0x80004005u);
    }
}

bool YRPP_STDCALL WalkLocomotionClass::Is_Moving_Now() {
    return Is_Moving() && LinkedTo->SpeedPercentage > 0.0 && HeadToCoord != CoordStruct::Empty;
}

void YRPP_STDCALL WalkLocomotionClass::Move_To(CoordStruct to) {
    if (LinkedTo->IsUnderEMP() || LinkedTo->IsBeingWarpedOut() || LinkedTo->IsWarpingIn()) return;
    DestinationCoord = to;
    if (to != CoordStruct::Empty) {
        if (static_cast<unsigned>(MapClass::Instance.GetCellAt(to)->Flags) & 0x100u)
            DestinationCoord.Z += TacticalClass::PixelToZ(2 * Unsorted::CellHeightInPixels);
        IsMoving = true;
    } else if (HeadToCoord == CoordStruct::Empty) {
        const bool was_moving = IsMoving;
        IsMoving = false;
        if (was_moving) LinkedTo->vt_entry_54C();
    }
}

void YRPP_STDCALL WalkLocomotionClass::Stop_Moving() {
    DestinationCoord = CoordStruct::Empty;
    if (HeadToCoord == CoordStruct::Empty) {
        IsMoving = IsReallyMoving = false;
        LinkedTo->vt_entry_54C();
    }
}

void YRPP_STDCALL WalkLocomotionClass::Do_Turn(DirStruct dir) {
    LinkedTo->PrimaryFacing.SetCurrent(dir);
}

void YRPP_STDCALL WalkLocomotionClass::Mark_All_Occupation_Bits(MarkType mark) {
    if (mark == MarkType::Up) LinkedTo->UnmarkAllOccupationBits(Head_To_Coord());
}

bool YRPP_STDCALL WalkLocomotionClass::Is_Moving_Here(CoordStruct to) {
    const auto head = Head_To_Coord();
    // The target truncates signed division, then compares the cell WORDs.
    return static_cast<short>(head.X / 256) == static_cast<short>(to.X / 256)
        && static_cast<short>(head.Y / 256) == static_cast<short>(to.Y / 256)
        && std::abs(static_cast<long long>(head.Z) - to.Z) <= Unsorted::LevelHeight;
}

bool WalkLocomotionClass::Mark_Head_To(const CoordStruct& coord) {
    LinkedTo->UnmarkAllOccupationBits(HeadToCoord != CoordStruct::Empty ? HeadToCoord : LinkedTo->Location);
    if (coord != CoordStruct::Empty) {
        bool any_spot = false;
        const auto mission = LinkedTo->GetCurrentMission();
        if (mission == Mission::Capture || mission == Mission::Enter || mission == Mission::Eaten
            || mission == Mission::Area_Guard || mission == Mission::Patrol) {
            if (auto* destination = LinkedTo->Destination) {
                const auto kind = destination->WhatAmI();
                if (kind == AbstractType::Unit || kind == AbstractType::Aircraft) {
                    const CellStruct cell{static_cast<short>(coord.X / 256),static_cast<short>(coord.Y / 256)};
                    any_spot = static_cast<ObjectClass*>(destination)->GetMapCoords() == cell;
                } else if (kind == AbstractType::Building) {
                    any_spot = MapClass::Instance.GetCellAt(coord)->GetBuilding() == destination;
                }
            }
        }
        if (auto* owner = LinkedTo->SlaveOwner; owner && owner->SlaveManager) {
            auto* cell = MapClass::Instance.GetCellAt(coord);
            if (cell->FindTechnoNearestTo({0,0},false) == owner) {
                auto* slave = LinkedTo->WhatAmI() == AbstractType::Infantry ? static_cast<InfantryClass*>(LinkedTo) : nullptr;
                if (owner->SlaveManager->IsSlaveAtCell(slave,cell)) any_spot = true;
            }
        }
        auto* cell = MapClass::Instance.GetCellAt(coord);
        const bool bridge = (static_cast<unsigned>(cell->Flags) & 0x100u)
            && LinkedTo->Location.Z > 3 * Unsorted::LevelHeight + MapClass::Instance.GetCellFloorHeight(coord);
        cell->FindInfantrySubposition(&HeadToCoord,coord,any_spot,bridge,false);
        if (!MapClass::Instance.GetCellAt(HeadToCoord)->CollectCrate(LinkedTo) && !LinkedTo->InLimbo) {
            HeadToCoord = CoordStruct::Empty;
            if (!LinkedTo->IsAlive) return false;
        }
    } else HeadToCoord = coord;
    if (HeadToCoord != CoordStruct::Empty) {
        LinkedTo->MarkAllOccupationBits(HeadToCoord);
        return true;
    }
    LinkedTo->MarkAllOccupationBits(LinkedTo->Location);
    return false;
}

void YRPP_STDCALL WalkLocomotionClass::Force_Immediate_Destination(CoordStruct coord) {
    const bool was_moving = IsMoving;
    Mark_Head_To(coord);
    if (HeadToCoord == CoordStruct::Empty && DestinationCoord == CoordStruct::Empty) {
        IsMoving = false;
        if (was_moving) LinkedTo->vt_entry_54C();
    }
}
