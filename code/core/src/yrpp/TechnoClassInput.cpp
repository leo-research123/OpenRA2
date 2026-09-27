// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::What_Action / Can_Player_Move / Can_Player_Fire.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md. YR 0x700600/0x700C40/0x7010D0.
#include "yrpp/TechnoClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/BombClass.h"
#include "scenario_runtime.hpp"
#include <cstdlib>

Action TechnoClass::MouseOverObject(const ObjectClass* object,bool ignoreForce) const {
    if(Berzerk || (DirectRockerLinkedUnit&&!GetTechnoType()->Pushy) || !object)return Action::None;
    auto* target=const_cast<ObjectClass*>(object);
    const bool ours=Owner->IsControlledByCurrentPlayer();
    const auto selectable=[&](bool checkWarp) {
        if((!IsALoaner||!DiscoveredByCurrentPlayer)&&target->CanBeSelected()&&!target->IsSelected
            && (!checkWarp||!target->IsBeingWarpedOut()))return Action::Select;
        return Action::None;
    };
    if(IsBeingWarpedOut())return (!IsArmed()||!ours||target->GetOwningHouseIndex()==GetOwningHouseIndex())?selectable(false):Action::None;
    if(target==this&&CurrentObjects.Count==1&&AttachedBomb&&AttachedBomb->OwnerHouse->IsControlledByCurrentPlayer()) {
        if(AttachedBomb->IsDeathBomb()?RulesClass::Instance->CanDetonateDeathBomb:RulesClass::Instance->CanDetonateTimeBomb)return Action::Detonate;
    }
    const auto* input=InputManagerClass::Instance;
    const bool move=!ignoreForce&&input&&input->IsForceMoveKeyPressed();
    bool fire=!ignoreForce&&input&&input->IsForceFireKeyPressed();
    const bool select=!ignoreForce&&input&&input->IsForceSelectKeyPressed();
    if(select){if(fire)fire=false;else if(ours&&!IsALoaner)return Action::ToggleSelect;}
    if(target==this&&CurrentObjects.Count==1) {
        if(ours)return Action::Self_Deploy;
        const auto& runtime=game::scenario_runtime();
        if(Owner->IsInPlayerControl&&runtime.session_mode(runtime.context)==int(GameMode::Campaign)) {
            if(WhatAmI()==AbstractType::Unit&&GetTechnoType()->Passengers>0&&Passengers.NumPassengers>0)return Action::Self_Deploy;
            if(WhatAmI()==AbstractType::Aircraft&&static_cast<const AircraftClass*>(this)->HasPassengers)return Action::Self_Deploy;
        }
    }
    if(ours&&Owner->IsAlliedWith(target)&&WhatAmI()!=AbstractType::Aircraft&&fire&&move&&IsControllable())return Action::GuardArea;
    if(move&&ours&&IsControllable())return Action::Move;
    const auto* type=GetTechnoType();const int index=SelectWeapon(target);
    if(target->IsSurfaced()) {
        const auto* weapon=GetWeapon(index)->WeaponType;
        auto* techno=(target->AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None?static_cast<TechnoClass*>(target):nullptr;
        const bool reveal=RulesClass::Instance->AttackCursorOnDisguise || (techno&&(techno->GetTechnoType()->PermaDisguise||techno->DisguiseBlinkTimer.GetTimeLeft()));
        const bool disguised=target->IsDisguisedAs(Owner)&&!reveal;
        const bool terrain=CanDisguiseAs(target);
        const bool hiddenTerrain=disguised&&!target->GetDisguiseHouse(true)&&terrain;
        const bool clearsCargo=weapon&&weapon->Warhead->IvanBomb&&target->GetTechnoType()&&target->GetTechnoType()->Passengers>0;
        const bool parasite=weapon&&weapon->Warhead->Sonic&&(target->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None&&static_cast<FootClass*>(target)->ParasiteEatingMe;
        const bool friendly=type->AttackFriendlies||(type->AttackCursorOnFriendlies&&!clearsCargo)||parasite;
        if(ours&&(fire||((!Owner->IsAlliedWith(target)||friendly)&&(!disguised||hiddenTerrain||disguised&&friendly)
                &&(target->GetType()->LegalTarget||terrain)))) {
            const bool infiltrator=WhatAmI()==AbstractType::Infantry&&static_cast<const InfantryClass*>(this)->Type->Infiltrate;
            if((IsArmed()||infiltrator)&&(IsControllable()||IsCloseEnough(target,index))) {
                bool special=false;
                if(infiltrator&&target->WhatAmI()==AbstractType::Building) {
                    const auto* inf=static_cast<const InfantryClass*>(this)->Type;
                    const auto* building=static_cast<BuildingClass*>(target)->Type;
                    special=(!inf->Agent&&building->Capturable)||(inf->Agent&&building->Spyable)||(inf->C4&&building->CanC4);
                }
                if(GetFireError(target,index,true)!=FireError::ILLEGAL||special||fire)return Action::Attack;
            }
        }
    }
    return (!IsArmed()||!ours||target->GetOwningHouseIndex()==GetOwningHouseIndex())?selectable(true):Action::None;
}

bool TechnoClass::CanBeSelectedNow() const {
    if(SlaveOwner || Deactivated || BunkerLinkedItem)return false;
    if(IsTether && GetCell()->GetBuilding())return false;
    return ObjectClass::CanBeSelectedNow();
}
bool TechnoClass::IsActive() const {
    if(Deactivated || !Owner->IsControlledByCurrentPlayer())return false;
    auto* weapon=GetTurretWeapon();
    return weapon && weapon->WeaponType && !IsUnderEMP();
}
bool TechnoClass::IsControllable() const {
    if(!Owner->IsControlledByCurrentPlayer() || IsUnderEMP() || BunkerLinkedItem
        || GetTechnoType()->Spawned || IsParalyzed() || !IsNotWarping() || SlaveOwner || Deactivated)return false;
    if(SpawnManager && SpawnManager->CountLaunchingSpawns()>0
        && SpawnManager->CountLaunchingSpawns()<GetTechnoType()->SpawnsNumber)return false;
    return true;
}
bool TechnoClass::CanDisguiseAs(AbstractClass* target) const {
    // Despite the legacy name, 0x70EF00 means an attackable terrain target.
    auto* weapon=GetWeapon(SelectWeapon(target))->WeaponType;
    if(!weapon)return false;
    if(target->WhatAmI()==AbstractType::Terrain)return RulesClass::Instance->TreeTargeting || weapon->TerrainFire;
    if(!weapon->TerrainFire)return false;
    if(target->WhatAmI()==AbstractType::Cell) {
        const int index=static_cast<CellClass*>(target)->OverlayTypeIndex;
        if(index!=-1 && OverlayTypeClass::Array[index]->IsARock)return true;
    }
    if((target->AbstractFlags & ::AbstractFlags::Object)!=::AbstractFlags::None) {
        auto* object=static_cast<ObjectClass*>(target);
        return object->IsDisguised() && !object->GetDisguise(true);
    }
    return false;
}
Action TechnoClass::MouseOverCell(const CellStruct* where,bool checkFog,bool ignoreForce) const {
    if(Berzerk || (DirectRockerLinkedUnit && !GetTechnoType()->Pushy))return Action::None;
    auto& map=MapClass::Instance;auto* cell=map.GetCellAt(*where);
    auto* input=InputManagerClass::Instance;
    bool fire=!ignoreForce && input && input->IsForceFireKeyPressed();
    bool select=!ignoreForce && input && input->IsForceSelectKeyPressed();
    const bool move=!ignoreForce && input && input->IsForceMoveKeyPressed();
    if(select && fire){select=false;fire=false;}
    if(IsBeingWarpedOut() || IsWarpingIn())return Action::None;
    // Deprecated FoggedObjectClass is retained as a header-only placeholder;
    // base-game maps do not enable memory fog, so this branch is not implemented.
    if(ScenarioClass::Instance->SpecialFlags.FogOfWar && checkFog && cell->FoggedObjects
        && cell->FoggedObjects->Count)std::abort();
    auto* overlay=cell->OverlayTypeIndex!=-1?OverlayTypeClass::Array[cell->OverlayTypeIndex]:nullptr;
    const bool engineer=IsEngineer();
    if(Owner->IsControlledByCurrentPlayer() && ((fire && move) || unknown_bool_43A)
        && IsControllable() && (IsActive() || engineer)) {
        auto location=*where;
        return HouseClass::CurrentPlayer->GetPlanningWaypointAt(&location) || unknown_bool_43A
            ?Action::PatrolWaypoint:Action::GuardArea;
    }
    if(Owner->IsControlledByCurrentPlayer())if(auto* equipped=GetTurretWeapon();equipped && equipped->WeaponType) {
        const int weapon=SelectWeapon(cell);
        bool cliff=cell->Tile_Is_DestroyableCliff();
        if(cliff && GetTurretWeapon()->WeaponType->Warhead->Sonic)cliff=false;
        if(fire || (overlay && overlay->LegalTarget) || cliff) {
            auto* warhead=GetTurretWeapon()->WeaponType->Warhead;
            const bool enemy=cell->WallOwnerIndex!=-1 && !HouseClass::Array[cell->WallOwnerIndex]->IsAlliedWith(Owner);
            // VeinholeMonster has no native class yet. The native registry
            // currently has none; refuse if such an object is introduced.
            for(auto* object:AbstractClass::Array)if(object->WhatAmI()==AbstractType::VeinholeMonster)std::abort();
            if(fire || !overlay || (enemy && overlay->Wall
                && (warhead->Wall || (warhead->Wood && overlay->Armor==Armor::Wood)))) {
                if(IsControllable() || IsCloseEnough3D({int(where->X)*256+128,int(where->Y)*256+128,0},weapon))return Action::Attack;
            }
        }
        if(CanDisguiseAs(cell)
            && (IsControllable() || IsCloseEnough3D({int(where->X)*256+128,int(where->Y)*256+128,0},weapon)))return Action::Attack;
    }
    if(!Owner->IsControlledByCurrentPlayer() || (!IsControllable() && !IsUnitFactory()))return Action::None;
    if(!map.IsWithinUsableArea(*where,true))return Action::NoMove;
    if(select || (IsUnitFactory() && move))return Action::Move;
    if(!IsControllable())return Action::NoMove;
    if(checkFog || static_cast<int>(IsCellOccupied(cell,FacingType::None,-1,nullptr,true))<=1)return Action::Move;
    if(WhatAmI()==AbstractType::Building && GetTechnoType()->UndeploysInto && GetTechnoType()->UndeploysInto->ResourceGatherer)return Action::Move;
    if(GetTechnoType()->IsSubterranean && static_cast<int>(IsCellOccupied(cell,FacingType::None,-1,nullptr,false))<=1)return Action::Move;
    return Action::NoMove;
}
