// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 hover.cpp, calibrated to YR 0x513C20..0x5172B0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HoverLocomotionClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include <bit>
#include <cstdlib>

HoverLocomotionClass::HoverLocomotionClass() noexcept
    : DestinationCoord(CoordStruct::Empty), HeadToCoord(CoordStruct::Empty), LocomotionFacing(0), MaxSpeed(0),
      CurrentSpeed(0), BoostSpeed(1), CurrentWobbles(0), unknown_bool_68(false), unknown_int_6C(0),
      unknown_bool_70(false) {
    LocomotionFacing.RotationTimer.Start(0);
}
HoverLocomotionClass::~HoverLocomotionClass() = default;
HRESULT YRPP_STDCALL HoverLocomotionClass::GetClassID(CLSID* output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    *output = CLSIDs::Hover;
    return 0;
}
HRESULT YRPP_STDCALL HoverLocomotionClass::Link_To_Object(void* object) {
    const auto result = LocomotionClass::Link_To_Object(object);
    LocomotionFacing = FacingClass(std::bit_cast<int>(unsigned(LinkedTo->GetTechnoType()->ROT) * 2u));
    LocomotionFacing.RotationTimer.Start(0);
    return result;
}
bool YRPP_STDCALL HoverLocomotionClass::Is_Moving() {
    return DestinationCoord != CoordStruct::Empty || HeadToCoord != CoordStruct::Empty;
}
bool YRPP_STDCALL HoverLocomotionClass::Is_Moving_Now() { return Is_Moving() && MaxSpeed != 0.0; }
CoordStruct YRPP_STDCALL HoverLocomotionClass::Destination() { return DestinationCoord; }
CoordStruct YRPP_STDCALL HoverLocomotionClass::Head_To_Coord() {
    return HeadToCoord != CoordStruct::Empty ? HeadToCoord : LinkedTo->Location;
}
Layer YRPP_STDCALL HoverLocomotionClass::In_Which_Layer() { return Layer::Ground; }
void YRPP_STDCALL HoverLocomotionClass::Move_To(CoordStruct to) {
    DestinationCoord = to;
    if (LinkedTo->IsParalyzed()) return;
    if (Is_Powered() && Is_Ion_Sensitive() && LinkedTo->Owner->IonSensitivesShouldBeOffline()) Power_Off();
    DestinationCoord.Z = MapClass::Instance.GetCellFloorHeight(DestinationCoord);
    if (MapClass::Instance.GetCellAt(to)->ContainsBridge()) DestinationCoord.Z += CellClass::BridgeHeight;
    if (to != CoordStruct::Empty && Is_Powered()) {
        if (!Is_Moving_Now()) {
            LinkedTo->SetSpeedPercentage(1.0);
            Start_Of_Move(0);
            While_Moving(true);
        } else if (HeadToCoord == CoordStruct::Empty)
            Start();
    }
}
void YRPP_STDCALL HoverLocomotionClass::Stop_Moving() {
    if (DestinationCoord != HeadToCoord) {
        DestinationCoord = CoordStruct::Empty;
        LinkedTo->PathDirections[0] = -1;
    }
}
void YRPP_STDCALL HoverLocomotionClass::Do_Turn(DirStruct direction) { LinkedTo->PrimaryFacing.SetDesired(direction); }
void HoverLocomotionClass::Do_Shove() {
    unknown_bool_68 = true;
    unknown_int_6C = ScenarioClass::Instance->Random.RandomRanged(20, 30);
    if (ScenarioClass::Instance->Random.RandomRanged(0, 99) < 50) unknown_int_6C = -unknown_int_6C;
}
bool YRPP_STDCALL HoverLocomotionClass::Power_Off() {
    if (Is_Powered() && LinkedTo->CurrentMission != Mission::Sleep && LinkedTo->CurrentMission != Mission::Enter)
        Do_Shove();
    if (Is_Moving()) Stop_Moving();
    return LocomotionClass::Power_Off();
}
bool YRPP_STDCALL HoverLocomotionClass::Is_Powered() {
    return LocomotionClass::Is_Powered() || LinkedTo->GetHeight() > 0;
}
bool YRPP_STDCALL HoverLocomotionClass::Is_Ion_Sensitive() {
    if (LinkedTo->HasAnyLink()) {
        auto* link = LinkedTo->GetNthLink();
        if (link->WhatAmI() == AbstractType::Building &&
            static_cast<BuildingClass*>(LinkedTo->GetNthLink())->Type->WeaponsFactory)
            return false;
    }
    auto* building = MapClass::Instance.GetCellAt(LinkedTo->Location)->GetBuilding();
    if (building && building->Type->WeaponsFactory) {
        const auto from = CellClass::Coord2Cell(building->Location), at = CellClass::Coord2Cell(LinkedTo->Location);
        const short x = short(at.X - from.X), y = short(at.Y - from.Y);
        if ((x == 0 || x == 2 || x == 3) && y == 1) return false;
    }
    return true;
}
bool YRPP_STDCALL HoverLocomotionClass::Push(DirStruct direction) {
    if (!Is_Powered() || unknown_bool_70) return false;
    auto cell = LinkedTo->GetMapCoords();
    const auto delta = Unsorted::AdjacentCell[direction.GetValue<3>()];
    cell.X = short(cell.X + delta.X);
    cell.Y = short(cell.Y + delta.Y);
    if (LinkedTo->IsCellOccupied(MapClass::Instance.GetCellAt(cell), FacingType(-1), -1, nullptr, true) != Move::OK)
        return false;
    unknown_bool_70 = true;
    const CoordStruct to{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0};
    if (Is_Moving()) {
        LinkedTo->PathDirections[0] = -1;
        if (HeadToCoord != CoordStruct::Empty) LinkedTo->UnmarkAllOccupationBits(HeadToCoord);
        HeadToCoord = to;
        HeadToCoord.Z = MapClass::Instance.GetCellFloorHeight(HeadToCoord);
        if (LinkedTo->Location.Z >= HeadToCoord.Z + 3 * Unsorted::LevelHeight) HeadToCoord.Z += CellClass::BridgeHeight;
        LinkedTo->MarkAllOccupationBits(HeadToCoord);
    } else
        Move_To(to);
    return true;
}
bool YRPP_STDCALL HoverLocomotionClass::Shove(DirStruct direction) {
    if (!Push(direction)) return false;
    Do_Shove();
    return true;
}
void YRPP_STDCALL HoverLocomotionClass::Mark_All_Occupation_Bits(MarkType mark) {
    // YR only releases the reservation here; other marks are no-ops.
    if (mark == MarkType::Up) LinkedTo->UnmarkAllOccupationBits(Head_To_Coord());
}
bool YRPP_STDCALL HoverLocomotionClass::Is_Moving_Here(CoordStruct to) {
    const auto at = Head_To_Coord();
    const int dz = std::bit_cast<int>(unsigned(at.Z) - unsigned(to.Z));
    return CellClass::Coord2Cell(at) == CellClass::Coord2Cell(to) && std::abs(dz) <= Unsorted::LevelHeight;
}
void HoverLocomotionClass::Stop() {
    LinkedTo->SetDestination(nullptr, true);
    CurrentSpeed = MaxSpeed = 0;
    LinkedTo->SetSpeedPercentage(0);
}
void HoverLocomotionClass::Stop_Driver() {
    CurrentSpeed = MaxSpeed = 0;
    LinkedTo->SetSpeedPercentage(0);
    if (HeadToCoord != CoordStruct::Empty) {
        LinkedTo->UnmarkAllOccupationBits(HeadToCoord);
        HeadToCoord = CoordStruct::Empty;
    }
}
void HoverLocomotionClass::Start() {
    const bool anew =
        HeadToCoord == CoordStruct::Empty || LinkedTo->GetMapCoords() == CellClass::Coord2Cell(HeadToCoord);
    MaxSpeed = 1;
    LinkedTo->SetSpeedPercentage(1);
    Start_Of_Move(0);
    if (anew) While_Moving(true);
}
