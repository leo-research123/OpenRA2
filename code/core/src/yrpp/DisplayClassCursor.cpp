// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 display.cpp Mouse_Left_Up.
// Copyright Electronic Arts Inc. / OpenTS contributors. EA Section 7 terms:
// code/third_party/opents/LICENSE.md. Calibrated to YR 0x004AAE90.
#include "yrpp/DisplayClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/WaypointPathClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/Drawing.h"
#include "yrpp/Unsorted.h"

namespace {
WORD pack(BYTE r,BYTE g,BYTE b) {
    return WORD(((r>>Drawing::RedShiftRight)<<Drawing::RedShiftLeft)
        |((g>>Drawing::GreenShiftRight)<<Drawing::GreenShiftLeft)
        |((b>>Drawing::BlueShiftRight)<<Drawing::BlueShiftLeft));
}
TechnoClass* techno(ObjectClass* object) {
    return object && (object->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None
        ? static_cast<TechnoClass*>(object):nullptr;
}
}

#if !defined(RA2_YRPP_GAME)
bool DisplayClass::ConvertAction(const CellStruct& cell,bool shrouded,ObjectClass* object,Action action,bool mini) {
    unknown_bool_11D0=false;
    AbstractClass* target=object;
    if(object) {
        // 0x0040DD70 uses WhatAmI(), unlike the flag check for selection below.
        TechnoClass* unit=nullptr;
        switch(object->WhatAmI()) {
            case AbstractType::Aircraft:case AbstractType::Building:
            case AbstractType::Infantry:case AbstractType::Unit:
                unit=static_cast<TechnoClass*>(object);break;
            default:break;
        }
        if(unit && (unit->WhatAmI()!=AbstractType::Building
            || !static_cast<BuildingClass*>(unit)->Type->InvisibleInGame) && !Unsorted::ArmageddonMode)
            unit->IsMouseHovering=true;
    } else if(cell!=CellStruct{0,0})target=MapClass::Instance.GetCellAt(cell);

    auto& map=MapClass::Instance;
    auto* house=HouseClass::CurrentPlayer;
    // The original query accepts a mutable pointer but never changes the cell.
    auto* where=const_cast<CellStruct*>(&cell);
    if(PlanningMode && DraggedWaypoint && !house->GetPlanningWaypointAt(where)
        && map.IsWithinUsableArea(cell,true)) {
        auto& at=DraggedWaypoint->Coords;
        at={int(cell.X)*256+128,int(cell.Y)*256+128,0};
        const int bridge=(unsigned(map.GetCellAt(cell)->Flags)&0x100u)?CellClass::BridgeHeight:0;
        at.Z=map.GetCellFloorHeight(at)+bridge;
    }
    if(house->GetPlanningWaypointAt(where)) {
        switch(action) {
            case Action::Move:case Action::NoMove:action=Action::FollowWaypoint;break;
            case Action::Enter:case Action::Capture:case Action::Repair:action=Action::EnterWaypoint;break;
            case Action::Attack:case Action::Harvest:action=Action::AttackWaypoint;break;
            default:break;
        }
    }
    // Native/headless hosts may dispatch input before cursor art is installed.
    // Cursor conversion is unavailable until the real palette exists; do not
    // manufacture palette storage or report a successful device conversion.
    if(!FileSystem::MOUSE_PAL || !FileSystem::MOUSE_PAL->PaletteData)return false;
    auto* palette=static_cast<WORD*>(FileSystem::MOUSE_PAL->PaletteData);
    if(!WaypointColorRed && !WaypointColorGreen && !WaypointColorBlue) {
        const WORD color=palette[1];
        WaypointColorRed=BYTE((color>>Drawing::RedShiftLeft)<<Drawing::RedShiftRight);
        WaypointColorGreen=BYTE((color>>Drawing::GreenShiftLeft)<<Drawing::GreenShiftRight);
        WaypointColorBlue=BYTE((color>>Drawing::BlueShiftLeft)<<Drawing::BlueShiftRight);
    }
    int path=-1;
    switch(action) {
        case Action::PlaceWaypoint:case Action::TibSunBug:case Action::LoopWaypointPath:
            path=house->SelectedPathIndex;break;
        case Action::EnterWaypointMode:case Action::FollowWaypoint:case Action::SelectWaypoint:
        case Action::AttackWaypoint:case Action::EnterWaypoint:case Action::PatrolWaypoint: {
            BYTE index=0;
            house->GetPlanningWaypointProperties(house->GetPlanningWaypointAt(where),path,index);
            if(!PlanningMode)house->SelectedPathIndex=path;
            break;
        }
        default:
            palette[1]=pack(WaypointColorRed,WaypointColorGreen,WaypointColorBlue);
            if(!PlanningMode)house->SelectedPathIndex=-1;
            break;
    }
    const int base=path==-1?0:8*(path%12);
    // YR overwrites entries 1..8 even in the default case (including the
    // restored entry 1 above); this apparently redundant write is original.
    for(int i=0;i<8;++i) {
        const auto& color=FileSystem::WAYPOINT_PAL.Entries[(base+i)%256];
        palette[i+1]=pack(color.R,color.G,color.B);
    }

    // Original 0x0070F0B0 remaps the waypoint-adjusted action, querying the
    // attack-move mode for every action, even if no remapping is needed.
    const bool attack_move=Game::IsAttackMoveMode();
    if(attack_move && action==Action::Move)action=Action::AttackMoveNav;
    else if(attack_move && action==Action::Attack)action=Action::AttackMoveTar;
    using Cursor=MouseCursorType;
    auto cursor=Cursor::Default;
    if(shrouded) {
        switch(action) {
            case Action::Move:case Action::Attack:cursor=Cursor::Move;break;
            case Action::NoMove: {
                auto* selected=ObjectClass::CurrentObjects.Count?techno(ObjectClass::CurrentObjects[0]):nullptr;
                cursor=selected && selected->GetTechnoType()->MoveToShroud?Cursor::Move:Cursor::NoMove;
                break;
            }
            case Action::Eaten:case Action::NoRepair:case Action::NoGRepair:cursor=Cursor::NoRepair;break;
            case Action::Sell:case Action::SellUnit:case Action::NoSell:cursor=Cursor::NoSell;break;
            case Action::Tote:case Action::Heal:case Action::TogglePower:case Action::NoTogglePower:
            case Action::PlaceWaypoint:case Action::TibSunBug:case Action::EnterWaypointMode:
            case Action::FollowWaypoint:case Action::SelectWaypoint:case Action::LoopWaypointPath:
            case Action::AttackWaypoint:case Action::PatrolWaypoint:cursor=Cursor::Disallowed;break;
            case Action::Nuke:cursor=Cursor::Nuke;break;
            case Action::GuardArea:cursor=Cursor::Protect;break; // 0x00731CC0
            case Action::NoDeploy:cursor=Cursor::NoDeploy;break;
            case Action::NoEnter:case Action::NoEnterTunnel:cursor=Cursor::NoEnter;break;
            case Action::IronCurtain:cursor=Cursor::IronCurtain;break;
            case Action::LightningStorm:cursor=Cursor::LightningStorm;break;
            case Action::ChronoSphere:case Action::ChronoWarp:cursor=Cursor::Chronosphere;break;
            case Action::ParaDrop:case Action::AmerParaDrop:cursor=Cursor::ParaDrop;break;
            case Action::EnterWaypoint:
                SetCursor(Cursor::Disallowed,mini); // Original fall-through makes two virtual calls.
                [[fallthrough]];
            case Action::PlaceBeacon:cursor=Cursor::Beacon;break;
            case Action::DisarmBomb:cursor=Cursor::Disarm;break;
            case Action::SelectBeacon:cursor=Cursor::Select;break;
            case Action::AttackMoveNav:case Action::AttackMoveTar:cursor=Cursor::AttackOutOfRange2;break;
            case Action::PsychicDominator:cursor=Cursor::PsychicDominator;break;
            case Action::SpyPlane:cursor=Cursor::SpyPlane;break;
            case Action::GeneticConverter:cursor=Cursor::GeneticMutator;break;
            case Action::ForceShield:cursor=Cursor::ForceShield;break;
            case Action::NoForceShield:cursor=Cursor::NoForceShield;break;
            case Action::PsychicReveal:cursor=Cursor::PsychicReveal;break;
            default:break;
        }
    } else {
        switch(action) {
            case Action::Move:cursor=Cursor::Move;break;
            case Action::NoMove:case Action::NoIvanBomb:cursor=Cursor::NoMove;break;
            case Action::Enter:case Action::Capture:case Action::Repair:case Action::EnterTunnel:cursor=Cursor::Enter;break;
            case Action::Self_Deploy:case Action::AreaAttack:cursor=Cursor::Deploy;break;
            case Action::Attack: {
                auto* selected=target && ObjectClass::CurrentObjects.Count==1?techno(ObjectClass::CurrentObjects[0]):nullptr;
                cursor=selected && selected->IsCloseEnoughToAttack(target)?Cursor::Attack:Cursor::AttackOutOfRange;
                break;
            }
            case Action::Harvest:cursor=Cursor::AttackOutOfRange;break;
            case Action::Select:case Action::ToggleSelect:case Action::SelectBeacon:cursor=Cursor::Select;break;
            case Action::Eaten:cursor=Cursor::EngineerRepair;break;
            case Action::Sell:cursor=Cursor::Sell;break;
            case Action::SellUnit:cursor=Cursor::SellUnit;break;
            case Action::NoSell:cursor=Cursor::NoSell;break;
            case Action::NoRepair:case Action::NoGRepair:cursor=Cursor::NoRepair;break;
            case Action::Sabotage:case Action::Detonate:case Action::DetonateAll:case Action::Demolish:cursor=Cursor::Demolish;break;
            case Action::Tote:case Action::Heal:case Action::TogglePower:case Action::NoTogglePower:
            case Action::PlaceWaypoint:case Action::TibSunBug:case Action::EnterWaypointMode:case Action::FollowWaypoint:
            case Action::SelectWaypoint:case Action::LoopWaypointPath:case Action::AttackWaypoint:
            case Action::EnterWaypoint:case Action::PatrolWaypoint:cursor=Cursor::Disallowed;break;
            case Action::Nuke:cursor=Cursor::Nuke;break;
            case Action::GuardArea:cursor=Cursor::Protect;break;
            case Action::Damage:return true; // Original returns 27 without changing the cursor.
            case Action::GRepair:cursor=Cursor::Repair;break;
            case Action::NoDeploy:cursor=Cursor::NoDeploy;break;
            case Action::NoEnter:cursor=Cursor::NoEnter;break;
            case Action::IronCurtain:cursor=Cursor::IronCurtain;break;
            case Action::LightningStorm:cursor=Cursor::LightningStorm;break;
            case Action::ChronoSphere:case Action::ChronoWarp:cursor=Cursor::Chronosphere;break;
            case Action::ParaDrop:case Action::AmerParaDrop:cursor=Cursor::ParaDrop;break;
            case Action::IvanBomb:cursor=Cursor::IvanBomb;break;
            case Action::DisarmBomb:cursor=Cursor::Disarm;break;
            case Action::PlaceBeacon:cursor=Cursor::Beacon;break;
            case Action::AttackMoveNav:case Action::AttackMoveTar:cursor=Cursor::AttackOutOfRange2;break;
            case Action::PsychicDominator:cursor=Cursor::PsychicDominator;break;
            case Action::SpyPlane:cursor=Cursor::SpyPlane;break;
            case Action::GeneticConverter:cursor=Cursor::GeneticMutator;break;
            case Action::ForceShield:cursor=Cursor::ForceShield;break;
            case Action::NoForceShield:cursor=Cursor::NoForceShield;break;
            case Action::Airstrike:cursor=Cursor::AirStrike;break;
            case Action::PsychicReveal:cursor=Cursor::PsychicReveal;break;
            default:break;
        }
    }
    return SetCursor(cursor,mini);
}
#endif
