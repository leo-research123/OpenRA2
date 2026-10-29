// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 teleport.cpp, calibrated to YR 0x718000..0x71A1B0.
// Native ordinary teleport arm, including the harvester's zero-delay return.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TeleportLocomotionClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/RulesClass.h"
#include <cstring>
#include <cmath>
#include <bit>
#include <cstdint>
#include "yrpp/BulletClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/VocClass.h"
#include "x87_integer.hpp"

#if defined(RA2_YRPP_GAME)
TeleportLocomotionClass::TeleportLocomotionClass() noexcept : LocomotionClass(noinit_t()) {
    reinterpret_cast<TeleportLocomotionClass*(YRPP_THISCALL*)(TeleportLocomotionClass*)>(0x718000)(this);
}
#else
TeleportLocomotionClass::TeleportLocomotionClass() noexcept
    : MovingDestination{}, LastCoords{}, Moving(false), unknown_bool_35(false), unknown_bool_36(false), State(0),
      Timer{}, Piggybackee(nullptr) {
    Timer.Start(0);
}
#endif
TeleportLocomotionClass::~TeleportLocomotionClass() {
    if (Piggybackee) Piggybackee->Release();
}
HRESULT YRPP_STDCALL TeleportLocomotionClass::QueryInterface(REFIID iid, void** output) {
    const auto result = LocomotionClass::QueryInterface(iid, output);
    if (result != static_cast<HRESULT>(0x80004002u)) return result;
    constexpr GUID piggy{0x92FEA800, 0xA184, 0x11D1, {0xB7, 0x0A, 0, 0xA0, 0x24, 0xDD, 0xAF, 0xD1}};
    if (std::memcmp(&iid, &piggy, sizeof(iid))) return result;
    *output = static_cast<IPiggyback*>(this);
    AddRef();
    return 0;
}
ULONG YRPP_STDCALL TeleportLocomotionClass::AddRef() { return LocomotionClass::AddRef(); }
ULONG YRPP_STDCALL TeleportLocomotionClass::Release() { return LocomotionClass::Release(); }
HRESULT YRPP_STDCALL TeleportLocomotionClass::GetClassID(CLSID* output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    *output = CLSIDs::Teleport;
    return 0;
}
HRESULT YRPP_STDCALL TeleportLocomotionClass::Begin_Piggyback(ILocomotion* pointer) {
    if (!pointer) return static_cast<HRESULT>(0x80004003u);
    if (Piggybackee) return static_cast<HRESULT>(0x80004005u);
    Piggybackee = pointer;
    pointer->AddRef();
    return 0;
}
HRESULT YRPP_STDCALL TeleportLocomotionClass::End_Piggyback(ILocomotion** pointer) {
    if (!pointer) return static_cast<HRESULT>(0x80004003u);
    if (LinkedTo) {
        LinkedTo->BeingManipulatedBy = nullptr;
        LinkedTo->ChronoWarpedByHouse = nullptr;
    }
    if (!Piggybackee) return 1;
    *pointer = Piggybackee;
    Piggybackee = nullptr;
    return 0;
}
bool YRPP_STDCALL TeleportLocomotionClass::Is_Piggybacking() { return Piggybackee != nullptr; }
bool YRPP_STDCALL TeleportLocomotionClass::Is_Ok_To_End() {
    // 0x719F54 gates restoration on immobilization, independently of temporal targeting.
    return !Is_Moving() && Piggybackee && !unknown_bool_35 && !LinkedTo->IsImmobilized && !State &&
           !LinkedTo->IsAttackedByLocomotor;
}
HRESULT YRPP_STDCALL TeleportLocomotionClass::Piggyback_CLSID(GUID* id) {
#if defined(RA2_YRPP_GAME)
    return reinterpret_cast<HRESULT(YRPP_STDCALL*)(IPiggyback*, GUID*)>(0x719F80)(static_cast<IPiggyback*>(this), id);
#else
    if (!id) return static_cast<HRESULT>(0x80004003u);
    constexpr GUID iid{0x10C, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
    IPersistStream* persist = nullptr;
    try {
        const auto queried = Piggybackee ? Piggybackee->QueryInterface(iid, reinterpret_cast<void**>(&persist))
                                        : QueryInterface(iid, reinterpret_cast<void**>(&persist));
        // The original COM runtime raises non-E_NOINTERFACE failures. Native
        // HRESULT entries contain failures locally instead of unwinding into a host.
        if (queried < 0) {
            return queried == static_cast<HRESULT>(0x80004002u) ? static_cast<HRESULT>(0x80004005u) : queried;
        }
        if (!persist) return static_cast<HRESULT>(0x80004005u);
        const auto result = persist->GetClassID(id);
        auto* released = persist;
        persist = nullptr;
        released->Release();
        return result;
    } catch (...) {
        if (persist) {
            try { persist->Release(); } catch (...) {}
        }
        return static_cast<HRESULT>(0x80004005u);
    }
#endif
}
HRESULT YRPP_STDCALL TeleportLocomotionClass::Load(IStream* stream) {
#if defined(RA2_YRPP_GAME)
    return reinterpret_cast<HRESULT(YRPP_STDCALL*)(TeleportLocomotionClass*, IStream*)>(0x719CA0)(this, stream);
#else
    // Native persistence and pointer relocation are not implemented. Do not
    // consume a stream or report a restored locomotor when neither can succeed.
    return static_cast<HRESULT>(stream ? 0x80004001u : 0x80004003u);
#endif
}
HRESULT YRPP_STDCALL TeleportLocomotionClass::Save(IStream* stream, BOOL clearDirty) {
#if defined(RA2_YRPP_GAME)
    return reinterpret_cast<HRESULT(YRPP_STDCALL*)(TeleportLocomotionClass*, IStream*, BOOL)>(0x719D40)(this, stream, clearDirty);
#else
    (void)clearDirty;
    return static_cast<HRESULT>(stream ? 0x80004001u : 0x80004003u);
#endif
}

bool YRPP_STDCALL TeleportLocomotionClass::Is_Moving() { return Moving; }
CoordStruct YRPP_STDCALL TeleportLocomotionClass::Destination() {
    return Is_Moving() ? MovingDestination : LinkedTo->Location;
}
void YRPP_STDCALL TeleportLocomotionClass::Stop_Moving() {
    MovingDestination = CoordStruct::Empty;
    Moving = false;
    unknown_bool_36 = false;
}
void YRPP_STDCALL TeleportLocomotionClass::Do_Turn(DirStruct dir) { LinkedTo->PrimaryFacing.SetDesired(dir); }
Layer YRPP_STDCALL TeleportLocomotionClass::In_Which_Layer() { return Layer::Ground; }
void YRPP_STDCALL TeleportLocomotionClass::Mark_All_Occupation_Bits(MarkType mark) {
    if (mark == MarkType::Up) LinkedTo->UnmarkAllOccupationBits(Head_To_Coord());
}
void YRPP_STDCALL TeleportLocomotionClass::Move_To(CoordStruct to) {
    if (LinkedTo->IsUnderEMP() || LinkedTo->IsParalyzed() || LinkedTo->IsBeingWarpedOut() ||
        LinkedTo->IsWarpingIn()) {
        LinkedTo->Destination = nullptr;
        return;
    }
    if (LinkedTo->UnlimboingInfantry && LinkedTo->WhatAmI() == AbstractType::Infantry)
        MapClass::Instance.GetCellAt(to)->ScatterContent(CoordStruct::Empty, true, true, false);
    SelectChronoMoveDestination(to);
    if (LastCoords == CoordStruct::Empty)
        LinkedTo->SetDestination(nullptr, true);
    else {
        Moving = true;
        MovingDestination = LastCoords;
    }
}
bool TeleportLocomotionClass::IsStill() { return !Is_Moving(); }

bool YRPP_STDCALL TeleportLocomotionClass::Process() {
    auto& foot = *LinkedTo;
    auto& rules = *RulesClass::Instance;
    const auto effect = [&] {
        if (void* memory = YRMemory::Allocate(sizeof(AnimClass)))
            ::new (memory) AnimClass(rules.WarpOut, foot.Location, 0, 1, 0x600, 0, false);
    };
    const auto sound = [&](bool entering) {
        auto* type = foot.GetTechnoType();
        int index = entering ? type->ChronoInSound : type->ChronoOutSound;
        if (index == -1) index = entering ? rules.ChronoInSound : rules.ChronoOutSound;
        if (index != -1) VocClass::PlayAt(index, foot.Location);
    };
    if (foot.WarpingOut && !State && !foot.unknown_280) {
        ProcessChronoWait();
        return false;
    }
    if (!State && foot.unknown_280) {
        State = int(foot.unknown_280);
        return false;
    }
    if (foot.IsImmobilized && !State) {
        foot.BeingWarpedOut = true;
        Timer.Start(60);
        ++State;
        return false;
    }
    if (!foot.IsImmobilized && State <= 0) {
        if (!Is_Moving()) return false;
        if (foot.Location == LastCoords || LastCoords == CoordStruct::Empty) {
            foot.SetDestination(nullptr, true);
            Stop_Moving();
            return false;
        }
        foot.RemoveFromTargeting();
        for (int i = BulletClass::Array.Count - 1; i >= 0; --i)
            if (BulletClass::Array[i]->Target == &foot) BulletClass::Array[i]->LoseTarget();
        effect();
        const auto at = foot.GetCoords();
        const auto difference = [](int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); };
        const int x = difference(at.X, LastCoords.X), y = difference(at.Y, LastCoords.Y),
                  z = difference(at.Z, LastCoords.Z);
        const int length = game::x87_integer(Math::sqrt(double(x) * x + double(z) * z + double(y) * y));
        Timer.Start(rules.ChronoTrigger ? length / rules.ChronoDistanceFactor : 0);
        if (Timer.GetTimeLeft() <= rules.ChronoMinimumDelay) Timer.Start(rules.ChronoMinimumDelay);
        if (length < rules.ChronoRangeMinimum) Timer.Start(rules.ChronoMinimumDelay);
        foot.WarpingOut = true;
        if (foot.WhatAmI() == AbstractType::Unit && static_cast<UnitClass&>(foot).Type->Harvester) {
            Timer.Start(0);
            foot.WarpingOut = false;
        }
        if (foot.ParasiteEatingMe) foot.ParasiteEatingMe->ParasiteImUsing->ExitUnit();
        foot.Mark(MarkType::Up);
        sound(false);
        foot.SetLocation(LastCoords);
        auto* cell = MapClass::Instance.GetCellAt(CellClass::Coord2Cell(LastCoords));
        foot.SetLocation(LastCoords);
        foot.OnBridge = (unsigned(cell->Flags) & 0x100u) != 0;
        foot.SetHeight(0);
        foot.Mark(MarkType::Down);
        sound(true);
        foot.UpdatePosition(PCPType::End);
        Stop_Moving();
        cell->CollectCrate(&foot);
        foot.SetDestination(nullptr, true);
        effect();
        foot.unknown_280 = 0;
        return false;
    }
    switch (State) {
        case 1:
        case 6:
            ProcessChronoWait();
            break;
        case 2:
            effect();
            foot.Mark(MarkType::Up);
            sound(false);
            foot.WarpingOut = true;
            foot.IsImmobilized = false;
            foot.BeingWarpedOut = false;
            foot.OnBridge = false;
            {
                const bool ready = PrepareChronoDestination(foot.ChronoDestCoords, false);
                ++State;
                if (ready) ++State;
            }
            break;
        case 3:
            if (PrepareChronoDestination(foot.ChronoDestCoords, false)) ++State;
            foot.ChronoLockRemaining = rules.ChronoDelay;
            break;
        case 4:
            PrepareChronoDestination(foot.ChronoDestCoords, true);
            foot.SetLocation(LastCoords);
            foot.SetHeight(0);
            foot.Mark(MarkType::Down);
            ++State;
            break;
        case 5:
            foot.SetLocation(LastCoords);
            foot.SetHeight(0);
            foot.Mark(MarkType::Down);
            sound(true);
            if (!MapClass::Instance.IsWithinUsableArea(foot.GetMapCoords(), true)) foot.IsInPlayfield = false;
            if (!foot.unknown_280) CheckChronoLanding(LastCoords);
            if (!foot.IsAlive) return false;
            foot.UpdatePosition(PCPType::End);
            Stop_Moving();
            foot.BeingManipulatedBy = nullptr;
            foot.ChronoWarpedByHouse = nullptr;
            foot.SetArchiveTarget(nullptr);
            foot.SetDestination(nullptr, true);
            Timer.Start(foot.ChronoLockRemaining);
            effect();
            ++State;
            break;
        case 7:
            foot.WarpingOut = false;
            foot.SetArchiveTarget(nullptr);
            foot.SetDestination(nullptr, true);
            Moving = false;
            foot.unknown_280 = 0;
            State = 0;
            break;
    }
    return false;
}
