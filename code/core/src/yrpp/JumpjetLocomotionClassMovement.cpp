// Original YR 0x54AEC0 and 0x54B980..0x54D92D. State order is significant.
#include "yrpp/JumpjetLocomotionClass.h"
#include "visibility_grid.hpp"
#include "yrpp/AircraftTrackerClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
namespace {
DirStruct jumpjet_direction(const CoordStruct& from, const CoordStruct& to) {
    return DirStruct(game::x87_integer((Math::atan2(double(from.Y) - to.Y, double(to.X) - from.X) - Math::HalfPi) *
                                       -10430.060040584269));
}
void unmark_airborne(FootClass& foot) {
    const bool onMap = foot.IsOnMap;
    foot.Mark(MarkType::Up);
    foot.IsOnMap = onMap;
}
int bridge_relative_height(FootClass& foot) {
    int height = foot.GetHeight();
    if (!foot.OnBridge && foot.GetCell()->ContainsBridge() && height >= CellClass::BridgeHeight)
        height -= CellClass::BridgeHeight;
    return height;
}
void update_flight_index(FootClass& foot) {
    const auto at = foot.GetMapCoords();
    if (at != foot.LastFlightMapCoords) AircraftTrackerClass::Instance.Update(&foot, foot.LastFlightMapCoords, at);
}
void move_aside(FootClass& foot) {
    foot.SetDestination(
        foot.GetCell()->GetNeighbourCell(static_cast<FacingType>(ScenarioClass::Instance->Random.RandomRanged(0, 7))),
        true);
}
void next_planning_node(FootClass& foot) {
    // 0x705D60 -> 0x6385C0 first returns false for a null PlanningToken.
    // Retain the existing original entry for active planning, whose execution
    // contract is separate from ordinary Jumpjet movement.
    if (foot.PlanningToken) foot.TryNextPlanningTokenNode();
}
UnitClass* jumpjet_vehicle(FootClass* foot) {
    return foot && foot->WhatAmI() == AbstractType::Unit ? static_cast<UnitClass*>(foot) : nullptr;
}
}  // namespace
bool YRPP_STDCALL JumpjetLocomotionClass::Process() {
    const auto layer = In_Which_Layer();
    if (Is_Moving() || Is_Moving_Now()) {
        MovementAI();
        if (!LinkedTo->IsAlive) return false;
        // 0x53A130 is an original unconditional false stub; its crash arm is dead.
        if (LinkedTo->IsCrashing && State != Crashing && State != Unknown && LinkedTo->GetHeight() > 0) {
            const auto center = LinkedTo->GetCell()->GetCoords();
            if (!LinkedTo->IsAttackedByLocomotor ||
                (LinkedTo->Location.X == center.X && LinkedTo->Location.Y == center.Y)) {
                CurrentHeight = -5;
                State = Crashing;
                if (LinkedTo->WhatAmI() == AbstractType::Infantry)
                    static_cast<InfantryClass*>(LinkedTo)->PlayAnim(Sequence::AirDeathStart, false, false);
            } else
                DestinationCoords = center;
        }
        switch (State) {
            case Grounded:
                ProcessGrounded();
                break;
            case Ascending:
                ProcessAscending();
                break;
            case Hovering:
                ProcessHovering();
                break;
            case Cruising:
                ProcessCruising();
                break;
            case Descending:
                ProcessDescending();
                break;
            case Crashing:
                ProcessCrashing();
                break;
            default:
                break;
        }
        if (LinkedTo->IsSelected && LinkedTo->Owner != HouseClass::CurrentPlayer &&
            (MapClass::Instance.IsLocationShrouded(LinkedTo->Location) ||
             (ScenarioClass::Instance->SpecialFlags.FogOfWar &&
              MapClass::Instance.IsLocationFogged(LinkedTo->Location))))
            LinkedTo->Deselect();
        if (!LinkedTo->DiscoveredByCurrentPlayer && LinkedTo->Owner != HouseClass::CurrentPlayer &&
            !MapClass::Instance.IsLocationShrouded(LinkedTo->Location))
            LinkedTo->DiscoveredBy(HouseClass::CurrentPlayer);
    }
    if (LinkedTo->IsAlive && In_Which_Layer() != layer) DisplayClass::Submit(LinkedTo);
    return false;
}
void JumpjetLocomotionClass::ProcessGrounded() {
    if (!Is_Moving()) return;
    LinkedTo->SetSpeedPercentage(1.0);
    LocomotionFacing.SetCurrent(LinkedTo->PrimaryFacing.Current());
    CurrentSpeed = MaxSpeed = 0;
    CurrentHeight = Height;
    if (LinkedTo->GetLastFlightMapCoords() == CellStruct::Empty) AircraftTrackerClass::Instance.Add(LinkedTo);
    State = Ascending;
}
void JumpjetLocomotionClass::ProcessAscending() {
    const int height = bridge_relative_height(*LinkedTo);
    update_flight_index(*LinkedTo);
    if (height >= CurrentHeight) {
        if (AircraftTrackerClass::Instance.IsJumpjet(LinkedTo)) {
            move_aside(*LinkedTo);
            unmark_airborne(*LinkedTo);
            State = Cruising;
        } else {
            LinkedTo->GetCell()->TryAssignJumpjet(LinkedTo);
            unmark_airborne(*LinkedTo);
            State = Hovering;
        }
    } else if (height >
               CurrentHeight / (jumpjet_vehicle(LinkedTo) && LinkedTo->GetTechnoType()->BalloonHover ? 2 : 4)) {
        MaxSpeed = Speed;
        LocomotionFacing.SetDesired(jumpjet_direction(LinkedTo->Location, DestinationCoords));
        if (auto* link = LinkedTo->GetNthLink(); link && link->WhatAmI() == AbstractType::Building) {
            auto* type = static_cast<BuildingClass*>(link)->Type;
            if (type->WeaponsFactory || type->GDIBarracks || type->NODBarracks || type->YuriBarracks)
                LinkedTo->INoticeSink_Unknown(0x117B, 0);
        }
        if (LinkedTo->GetMapCoords() == CellClass::Coord2Cell(DestinationCoords) &&
            !LinkedTo->GetTechnoType()->BalloonHover) {
            CurrentHeight = LinkedTo->GetHeight();
            unmark_airborne(*LinkedTo);
            State = Cruising;
        }
    }
}
void JumpjetLocomotionClass::ProcessHovering() {
    if (!Is_Moving()) return;
    auto& foot = *LinkedTo;
    const auto& type = *foot.GetTechnoType();
    if (foot.Location.X == DestinationCoords.X && foot.Location.Y == DestinationCoords.Y) {
        if (foot.Target) return;
        auto* vehicle = jumpjet_vehicle(&foot);
        if (vehicle && vehicle->Type->IsSimpleDeployer && !foot.IsAttackedByLocomotor) {
            if (type.DeployToLand && !foot.InAir) {
                CurrentHeight = Height;
                unmark_airborne(foot);
                State = Hovering;
                next_planning_node(foot);
                return;
            }
        } else if (type.BalloonHover) {
            unmark_airborne(foot);
            State = Hovering;
            next_planning_node(foot);
            return;
        }
        if (foot.GetCell()->Jumpjet == &foot) foot.GetCell()->TryAssignJumpjet(nullptr);
        State = Descending;
        next_planning_node(foot);
        return;
    }
    LocomotionFacing.SetDesired(jumpjet_direction(foot.Location, DestinationCoords));
    if (!foot.GetCell()->Jumpjet) {
        auto* old = MapClass::Instance.GetCellAt(foot.LastFlightMapCoords);
        if (old->Jumpjet == &foot) old->TryAssignJumpjet(nullptr);
    }
    foot.GetCell()->TryAssignJumpjet(nullptr);
    unmark_airborne(foot);
    State = Cruising;
}
void JumpjetLocomotionClass::ProcessCruising() {
    auto& foot = *LinkedTo;
    update_flight_index(foot);
    LocomotionFacing.SetDesired(jumpjet_direction(foot.Location, DestinationCoords));
    const auto remaining = DirStruct(LocomotionFacing.Desired().Raw - LocomotionFacing.Current().Raw);
    const auto turn = remaining.GetValue<8>();
    const auto& type = *foot.GetTechnoType();
    const double dx = double(foot.Location.X) - DestinationCoords.X, dy = double(foot.Location.Y) - DestinationCoords.Y;
    const int distance = game::x87_integer(Math::sqrt(dx * dx + dy * dy));
    auto* vehicle = jumpjet_vehicle(&foot);
    if (distance < 20) {
        CurrentSpeed = MaxSpeed = 0;
        auto at = foot.Location;
        at.X = DestinationCoords.X;
        at.Y = DestinationCoords.Y;
        const bool onMap = foot.IsOnMap;
        foot.IsOnMap = false;
        foot.SetLocation(at);
        foot.IsOnMap = onMap;
        if (foot.IsAttackedByLocomotor) {
            CurrentHeight = 0;
            if (foot.LocomotorSource) foot.ReleaseLocomotor(true);
            State = Descending;
        } else if (foot.Target || type.BalloonHover) {
            if (AircraftTrackerClass::Instance.IsJumpjet(&foot))
                move_aside(foot);
            else {
                foot.GetCell()->TryAssignJumpjet(&foot);
                unmark_airborne(foot);
                State = Hovering;
            }
        } else if (vehicle && vehicle->Type->IsSimpleDeployer && type.DeployToLand) {
            if (AircraftTrackerClass::Instance.IsJumpjet(&foot))
                move_aside(foot);
            else {
                foot.GetCell()->TryAssignJumpjet(&foot);
                CurrentHeight = Height;
                unmark_airborne(foot);
                State = Hovering;
            }
            next_planning_node(foot);
        } else {
            CurrentHeight = 0;
            State = Descending;
            next_planning_node(foot);
        }
    } else if (distance < Speed) {
        MaxSpeed = Speed / 8;
        if (!foot.Target) CurrentHeight = Height / 2;
    } else if (distance < 2 * Speed) {
        MaxSpeed = Speed / 4;
        if (!foot.Target) CurrentHeight = Height / 2;
        if (turn > TurnRate) MaxSpeed = std::max(1.0, double(Speed / 10));
    } else if (distance < 50 * Speed / TurnRate) {
        MaxSpeed = Speed / 2;
        if (!foot.Target) CurrentHeight = game::x87_integer(Height * 0.75);
        if (turn > 5 * TurnRate) MaxSpeed = std::max(1.0, double(Speed / 5));
    } else {
        CurrentHeight = Height;
        MaxSpeed = Speed;
    }
    const auto land = MapClass::Instance.GetCellAt(DestinationCoords)->LandType;
    if (type.BalloonHover || land == LandType::Water || land == LandType::Beach || (vehicle && type.DeployToLand))
        CurrentHeight = Height;
}
void JumpjetLocomotionClass::ProcessDescending() {
    auto& foot = *LinkedTo;
    auto& map = MapClass::Instance;
    auto* dest = map.GetCellAt(DestinationCoords);
    if (!foot.IsAttackedByLocomotor) {
        const bool enter = foot.QueuedMission == Mission::Enter || foot.GetCurrentMission() == Mission::Enter;
        if ((dest->LandType == LandType::Water || dest->LandType == LandType::Beach) && !enter) {
            if (AircraftTrackerClass::Instance.IsJumpjet(&foot)) {
                move_aside(foot);
                unmark_airborne(foot);
                State = Cruising;
            } else {
                foot.GetCell()->TryAssignJumpjet(&foot);
                unmark_airborne(foot);
                State = Hovering;
                CurrentHeight = Height;
            }
            return;
        }
        const auto occupied = foot.IsCellOccupied(dest, FacingType(-1), -1, nullptr, true);
        const auto spot = CellClass::InfantrySubpositionIndex(DestinationCoords);
        const bool available = dest->IsInfantrySubpositionFree(spot, dest->ContainsBridge()) || DestinationReached ||
                               (foot.WhatAmI() == AbstractType::Unit && spot == 0);
        if (!enter) {
            if (!available ||
                (!dest->ContainsBridge() && (int(occupied) > 2 || (int(occupied) == 2 && !DestinationReached)))) {
                Stop_Moving();
                return;
            }
            if (!DestinationReached) {
                DestinationReached = true;
                foot.MarkAllOccupationBits(DestinationCoords);
            }
        }
    }
    if (auto* vehicle = jumpjet_vehicle(&foot);
        vehicle && vehicle->Type->IsSimpleDeployer && !foot.IsAttackedByLocomotor) {
        const DirStruct facing(RulesClass::Instance->DeployDir << 8);
        if (LocomotionFacing.Current() != facing) LocomotionFacing.SetDesired(facing);
    }
    CurrentHeight = 0;
    if (bridge_relative_height(foot) != 0) return;
    foot.SetSpeedPercentage(0);
    foot.Mark(MarkType::Up);
    if (!foot.IsAttackedByLocomotor) foot.SetLocation(DestinationCoords);
    if (foot.Location.Z > map.GetCellFloorHeight(foot.Location) && foot.GetCell()->ContainsBridge())
        foot.OnBridge = true;
    foot.Mark(MarkType::Down);
    DestinationCoords = CoordStruct::Empty;
    IsMoving = false;
    foot.UpdatePosition(PCPType::End);
    foot.SetDestination(nullptr, true);
    if (!foot.IsAlive || foot.InLimbo || foot.IsFallingDown) return;
    {
        game::SightMoveScope sight_move;
        foot.vt_entry_48C(false, 0, false, nullptr);
        if (foot.IsAlive) foot.UpdateSight(false, 0, false, nullptr, 0);
    }
    if (foot.IsAlive) {
        auto at = foot.Location;
        map.RevealArea3(&at, foot.LastSightRange - 3, foot.LastSightRange + 3, false);
    }
    if (foot.GetCell()->Jumpjet == &foot) foot.GetCell()->TryAssignJumpjet(nullptr);
    AircraftTrackerClass::Instance.Remove(&foot);
    State = Grounded;
    foot.GetCell()->CollectCrate(&foot);
    foot.IsLetGoByLocomotor = true;
    foot.IsBeingManipulated = false;
    foot.IsCrashing = false;
    DestinationReached = false;
    if (jumpjet_vehicle(&foot) && foot.GetTechnoType()->DeployToLand) foot.InAir = false;
}
int JumpjetLocomotionClass::FlightFloor() {
    auto& map = MapClass::Instance;
    auto at = LinkedTo->Location;
    auto* cell = map.GetCellAt(at);
    int height = cell->GetJumpjetHeight();
    if (cell->ContainsBridge()) height += CellClass::BridgeHeight;
    if (CurrentSpeed <= 0) return height;
    const auto delta = Unsorted::AdjacentCoord[LocomotionFacing.Current().GetValue<3>()];
    at.X += delta.X;
    at.Y += delta.Y;
    cell = map.GetCellAt(at);
    const int next = cell->GetJumpjetHeight();
    // 0x54D906 adds the NEXT cell's bridge height to the CURRENT height.
    if (cell->ContainsBridge()) height += CellClass::BridgeHeight;
    return next > height ? next : (next + height) / 2;
}
void JumpjetLocomotionClass::MovementAI() {
    auto& foot = *LinkedTo;
    auto& map = MapClass::Instance;
    const bool mark = State != Hovering && State != Cruising, onMap = foot.IsOnMap;
    if (mark)
        foot.Mark(MarkType::Up);
    else
        foot.IsOnMap = false;
    if (MaxSpeed > CurrentSpeed) CurrentSpeed = std::min(CurrentSpeed + Accel, double(Speed));
    if (MaxSpeed < CurrentSpeed) CurrentSpeed = std::max(CurrentSpeed - double(Accel) * 1.5, 0.0);
    foot.SetSpeedPercentage(CurrentSpeed / Speed);
    const bool same = foot.GetMapCoords() == CellClass::Coord2Cell(DestinationCoords);
    if ((State == Hovering || State == Cruising) && !NoWobbles)
        CurrentWobbles += Math::TwoPi / (15.0 / Wobbles);
    else
        CurrentWobbles = 0;
    const int targetHeight = game::x87_integer(Math::sin(CurrentWobbles) * Deviation + CurrentHeight);
    const int oldZ = foot.GetZ();
    int z = oldZ;
    int floor = map.GetCellFloorHeight(foot.Location);
    if (foot.GetCell()->ContainsBridge() && double(floor + 4 * Unsorted::LevelHeight) - Crash <= foot.Location.Z)
        floor += CellClass::BridgeHeight;
    int relative = (State == Descending || State == Grounded || (same && !foot.GetTechnoType()->BalloonHover))
                       ? oldZ - floor
                       : oldZ - FlightFloor();
    bool changed = false;
    if (relative < targetHeight) {
        int height = foot.GetHeight();
        if (foot.GetCell()->ContainsBridge() && !foot.OnBridge &&
            foot.Location.Z >= CellClass::BridgeHeight + map.GetCellFloorHeight(foot.Location))
            height -= CellClass::BridgeHeight;
        if (!height) {
            foot.UnmarkAllOccupationBits(foot.Location);
            foot.OnBridge = false;
        }
        z = targetHeight >= double(relative) + Climb ? game::x87_integer(double(z) + Climb)
                                                     : z + targetHeight - relative;
        changed = true;
    } else if (relative > targetHeight) {
        z = targetHeight <= double(relative) - Climb ? game::x87_integer(double(z) - Climb)
                                                     : oldZ + targetHeight - relative;
        if (z <= floor) z = floor;
        relative = std::max(relative, 0);
        changed = true;
    }
    if (!same) {
        if (relative < targetHeight / 2) CurrentSpeed = 0;
        if (relative < targetHeight / 4) CurrentSpeed = 0;
    }
    if (changed) {
        foot.SetZ(z);
        if (mark) foot.Mark(MarkType::Up);
    }
    auto at = foot.Location;
    const double speed = game::x87_integer(CurrentSpeed);
    const double angle = (std::bit_cast<short>(LocomotionFacing.Current().Raw) - 0x3FFF) * -0.00009587672516830327;
    at.Y = game::x87_integer(double(at.Y) - Math::sin(angle) * speed);
    at.X = game::x87_integer(Math::cos(angle) * speed + double(at.X));
    foot.SetLocation(at);
    if (!foot.IsAttackedByLocomotor) {
        const auto* infantry = foot.WhatAmI() == AbstractType::Infantry ? static_cast<InfantryClass*>(&foot) : nullptr;
        if (infantry && infantry->Type->JumpJetTurn && State == Hovering && foot.Target) {
            DirStruct facing;
            foot.GetDirectionTo(&facing, foot.Target);
            LocomotionFacing.SetCurrent(facing);
        } else
            foot.PrimaryFacing.SetCurrent(LocomotionFacing.Current());
    }
    if (mark)
        foot.Mark(MarkType::Down);
    else
        foot.IsOnMap = onMap;
}
