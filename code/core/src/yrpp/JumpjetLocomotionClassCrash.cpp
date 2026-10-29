// YR 0x54CA90: falling Jumpjets, including the original locomotor-attack arm.
#include "yrpp/JumpjetLocomotionClass.h"
#include "yrpp/AircraftTrackerClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/VocClass.h"
#include "x87_integer.hpp"
void JumpjetLocomotionClass::ProcessCrashing() {
    auto& foot = *LinkedTo;
    auto& map = MapClass::Instance;
    auto at = foot.Location;
    const int oldZ = at.Z;
    if (foot.IsAttackedByLocomotor) {
        CurrentSpeed = MaxSpeed = 0;
        at.Z = game::x87_integer(double(at.Z) - Crash);
        Crash = float(double(Crash) + 1.0);
    } else {
        at.Z = game::x87_integer(double(at.Z) - Crash);
        LocomotionFacing.SetDesired(DirStruct(LocomotionFacing.Current().Raw + 0x2D00));
    }
    const int bridgeZ = CellClass::BridgeHeight + map.GetCellFloorHeight(at);
    const bool bridge = map.GetCellAt(at)->ContainsBridge() && oldZ >= bridgeZ && at.Z < bridgeZ;
    if (map.CoordinatesLegal(CellClass::Coord2Cell(at))) {
        DisplayClass::Remove(&foot);
        foot.Mark(MarkType::Up);
        foot.SetLocation(at);
        foot.Mark(MarkType::Down);
        DisplayClass::Submit(&foot);
    }
    bool fatal = false;
    if ((foot.GetHeight() <= 0 || bridge) && foot.IsBeingManipulated) {
        bool crushed = false;
        auto* victim = foot.GetCell()->FirstObject;
        if (victim == &foot) victim = victim->NextObject;
        if (victim && (victim->WhatAmI() == AbstractType::Unit || victim->WhatAmI() == AbstractType::Building))
            fatal = true;
        else {
            if (victim && victim->WhatAmI() == AbstractType::Infantry) {
                while (victim) {
                    auto* next = victim->NextObject;
                    VocClass::PlayAt(victim->GetType()->CrushSound, foot.Location, nullptr);
                    if (foot.BeingManipulatedBy)
                        victim->RegisterDestruction(foot.BeingManipulatedBy);
                    else if (foot.ChronoWarpedByHouse)
                        victim->RegisterKill(foot.ChronoWarpedByHouse);
                    else
                        victim->RegisterDestruction(&foot);
                    victim->Mark(MarkType::Up);
                    victim->Limbo();
                    victim->UnInit();
                    victim = next;
                }
            } else if (victim) {
                victim->ReceiveDamage(&victim->Health, 0, RulesClass::Instance->CrushWarhead,
                                      foot.BeingManipulatedBy ? foot.BeingManipulatedBy : &foot, true, false,
                                      foot.BeingManipulatedBy ? nullptr : foot.ChronoWarpedByHouse);
            }
            crushed = true;
            auto* cell = foot.GetCell();
            const bool water = cell->LandType == LandType::Water && !bridge,
                       beach = cell->LandType == LandType::Beach && !bridge;
            auto move = foot.IsCellOccupied(cell, FacingType(-1), -1, nullptr, true);
            if (foot.WhatAmI() == AbstractType::Aircraft &&
                !cell->IsClearToMove(SpeedType(2), true, true, -1, MovementZone(0), -1, true))
                move = Move(7);
            if ((foot.GetTechnoType()->JumpJet &&
                 !cell->IsClearToMove(SpeedType(2), true, true, -1, MovementZone(0), -1, true)) ||
                move != Move::OK) {
                if (water && !beach) {
                    foot.IsSinking = true;
                    foot.Stun();
                    if (foot.SlaveManager) {
                        foot.SlaveManager->Killed(foot.BeingManipulatedBy, nullptr);
                        GameDelete(foot.SlaveManager);
                        foot.SlaveManager = nullptr;
                    }
                    if (foot.BeingManipulatedBy)
                        foot.RegisterDestruction(foot.BeingManipulatedBy);
                    else if (foot.ChronoWarpedByHouse)
                        foot.RegisterKill(foot.ChronoWarpedByHouse);
                } else
                    fatal = true;
            } else if ((foot.SpeedPercentage != 5.0 && foot.SpeedPercentage != 7.0) || (water && !beach)) {
                if (cell->Jumpjet == &foot) cell->TryAssignJumpjet(nullptr);
                State = Descending;
                foot.SetDestination(nullptr, true);
            } else
                fatal = true;
        }
        if (foot.GetHeight() > 0 && !bridge) {
            if (!foot.IsAttackedByLocomotor || fatal) {
                foot.BeingManipulatedBy = nullptr;
                foot.ChronoWarpedByHouse = nullptr;
            }
        } else if (bridge && crushed)
            foot.OnBridge = true;
    }
    if ((foot.GetHeight() <= 0 || bridge) && (!foot.IsAttackedByLocomotor || fatal)) {
        if (foot.GetCell()->Jumpjet == &foot) foot.GetCell()->TryAssignJumpjet(nullptr);
        AircraftTrackerClass::Instance.Remove(&foot);
        MaxSpeed = 0;
        State = Unknown;
        foot.INoticeSink_Unknown(0x117C, 0);
        foot.BeingManipulatedBy = nullptr;
        foot.ChronoWarpedByHouse = nullptr;
    }
}
