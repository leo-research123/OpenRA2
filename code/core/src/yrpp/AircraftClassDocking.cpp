// Original aircraft docking, navigation and landing: YR 0x41BBD0/0x41AA80/0x419C80.
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/TeamClass.h"
#include "com_error.hpp"
#include "x87_integer.hpp"
#include <algorithm>

BuildingClass* AircraftClass::TryNearestDockBuilding(TypeList<BuildingTypeClass*>* types, DWORD, DWORD busy) const {
    auto* self = const_cast<AircraftClass*>(this);
    if (Type->AirportBound && DockNowHeadingTo) {
        if (self->SendCommand(RadioCommand::QueryCanEnter, DockNowHeadingTo) == RadioCommand::AnswerPositive)
            return DockNowHeadingTo;
        self->DockNowHeadingTo = nullptr;
    }
    auto* result = FootClass::TryNearestDockBuilding(types, 0, busy);
    self->DockNowHeadingTo = result;
    return result;
}

void AircraftClass::SetDestination(AbstractClass* target, bool immediate) {
    auto* next = target;
    if (target) {
        if (target->IsInAir()) {
            next = nullptr;
        } else {
            auto* building = target->WhatAmI() == AbstractType::Building ? static_cast<BuildingClass*>(target) : nullptr;
            if (building && (GetCurrentMission() == Mission::Enter || QueuedMission == Mission::Enter)) {
                if (!HasAnyLink()) {
                    if (!building->HasFreeLink(this)) {
                        SetArchiveTarget(building);
                        if (building->Type->Helipad) {
                            next = TryNearestDockBuilding(&Type->Dock, 0, 0);
                            SetDestination(nullptr, true);
                            Mission mission = Mission::Move;
                            if (next && SendCommand(RadioCommand::QueryCanEnter, static_cast<TechnoClass*>(next)) ==
                                            RadioCommand::AnswerPositive) {
                                SendCommand(RadioCommand::RequestLink, static_cast<TechnoClass*>(next));
                                mission = Mission::Enter;
                            } else
                                next = FindLandingLocation();
                            QueueMission(mission, false);
                            if (ReadyToNextMission()) NextMission();
                        }
                        if (building->Type->UnitRepair) {
                            QueueUpToEnter = building;
                            next = nullptr;
                        }
                    } else {
                        if (SendCommand(RadioCommand::RequestLoading, building) != RadioCommand::AnswerPositive) {
                            SendToFirstLink(RadioCommand::NotifyUnlink);
                            if (building->Type->UnitRepair || building->Type->UnitReload) {
                                SetArchiveTarget(building);
                                next = nullptr;
                            }
                        }
                        if (building->Type->UnitRepair || building->Type->UnitReload) {
                            if (Destination) SetArchiveTarget(Destination);
                        } else
                            SetArchiveTarget(next);
                    }
                } else if (building->HasFreeLink(this)) {
                    if (!building->ContainsLink(this)) {
                        if (SendToFirstLink(RadioCommand::RequestLoading) == RadioCommand::AnswerPositive) return;
                        SendToFirstLink(RadioCommand::NotifyUnlink);
                        if (building->Type->UnitRepair || building->Type->UnitReload) {
                            SetArchiveTarget(building);
                            next = nullptr;
                        }
                    }
                } else
                    SetArchiveTarget(next);
            }
            auto* cell = GetCell();
            if ((cell->Flags & CellFlags::BridgeHead) == CellFlags{}) {
                for (auto* occupant = cell->FirstObject; occupant; occupant = occupant->NextObject) {
                    if (occupant == this || occupant->WhatAmI() != AbstractType::Building) continue;
                    auto* underneath = static_cast<BuildingClass*>(occupant);
                    if (underneath->Type->UnitRepair || underneath->Type->UnitReload) {
                        if (!Locomotor) game::raise_com_error(static_cast<HRESULT>(0x80004003u));
                        if (!Locomotor->Is_Powered()) Locomotor->Power_On();
                        if (GetNthLink() == underneath && next != GetNthLink())
                            SendToFirstLink(RadioCommand::NotifyUnlink);
                    }
                    break;
                }
            }
        }
    }
    FootClass::SetDestination(next, immediate);
}

