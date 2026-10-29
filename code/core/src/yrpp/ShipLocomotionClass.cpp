// SPDX-License-Identifier: GPL-3.0-or-later
// Shared COM/state scaffolding from OpenTS 44fac744 drive.cpp, calibrated to
// Ship YR 0x69EC50..0x6A42F1. Copyright 2025 Electronic Arts Inc.;
// Copyright 2026 OpenTS contributors. EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ShipLocomotionClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

ShipLocomotionClass::ShipLocomotionClass() noexcept
    : CurrentRamp(0), PreviousRamp(0), SlopeTimer{}, DestinationCoord{}, HeadToCoord{}, SpeedAccum(0),
      movementspeed_50(0), TrackNumber(-1), TrackIndex(-1), IsOnShortTrack(false), IsTurretLockedDown(0),
      IsRotating(false), IsDriving(false), IsRocking(false), UnLocked(true), Piggybackee(nullptr) {
    SlopeTimer.Start(0);
}
ShipLocomotionClass::~ShipLocomotionClass() {
    // COM callbacks must not propagate exceptions through destruction.
    if (Piggybackee) try {
            Piggybackee->Release();
        } catch (...) {
        }
}
HRESULT YRPP_STDCALL ShipLocomotionClass::QueryInterface(REFIID iid, void** output) {
    const auto result = LocomotionClass::QueryInterface(iid, output);
    if (result != static_cast<HRESULT>(0x80004002u)) return result;
    constexpr GUID piggy{0x92FEA800, 0xA184, 0x11D1, {0xB7, 0x0A, 0, 0xA0, 0x24, 0xDD, 0xAF, 0xD1}};
    if (std::memcmp(&iid, &piggy, sizeof(iid))) return result;
    *output = static_cast<IPiggyback*>(this);
    try {
        AddRef();
        return 0;
    } catch (...) {
        *output = nullptr;
        return static_cast<HRESULT>(0x80004005u);
    }
}
ULONG YRPP_STDCALL ShipLocomotionClass::AddRef() { return LocomotionClass::AddRef(); }
ULONG YRPP_STDCALL ShipLocomotionClass::Release() { return LocomotionClass::Release(); }
HRESULT YRPP_STDCALL ShipLocomotionClass::GetClassID(CLSID* output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    *output = CLSIDs::Ship;
    return 0;
}
HRESULT YRPP_STDCALL ShipLocomotionClass::Begin_Piggyback(ILocomotion* pointer) {
    if (!pointer) return static_cast<HRESULT>(0x80004003u);
    if (Piggybackee) return static_cast<HRESULT>(0x80004005u);
    Piggybackee = pointer;
    try {
        pointer->AddRef();
        return 0;
    } catch (...) {
        Piggybackee = nullptr;
        return static_cast<HRESULT>(0x80004005u);
    }
}
HRESULT YRPP_STDCALL ShipLocomotionClass::End_Piggyback(ILocomotion** pointer) {
    if (!pointer) return static_cast<HRESULT>(0x80004003u);
    if (!Piggybackee) return 1;
    *pointer = Piggybackee;
    Piggybackee = nullptr;
    return 0;
}
bool YRPP_STDCALL ShipLocomotionClass::Is_Ok_To_End() {
    return !Is_Moving() && Piggybackee && UnLocked && !LinkedTo->IsAttackedByLocomotor;
}
HRESULT YRPP_STDCALL ShipLocomotionClass::Piggyback_CLSID(GUID* id) {
    if (!id) return static_cast<HRESULT>(0x80004003u);
    constexpr GUID iid{0x10C, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
    IPersistStream* persist = nullptr;
    try {
        auto* object = Piggybackee ? Piggybackee : static_cast<ILocomotion*>(this);
        if (object->QueryInterface(iid, reinterpret_cast<void**>(&persist)) < 0 || !persist)
            return static_cast<HRESULT>(0x80004005u);
        auto result = persist->GetClassID(id);
        auto* held = persist;
        persist = nullptr;
        held->Release();
        return result;
    } catch (...) {
        if (persist) try {
                persist->Release();
            } catch (...) {
            }
        return static_cast<HRESULT>(0x80004005u);
    }
}
bool YRPP_STDCALL ShipLocomotionClass::Is_Piggybacking() { return Piggybackee != nullptr; }
bool YRPP_STDCALL ShipLocomotionClass::Is_Moving() {
    return DestinationCoord != CoordStruct::Empty ||
           (HeadToCoord != CoordStruct::Empty &&
            (HeadToCoord.X != LinkedTo->Location.X || HeadToCoord.Y != LinkedTo->Location.Y));
}
bool YRPP_STDCALL ShipLocomotionClass::Is_Moving_Now() {
    return LinkedTo->PrimaryFacing.IsRotating() ||
           (Is_Moving() && HeadToCoord != CoordStruct::Empty && LinkedTo->GetCurrentSpeed() > 0);
}
bool YRPP_STDCALL ShipLocomotionClass::Is_Really_Moving_Now() { return Is_Moving_Now(); }
CoordStruct YRPP_STDCALL ShipLocomotionClass::Destination() { return DestinationCoord; }
CoordStruct YRPP_STDCALL ShipLocomotionClass::Head_To_Coord() {
    return HeadToCoord != CoordStruct::Empty ? HeadToCoord : LinkedTo->Location;
}
void YRPP_STDCALL ShipLocomotionClass::Move_To(CoordStruct to) {
    if (LinkedTo->IsUnderEMP() || LinkedTo->IsParalyzed() || LinkedTo->IsBeingWarpedOut() || LinkedTo->IsWarpingIn())
        return;
    DestinationCoord = to;
    if (to != CoordStruct::Empty && (unsigned(MapClass::Instance.GetCellAt(to)->Flags) & 0x100u))
        DestinationCoord.Z += CellClass::BridgeHeight;
}
void YRPP_STDCALL ShipLocomotionClass::Stop_Moving() {
    if (HeadToCoord != CoordStruct::Empty && LinkedTo->GetTechnoType()->IsTrain) {
        auto* unit = static_cast<UnitClass*>(LinkedTo);
        if (!unit->IsFollowerCar)
            for (auto* next = unit->FollowerCar; next;) {
                next->Locomotor->Stop_Moving();
                next = next->FollowerCar;
                if (next && next == next->FollowerCar) break;
            }
    }
    movementspeed_50 = std::min(movementspeed_50, double(0.3f));
    DestinationCoord = CoordStruct::Empty;
}
void YRPP_STDCALL ShipLocomotionClass::Do_Turn(DirStruct dir) { LinkedTo->PrimaryFacing.SetDesired(dir); }
void ShipLocomotionClass::Set_Slope(int ramp) {
    if (ramp != CurrentRamp) {
        PreviousRamp = CurrentRamp;
        CurrentRamp = ramp;
        SlopeTimer.Start(3);
    }
}
void YRPP_STDCALL ShipLocomotionClass::Force_New_Slope(int ramp) {
    PreviousRamp = CurrentRamp = ramp;
    SlopeTimer.Start(0);
}
void YRPP_STDCALL ShipLocomotionClass::Unlimbo() { Force_New_Slope(LinkedTo->GetCell()->SlopeIndex); }
Layer YRPP_STDCALL ShipLocomotionClass::In_Which_Layer() { return Layer::Ground; }
int YRPP_STDCALL ShipLocomotionClass::Z_Adjust() { return 0; }
ZGradient YRPP_STDCALL ShipLocomotionClass::Z_Gradient() { return LocomotionClass::Z_Gradient(); }
void YRPP_STDCALL ShipLocomotionClass::Lock() { UnLocked = false; }
void YRPP_STDCALL ShipLocomotionClass::Unlock() { UnLocked = true; }
int YRPP_STDCALL ShipLocomotionClass::Get_Track_Number() { return TrackNumber; }
int YRPP_STDCALL ShipLocomotionClass::Get_Track_Index() { return TrackIndex; }
int YRPP_STDCALL ShipLocomotionClass::Get_Speed_Accum() { return SpeedAccum; }
bool ShipLocomotionClass::Stop_Driver() {
    if (HeadToCoord == CoordStruct::Empty) return false;
    HeadToCoord = CoordStruct::Empty;
    IsDriving = false;
    return true;
}
bool ShipLocomotionClass::Start_Driver(const CoordStruct& head) {
    Stop_Driver();
    if (head == CoordStruct::Empty) return false;
    HeadToCoord = head;
    IsDriving = true;
    if (MapClass::Instance.GetCellAt(head)->CollectCrate(LinkedTo) && !LinkedTo->InLimbo) {
        Mark_Track(head, MarkType::Down);
        return true;
    }
    if (!LinkedTo->IsAlive) return false;
    HeadToCoord = CoordStruct::Empty;
    IsDriving = false;
    return false;
}
bool ShipLocomotionClass::Abandon_Navigation() {
    // The tested vector count is at 0x598; NavQueue is a different array.
    if (!LinkedTo->unknown_abstract_array_588.Count) {
        LinkedTo->SetDestination(nullptr, true);
        return false;
    }
    // 0x4DF0D0 clears the two navigation pointers, not NavQueue itself.
    LinkedTo->unknown_5A0 = nullptr;
    LinkedTo->Destination = nullptr;
    return LinkedTo->EnterIdleMode(false, true);
}
void YRPP_STDCALL ShipLocomotionClass::Force_Track(int track, CoordStruct coord) {
    TrackNumber = track;
    TrackIndex = 0;
    if (coord != CoordStruct::Empty && Start_Driver(coord)) {
        DestinationCoord = coord;
        movementspeed_50 = 1.0;
    }
}
Point2D ShipLocomotionClass::Smooth_Turn(const Point2D& offset, int& direction) {
    auto point = offset;
    unsigned face = unsigned(direction);
    const int flags = TurnTrack[TrackNumber].Flag;
    if (flags & 1) {
        std::swap(point.X, point.Y);
        face = (-64u - face) & 255u;
    }
    if (flags & 2) {
        point.X = std::bit_cast<int>(0u - unsigned(point.X));
        face = (0u - face) & 255u;
    }
    if (flags & 4) {
        point.Y = std::bit_cast<int>(0u - unsigned(point.Y));
        face = (128u - face) & 255u;
    }
    direction = int(face);
    point.X = std::bit_cast<int>(unsigned(point.X) + unsigned(HeadToCoord.X));
    point.Y = std::bit_cast<int>(unsigned(point.Y) + unsigned(HeadToCoord.Y));
    return point;
}
void ShipLocomotionClass::Mark_Track(const CoordStruct& head, MarkType mark) {
    if (head == CoordStruct::Empty) return;
    const auto apply = [&](const CoordStruct& at) {
        if (mark == MarkType::Up)
            LinkedTo->UnmarkAllOccupationBits(at);
        else if (mark == MarkType::Down || mark == MarkType::ChangeRedraw)
            LinkedTo->MarkAllOccupationBits(at);
    };
    if (!IsOnShortTrack && TrackNumber != -1) {
        const int raw = TurnTrack[TrackNumber].NormalTrackStructIndex;
        if (raw && RawTrack[raw].CellIndex > -1 && TrackIndex < RawTrack[raw].CellIndex) {
            const auto& point = RawTrack[raw].TrackPoint[RawTrack[raw].CellIndex];
            int face = point.Face;
            const auto at = Smooth_Turn(point.Point, face);
            if (mark == MarkType::Up || mark == MarkType::Down || mark == MarkType::ChangeRedraw)
                apply({at.X, at.Y, LinkedTo->GetZ()});
        }
    }
    apply(head);
}
void YRPP_STDCALL ShipLocomotionClass::Mark_All_Occupation_Bits(MarkType mark) {
    if (HeadToCoord != CoordStruct::Empty) Mark_Track(HeadToCoord, mark);
}
bool YRPP_STDCALL ShipLocomotionClass::Is_Moving_Here(CoordStruct to) {
    const auto head = Head_To_Coord();
    if (head == CoordStruct::Empty) return false;
    const auto same = [&](const CoordStruct& at) {
        const int delta = std::bit_cast<int>(unsigned(at.Z) - unsigned(to.Z));
        const int distance = delta < 0 ? std::bit_cast<int>(0u - unsigned(delta)) : delta;
        return short(at.X / 256) == short(to.X / 256) && short(at.Y / 256) == short(to.Y / 256) &&
               distance <= Unsorted::LevelHeight;
    };
    if (!IsOnShortTrack && TrackNumber != -1) {
        const int raw = TurnTrack[TrackNumber].NormalTrackStructIndex;
        if (raw && RawTrack[raw].CellIndex > -1 && TrackIndex < RawTrack[raw].CellIndex) {
            const auto& point = RawTrack[raw].TrackPoint[RawTrack[raw].CellIndex];
            int face = point.Face;
            const auto at = Smooth_Turn(point.Point, face);
            if (same({at.X, at.Y, LinkedTo->Location.Z})) return true;
        }
    }
    return same(head);
}
bool YRPP_STDCALL ShipLocomotionClass::Will_Jump_Tracks() {
    const int next = LinkedTo->PathDirections[0];
    if (next < 0 || next >= 8) return false;
    const auto& control = TurnTrack[TrackNumber];
    const int raw = IsOnShortTrack ? control.ShortTrackStructIndex : control.NormalTrackStructIndex;
    const int face = ((unsigned(control.Face) + 16u) >> 5) & 7u;
    if (face == next || !TrackIndex || RawTrack[raw].JumpIndex != TrackIndex) return false;
    const int nextRaw = TurnTrack[face * 8 + next].NormalTrackStructIndex;
    return nextRaw && RawTrack[nextRaw].EntryIndex;
}
