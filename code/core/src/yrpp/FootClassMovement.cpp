// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp Mark/Set_Coord/Is_In_Same_Zone/Current_Speed.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR 0x4D3710, 0x4D3780, 0x4D3810, 0x4DB1A0, 0x4DB810.
#include "yrpp/FootClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/WaypointPathClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/TubeClass.h"
#include <bit>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include "RulesClassReaders.hpp"

// OpenTS FootClass::Destination_Coord; YR Infantry vtable + 0x4C ->
// 0x4DBDF0. Path planning starts at the reserved head, even mid-step.
CoordStruct* FootClass::GetDestination(CoordStruct* output,TechnoClass*) const {
    if(TubeIndex>=0) {
        if(TubeIndex>=TubeClass::Array.Count || !TubeClass::Array[TubeIndex])std::abort();
        const auto exit=TubeClass::Array[TubeIndex]->ExitCell;
        *output={int(exit.X)*256+128,int(exit.Y)*256+128,0};
    }else {
        if(!Locomotor)std::abort();
        const auto head=Locomotor->Head_To_Coord();
        *output=head==CoordStruct::Empty?GetCoords():head;
    }
    return output;
}

// OpenTS Override/Restore_Mission; YR 0x4D8F40 / 0x4D8F80 also preserve the
// navigation destination when a temporary attack interrupts movement.
void FootClass::Override_Mission(Mission mission,AbstractClass* target,AbstractClass* destination){
 LastDestination=Destination;TechnoClass::Override_Mission(mission,target,destination);SetDestination(destination,true);
}
bool FootClass::Mission_Revert(){
 if(!TechnoClass::Mission_Revert())return false;
 SetDestination(LastDestination,true);return true;
}

// OpenTS Captured; YR 0x4DBED0.
bool FootClass::SetOwningHouse(HouseClass* house,bool) {
 if(GetTechnoType()->SensorsSight)RemoveSensorsAt(GetMapCoords());
 const bool changed=TechnoClass::SetOwningHouse(house,true);
 if(changed){
  if(Owner->IsHumanPlayer&&Team)Team->LiberateMember(this,-1,0);
  if(!IsInAir())LastMapCoords=GetMapCoords();
 }
 if(GetTechnoType()->SensorsSight)AddSensorsAt(GetMapCoords());
 return changed;
}

// OpenTS Mission_Capture; YR 0x4D4B20.
int FootClass::Mission_Capture() {
 auto* infantry=WhatAmI()==AbstractType::Infantry?static_cast<InfantryClass*>(this):nullptr;
 if(Target&&Target->WhatAmI()==AbstractType::Building&&!Destination&&infantry){
  const auto* type=infantry->Type;
  if(type->C4||HasAbility(Ability::C4)||type->Engineer||type->Occupier||type->Assaulter||type->Agent)SetDestination(Target,true);
 }
 if(!Destination){EnterIdleMode(false,true);if(GetCell()->GetBuilding())Scatter(CoordStruct::Empty,true,false);}
 if(!Target&&!Owner->IsControlledByHuman()&&(!infantry||!infantry->Type->Occupier)){
  SetTarget(nullptr);SetDestination(nullptr,true);QueueMission(Mission::Hunt,false);
 }
 return rule_integer(CurrentMissionControl()->Rate*900.0)+ScenarioClass::Instance->Random.RandomRanged(0,2);
}

