// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp::What_Action / Move_Order / Active_Click_With.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/WeaponTypeClass.h"
#include "scenario_runtime.hpp"
#include <cstdlib>

namespace {
CoordStruct center(const CellStruct& cell) {
    CoordStruct result{int(cell.X)*256+128,int(cell.Y)*256+128,0};
    result.Z=MapClass::Instance.GetCellFloorHeight(result);
    if((static_cast<unsigned>(MapClass::Instance.GetCellAt(result)->Flags)&0x100u)!=0)result.Z+=CellClass::BridgeHeight;
    return result;
}
}
Action FootClass::MouseOverObject(const ObjectClass* target,bool ignoreForce) const {
    auto action=TechnoClass::MouseOverObject(target,ignoreForce);
    if(!target)return action;
    auto at=target->GetCoords();at.Z=MapClass::Instance.GetCellFloorHeight(at);
    if(MapClass::Instance.GetCellAt(at)->ContainsBridge())at.Z+=CellClass::BridgeHeight;
    const auto& runtime=game::scenario_runtime();
    if(MapClass::Instance.IsLocationShrouded(at)&&action!=Action::None&&runtime.session_mode(runtime.context)==int(GameMode::Campaign))
        return GetTechnoType()->MoveToShroud?Action::Move:Action::NoMove;
    return action;
}
Action FootClass::MouseOverCell(const CellStruct* where,bool checkFog,bool ignoreForce) const {
    auto action=TechnoClass::MouseOverCell(where,checkFog,ignoreForce);
    if(MapClass::Instance.IsLocationShrouded(center(*where)) && action!=Action::None) {
        if(!GetTechnoType()->MoveToShroud || !MapClass::Instance.IsWithinUsableArea(*where,true))return Action::NoMove;
        if(action!=Action::PatrolWaypoint)return Action::Move;
    }
    return action;
}
CellStruct* FootClass::MoveOrder(CellStruct* output,const CellStruct* where,bool checkFog) {
    auto& map=MapClass::Instance;
    const auto action=MouseOverCell(where,checkFog,false);
    const bool inRadar=map.IsWithinUsableArea(*where,true);
    if(!GetTechnoType()->MoveToShroud && map.IsLocationShrouded(center(*where))){*output=CellStruct::Empty;return output;}
    auto result=*where;
    const auto source=GetDestination();auto* from=map.GetCellAt(source);
    auto zone=GetTechnoType()->MovementZone;
    if(zone==MovementZone::Subterrannean)zone=MovementZone::Normal;
    // YR 0x53A130 is a legitimate false stub (removed TS ion storm).
    const bool freeZone=IsInAir() || GetTechnoType()->IsSubterranean
        || (WhatAmI()==AbstractType::Infantry && static_cast<InfantryClass*>(this)->Type->JumpJet)
        || WhatAmI()==AbstractType::Aircraft;
    const bool fromBridge=IsOnBridge(nullptr);
    const bool shroud=map.IsLocationShrouded(center(*where));
    const bool toBridge=(static_cast<unsigned>(map.GetCellAt(*where)->Flags)&0x100u)!=0;
    if(freeZone && action==Action::NoMove)
        result=map.NearByLocation(*where,GetTechnoType()->SpeedType,-1,zone,toBridge,1,1,false,true,
            GetTechnoType()->IsSubterranean && GetHeight()<0,true,CellStruct{0,0},false,false);
    else if(!inRadar || shroud || (!freeZone && (!GetTechnoType()->Teleporter || action==Action::NoMove)
        && !map.IsSameCellZone(from->MapCoords,result,zone,fromBridge,toBridge,false)))
        result=map.NearByLocation(*where,GetTechnoType()->SpeedType,map.GetMovementZoneType(from->MapCoords,zone,fromBridge),
            zone,toBridge,1,1,false,true,false,true,CellStruct{0,0},false,false);
    *output=result;return output;
}
bool FootClass::CellClickedAction(Action,CellStruct* where,CellStruct* follow,bool ignoreForce) {
    if(Berzerk || (DirectRockerLinkedUnit && !GetTechnoType()->Pushy))return false;
    auto& map=MapClass::Instance;
    const auto action=MouseOverCell(where,false,ignoreForce);
    if(!map.IsWithinUsableArea(*where,true) && action!=Action::NoMove)return false;
    switch(action) {
        case Action::Move:case Action::AttackMoveNav:
            if(Unsorted::MoveFeedback) {
                const auto& runtime=game::scenario_runtime();
                const auto mode=static_cast<GameMode>(runtime.session_mode(runtime.context));
                if(mode!=GameMode::Campaign && mode!=GameMode::Skirmish)std::abort(); // network MoveFlashes list pending
                const auto at=center(*where);
                if(void* storage=YRMemory::Allocate(sizeof(AnimClass)))::new(storage) AnimClass(RulesClass::Instance->MoveFlash,at);
            }
            [[fallthrough]];
        case Action::NoMove: {
            CellStruct destination;MoveOrder(&destination,where,false);
            if(destination==CellStruct::Empty)return false;
            ClickedMission(Mission::Move,nullptr,map.GetCellAt(destination),map.GetCellAt(*follow));return true;
        }
        case Action::Attack:case Action::AttackMoveTar:
            for(auto* object:AbstractClass::Array)if(object->WhatAmI()==AbstractType::VeinholeMonster)std::abort();
            ClickedMission(Mission::Attack,map.GetCellAt(*where),nullptr,nullptr);return true;
        case Action::Harvest:ClickedMission(Mission::Harvest,nullptr,map.GetCellAt(*where),nullptr);return true;
        case Action::Enter:case Action::Capture:
            if(!IsControllable())return false;
            ClickedMission(action==Action::Enter?Mission::Enter:Mission::Capture,nullptr,map.GetCellAt(*where),nullptr);return true;
        case Action::Sabotage:ClickedMission(Mission::Sabotage,nullptr,map.GetCellAt(*where),nullptr);return true;
        case Action::GuardArea:
            if((!IsActive() && !IsEngineer()) || !IsControllable())return false;
            ClickedMission(Mission::Area_Guard,map.GetCellAt(*where),nullptr,nullptr);return true;
        case Action::EnterTunnel: {
            auto* tunnel=map.GetCellAt(*where)->GetTunnel();
            auto* exit=map.GetCellAt(tunnel->ExitCell)->GetTunnel();
            const auto delta=Unsorted::AdjacentCell[(exit->ExitFace-4)&7];
            const CellStruct destination{short(tunnel->ExitCell.X+delta.X),short(tunnel->ExitCell.Y+delta.Y)};
            ClickedMission(Mission::Move,nullptr,map.GetCellAt(destination),nullptr);return true;
        }
        case Action::PatrolWaypoint: {
            if(!IsControllable())return false;
            CellStruct destination;MoveOrder(&destination,where,false);
            ClickedMission(Mission::Patrol,nullptr,map.GetCellAt(destination),nullptr);return true;
        }
        default:return true;
    }
}
bool FootClass::ObjectClickedAction(Action action,ObjectClass* target,bool) {
    if(Berzerk||(DirectRockerLinkedUnit&&!GetTechnoType()->Pushy))return false;
    // 0x4D74E0 consumes the action chosen by the derived entry; unlike the
    // cell overload it must NOT call MouseOverObject again here.
    switch(action){
        case Action::Self_Deploy:case Action::AreaAttack:case Action::DetonateAll:
            ClickedMission(Mission::Unload,nullptr,nullptr,nullptr);return true;
        case Action::Attack:case Action::IvanBomb:case Action::AttackMoveTar:
        case Action::Demolish:case Action::Airstrike:
            if(!IsActive())return false;
            ClickedMission(Mission::Attack,target,nullptr,nullptr);return true;
        case Action::PatrolWaypoint:case Action::NoIvanBomb:return false;
        case Action::AttackSupport:{
            auto* slot=GetWeapon(0);
            const bool heals=slot&&slot->WeaponType&&slot->WeaponType->Damage<0;
            ClickedMission(heals?Mission::Area_Guard:Mission::Guard,this,nullptr,nullptr);return true;
        }
        case Action::Capture:
            if(!IsControllable())return false;
            ClickedMission(Mission::Capture,nullptr,target,nullptr);return true;
        case Action::Enter:
            if(!IsControllable()||!target||(target->AbstractFlags&::AbstractFlags::Techno)==::AbstractFlags::None)return false;
            ClickedMission(Mission::Enter,nullptr,target,nullptr);return true;
        case Action::Repair:
            if(!IsControllable())return false;
            ClickedMission(Mission::Eaten,nullptr,target,nullptr);return true;
        case Action::Move:case Action::NoMove:case Action::AttackMoveNav:
        case Action::Sabotage:case Action::GuardArea:case Action::Detonate:
            // Object movement, transport/radio and attached-bomb dispatch
            // retain explicit dependencies; no guessed cell-order fallback.
            std::abort();
        default:return true;
    }
}
