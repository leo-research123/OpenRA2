// YR 0x54AC40..0x54DFA0: original Jumpjet COM identity, orders and geometry.
#include "yrpp/JumpjetLocomotionClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Drawing.h"
#include "x87_integer.hpp"
#include "Matrix3DArithmetic.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>

JumpjetLocomotionClass::JumpjetLocomotionClass() noexcept
    : TurnRate{}, Speed{}, Climb{}, Crash{}, Height{}, Accel{}, Wobbles{}, Deviation{}, NoWobbles{}, unknown_3D{},
      unknown_3E{}, unknown_3F{}, DestinationCoords(CoordStruct::Empty), IsMoving(false), unknown_4D{}, unknown_4E{},
      unknown_4F{}, State(Grounded), LocomotionFacing(RulesClass::Instance->TurnRate), unknown_6C{}, unknown_6D{},
      unknown_6E{}, unknown_6F{}, CurrentSpeed(0), MaxSpeed(0), CurrentHeight(0), unknown_84{}, unknown_85{},
      unknown_86{}, unknown_87{}, CurrentWobbles(0), DestinationReached(false), unknown_91{}, unknown_92{},
      unknown_93{}, Piggybackee(nullptr) {
    LocomotionFacing.SetDesired(DirStruct(0x4000));
    LocomotionFacing.SetCurrent(DirStruct(0x4000));
}
JumpjetLocomotionClass::~JumpjetLocomotionClass() {
    if (Piggybackee) Piggybackee->Release();
}
HRESULT YRPP_STDCALL JumpjetLocomotionClass::QueryInterface(REFIID iid, void** output) {
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
HRESULT YRPP_STDCALL JumpjetLocomotionClass::GetClassID(CLSID* id) {
    if (!id) return static_cast<HRESULT>(0x80004003u);
    *id = CLSIDs::Jumpjet;
    return 0;
}
HRESULT YRPP_STDCALL JumpjetLocomotionClass::Link_To_Object(void* object) {
    auto* type = static_cast<FootClass*>(object)->GetTechnoType();
    TurnRate = type->JumpjetTurnRate;
    Speed = type->JumpjetSpeed;
    Climb = type->JumpjetClimb;
    Crash = type->JumpjetCrash;
    Height = std::max(type->JumpjetHeight, 2 * Unsorted::LevelHeight);
    Accel = type->JumpjetAccel;
    Wobbles = type->JumpjetWobbles;
    Deviation = type->JumpjetDeviation;
    NoWobbles = type->JumpjetNoWobbles;
    LocomotionFacing = FacingClass(TurnRate);
    LocomotionFacing.SetDesired(DirStruct(0x4000));
    LocomotionFacing.SetCurrent(DirStruct(0x4000));
    return LocomotionClass::Link_To_Object(object);
}
HRESULT YRPP_STDCALL JumpjetLocomotionClass::Begin_Piggyback(ILocomotion* pointer) {
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
HRESULT YRPP_STDCALL JumpjetLocomotionClass::End_Piggyback(ILocomotion** pointer) {
    if (!pointer) return static_cast<HRESULT>(0x80004003u);
    if (LinkedTo) {
        if (LinkedTo->IsAttackedByLocomotor || LinkedTo->IsLetGoByLocomotor) {
            LinkedTo->IsAttackedByLocomotor = LinkedTo->IsLetGoByLocomotor = false;
            if (!LinkedTo->Owner->IsControlledByHuman() && LinkedTo->GetTechnoType()->ResourceGatherer)
                LinkedTo->QueueMission(Mission::Harvest, false);
        }
        LinkedTo->BeingManipulatedBy = nullptr;
        LinkedTo->ChronoWarpedByHouse = nullptr;
    }
    if (!Piggybackee) return 1;
    *pointer = Piggybackee;
    Piggybackee = nullptr;
    return 0;
}
bool YRPP_STDCALL JumpjetLocomotionClass::Is_Ok_To_End() {
    return !Is_Moving() && Piggybackee && !unknown_91 &&
           (!LinkedTo->IsAttackedByLocomotor || LinkedTo->IsLetGoByLocomotor);
}
HRESULT YRPP_STDCALL JumpjetLocomotionClass::Piggyback_CLSID(GUID* id) {
    if (!id) return static_cast<HRESULT>(0x80004003u);
    constexpr GUID persistIID{0x109, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
    IPersist* persist = nullptr;
    try {
        auto* source = Piggybackee ? Piggybackee : static_cast<ILocomotion*>(this);
        if (source->QueryInterface(persistIID, reinterpret_cast<void**>(&persist)) < 0 || !persist)
            return static_cast<HRESULT>(0x80004005u);
        const auto result = persist->GetClassID(id);
        auto* held = persist;
        persist = nullptr;
        held->Release();
        return result;
    } catch (...) {
        if (persist) {
            try {
                persist->Release();
            } catch (...) {
            }
        }
        return static_cast<HRESULT>(0x80004005u);
    }
}
bool YRPP_STDCALL JumpjetLocomotionClass::Is_Moving() { return IsMoving; }
bool YRPP_STDCALL JumpjetLocomotionClass::Is_Moving_Now() { return State != Grounded && State != Hovering; }
CoordStruct YRPP_STDCALL JumpjetLocomotionClass::Destination() {
    return Is_Moving() ? DestinationCoords : CoordStruct::Empty;
}
CoordStruct YRPP_STDCALL JumpjetLocomotionClass::Head_To_Coord() {
    return State != Grounded ? DestinationCoords : LinkedTo->Location;
}
void YRPP_STDCALL JumpjetLocomotionClass::Do_Turn(DirStruct dir) { LinkedTo->PrimaryFacing.SetCurrent(dir); }
Layer YRPP_STDCALL JumpjetLocomotionClass::In_Which_Layer() {
    int height = LinkedTo->GetHeight();
    if (!LinkedTo->OnBridge && LinkedTo->GetCell()->ContainsBridge() && height >= CellClass::BridgeHeight &&
        !LinkedTo->IsFallingDown)
        height -= CellClass::BridgeHeight;
    if (!LinkedTo->IsInAir() || !height) return Layer::Ground;
    return height >= Height ? Layer::Top : Layer::Air;
}
void YRPP_STDCALL JumpjetLocomotionClass::Mark_All_Occupation_Bits(MarkType mark) {
    if (mark != MarkType::Up) return;
    const auto head = Head_To_Coord();
    if (head != CoordStruct::Empty && (State == Grounded || DestinationReached)) {
        LinkedTo->UnmarkAllOccupationBits(head);
        DestinationReached = false;
    }
}
CoordStruct JumpjetLocomotionClass::LandingCoord(const CoordStruct& at) {
    auto& map = MapClass::Instance;
    auto result = LinkedTo && LinkedTo->WhatAmI() == AbstractType::Unit ? map.GetCellAt(at)->GetCoords()
                                                                        : map.PickInfantrySublocation(at, false);
    if (result != CoordStruct::Empty) {
        result.Z = map.GetCellFloorHeight(result);
        if (map.GetCellAt(result)->ContainsBridge()) result.Z += CellClass::BridgeHeight;
    }
    return result;
}
void YRPP_STDCALL JumpjetLocomotionClass::Move_To(CoordStruct to) {
    if (DestinationCoords != CoordStruct::Empty && State != Grounded && DestinationReached) {
        LinkedTo->UnmarkAllOccupationBits(DestinationCoords);
        DestinationReached = false;
    }
    DestinationCoords = to;
    if (to == CoordStruct::Empty) {
        IsMoving = false;
        return;
    }
    auto& map = MapClass::Instance;
    const bool vehicle = LinkedTo->WhatAmI() == AbstractType::Unit;
    const bool alt = (!vehicle || LinkedTo->GetTechnoType()->BalloonHover) && map.GetCellAt(to)->ContainsBridge();
    const auto cell =
        map.NearByLocation(CellClass::Coord2Cell(to), SpeedType(3), -1, MovementZone(9), alt, 1, 1, false, false, false,
                           !vehicle || LinkedTo->GetTechnoType()->BalloonHover, CellStruct::Empty, false, false);
    const auto landing = LandingCoord({cell.X * 256 + 128, cell.Y * 256 + 128, 0});
    if (landing == CoordStruct::Empty) return;
    const bool enter = LinkedTo->QueuedMission == Mission::Enter || LinkedTo->GetCurrentMission() == Mission::Enter;
    if (!vehicle || (!LinkedTo->GetTechnoType()->BalloonHover && !enter)) DestinationCoords = landing;
    if (!enter) LinkedTo->Destination = map.GetCellAt(DestinationCoords);
    IsMoving = true;
    if (State == Descending) {
        State = Ascending;
        CurrentHeight = Height;
        LinkedTo->InAir = false;
        if (LinkedTo->GetCurrentMission() == Mission::Unload) {
            LinkedTo->QueueMission(Mission::Guard, true);
            LinkedTo->unknown_bool_B8 = true;
            if (LinkedTo->ReadyToNextMission()) LinkedTo->NextMission();
        }
    }
}
void YRPP_STDCALL JumpjetLocomotionClass::Stop_Moving() {
    if (Unsorted::ScenarioInit || !IsMoving) return;
    if (DestinationCoords != CoordStruct::Empty && State != Grounded && DestinationReached) {
        LinkedTo->UnmarkAllOccupationBits(DestinationCoords);
        DestinationReached = false;
    }
    const auto* type = LinkedTo->GetTechnoType();
    auto& map = MapClass::Instance;
    const auto at = map.NearByLocation(LinkedTo->GetMapCoords(), type->SpeedType, -1, type->MovementZone, false, 1, 1,
                                       false, false, false, true, CellStruct::Empty, false, false);
    if (at == CellStruct::Empty) {
        if (LinkedTo->Health > 0) {
            LinkedTo->ReceiveDamage(&LinkedTo->Health, 0, RulesClass::Instance->C4Warhead, nullptr, true, true,
                                    nullptr);
            DestinationCoords = CoordStruct::Empty;
        }
    } else {
        CoordStruct to{at.X * 256 + 128, at.Y * 256 + 128, 0};
        to.Z = map.GetCellFloorHeight(to);
        if (map.GetCellAt(at)->ContainsBridge()) to.Z += CellClass::BridgeHeight;
        Move_To(to);
    }
}
Matrix3D YRPP_STDCALL JumpjetLocomotionClass::Draw_Matrix(VoxelIndexKey* key) {
    const float side = LinkedTo->AngleRotatedSideways, front = LinkedTo->AngleRotatedForwards;
    if (!LinkedTo->GetTechnoType()->TiltCrashJumpjet || (std::abs(side) < 0.005 && std::abs(front) < 0.005))
        return LocomotionClass::Draw_Matrix(key);
    if (key) key->Invalidate();
    const float y = matrix_store_float(LinkedTo->GetTechnoType()->VoxelScaleX),
                x = matrix_store_float(LinkedTo->GetTechnoType()->VoxelScaleY);
    const float fs = matrix_store_float(Math::sin(front)), fc = matrix_store_float(Math::cos(front)),
                ss = matrix_store_float(Math::sin(side)), sc = matrix_store_float(Math::cos(side));
    auto vertical = Matrix3D::GetIdentity(), tilt = Matrix3D::GetIdentity();
    vertical.TranslateZ(float(game::x87_integer(std::abs(double(Math::sin(side))) * y + std::abs(double(fs)) * x)));
    tilt.TranslateX(float(game::x87_integer(x - double(game::x87_integer(double(fc) * x)))) * (front < 0 ? -1 : 1));
    tilt.TranslateY(float(game::x87_integer(y - double(game::x87_integer(double(sc) * y)))) * (side > 0 ? -1 : 1));
    tilt.RotateX(side);
    tilt.RotateY(front);
    return drawing_matrix_product(drawing_matrix_product(vertical, LocomotionClass::Draw_Matrix(key)), tilt);
}