// OpenTS Mission_Guard_Area; YR 0x4D6AA0.
int FootClass::Mission_Enter(){
 auto* transport=GetNthLink();
 if(!transport&&ArchiveTarget&&(ArchiveTarget->AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None)transport=static_cast<TechnoClass*>(ArchiveTarget);
 if(transport){
  if(SendCommand(RadioCommand::RequestLoading,transport)==RadioCommand::AnswerPositive||IsTether){
   if(!Destination&&unknown_abstract_array_588.Count>0){
    IPiggyback* piggy=nullptr;
    constexpr GUID iid{0x92FEA800,0xA184,0x11D1,{0xB7,0x0A,0,0xA0,0x24,0xDD,0xAF,0xD1}};
    if(Locomotor){const auto status=Locomotor->QueryInterface(iid,reinterpret_cast<void**>(&piggy));if(status<0&&status!=static_cast<HRESULT>(0x80004002u))std::abort();}
    if(piggy&&piggy->Is_Ok_To_End()){
#if defined(_MSC_VER)
     Locomotor=nullptr;ILocomotion* restored=nullptr;piggy->End_Piggyback(&restored);Locomotor.Attach(restored);
#else
     if(Locomotor)Locomotor->Release();Locomotor=nullptr;piggy->End_Piggyback(&Locomotor);
#endif
    }
    if(unknown_abstract_array_588.Count){SetDestination(unknown_abstract_array_588[0],false);unknown_abstract_array_588.RemoveItem(0);}
    if(piggy)piggy->Release();
   }else if(GetTechnoType()->Teleporter){auto* next=Destination;unknown_5A0=nullptr;Destination=nullptr;SetDestination(next,true);}
  }else{SendToFirstLink(RadioCommand::NotifyUnlink);EnterIdleMode(false,true);}
 }else{
  if(!ApproachEnterQueue()&&(!Destination||(Destination->WhatAmI()!=AbstractType::Unit&&Destination->WhatAmI()!=AbstractType::Aircraft)))EnterIdleMode(false,true);
  NextMission();
 }
 return rule_integer(CurrentMissionControl()->Rate*900.0)+ScenarioClass::Instance->Random.RandomRanged(0,2);
}

int FootClass::Mission_AreaGuard() {
    const auto delay=[&]{return rule_integer(CurrentMissionControl()->Rate*900.0);};
    if(BunkerLinkedItem){QueueMission(Mission::Guard,true);return 1;}
    if(ShouldEnterAbsorber){EnterGrinder();return delay();}
    if(ShouldEnterOccupiable){EnterTankBunker();return delay();}
    if(ShouldGarrisonStructure){EnterBattleBunker();return delay();}
    if(!Owner->IsControlledByHuman()&&NavQueue.Count&&!Destination&&NavQueue[0]==ArchiveTarget&&NavQueue[0]) {
        auto* next=NavQueue[0];SetDestination(next,true);NavQueue.RemoveItem(0);if(unknown_bool_6B1)NavQueue.AddItem(next);
    }
    const bool harvester=WhatAmI()==AbstractType::Unit&&static_cast<UnitClass*>(this)->Type->Harvester;
    if(ArchiveTarget&&ArchiveTarget->WhatAmI()==AbstractType::Cell) {
        auto* building=MapClass::Instance.GetCellAt(ArchiveTarget->GetCoords())->GetBuilding();
        if(building&&Owner->IsAlliedWith(building->Owner)&&!Owner->IsControlledByHuman()&&!harvester) {
            const auto at=GetMapCoords();
            const auto zone=MapClass::Instance.GetMovementZoneType(at,MovementZone::Normal,false);
            auto next=MapClass::Instance.NearByLocation(CellClass::Coord2Cell(ArchiveTarget->GetCoords()),SpeedType::Foot,zone,
                MovementZone::Normal,false,1,1,false,false,false,true,CellStruct::Empty,false,false);
            if(next!=CellStruct::Empty)SetArchiveTarget(MapClass::Instance.GetCellAt(next));
        }
    }
    if(harvester){QueueMission(Mission::Harvest,false);NextMission();return ScenarioClass::Instance->Random.RandomRanged(1,10)+1;}
    if(SlaveManager)SlaveManager->BeginScanning();
    if(!ArchiveTarget&&QueuedMission==Mission::None)SetArchiveTarget(MapClass::Instance.GetCellAt(Location));
    if(!Owner->IsControlledByHuman()&&WhatAmI()==AbstractType::Infantry
        &&(static_cast<InfantryClass*>(this)->Type->C4||HasAbility(Ability::C4))
        &&GetCurrentMission()!=Mission::Sabotage&&Target&&Target->WhatAmI()==AbstractType::Building){QueueMission(Mission::Sabotage,false);return 1;}
    int radius=rule_integer(GetGuardRange(1)*1.1);
    if(ArchiveTarget) {
        if((ArchiveTarget->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None)radius=RulesClass::Instance->GuardModeStray;
        const auto own=GetCoords(),anchor=ArchiveTarget->GetCoords();
        const double x=double(own.X)-anchor.X,y=double(own.Y)-anchor.Y,z=double(own.Z)-anchor.Z;
        if(!IsFiring&&!Destination&&int(std::sqrt(x*x+y*y+z*z))>radius){SetTarget(nullptr);SetDestination(ArchiveTarget,true);}
        if(Target)ApproachTarget(0);
        else {
            if(CanPassiveAcquireTargets()&&TargetingTimerFinished()){auto origin=ArchiveTarget->GetCoords();TargetAndEstimateDamage(origin,ThreatType(2));}
            if(Target)return 1;
            UpdateIdleAction();
        }
    }
    if(!Target)if(auto* secondary=GetWeapon(1)->WeaponType;secondary&&secondary->Warhead->ElectricAssault) {
        for(int i=0;i<8;++i){auto at=GetMapCoords(),offset=Unsorted::AdjacentCell[i];
            auto* building=MapClass::Instance.GetCellAt(CellStruct{short(at.X+offset.X),short(at.Y+offset.Y)})->GetBuilding();
            if(building&&building->Type->Overpowerable&&building->GetOwningHouse()==GetOwningHouse()) {
                SetTarget(building);unknown_bool_68E=true;QueueMission(Mission::Attack,false);break;
            }
        }
    }
    int result=delay();if(WhatAmI()==AbstractType::Aircraft)result*=2;
    result+=ScenarioClass::Instance->Random.RandomRanged(1,5);
    auto* primary=GetWeapon(0)->WeaponType;
    if(Target&&((WhatAmI()==AbstractType::Infantry&&GetTechnoType()->CloseRange)||(primary&&primary->Range<=512))) {
        const auto own=GetCoords(),target=Target->GetCoords();const double x=double(own.X)-target.X,y=double(own.Y)-target.Y;
        const int distance=int(std::sqrt(x*x+y*y));if(distance<=768&&double(distance)>=281.6)result/=6;
    }
    return result;
}

// OpenTS Enter_Idle_Mode / Handle_Navigation_List, calibrated to YR.
Mission FootClass::RespondMegaEventMission(EventClass* event) {
    const auto mission=static_cast<Mission>(static_cast<signed char>(event->MegaMission.Mission));
    if(mission!=Mission::AttackMove){ClearMegaMissionData();return mission;}
    const bool target=event->MegaMission.Target.m_RTTI!=0;
    if(!CanAttackOnTheMove())return target?Mission::Attack:Mission::Move;
    HaveAttackMoveTarget=false;MegaMission=Mission::AttackMove;
    if(event->MegaMission.Destination.m_RTTI){MegaDestination=event->MegaMission.Destination.As_Abstract();return Mission::Move;}
    if(target){MegaTarget=event->MegaMission.Target.As_Abstract();return Mission::Attack;}
    ClearMegaMissionData();return mission;
}
bool FootClass::EnterIdleMode(bool initial,bool resume) {
    if(unknown_bool_6B3)return false;
    unknown_bool_6B3=true;
    TechnoClass::EnterIdleMode(initial,resume);
    if(ShouldScatterInNextIdle){ShouldScatterInNextIdle=false;Scatter(CoordStruct::Empty,true,false);}
    struct HeldPiggy {
        IPiggyback* pointer=nullptr;
        ~HeldPiggy(){if(pointer)pointer->Release();}
    } piggy;
    bool ended=false;
    if(Locomotor) {
        constexpr GUID iid{0x92FEA800,0xA184,0x11D1,{0xB7,0x0A,0,0xA0,0x24,0xDD,0xAF,0xD1}};
        const HRESULT result=Locomotor->QueryInterface(iid,reinterpret_cast<void**>(&piggy.pointer));
        if(result<0 && result!=static_cast<HRESULT>(0x80004002u))std::abort();
        // This entrance does NOT call Is_Piggybacking before Is_Ok_To_End.
        if(piggy.pointer && piggy.pointer->Is_Ok_To_End()) {
#if defined(_MSC_VER)
            Locomotor=nullptr;
#else
            if(Locomotor)Locomotor->Release();Locomotor=nullptr;
#endif
            ILocomotion* restored=nullptr;
            piggy.pointer->End_Piggyback(&restored);
#if defined(_MSC_VER)
            Locomotor.Attach(restored);
#else
            Locomotor=restored;
#endif
            ended=true;
        }
    }
    if(unknown_abstract_array_588.Count>0) {
        SetDestination(unknown_abstract_array_588[0],false);
        // SetDestination may change the vector: remove from its new state.
        unknown_abstract_array_588.RemoveItem(0);
        return true;
    }
    if((CurrentMission!=Mission::Patrol || MissionStatus==0) && PlanningPathIdx!=-1 && resume) {
        auto* path=HouseClass::CurrentPlayer->PlanningPaths[PlanningPathIdx];
        FollowWaypoint(path->GetWaypointAfter(path->GetWaypoint(WaypointIndex)));
    }
    if(ended)return true;
    if(WhatAmI()==AbstractType::Infantry && ArchiveTarget && !SlaveOwner) {
        const auto own=GetCoords(), archived=ArchiveTarget->GetCoords();
        if(short(own.X/256)!=short(archived.X/256) || short(own.Y/256)!=short(archived.Y/256)) {
            auto* destination=ArchiveTarget;
            if(CurrentMission!=Mission::Area_Guard){QueueMission(Mission::Move,false);SetArchiveTarget(nullptr);}
            SetDestination(destination,true);
        }
    }
    SetSpeedPercentage(0.0);
    return false;
}

void FootClass::HandleNavigationList() {
    if(!Destination && NavQueue.Count>0)if(auto* next=NavQueue[0]) {
        SetDestination(next,true);
        NavQueue.RemoveItem(0);
        if(unknown_bool_6B1)NavQueue.AddItem(next);
    }
}
void FootClass::UpdateAttackMove() {
    // YR 0x4DF3A0: range checks precede acquisition; direct target field
    // assignments here intentionally do not call SetTarget.
    if(MegaDestination) {
        if(HaveAttackMoveTarget) {
            if(InAuxiliarySearchRange(Target))return;
            Target=nullptr;
        }
        auto at=Location;
        if(TargetAndEstimateDamage(at,static_cast<ThreatType>(1))) {
            QueueMission(Mission::Attack,true);HaveAttackMoveTarget=true;
        }
    }else if(!MegaTarget || !InAuxiliarySearchRange(MegaTarget)) {
        if(HaveAttackMoveTarget && InAuxiliarySearchRange(Target))return;
        Target=nullptr;
        auto at=Location;
        if(TargetAndEstimateDamage(at,static_cast<ThreatType>(1))) {
            QueueMission(Mission::Attack,true);HaveAttackMoveTarget=true;
        }else Target=MegaTarget;
    }
}
bool FootClass::RefreshMegaMission() {
    if(MegaMission!=Mission::AttackMove)return false;
    if(HaveAttackMoveTarget)HaveAttackMoveTarget=false;
    else if(GetCurrentMission()!=Mission::Guard && GetCurrentMission()!=Mission::Guard) {
        ClearMegaMissionData();return false;
    }
    return ContinueMegaMission();
}
bool FootClass::ContinueMegaMission() {
    HaveAttackMoveTarget=false;
    if(MegaDestination){QueueMission(Mission::Move,true);SetDestination(MegaDestination,true);return true;}
    if(MegaTarget){QueueMission(Mission::Attack,true);SetTarget(MegaTarget);return true;}
    ClearMegaMissionData();return false;
}

// OpenTS Do_MISSION_ATTACK; YR 0x4D4DC0 also handles hover takeoff,
// requested reacquisition and the shorter close-range mission interval.
int FootClass::Mission_Attack() {
    if(GetTechnoType()->HoverAttack && !GetHeight()) {
        CellStruct where;NearbyLocation(&where,this);
        SetDestination(MapClass::Instance.GetCellAt(where),true);
    }
    if(unknown_bool_68E) {
        auto origin=Location;
        if(auto* acquired=GreatestThreat(static_cast<ThreatType>(1),&origin,false)) {
            SetTarget(acquired);unknown_bool_68E=false;
        }
    }
    if(Target)ApproachTarget(0);
    else EnterIdleMode(false,true);
    const double rate=CurrentMissionControl()->Rate*900.0;
    const int ticks=std::isfinite(rate) && rate>=-2147483648.0 && rate<2147483648.0?int(rate):INT32_MIN;
    const int delay=std::bit_cast<int>(static_cast<unsigned>(ticks)+static_cast<unsigned>(ScenarioClass::Instance->Random.RandomRanged(0,2)));
    if(!Target)return delay;
    if(!(WhatAmI()==AbstractType::Infantry && GetTechnoType()->CloseRange)
        && (!GetWeapon(0)->WeaponType || GetWeapon(0)->WeaponType->Range>512))return delay;
    const auto from=GetCoords(),to=Target->GetCoords();
    const double x=double(from.X)-to.X,y=double(from.Y)-to.Y;
    const int distance=static_cast<int>(std::sqrt(x*x+y*y));
    return distance<=768 && double(distance)>=281.6?delay/2:delay;
}

int FootClass::Mission_Move() {
    if(!Destination) {
        if(!Locomotor)std::abort();
        if(!Locomotor->Is_Moving() && QueuedMission==Mission::None){EnterIdleMode(false,true);return 1;}
    }
    // OpenTS Do_MISSION_MOVE has target scanning here; YR 0x4D4200 does NOT.
    const double rate=CurrentMissionControl()->Rate*900.0;
    const int delay=std::isfinite(rate) && rate>=-2147483648.0 && rate<2147483648.0?int(rate):INT32_MIN;
    return std::bit_cast<int>(static_cast<unsigned>(delay)
        +static_cast<unsigned>(ScenarioClass::Instance->Random.RandomRanged(0,2)));
}

// OpenTS Mission_Guard, calibrated to YR 0x4D5070. This is unit mission
// execution, not a commander issuing orders to a faction.
int FootClass::Mission_Guard() {
    const auto delay=[&] {
        const double value=CurrentMissionControl()->Rate*900.0;
        return std::isfinite(value) && value>=-2147483648.0 && value<2147483648.0?int(value):INT32_MIN;
    };
    if(ShouldEnterAbsorber){EnterGrinder();return delay();}
    if(ShouldEnterOccupiable){EnterTankBunker();return delay();}
    if(ShouldGarrisonStructure){EnterBattleBunker();return delay();}
    if(Target) {
        if(GetTechnoType()->HoverAttack && GetHeight()==0) {
            CellStruct cell;NearbyLocation(&cell,this);SetDestination(MapClass::Instance.GetCellAt(cell),true);
        }
    }else {
        BuildingClass* receiver=nullptr;
        auto* secondary=GetWeapon(1)->WeaponType;
        if(secondary && secondary->Warhead->ElectricAssault)for(int i=0;i<8;++i) {
            const auto cell=GetMapCoords(),offset=Unsorted::AdjacentCell[i];
            auto* building=MapClass::Instance.GetCellAt(CellStruct{short(cell.X+offset.X),short(cell.Y+offset.Y)})->GetBuilding();
            if(building && building->Type->Overpowerable && building->GetOwningHouse()==GetOwningHouse()){receiver=building;break;}
        }
        if(receiver){SetTarget(receiver);unknown_bool_68E=true;QueueMission(Mission::Attack,false);}
        else UpdateIdleAction();
    }
    const int result=delay();
    if(WhatAmI()==AbstractType::Infantry && !Owner->IsControlledByHuman()
        && (static_cast<InfantryClass*>(this)->Type->C4 || HasAbility(Ability::C4))
        && GetCurrentMission()!=Mission::Sabotage && Target && Target->WhatAmI()==AbstractType::Building)
        QueueMission(Mission::Sabotage,false);
    int remaining=RearmTimer.TimeLeft;
    if(RearmTimer.StartTime!=-1) {
        const int elapsed=std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame)-static_cast<unsigned>(RearmTimer.StartTime));
        remaining=elapsed>=remaining?0:std::bit_cast<int>(static_cast<unsigned>(remaining)-static_cast<unsigned>(elapsed));
    }
    if(remaining)return remaining;
    if(GetTechnoType()->DistributedFire && CurrentTargets.Count>0)return 0;
    return std::bit_cast<int>(static_cast<unsigned>(result)+static_cast<unsigned>(ScenarioClass::Instance->Random.RandomRanged(0,2)));
}

