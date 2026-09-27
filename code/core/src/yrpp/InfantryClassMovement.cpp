// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp Assign_Destination/Stop_Driver/On_Movement_Blocked/
// JumpJet_To_Walk. YR 0x51AA40/0x51DAF0/0x521DD0/0x521EB0, including COM branches
// omitted by the target decompiler's mistaken no-return exception helper.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/InfantryClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/RadSiteClass.h"
#include "yrpp/BuildingClass.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <bit>

// OpenTS Enter_Idle_Mode, YR 0x51CBA0. Execute the existing order; this does
// not select a destination or add a faction-level commander.
bool InfantryClass::EnterIdleMode(bool initial,bool resume) {
    const bool result=FootClass::EnterIdleMode(initial,resume);
    if(HaveMegaMission() || GetCurrentMission()==Mission::Wait)return result;
    Mission order;
    if(Target) {
        order=Mission::Attack;
        if(GetCurrentMission()==Mission::Sabotage)order=Mission::Sabotage;
        if(GetCurrentMission()==Mission::Capture)order=Mission::Capture;
    } else {
        HandleNavigationList();
        if(Destination) {
            order=Mission::Move;
            if(GetCurrentMission()==Mission::Capture)order=Mission::Capture;
            if(GetCurrentMission()==Mission::Sabotage)order=Mission::Sabotage;
        } else {
            if(GetCurrentMission()==Mission::Guard || GetCurrentMission()==Mission::Area_Guard
                || (GetCurrentMission()!=Mission::None && (CurrentMissionControl()->Zombie || CurrentMissionControl()->Paralyzed)))return false;
            // 0x50B730 is human/player-control state, NOT pointer == CurrentPlayer.
            if(Owner->IsControlledByHuman() || Team) {
                order=CurrentMission!=Mission::Area_Guard
                    && ((!HasAbility(Ability::GuardArea) && !Type->DefaultToGuardArea) || Team)?Mission::Guard:Mission::Area_Guard;
            } else if(Owner->IQLevel2>=RulesClass::Instance->GuardArea || CurrentMission==Mission::Area_Guard) {
                order=SlaveOwner || (!IsArmed() && !Type->Engineer && !Type->VehicleThief)?Mission::Guard:Mission::Area_Guard;
            } else order=Mission::Guard;
        }
    }
    if(CurrentMission!=Mission::Patrol && CurrentMission!=Mission::Area_Guard)QueueMission(order,false);
    return result;
}

int InfantryClass::GetCurrentSpeed() const {
    const int speed=FootClass::GetCurrentSpeed();
    // 0x521DA4..0x521DB9 subtracts trunc(speed/3), NOT trunc(2*speed/3).
    // For example, a GI with speed 10 crawls at 7 rather than 6 leptons.
    return Crawling?(Type->Crawls?speed-speed/3:speed+speed/2):speed;
}

