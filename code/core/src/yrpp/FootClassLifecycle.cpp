// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp constructor/destructor, adapted to YR 0x004D31E0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Native presentation lifecycle only: no locomotor, team or movement service is started.
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/AircraftTrackerClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/ParasiteClass.h"
#include <cstdlib>
namespace { DynamicVectorClass<FootClass*> feet; }
DynamicVectorClass<FootClass*>& FootClass::Array=feet;
void FootClass::AddPassenger(FootClass* passenger){
 TechnoClass::AddPassenger(passenger);
 if(GetTechnoType()->Gunner&&Passengers.NumPassengers==1)ReceiveGunner(passenger);
}
FootClass* FootClass::RemoveFirstPassenger(){
 auto* passenger=Passengers.RemoveFirstPassenger();
 if(GetTechnoType()->Gunner&&!Passengers.NumPassengers)RemoveGunner(passenger);
 return passenger;
}
FootClass::FootClass(HouseClass* owner) noexcept : TechnoClass(owner),
 PlanningPathIdx{-1},WaypointNearbyAccessibleCellDelta{},WaypointCell{},unknown_52C{},ThreatAvoidanceCoefficient{},
 WalkedFramesSoFar{},IsMoveSoundPlaying{},MoveSoundDelay{},MoveSoundAudioController{},CurrentMapCoords{},LastMapCoords{},
 LastFlightMapCoords{},CurrentJumpjetMapCoords{},CurrentTunnelCoords{},unused_574{},SpeedPercentage{},SpeedMultiplier{1.0},
 unknown_abstract_array_588{},unknown_5A0{},Destination{},LastDestination{},NavQueue{},MegaMission{Mission::None},
 MegaDestination{},MegaTarget{},unknown_5D0{},HaveAttackMoveTarget{},Team{},NextTeamMember{},unknown_5DC{},PathDirections{},
 PathDelayTimer{},PathWaitTimes{10},unknown_timer_650{},SightTimer{},BlockagePathTimer{},Locomotor{},
 unknown_point3d_678{-1,-1,-1},TubeIndex{-1},TubeFaceIndex{},WaypointIndex{},ShouldScatterInNextIdle{},IsScanLimited{},
 IsInitiated{},ShouldScanForTarget{},unknown_bool_68B{},IsDeploying{},IsFiring{},unknown_bool_68E{},ShouldEnterAbsorber{},
 ShouldEnterOccupiable{},ShouldGarrisonStructure{},ParasiteEatingMe{},LastBeParasitedStartFrame{},ParasiteImUsing{},
 ParalysisTimer{},unknown_bool_6AC{},IsAttackedByLocomotor{},IsLetGoByLocomotor{},unknown_bool_6AF{},unknown_bool_6B0{},
 unknown_bool_6B1{},unknown_bool_6B2{},unknown_bool_6B3{},DrawingYOffset{},IsCrushingSomething{},FrozenStill{true},
 IsWaitingBlockagePath{},unknown_bool_6B8{},unused_6BC{} {
 AbstractFlags|=::AbstractFlags::Foot;
 // YR initializes only the first path sentinel, unlike OpenTS's fill.
 PathDirections[0]=-1;
 if(!Array.AddItem(this))std::abort();
}
FootClass::~FootClass() {
 delete ParasiteImUsing;ParasiteImUsing=nullptr;
 Array.Remove(this);
 MoveSoundAudioController.~AudioController();
 unknown_abstract_array_588.~DynamicVectorClass();NavQueue.~DynamicVectorClass();
#if defined(_MSC_VER)
 Locomotor.~ILocomotionPtr();
#else
 if(Locomotor)Locomotor->Release();
#endif
}

// OpenTS Detach_All / Delete_This; YR 0x4D9720 / 0x4DE5D0.
void FootClass::Disappear(bool permanently) {
 if(permanently) {
  if(Team && !Unsorted::ScenarioInit)Team->LiberateMember(this,-1,0);
  SendToFirstLink(RadioCommand::NotifyUnlink);
 }else if(auto* link=GetNthLink();link && !Owner->IsAlliedWith(link)) {
  SendToFirstLink(RadioCommand::NotifyUnlink);
 }
 ObjectClass::Disappear(permanently);
}
void FootClass::UnInit() {
 if(CaptureManager)CaptureManager->FreeAll();
 if(LocomotorTarget)ReleaseLocomotor(true);
 if(Team)Team->LiberateMember(this,-1,0);
 ObjectClass::UnInit();
}
void FootClass::Stun() {
 SetDestination(nullptr,true);PathDirections[0]=-1;
 StopMoving();TechnoClass::Stun();
}
// OpenTS Limbo, with YR adjacency, COM locomotion and aircraft tracking.
bool FootClass::Limbo() {
 if(!InLimbo && GetType()) {
  for(int i=0;i<8;++i){const auto offset=Unsorted::AdjacentCell[i];
   --MapClass::Instance.GetCellAt(CellStruct{short(LastMapCoords.X+offset.X),short(LastMapCoords.Y+offset.Y)})->BlockedNeighbours;
  }
  if(!Unsorted::ScenarioInit)StopMoving();
  if(Locomotor){Locomotor->Mark_All_Occupation_Bits(MarkType::Up);Locomotor->Limbo();}
  MoveSoundAudioController.End();
  if(GetTechnoType()->SensorsSight)RemoveSensorsAt(CellStruct::Empty);
  if(GetLastFlightMapCoords()!=CellStruct::Empty)AircraftTrackerClass::Instance.Remove(this);
 }
 return TechnoClass::Limbo();
}
bool FootClass::Unlimbo(const CoordStruct& where,DirType facing){
 if(!TechnoClass::Unlimbo(where,facing))return false;
 Locomotor->Unlimbo();Locomotor->Power_On();DiscoveredBy(Owner);
 PathDirections[0]=-1;
 const auto cell=GetMapCoords();
 for(int i=0;i<8;++i){const auto delta=Unsorted::AdjacentCell[i];
  ++MapClass::Instance.GetCellAt(CellStruct{short(cell.X+delta.X),short(cell.Y+delta.Y)})->BlockedNeighbours;
 }
 if(IsInAir()){if(GetTechnoType()->ConsideredAircraft)AircraftTrackerClass::Instance.Add(this);}
 else LastMapCoords=cell;
 ThreatAvoidanceCoefficient=GetTechnoType()->ThreatAvoidanceCoefficient;
 if(GetTechnoType()->SensorsSight)AddSensorsAt(CellStruct::Empty);
 return true;
}