void FootClass::SetSpeedPercentage(double percentage) {
    // Preserve the target's unordered comparison branch (NaN becomes 1).
    SpeedPercentage=percentage<1.0?(percentage>0.0?percentage:0.0):1.0;
}
int FootClass::GetCurrentSpeed() const {
    // YR 0x50C050 uses the type-specific house multipliers, not GroundspeedMult.
    const auto kind=GetTechnoType()->WhatAmI();
    const double bias=kind==AbstractType::AircraftType?Owner->Type->SpeedAircraftMult
        :kind==AbstractType::UnitType?Owner->Type->SpeedUnitsMult
        :kind==AbstractType::InfantryType?Owner->Type->SpeedInfantryMult:1.0;
    int speed=int(GetDefaultSpeed()*bias*SpeedMultiplier);
    if(HasAbility(Ability::Faster))speed=int(speed*RulesClass::Instance->VeteranSpeed);
    speed=int(speed*SpeedPercentage);
    if(WhatAmI()==AbstractType::Unit && static_cast<const UnitClass*>(this)->FlagHouseIndex!=-1)speed/=2;
    return speed;
}
Layer FootClass::InWhichLayer() const { return Locomotor->In_Which_Layer(); }
bool FootClass::IsOnBridge(TechnoClass* docker) const {
    if(TubeIndex>=0)return false;
    const auto destination=GetDestination(docker);
    auto& map=MapClass::Instance;
    const int to=map.GetCellFloorHeight(destination),from=map.GetCellFloorHeight(Location);
    if(!OnBridge && from-to>3*Unsorted::LevelHeight
        && (static_cast<unsigned>(map.GetCellAt(destination)->Flags)&0x100u))return true;
    return OnBridge && to-from>3*Unsorted::LevelHeight?false:OnBridge;
}
bool FootClass::IsInSameZoneAsCoords(const CoordStruct& coord) {
    const auto movement=GetTechnoType()->MovementZone;
    if(movement==MovementZone::None)return true;
    const CellStruct to{short(coord.X/256),short(coord.Y/256)};
    if(to==CellStruct::Empty)return false;
    const auto at=GetDestination();
    const CellStruct from{short(at.X/256),short(at.Y/256)};
    return MapClass::IsSameCellZone(from,to,movement,IsOnBridge(),
        (static_cast<unsigned>(MapClass::Instance.GetCellAt(to)->Flags)&0x100u)!=0,vt_entry_320());
}
bool FootClass::IsInSameZoneAs(AbstractClass* target) {
    const auto at=target->GetDestination(this);
    const CoordStruct center{int(short(at.X/256))*256+128,int(short(at.Y/256))*256+128,0};
    return IsInSameZoneAsCoords(center);
}
bool FootClass::Mark(MarkType mark) {
    if(mark==MarkType::Change)return true;
    if(!TechnoClass::Mark(mark))return false;
    if(InWhichLayer()==Layer::Ground){
        auto cell=GetMapCoords();
        if(mark==MarkType::Up)MapClass::Instance.RemoveContentAt(&cell,this);
        else if(mark==MarkType::Down || mark==MarkType::ChangeRedraw)MapClass::Instance.AddContentAt(&cell,this);
    }
    return true;
}
void FootClass::SetLocation(const CoordStruct& coord) {
    const bool changed=Location!=coord,down=IsOnMap;
    if(down)Mark(MarkType::Up);
    ObjectClass::SetLocation(coord);
    if(down)Mark(MarkType::Down);
    if(changed && GetTechnoType()->OpenTopped)UpdatePassengerCoords();
}
void FootClass::SetDestination(AbstractClass* destination,bool) {
    unknown_5A0=nullptr;
    if(destination && (IsAttackedByLocomotor || InOpenToppedTransport || BunkerLinkedItem))return;
    // Release reciprocal magnetron links in the target's order.
    if(destination && LocomotorTarget)ReleaseLocomotor(true);
    Destination=destination;
    if(!destination && IsAttackedByLocomotor && LocomotorSource){
        LocomotorSource->LocomotorTarget=nullptr;LocomotorSource=nullptr;IsLetGoByLocomotor=true;
    }
    if(Destination){
        if(FireParticleSystem){FireParticleSystem->UnInit();FireParticleSystem=nullptr;}
        if(!Locomotor)std::abort();
        // 0x4D9563..0x4D966D: real COM identity, held query reference and
        // the one-shot skip at Foot + 0x6AC. The docker argument is this.
        constexpr GUID persist_iid{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};
        IPersist* persist=nullptr;
        if(Locomotor->QueryInterface(persist_iid,reinterpret_cast<void**>(&persist))<0 || !persist)std::abort();
        CLSID clsid{};
        if(persist->GetClassID(&clsid)<0){persist->Release();std::abort();}
        if(!std::memcmp(&clsid,&LocomotionClass::CLSIDs::Hover,sizeof(clsid)) && !PathDelayTimer.GetTimeLeft())PathDelayTimer.Start(1);
        if(unknown_bool_6AC)unknown_bool_6AC=false;
        else Locomotor->Move_To(Destination->GetDestination(this));
        persist->Release();
    }else if(WhatAmI()!=AbstractType::Aircraft || (CurrentMission!=Mission::Attack && QueuedMission!=Mission::Attack) || !Target){
        if(!Locomotor)std::abort();Locomotor->Stop_Moving();Destination=destination;
    }
    IsWaitingBlockagePath=false;
    BlockagePathTimer.Start(RulesClass::Instance->BlockagePathDelay);PathDelayTimer.Start(0);
}
bool FootClass::StopMoving(){if(!Locomotor)std::abort();Locomotor->Stop_Moving();return false;}
void FootClass::vt_entry_4F4(){if(GetCurrentMission()==Mission::Hunt){SetTarget(nullptr);SetDestination(nullptr,true);}}
