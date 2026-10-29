#include "debug/debug_state.hpp"
// YR 0x7404B0 / 0x73FD50: preserve derived action order and query side effects.
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/FoggedObjectClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/PlanningTokenClass.h"

bool UnitClass::CanDeployNow() const {
    if (Type->DeployToFire) return true;
    return Type->DeploysInto && Type->DeploysInto->TickTank && !Owner->IsControlledByHuman() && Target &&
           Target->What_Am_I() == AbstractType::Unit && MapClass::Instance.GetCellAt(Location)->CanBuildHere();
}

Action UnitClass::MouseOverCell(const CellStruct* where, bool checkFog, bool ignoreForce) const {
    if (!game::debug_can_control_house(Owner)) return Action::None;
    auto& map = MapClass::Instance;
    auto* cell = map.GetCellAt(*where);
    auto action = FootClass::MouseOverCell(where, checkFog, ignoreForce);
    if (CombatDamage(-1) < 0 && game::debug_can_control_house(Owner) && action == Action::Attack)
        action = Action::AttackSupport;
    int remembered = -1;
    if (checkFog && ScenarioClass::Instance->SpecialFlags.FogOfWar && cell->FoggedObjects)
        for (auto* object : *cell->FoggedObjects)
            if (object->CanDraw && object->RTTI == AbstractType::Overlay) remembered = int(object->Overlay);
    CoordStruct position{int(where->X) * 256 + 128, int(where->Y) * 256 + 128, 0};
    position.Z = map.GetCellFloorHeight(position);
    if (map.GetCellAt(position)->ContainsBridge()) position.Z += CellClass::BridgeHeight;
    const bool shrouded = game::debug_player_location_shrouded(position);
    if (action == Action::Move) {
        if (!shrouded) {
            const bool ore = cell->LandType == LandType::Tiberium ||
                             (remembered != -1 && OverlayTypeClass::Array[remembered]->Tiberium && checkFog);
            if (ore && Type->Harvester) return Action::Harvest;
            if ((cell->LandType == LandType::Weeds || (remembered == 126 && cell->OverlayData >= 0x30 && checkFog)) &&
                Type->Weeder)
                return Action::Harvest;
            if (ore && Type->ResourceDestination && Type->ResourceGatherer) return Action::Harvest;
        }
        if (map.GetCellAt(*where)->GetTunnel())
            return map.GetCellAt(*where)->CanEnterTunnel(this) ? Action::EnterTunnel : Action::NoEnterTunnel;
        if (Deployed || Deploying || Undeploying) action = Action::None;
    }
    if (game::debug_can_control_house(Owner) && Type->IsSimpleDeployer && Deployed) {
        if (action == Action::Move) return Action::NoMove;
        if (action != Action::Attack) return action;
        if (!Type->DeployFire) return Action::NoMove;
        const int weapon = SelectWeapon(nullptr);
        if (!IsCloseEnough3D({int(where->X) * 256 + 128, int(where->Y) * 256 + 128, 0}, weapon)) return Action::NoMove;
    }
    if (action == Action::Attack && !vt_entry_310()) return Action::None;
    return action;
}