// OpenTS Do_MISSION_ATTACK / Approach_Target, with YR's deployed and
// garrison branches (0x51F3E0 / 0x522340). Target scanning and Foot's
// firing-position search remain original dependencies, never host movement.
int InfantryClass::Mission_Attack() {
    auto* building=Target && Target->WhatAmI()==AbstractType::Building?static_cast<BuildingClass*>(Target):nullptr;
    if((Type->C4 || HasAbility(Ability::C4)) && building && building->Type->CanC4 && !building->Type->InvisibleInGame) {
        SetDestination(Target,true);QueueMission(Mission::Sabotage,false);return 1;
    }
    if(!Owner->IsControlledByHuman() && building
        && (Type->Infiltrate || ((Type->Occupier || Type->Assaulter) && building->CanBeOccupiedBy(this)))) {
        SetDestination(Target,true);ForceMission(Mission::Capture);return 1;
    }
    if(!Owner->IsControlledByHuman() || SequenceAnim<Sequence::Deploy || SequenceAnim>Sequence::DeployedIdle)
        return FootClass::Mission_Attack();
    vt_entry_428();
    const double rate=CurrentMissionControl()->Rate*900.0;
    const int ticks=std::isfinite(rate) && rate>=-2147483648.0 && rate<2147483648.0?int(rate):INT32_MIN;
    return std::bit_cast<int>(static_cast<unsigned>(ticks)+static_cast<unsigned>(ScenarioClass::Instance->Random.RandomRanged(0,2)));
}
AbstractClass* InfantryClass::ApproachTarget(DWORD queryOnly) {
    if(!Target)return nullptr;
    const bool deployed=SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle;
    if(!deployed)return FootClass::ApproachTarget(0); // YR deliberately ignores queryOnly here.
    if(GetOwningHouse()->IsControlledByHuman() || !Type->Deployer || !Type->DeployFire)return nullptr;
    const bool inRange=IsCloseEnough(Target,Type->DeployFireWeapon);
    auto* weapon=GetWeapon(Type->DeployFireWeapon)->WeaponType;
    if(!weapon || weapon->AreaFire)return nullptr;
    const bool nowDeployed=SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle;
    if(nowDeployed) {
        if(!inRange && !static_cast<unsigned char>(queryOnly))PlayAnim(Sequence::Undeploy);
    } else if(inRange && !static_cast<unsigned char>(queryOnly)) {
        if(!Locomotor)std::abort();
        if(!Locomotor->Is_Moving())PlayAnim(Sequence::Deploy);
        else {
            Locomotor->Stop_Moving();ShouldDeploy=true;
            CurrentMissionControl();ScenarioClass::Instance->Random.RandomRanged(0,2);
        }
    }
    return nullptr;
}

int InfantryClass::Mission_Move() {
    if(SequenceAnim<Sequence::Deploy || SequenceAnim>Sequence::DeployedIdle)return FootClass::Mission_Move();
    if(GetOwningHouse()->IsControlledByHuman()){SetDestination(nullptr,true);return 1;}
    if(Type->UndeployDelay>-1)return FootClass::Mission_Move();
    PlayAnim(Sequence::Undeploy,false,false);
    return Type->Sequence->GetSequence(Sequence::Undeploy).CountFrames;
}