int AircraftClass::Mission_Enter() {
    // Legacy approach slots remain distinct even when this handler groups them.
    enum EnterState : int {
        Initial = 0,
        TakeOff = 1,
        Altitude = 2,
        Stack = 3,
        Downwind = 4,
        Crosswind = 5,
        Travel = 6,
        Landing = 7
    };
    if (!GetHeight() && Type->AirportBound && Destination && Destination != GetCell()->GetBuilding()) {
        DockNowHeadingTo = Destination->WhatAmI() == AbstractType::Building
                               ? static_cast<BuildingClass*>(Destination) : nullptr;
        SendToFirstLink(RadioCommand::NotifyUnlink);
        QueueMission(Mission::Move, true);
        return 1;
    }
    if (GetHeight() > 0 && DockNowHeadingTo != Destination && Destination &&
        Destination->WhatAmI() == AbstractType::Building)
        DockNowHeadingTo = static_cast<BuildingClass*>(Destination);
    if (!Destination && InWhichLayer() != Layer::Ground &&
        SendToFirstLink(RadioCommand::RequestLoading) != RadioCommand::AnswerPositive && !ApproachEnterQueue()) {
        EnterIdleMode(false, true);
        return 1;
    }
    switch (MissionStatus) {
        case EnterState::Initial:
            MissionStatus = EnterState::Travel;
            [[fallthrough]];
        case EnterState::TakeOff:
        case EnterState::Altitude:
        case EnterState::Stack:
        case EnterState::Downwind:
        case EnterState::Crosswind:
            IsCarryallNotLanding = true;
            return 1;
        case EnterState::Travel:
            if (Type->AirportBound && GetHeight()) SendToFirstLink(RadioCommand::RequestLoading);
            if (HasAnyLink()) {
                if (!Locomotor) game::raise_com_error(static_cast<HRESULT>(0x80004003u));
                if (Locomotor->Get_Status() == 1 || (Type->AirportBound && !GetHeight())) {
                    MissionStatus = EnterState::Landing;
                    IsCarryallNotLanding = false;
                }
                return 3;
            }
            if (!ApproachEnterQueue()) {
                if (!Locomotor) game::raise_com_error(static_cast<HRESULT>(0x80004003u));
                if (Locomotor->Get_Status() == 1) MissionStatus = EnterState::Landing;
                if (QueuedMission == Mission::None) {
                    SetDestination(nullptr, true);
                    EnterIdleMode(false, true);
                    IsCarryallNotLanding = true;
                    return 1;
                }
                IsCarryallNotLanding = true;
            }
            IsCarryallNotLanding = true;
            return 1;
        case EnterState::Landing:
            if (!Locomotor) game::raise_com_error(static_cast<HRESULT>(0x80004003u));
            if (Locomotor->Get_Status() == 1) {
                if (Destination) {
                    auto at = Destination->WhatAmI() == AbstractType::Building
                                  ? static_cast<BuildingClass*>(Destination)->GetDockCoords(this)
                                  : Destination->GetCoords();
                    at.X = Location.X + std::clamp(at.X - Location.X, -5, 5);
                    at.Y = Location.Y + std::clamp(at.Y - Location.Y, -5, 5);
                    at.Z = Location.Z;
                    SetLocation(at);
                }
                IsCarryallNotLanding = true;
                return 1;
            }
            IsCarryallNotLanding = true;
            switch (SendToFirstLink(RadioCommand::RequestCompleteEnter)) {
                case RadioCommand::AnswerPositive:
                    QueueMission(Mission::Guard, false);
                    break;
                case RadioCommand::unknown_5:
                    Limbo();
                    if (auto* carrier = GetNthLink()) carrier->AddPassenger(this);
                    break;
                default:
                    EnterIdleMode(false, true);
                    break;
            }
            return 1;
        default:
            return 1;
    }
}

AbstractClass* AircraftClass::FindLandingLocation() {
    int distance = -1;
    CellStruct best{};
    const auto inspect = [&](ObjectClass* object, bool building) {
        const auto delta = GetCoords() - object->GetCoords();
        int candidate = game::x87_integer(Math::sqrt(double(delta.X) * delta.X +
                                                    double(delta.Z) * delta.Z + double(delta.Y) * delta.Y));
        if (building && Type->Dock.Count && static_cast<BuildingClass*>(object)->Type == Type->Dock[0])
            candidate /= 4;
        if (distance != -1 && candidate >= distance) return;
        const auto cell = object->GetMapCoords();
        const auto zone = MapClass::Instance.GetMovementZoneType(cell, MovementZone::Normal, false);
        auto found = MapClass::Instance.NearByLocation(cell, SpeedType::Foot, zone, MovementZone::Normal,
                                                     false, building ? 3 : 1, building ? 3 : 1,
                                                     false, false, false, true, CellStruct::Empty, false, false);
        if (found != CellStruct::Empty) { distance = candidate; best = found; }
    };
    if (Type->Dock.Count)
        for (auto* building : Owner->Buildings)
            if (building && !building->InLimbo) inspect(building, true);
    if (distance != -1) {
        if (distance < 256) {
            const auto cell = GetMapCoords();
            const auto zone = MapClass::Instance.GetMovementZoneType(cell, MovementZone::Normal, false);
            best = MapClass::Instance.NearByLocation(cell, SpeedType::Foot, zone, MovementZone::Normal,
                                                    false, 1, 1, false, false, false, true,
                                                    CellStruct::Empty, false, false);
        }
        return MapClass::Instance.GetCellAt(best);
    }
    for (auto* object : ObjectClass::Array) {
        if (!object || object == this || object->InLimbo) continue;
        const auto kind = object->WhatAmI();
        if (kind != AbstractType::Infantry && kind != AbstractType::Aircraft &&
            kind != AbstractType::Building && kind != AbstractType::Unit)
            continue;
        if (static_cast<TechnoClass*>(object)->Owner == Owner) inspect(object, false);
    }
    return MapClass::Instance.GetCellAt(distance == -1 ? GetMapCoords() : best);
}