Action UnitClass::MouseOverObject(const ObjectClass* object, bool ignoreForce) const {
    auto action = FootClass::MouseOverObject(object, ignoreForce);
    if (action == Action::ToggleSelect) return action;
    auto* target = const_cast<ObjectClass*>(object);
    auto* self = const_cast<UnitClass*>(this);
    bool repair_override = false;
    if (CombatDamage(-1) < 0 && game::debug_can_control_house(Owner) && action != Action::Self_Deploy) {
        if (!Owner->IsAlliedWith(target)) {
            action = Action::None;
            repair_override = true;
        } else if (target->IsStrange() && target != this && target->IsSurfaced()) {
            if ((target->WhatAmI() != AbstractType::Aircraft ||
                 !MapClass::Instance.GetCellAt(target->GetCoords())->GetBuilding()) &&
                target->GetHealthPercentage() < RulesClass::Instance->ConditionGreen) {
                action = Action::GRepair;
                repair_override = true;
            }
        } else if (target->WhatAmI() != AbstractType::Building) {
            action = Action::Select;
            repair_override = true;
        }
    }
    if (!repair_override && action == Action::Attack) {
        if (!IsActive())
            action = (Type->Crusher || HasAbility(Ability::Crusher)) && target->IsCrushable(self) ? Action::Move
                                                                                                  : Action::Select;
    } else if (!repair_override && action == Action::Self_Deploy) {
        if (PlanningNodeClass::PlanningModeActive)
            action = Action::NoDeploy;
        else {
            if (Type->DeploysInto) {
                const auto coords = GetCoords();
                CellStruct cell{short(coords.X / 256), short(coords.Y / 256)};
                if (Type->DeploysInto->GetFoundationWidth() > 2 || Type->DeploysInto->GetFoundationHeight(false) > 2) {
                    --cell.X;
                    --cell.Y;
                }
                self->Mark(MarkType::Up);
                Locomotor->Mark_All_Occupation_Bits(MarkType::Up);
                const bool allowed = Type->DeploysInto->CanCreateHere(cell, nullptr);
                Locomotor->Mark_All_Occupation_Bits(MarkType::Down);
                self->Mark(MarkType::Down);
                if (!allowed) action = Action::NoDeploy;
            } else if (!Type->IsSimpleDeployer) {
                if (Type->Passengers > 0 && !BunkerLinkedItem) {
                    if (!Passengers.NumPassengers)
                        action = Action::NoDeploy;
                    else if (!OnBridge) {
                        auto* cell = MapClass::Instance.GetCellAt(Location);
                        auto* first = Passengers.GetFirstPassenger();
                        if (first &&
                            GroundType::Array[int(cell->LandType)].Cost[int(first->GetTechnoType()->SpeedType)] <
                                0.01 &&
                            cell->LandType != LandType::Water)
                            action = Action::NoDeploy;
                        if (Type->SpeedType != SpeedType::Hover && GetHeight() != 0) action = Action::NoDeploy;
                    }
                } else if (!Type->DeployFire && !BunkerLinkedItem)
                    action = Action::None;
            }
            if (action == Action::Self_Deploy && !CanDeploySlashUnload()) action = Action::NoDeploy;
        }
    }
    auto* techno = target && (target->AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None
                       ? static_cast<TechnoClass*>(target)
                       : nullptr;
    auto radio = [&] { return self->SendCommand(static_cast<RadioCommand>(15), static_cast<TechnoClass*>(target)); };
    if (action == Action::Select && game::debug_can_control_house(Owner) && techno &&
        techno->Owner->IsAlliedWith(this) && !techno->IsBeingWarpedOut() &&
        target->WhatAmI() == AbstractType::Building && int(radio()) == 1) {
        if (Type->MovementZone == MovementZone::Fly)
            action = Action::NoEnter;
        else
            action = static_cast<BuildingClass*>(target)->Type->Grinding ? Action::Repair : Action::Enter;
    }
    if (game::debug_can_control_house(Owner) && action == Action::Select &&
        target->WhatAmI() == AbstractType::Building && static_cast<BuildingClass*>(target)->Type->UnitRepair &&
        int(radio()) == 1 && !static_cast<BuildingClass*>(target)->HasAnyLink() &&
        !static_cast<TechnoClass*>(target)->Passengers.FirstPassenger)
        action = Action::Move;
    if (game::debug_can_control_house(Owner) && action == Action::Select) {
        if (target->WhatAmI() == AbstractType::Building && static_cast<BuildingClass*>(target)->Type->Bunker &&
            int(radio()) == 1 && !static_cast<BuildingClass*>(target)->HasAnyLink() &&
            !static_cast<TechnoClass*>(target)->Passengers.FirstPassenger)
            action = Action::Move;
    } else if (action == Action::Attack && !IsArmed())
        action = Action::AttackSupport;
    if (action != Action::NoEnter && Owner->IsAlliedWith(target) && game::debug_can_control_house(Owner) &&
        !target->IsBeingWarpedOut() && !target->IsWarpingIn() && techno && action != Action::Attack &&
        action != Action::GRepair && action != Action::GuardArea) {
        const auto* type = target->GetTechnoType();
        if (target != this && type && type->Passengers > 0) {
            if ((target->AbstractFlags & ::AbstractFlags::Foot) != ::AbstractFlags::None &&
                static_cast<FootClass*>(target)->Locomotor->Is_Moving())
                action = Action::NoEnter;
            else {
                const int answer = int(radio());
                if (answer == 1)
                    action = Action::Enter;
                else if (answer == 10)
                    action = Action::NoEnter;
            }
        }
    }
    if (game::debug_can_control_house(Owner) && Type->IsSimpleDeployer && Deployed) {
        if (action == Action::Move)
            action = Action::NoMove;
        else if (action == Action::Attack && (!Type->DeployFire || !IsCloseEnough(target, SelectWeapon(nullptr))))
            return Action::NoMove;
    }
    if (action == Action::None) {
        if (game::debug_can_control_house(Owner) && IsControllable()) action = Action::NoMove;
    } else if (action == Action::Attack) {
        if (!vt_entry_310()) return Action::None;
        if (target->GetType()->Immune) action = Action::NoMove;
        return action;
    }
    if (action == Action::NoMove && target->IsDisguised() && !target->GetDisguiseHouse(true) &&
        target->GetDisguise(true)->WhatAmI() == AbstractType::TerrainType)
        action = Action::Move;
    return action;
}