// YR adds shared deployed-guard handling to the OpenTS Guard/AreaGuard
// overrides. Keep its effect calls explicit; do not inherit base idle delays.
int InfantryClass::Mission_Guard() {
    const int result=Guard_Deploy_AI();return result==-1?FootClass::Mission_Guard():result;
}
int InfantryClass::Mission_Unload() {
    // OpenTS Foot's Do_MISSION_UNLOAD base; YR's added infantry override 0x51F6E0.
    if(!Type->Deployer)return FootClass::Mission_Unload();
    int delay=-1;
    if(SequenceAnim>=Sequence::Deploy&&SequenceAnim<=Sequence::DeployedIdle){
        if(Type->UndeployDelay<=-1)PlayAnim(Sequence::Undeploy,true,false);
    }else{
        PlayAnim(Sequence::Deploy,true,false);
        auto* weapon=GetDeployWeapon();
        if(weapon&&weapon->WeaponType&&weapon->WeaponType->AreaFire){
            if(!_strcmpi(Type->ID,"DESO"))delay=Type->Sequence->GetSequence(Sequence::Deploy).CountFrames+1;
            else SetTarget(MapClass::Instance.GetCellAt(GetMapCoords()));
        }
        if(Type->UndeployDelay>-1)delay=Type->UndeployDelay;
    }
    ForceMission(Mission::Guard);SetDestination(nullptr,true);
    return delay>-1?delay:FootClass::Mission_Unload();
}
void InfantryClass::vt_entry_428() {
    const int weapon=SelectWeapon(nullptr);
    if(Target&&IsCloseEnough(Target,weapon))return;
    auto origin=Location;
    auto* target=GreatestThreat(static_cast<ThreatType>(1),&origin,false);
    if(Target||target)SetTarget(target);
    if(!target&&CurrentMission!=Mission::Guard&&!Type->JumpJet)EnterIdleMode(false,true);
}
int InfantryClass::Mission_AreaGuard() {
    const int result=Guard_Deploy_AI();return result==-1?FootClass::Mission_AreaGuard():result;
}
int InfantryClass::Guard_Deploy_AI() {
    const auto cellOf=[](const CoordStruct& at){return CellStruct{short(at.X/256),short(at.Y/256)};};
    const auto add=[](int a,int b){return std::bit_cast<int>(static_cast<unsigned>(a)+static_cast<unsigned>(b));};
    const auto delay=[&](int minimum,int maximum) {
        const double value=CurrentMissionControl()->Rate*900.0;
        const int ticks=std::isfinite(value) && value>=-2147483648.0 && value<2147483648.0?int(value):INT32_MIN;
        return std::bit_cast<int>(static_cast<unsigned>(ticks)+static_cast<unsigned>(ScenarioClass::Instance->Random.RandomRanged(minimum,maximum)));
    };
    if(SequenceAnim<Sequence::Deploy || SequenceAnim>Sequence::DeployedIdle) {
        if(!GetOwningHouse()->IsControlledByHuman() && Type->Deployer && Type->DeployFire && Type->UndeployDelay<=-1
            && !Destination && add(CurrentMissionStartTime,RulesClass::Instance->AIAutoDeployFrameDelay[int(Owner->AIDifficulty)])<Unsorted::CurrentFrame
            && (!ArchiveTarget || cellOf(GetCoords())==cellOf(ArchiveTarget->GetCoords())) && !Type->ImmuneToRadiation) {
            if(!Locomotor)std::abort();
            if(Locomotor->Is_Moving()){Locomotor->Stop_Moving();ShouldDeploy=true;return delay(0,2);}
            PlayAnim(Sequence::Deploy);return Type->Sequence->GetSequence(Sequence::Deploy).CountFrames;
        }
        return -1;
    }
    if(Type->UndeployDelay>-1){PlayAnim(Sequence::Undeploy);return Type->Sequence->GetSequence(Sequence::Undeploy).CountFrames;}
    if(!Type->DeployFire)return -1;
    if(!Type->ImmuneToRadiation){vt_entry_428();return delay(0,2);}
    auto* site=MapClass::Instance.GetCellAt(GetMapCoords())->GetRadSite();
    if(!site || site->GetRadLevel()<GetWeapon(1)->WeaponType->RadLevel/3) {
        SetTarget(MapClass::Instance.GetCellAt(GetMapCoords()));
        if(GetFireError(Target,1,true)==FireError::OK) {
            Fire(Target,1);SetTarget(nullptr);PlayAnim(Sequence::DeployedFire);
            return Type->Sequence->GetSequence(Sequence::DeployedFire).CountFrames;
        }
        SetTarget(nullptr);
    }
    return delay(10,20);
}

// OpenTS Fear_AI; YR 0x5200B0 removes dog/tiberium behavior and protects
// deployment sequences. This is posture execution, not a faction commander.
void InfantryClass::Fear_AI() {
    const auto deployed=[&]{return SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle;};
    const auto moving=[&]{if(!Locomotor)std::abort();return Locomotor->Is_Moving();};
    if(std::bit_cast<int>(PanicDurationLeft)>0) {
        if(!Type->Fearless)--PanicDurationLeft;
        if(!PanicDurationLeft && !Ammo && IsArmed())Ammo=Type->Ammo;
        if(Crawling) {
            if(std::bit_cast<int>(PanicDurationLeft)<50 && !deployed())PlayAnim(Sequence::Up);
        } else if(std::bit_cast<int>(PanicDurationLeft)>=50 && !deployed()
            && (!Owner->IsControlledByHuman() || (!Destination && !moving())) && !Type->Fraidycat)
            PlayAnim(Sequence::Down);
    }
    if(Type->Fraidycat && std::bit_cast<int>(PanicDurationLeft)>50 && !deployed()
        && !IsFallingDown && !moving() && !Destination)Scatter(CoordStruct::Empty,true,false);
}

