// Native display lifecycle, original 0x006F2B40; combat services are not started.
#include "yrpp/TechnoClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BombListClass.h"
namespace {DynamicVectorClass<TechnoClass*> objects;bool action_lines=true;CDTimerClass action_line_timer;}
bool& TechnoClass::ActionLines=action_lines;
CDTimerClass& TechnoClass::ActionLineTimer=action_line_timer;
// OpenTS Techno.Unlimbo, YR 0x6F6CA0. Pair ownership/threat/facing with
// Limbo when an original object (including a pouncing dog) re-enters the map.
bool TechnoClass::Unlimbo(const CoordStruct& where,DirType facing){
 if(!ObjectClass::Unlimbo(where,facing))return false;
 IsInPlayfield=MapClass::Instance.IsWithinUsableArea(CellClass::Coord2Cell(where),true);
 if(!IsAlive)return true;
 UpdateSight(0,0,0,0,0);
 auto at=Location;MapClass::Instance.RevealArea3(&at,0,LastSightRange+3,false);
 auto* type=GetTechnoType();
 if(type->BombSight)BombListClass::Instance.AddDetector(this);
 Owner->RegisterGain(this,false);
 PrimaryFacing.SetCurrent(DirStruct(short(unsigned(facing)<<8)));
 BarrelFacing.SetCurrent(DirStruct(short(0x4000)));
 BarrelFacing.SetDesired(DirStruct(short(0x4000-(type->FireAngle&0xFF)*256)));
 IsTurretRotateSoundPlaying=true;TurretIsRotating=false;
 UnlimboingInfantry=WhatAmI()==AbstractType::Infantry;EnterIdleMode(true,false);UnlimboingInfantry=false;
 if(ReadyToNextMission())NextMission();
 SightIncrease=char(10u*unsigned(where.Z/RulesClass::Instance->LeptonsPerSightIncrease));
 if(!IsInPlayfield)DiscoveredByCurrentPlayer=false;
 AddThreatToCell(GetCell());
 RadarClass::Instance.GetCrdOnRadar(&RadarPosition,&Location,true);
 return true;
}
bool TechnoClass::Limbo() {
 if(!InLimbo && GetType()) {
  auto* type=GetTechnoType();
  vt_entry_48C(false,0,type->RevealToAll,type->RevealToAll?HouseClass::CurrentPlayer:nullptr);
  auto at=Location;MapClass::Instance.RevealArea3(&at,LastSightRange-3,LastSightRange+3,false);
  if(type->GapGenerator)DestroyGap();
  if(type->BombSight)BombListClass::Instance.RemoveDetector(this);
  if(type->OpenTopped)for(auto* passenger=Passengers.FirstPassenger;passenger;) {
   passenger->SetTarget(nullptr);
   auto* next=passenger->NextObject;
   passenger=next && (next->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None?static_cast<FootClass*>(next):nullptr;
  }
  Owner->RegisterLoss(this,false);
  if(GetThreatValue()>0) {
   auto* cell=(AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None
       ?MapClass::Instance.GetCellAt(static_cast<FootClass*>(this)->LastMapCoords):GetCell();
   if(cell){cell->UpdateThreat(GetOwningHouseIndex(),-int(ThreatPosed));ThreatPosed=0;}
  }
  if(IsRadarTracked)RadarTrackingStop();
  TurretRotateSoundController.EndLooping();IsTurretRotateSoundPlaying=true;TurretIsRotating=false;
  GattlingSoundController.EndLooping();IsGattlingSoundPlaying=false;unknown_4BC=0;
  UnusedGattlingSoundController.EndLooping();IsUnusedGattlingSoundPlaying=false;unknown_4D8=0;
 }
 return RadioClass::Limbo();
}
void TechnoClass::Stun() {
 SetTarget(nullptr);SetDestination(nullptr,true);SendToEachLink(RadioCommand::NotifyUnlink);
 if((AbstractFlags & ::AbstractFlags::Foot)==::AbstractFlags::None || !static_cast<FootClass*>(this)->IsAttackedByLocomotor) {
  if(SpawnManager){SpawnManager->KillNodes();SpawnManager->ResetTarget();}
  Disappear(true);
 }
 Deselect();
}
DynamicVectorClass<TechnoClass*>& TechnoClass::Array=objects;
void TechnoClass::Undiscover(){if(DiscoveredByComputer&&!Owner->IsControlledByHuman())DiscoveredByComputer=false;}
int TechnoClass::GetOwningHouseIndex() const { return Owner ? Owner->ArrayIndex : -1; }
HouseClass* TechnoClass::GetOwningHouse() const { return Owner; }
TechnoClass::TechnoClass(HouseClass*pOwner) noexcept
 : RadioClass(),
 Flashing{},
 Animation{},
 Passengers{},
 Transporter{},
 LastFireBulletFrame{},
 CurrentTurretNumber{},
 unknown_int_128{},
 BehindAnim{},
 DeployAnim{},
 InAir{},
 CurrentWeaponNumber{},
 CurrentRanking{},
 CurrentGattlingStage{},
 GattlingValue{},
 TurretAnimFrame{},
 InitialOwner{},
 Veterancy{},
 align_154{},
 ArmorMultiplier{},
 FirepowerMultiplier{},
 IdleActionTimer{},
 RadarFlashTimer{},
 TargetingTimer{},
 IronCurtainTimer{},
 IronTintTimer{},
 IronTintStage{},
 AirstrikeTimer{},
 AirstrikeTintTimer{},
 AirstrikeTintStage{},
 ForceShielded{},
 Deactivated{},
 DrainTarget{},
 DrainingMe{},
 DrainAnim{},
 Disguised{},
 DisguiseCreationFrame{},
 InfantryBlinkTimer{},
 DisguiseBlinkTimer{},
 UnlimboingInfantry{},
 ReloadTimer{},
 RadarPosition{},
 DisplayProductionTo{},
 Group{},
 ArchiveTarget{},
 Owner{},
 CloakState{},
 CloakProgress{},
 CloakDelayTimer{},
 WarpFactor{},
 unknown_bool_250{},
 LastSightCoords{},
 LastSightRange{},
 LastSightHeight{},
 GapSuperCharged{},
 GeneratingGap{},
 GapRadius{},
 BeingWarpedOut{},
 WarpingOut{},
 unknown_bool_272{},
 unused_273{},
 TemporalImUsing{},
 TemporalTargetingMe{},
 IsImmobilized{},
 unknown_280{},
 ChronoLockRemaining{},
 ChronoDestCoords{},
 Airstrike{},
 Berzerk{},
 BerzerkDurationLeft{},
 SprayOffsetIndex{},
 Uncrushable{},
 DirectRockerLinkedUnit{},
 LocomotorTarget{},
 LocomotorSource{},
 Target{},
 LastTarget{},
 CaptureManager{},
 MindControlledBy{},
 MindControlledByAUnit{},
 MindControlRingAnim{},
 MindControlledByHouse{},
 SpawnManager{},
 SpawnOwner{},
 SlaveManager{},
 SlaveOwner{},
 OriginallyOwnedByHouse{},
 BunkerLinkedItem{},
 PitchAngle{},
 RearmTimer{},
 ChargeTurretDelay{},
 Ammo{},
 Value{},
 FireParticleSystem{},
 SparkParticleSystem{},
 NaturalParticleSystem{},
 DamageParticleSystem{},
 RailgunParticleSystem{},
 unk1ParticleSystem{},
 unk2ParticleSystem{},
 FiringParticleSystem{},
 Wave{},
 AngleRotatedSideways{},
 AngleRotatedForwards{},
 RockingSidewaysPerFrame{},
 RockingForwardsPerFrame{},
 HijackerInfantryType{},
 Tiberium{},
 unknown_34C{},
 UnloadTimer{},
 BarrelFacing{3},
 PrimaryFacing{},
 SecondaryFacing{},
 CurrentBurstIndex{},
 TargetLaserTimer{},
 unknown_short_3C8{},
 unknown_3CA{},
 CountedAsOwned{},
 IsSinking{},
 WasSinkingAlready{},
 unknown_bool_3CF{},
 IsUseless{},
 HasBeenAttacked{},
 Cloakable{},
 IsPrimaryFactory{},
 IsALoaner{},
 IsInPlayfield{},
 TurretRecoil{},
 BarrelRecoil{},
 IsTether{},
 IsAlternativeTether{},
 IsOwnedByCurrentPlayer{},
 DiscoveredByCurrentPlayer{},
 DiscoveredByComputer{},
 unknown_bool_41D{},
 unknown_bool_41E{},
 unknown_bool_41F{},
 SightIncrease{},
 RecruitableA{},
 RecruitableB{},
 IsRadarTracked{},
 IsOnCarryall{},
 IsCrashing{},
 WasCrashingAlready{},
 IsBeingManipulated{},
 BeingManipulatedBy{},
 ChronoWarpedByHouse{},
 unknown_bool_430{},
 IsMouseHovering{},
 ShouldBeReselectOnUnlimbo{},
 OldTeam{},
 CountedAsOwnedSpecial{},
 Absorbed{},
 unknown_bool_43A{},
 unknown_43C{},
 CurrentTargetThreatValues{},
 CurrentTargets{},
 AttackedTargets{},
 TurretRotateSoundController{},
 IsTurretRotateSoundPlaying{},
 TurretIsRotating{},
 GattlingSoundController{},
 IsGattlingSoundPlaying{},
 unknown_4BC{},
 UnusedGattlingSoundController{},
 IsUnusedGattlingSoundPlaying{},
 unknown_4D8{},
 QueuedVoiceSoundController{},
 QueuedVoiceIndex{},
 unknown_4F4{},
 unknown_bool_4F8{},
 unknown_4FC{},
 QueueUpToEnter{},
 EMPLockRemaining{},
 ThreatPosed{},
 ShouldLoseTargetNow{},
 FiringRadBeam{},
 PlanningToken{},
 Disguise{},
 DisguisedAsHouse{} {
 AbstractFlags|=::AbstractFlags::Techno;InitialOwner=Owner=pOwner;ArmorMultiplier=FirepowerMultiplier=1.0;
 Group=-1;HijackerInfantryType=-1;CurrentRanking=Rank::Invalid;
 // 0x6F2B40: these are sentinels, not zero-filled state. Otherwise the first
 // real frame falsely detects a promotion and attempts to play voice zero.
 QueuedVoiceIndex=unknown_4F4=0xFFFFFFFFu;
 TargetingTimer.Start(45);
 DiscoveredByCurrentPlayer=DiscoveredByComputer=true;IsOwnedByCurrentPlayer=pOwner&&pOwner==HouseClass::CurrentPlayer;Array.AddItem(this);
}
TechnoClass::~TechnoClass(){
 // 0x6F452C / 0x6F4531 retires the original token before the other managers
 // and before removing this unit from TechnoClass::Array.
 if(PlanningToken){PlanningToken->Destroy();PlanningToken=nullptr;}
 Array.Remove(this);
 // 0x6F4500 deletes the manager; ordinary death/UnInit already calls FreeAll.
 delete CaptureManager;CaptureManager=nullptr;
 Passengers.~PassengersClass();
 CurrentTargetThreatValues.~DynamicVectorClass();
 CurrentTargets.~DynamicVectorClass();
 AttackedTargets.~DynamicVectorClass();
 TurretRotateSoundController.~AudioController();
 GattlingSoundController.~AudioController();
 UnusedGattlingSoundController.~AudioController();
 QueuedVoiceSoundController.~AudioController();
}