namespace {
constexpr GUID persist_iid{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};
constexpr GUID piggy_iid{0x92FEA800,0xA184,0x11D1,{0xB7,0x0A,0,0xA0,0x24,0xDD,0xAF,0xD1}};
template<class T> struct Held {
    T* pointer=nullptr;
    ~Held(){if(pointer)pointer->Release();}
    void reset(T* value=nullptr){if(pointer)pointer->Release();pointer=value;}
};
void check(HRESULT result){if(result<0 && result!=static_cast<HRESULT>(0x80004002u))std::abort();}
bool is_class(ILocomotion* driver,const CLSID& clsid){
    if(!driver)std::abort();Held<IPersist> persist;check(driver->QueryInterface(persist_iid,reinterpret_cast<void**>(&persist.pointer)));
    if(!persist.pointer)std::abort();CLSID actual{};persist.pointer->GetClassID(&actual);
    return !std::memcmp(&actual,&clsid,sizeof(actual));
}
void query_piggy(ILocomotion* driver,Held<IPiggyback>& piggy){
    piggy.reset();if(driver)check(driver->QueryInterface(piggy_iid,reinterpret_cast<void**>(&piggy.pointer)));
}
void assign_driver(FootClass& foot,ILocomotion* driver){
#if defined(_MSC_VER)
    foot.Locomotor=driver;
#else
    if(driver)driver->AddRef();auto* old=foot.Locomotor;foot.Locomotor=driver;if(old)old->Release();
#endif
}
void end_piggy(FootClass& foot,IPiggyback* piggy){
    if(piggy && piggy->Is_Piggybacking() && piggy->Is_Ok_To_End()){
        assign_driver(foot,nullptr);ILocomotion* restored=nullptr;piggy->End_Piggyback(&restored);
#if defined(_MSC_VER)
        foot.Locomotor.Attach(restored);
#else
        foot.Locomotor=restored;
#endif
    }
}
void create_walk(FootClass& foot,Held<ILocomotion>& walk){
    check(LocomotionClass::CreateInstance(&walk.pointer,&LocomotionClass::CLSIDs::Walk,nullptr,7));
    if(!walk.pointer)std::abort();walk.pointer->Link_To_Object(&foot);
}
CellStruct cell_of(const CoordStruct& coord){return {short(coord.X/256),short(coord.Y/256)};}
}

// OpenTS Firing_AI; calibrated to YR 0x5206B0 and its post-COM assembly.
// Fire/GetFireError/ScatterContent remain real effect dependencies, not no-ops.
BulletClass* InfantryClass::Fire(AbstractClass* target,int index) {
    // YR 0x51DF60, after the inherited projectile/effect execution.
    IsFiring=false;
    auto* bullet=TechnoClass::Fire(target,index);
    if(bullet && !InLimbo && Type->Fraidycat && !Ammo) {
        PanicDurationLeft=300;
        if(GetCurrentMission()==Mission::Attack || GetCurrentMission()==Mission::Hunt)QueueMission(Mission::Guard,false);
    }
    return bullet;
}

void InfantryClass::Firing_AI() {
    if(!Target){IsFiring=false;return;}
    const int weapon=SelectWeapon(Target);
    if(!IsFiring) {
        const auto error=GetFireError(Target,weapon,true);
        if(error==FireError::ILLEGAL) {
            if(CombatDamage(weapon)<0) {
                auto* target=Target && (Target->AbstractFlags & ::AbstractFlags::Object)!=::AbstractFlags::None
                    ?static_cast<ObjectClass*>(Target):nullptr;
                if(!target || target->WhatAmI()!=AbstractType::Infantry
                    || target->GetHealthPercentage()>=RulesClass::Instance->ConditionGreen)SetTarget(nullptr);
            }
        } else if(error==FireError::CLOAKED)Uncloak(false);
        else if(error==FireError::OK) {
            if(Type->JumpJet && is_class(Locomotor,LocomotionClass::CLSIDs::Jumpjet))PlayAnim(Sequence::FireFly);
            else if(SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle)PlayAnim(Sequence::DeployedFire);
            else if(Airstrike && (SequenceAnim==Sequence::SecondaryFire || SequenceAnim==Sequence::SecondaryProne))Animation.Value=0;
            else if(weapon && Crawling && Type->Sequence->GetSequence(Sequence::SecondaryProne).CountFrames)
                PlayAnim(Sequence::SecondaryProne);
            else if(weapon && Type->Sequence->GetSequence(Sequence::SecondaryFire).CountFrames)
                PlayAnim(Sequence::SecondaryFire);
            else PlayAnim(Crawling?Sequence::FireProne:Sequence::FireUp);
            IsFiring=true;DirStruct direction;PrimaryFacing.SetDesired(*GetDirectionTo(&direction,Target));
            if(Target==Destination){AbortMotion();StopMoving();}
        }
    }
    int fireStage=Crawling?Type->FireProne:Type->FireUp;
    if(weapon) {
        if(Crawling) {
            if(Type->Sequence->GetSequence(Sequence::SecondaryProne).CountFrames)fireStage=Type->SecondaryProne;
        } else if(Type->Sequence->GetSequence(Sequence::SecondaryFire).CountFrames)fireStage=Type->SecondaryFire;
    }
    if(IsFiring && Animation.Value==fireStage) {
        const int currentWeapon=SelectWeapon(Target);
        if(GetFireError(Target,currentWeapon,true)!=FireError::OK) {
            IsFiring=false;
            if(Crawling)PlayAnim(Sequence::Prone);
            else PlayAnim(SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle?Sequence::Deployed:Sequence::Ready);
        } else Fire(Target,currentWeapon);
        if(Target && GetWeapon(0)->WeaponType->Speed<RulesClass::Instance->Incoming) {
            const auto from=Location;
            MapClass::Instance.GetCellAt(Target->GetCoords())->ScatterContent(from,true,false,false);
        }
    }
}

// OpenTS Movement_AI; target 0x520F40. This chooses movement posture and
// validates the existing order. It does not autonomously choose a destination.
void InfantryClass::Movement_AI() {
    const auto moving=[&]{if(!Locomotor)std::abort();return Locomotor->Is_Moving();};
    if(GetCurrentMission()==Mission::Move && !moving()) {
        if(Destination){SetDestination(Destination,true);SetSpeedPercentage(1.0);}
        else EnterIdleMode(false,true);
    }
    if(Destination) {
        const auto target=cell_of(Destination->GetDestination(this));
        if((!unknown_bool_6DC || IsCellOccupied(GetCellAgain(),FacingType::None,-1,nullptr,true)!=Move::No)
            && !moving() && !IsTether && Destination && IsInPlayfield) {
            const auto from=cell_of(GetDestination());
            const bool leave=vt_entry_320();
            const bool bridge=(static_cast<unsigned>(MapClass::Instance.GetCellAt(target)->Flags)&0x100u)!=0;
            const bool fromBridge=IsOnBridge(nullptr);
            if(!MapClass::IsSameCellZone(from,target,Type->MovementZone,fromBridge,bridge,leave)
                && !Type->Infiltrate && GetCurrentMission()!=Mission::Enter)SetDestination(nullptr,true);
        }
    }
    if(!Locomotor)std::abort();
    if(Locomotor->Is_Really_Moving_Now()) {
        if(Type->JumpJet && is_class(Locomotor,LocomotionClass::CLSIDs::Jumpjet)) {
            if(!IsFiring)PlayAnim(SpeedPercentage>0.8?Sequence::Fly:Sequence::Hover);
        } else PlayAnim(Crawling?Sequence::Crawl:Sequence::Walk);
    } else {
        switch(SequenceAnim) {
            case Sequence::Walk:case Sequence::Fly:case Sequence::Hover:PlayAnim(Sequence::Ready);break;
            case Sequence::Crawl:PlayAnim(Sequence::Prone);break;
            case Sequence::Swim:PlayAnim(Sequence::Tread);break;
            default:break;
        }
    }
}

void InfantryClass::SetDestination(AbstractClass* destination,bool immediate){
    if(GetOwningHouse()->IsControlledByHuman() && SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle)return;
    if(Type->JumpJet && destination && cell_of(GetCoords())==cell_of(destination->GetCoords()))return;
    if(DirectRockerLinkedUnit){DirectRockerLinkedUnit->DirectRockerLinkedUnit=nullptr;DirectRockerLinkedUnit=nullptr;}
    if(!Locomotor)std::abort();
    if(Locomotor->Is_Moving() && destination && MapClass::Instance.GetCellAt(GetCoords())->IsClearToMove(Type->SpeedType,true,false,-1,MovementZone::Normal,-1,true)){
        if(Type->JumpJet || CurrentMission!=Mission::Attack || Destination!=destination)StopMoving();
    }
    if(Owner->IsControlledByHuman() && destination && Destination==destination && Crawling && !Type->Fraidycat && !Type->Cyborg)PlayAnim(Sequence::Up);
    if((GetCurrentMission()==Mission::Enter || QueuedMission==Mission::Enter) && !HasAnyLink()){
        auto* techno=destination && (destination->AbstractFlags & ::AbstractFlags::Techno)!=::AbstractFlags::None?static_cast<TechnoClass*>(destination):nullptr;
        if(techno){
            if(techno->HasAnyLink()){
                const auto kind=techno->WhatAmI();
                if(kind==AbstractType::Unit || kind==AbstractType::Building){QueueUpToEnter=techno;SetArchiveTarget(nullptr);destination=nullptr;}
                else SetArchiveTarget(destination);
            }else if(SendCommand(RadioCommand::RequestLink,techno)==RadioCommand::AnswerPositive){
                const auto reply=SendToFirstLink(RadioCommand::RequestLoading);
                if(reply==RadioCommand::AnswerLoading)return;
                if(reply!=RadioCommand::AnswerPositive)SendToFirstLink(RadioCommand::NotifyUnlink);
            }
        }
    }else PathDirections[0]=-1;
    if(destination && Type->JumpJet && Locomotor->Is_Moving() && is_class(Locomotor,LocomotionClass::CLSIDs::Walk)){
        if(NavQueue.AddItem(destination)){
            for(int i=NavQueue.Count-1;i>0;--i)NavQueue[i]=NavQueue[i-1];NavQueue[0]=destination;
        }
        auto* cell=GetCellAgain();destination=cell;
        if(cell && (static_cast<unsigned>(cell->Flags)&0x100u))destination=nullptr;
    }
    if(Type->JumpJet && destination && !Locomotor->Is_Moving()){
        const bool fly=ShouldJumpJetFly(cell_of(GetDestination()),cell_of(destination->GetCoords())) || Type->BalloonHover;
        const bool jumpjet=is_class(Locomotor,LocomotionClass::CLSIDs::Jumpjet);
        if(jumpjet && !fly){
            Held<IPiggyback> old;query_piggy(Locomotor,old);end_piggy(*this,old.pointer);
            Held<ILocomotion> walk;create_walk(*this,walk);Held<IPiggyback> piggy;query_piggy(walk.pointer,piggy);
            if(piggy.pointer){piggy.pointer->Begin_Piggyback(Locomotor);assign_driver(*this,walk.pointer);}
        }else if(!jumpjet && fly){Held<IPiggyback> piggy;query_piggy(Locomotor,piggy);end_piggy(*this,piggy.pointer);}
    }
    FootClass::SetDestination(destination,immediate);
}
bool InfantryClass::StopMoving(){
    if(SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle)PlayAnim(Sequence::Deployed);
    else PlayAnim(Crawling?Sequence::Prone:Sequence::Ready);
    const auto facing=static_cast<FacingType>(PrimaryFacing.Current().GetFacing<8>());
    unknown_bool_6DC=IsCellOccupied(MapClass::Instance.GetCellAt(Location),facing,GetCellLevel(),nullptr,true)!=Move::OK;
    return FootClass::StopMoving();
}
void InfantryClass::vt_entry_4F4(){
    FootClass::vt_entry_4F4();
    if(Destination && DistanceFrom(Destination)<1024 && Type->JumpJet)is_class(Locomotor,LocomotionClass::CLSIDs::Jumpjet);
}
bool InfantryClass::vt_entry_4F8(){
    if(Destination){int count=0;while(count<24 && PathDirections[count]!=-1 && PathDirections[count]!=8)++count;if(count>=4)return false;}
    if(!Type->JumpJet || !is_class(Locomotor,LocomotionClass::CLSIDs::Jumpjet))return false;
    Held<IPiggyback> current;query_piggy(Locomotor,current);
    if(!current.pointer || current.pointer->Is_Piggybacking())return false;
    Held<ILocomotion> walk;create_walk(*this,walk);Held<IPiggyback> piggy;query_piggy(walk.pointer,piggy);
    if(!piggy.pointer)return false;
    PathDirections[0]=-1;piggy.pointer->Begin_Piggyback(Locomotor);assign_driver(*this,walk.pointer);
    Locomotor->Move_To(Destination->GetCoords());return true;
}
