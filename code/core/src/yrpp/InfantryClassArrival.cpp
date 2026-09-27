// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp::Per_Cell_Process, calibrated to YR 0x519630.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/InfantryClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/Randomizer.h"
#include "yrpp/VocClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/Unsorted.h"
#include "scenario_runtime.hpp"
#include "type_resources.hpp"
#include <bit>
#include <cmath>
#include <cstdlib>

namespace {
CellStruct cell_of(const CoordStruct& at){return {short(at.X/256),short(at.Y/256)};}
int truncate(double value){return std::isfinite(value)&&value>=-2147483648.0&&value<2147483648.0?int(value):INT32_MIN;}
bool multi_engineer() {
    const auto& runtime=game::scenario_runtime();
    if(runtime.session_mode(runtime.context)==int(GameMode::Campaign))return false;
    // This is session.Options, not Rules.MultiEngineer. A detached native
    // caller must bind the real session before exercising enemy engineers.
    auto* session=runtime.start?runtime.start->session:runtime.houses?runtime.houses->session:nullptr;
    if(!session)std::abort();
    return session->GameMode!=GameMode::Campaign && session->Config.MultiEngineer;
}
void entered(TechnoClass* target,InfantryClass* actor) {
    if(target->AttachedTag)target->AttachedTag->RaiseEvent(TriggerEvent::EnteredBy,actor,CellStruct::Empty);
}
void consumed(InfantryClass* actor) {
    if(actor->AttachedTag)actor->AttachedTag->RaiseEvent(TriggerEvent::DestroyedByAnything,actor,CellStruct::Empty);
    actor->UnInit();
}
void relinquish_control(InfantryClass* actor) {
    if(actor->MindControlledBy && actor->MindControlledBy->CaptureManager)
        actor->MindControlledBy->CaptureManager->FreeUnit(actor);
}
}

// YR-only garrison addition, 0x522910. Limbo keeps the original infantry
// object alive in Building.Occupants; no replacement passenger object.
void InfantryClass::Garrison(BuildingClass* building) {
 if(Type->Occupier){
  Limbo();building->Occupants.AddItem(this);building->UpdateThreatInCell(building->GetCell());
  if(building->Occupants.Count==1){
   building->Mark(MarkType::Change);
   if(Owner->IsControlledByCurrentPlayer()&&!game::type_resources().audio_unavailable){VoxClass::Play("EVA_StructureGarrisoned",-1,-1);VocClass::PlayAt(RulesClass::Instance->BuildingGarrisonedSound,GetCoords());}
  }
  if(Owner->IsHumanPlayer)ShouldEnterOccupiable=ShouldGarrisonStructure=false;
 }else if(Type->Assaulter){building->KillOccupants(this);SetDestination(nullptr,true);Scatter(building->GetCoords(),true,true);}
}

void InfantryClass::UpdatePosition(PCPType reason) {
    auto& map=MapClass::Instance;
    auto* arrival_cell=map.GetCellAt(Location);
    if(reason!=PCPType::End){if(IsAlive)FootClass::UpdatePosition(reason);return;}
    if(GetCurrentMission()==Mission::Capture && (Type->Assaulter || Type->Occupier)
        && Destination && Destination->WhatAmI()==AbstractType::Building) {
        auto* building=static_cast<BuildingClass*>(Destination);
        if(building==arrival_cell->GetBuilding()) {
            if(building->CanBeOccupiedBy(this)){entered(building,this);Garrison(building);}
            else {SetDestination(nullptr,true);Scatter(building->GetCoords(),true,true);}
            return;
        }
    }
    if(GetCurrentMission()==Mission::Eaten)if(auto* building=arrival_cell->GetBuilding();building && (building==Destination || building==Target)) {
        entered(building,this);
        if(GetTechnoType()->VoiceDie.Count>0 && Owner->IsControlledByCurrentPlayer()) {
            const auto at=Location;auto& voices=GetTechnoType()->VoiceDie;
            const unsigned index=static_cast<unsigned>(Randomizer::Global.Random())%static_cast<unsigned>(voices.Count);
            VocClass::PlayAt(voices[int(index)],at);
        }
        if(GetTechnoType()->DieSound.Count>0) {
            const auto at=Location;auto& sounds=GetTechnoType()->DieSound;
            const unsigned index=static_cast<unsigned>(Randomizer::Global.Random())%static_cast<unsigned>(sounds.Count);
            VocClass::PlayAt(sounds[int(index)],at);
        }
        building->Owner->GiveMoney(GetRefund());
        if(AttachedTag)AttachedTag->RaiseEvent(TriggerEvent::DestroyedByAnything,this,CellStruct::Empty);
        if(building->Type->Grinding) {
            VocClass::PlayAt(RulesClass::Instance->EnterGrinderSound,GetCoords());
            if(building->GetAnim(BuildingAnimSlot::Active)) {
                building->DestroyNthAnim(BuildingAnimSlot::Active);
                building->PlayNthAnim(BuildingAnimSlot::Special,building->GetHealthPercentage()<=RulesClass::Instance->ConditionYellow,false,0);
            }
        }
        UnInit();return;
    }
    if(GetCurrentMission()==Mission::Capture || GetCurrentMission()==Mission::Area_Guard || GetCurrentMission()==Mission::Patrol) {
        if(Destination && (Destination->AbstractFlags & ::AbstractFlags::Techno)!=::AbstractFlags::None
            && static_cast<TechnoClass*>(Destination)->IsStrange() && Destination->WhatAmI()==AbstractType::Building) {
            auto* target=static_cast<TechnoClass*>(Destination);
            if(GetMapCoords()==target->GetMapCoords()) {
                entered(target,this);target->Disappear(false);target->SetOwningHouse(Owner,true);
                target->HijackerInfantryType=Type->ArrayIndex;target->GotHijacked();
                if(AttachedTag && AttachedTag->ShouldReplace())target->AttachTrigger(AttachedTag);
                consumed(this);return;
            }
            if(!Locomotor)std::abort();const CoordStruct at=Locomotor->Destination();
            if(at==CoordStruct::Empty || GetMapCoords()==cell_of(at)){SetDestination(Destination,true);return;}
        }
        auto* building=arrival_cell->GetBuilding();
        if(building && (building==Destination || building==Target)) {
            entered(building,this);
            if(Type->Engineer) {
                if(building->WhatAmI()==AbstractType::Building && building->Type->BridgeRepairHut) {
                    if(Owner->IsControlledByCurrentPlayer() && RadarEventClass::Create(RadarEventType::BridgeRepaired,GetMapCoords()))
                        VoxClass::Play("EVA_BridgeRepaired",-1,-1);
                    if(RulesClass::Instance->RepairBridgeSound!=-1)VocClass::PlayAt(RulesClass::Instance->RepairBridgeSound,building->Location);
                    bool wooden=false;
                    for(int y=-2;y<3;++y)for(int x=-2;x<3;++x) {
                        auto at=GetMapCoords();const int tile=map.GetCellAt(CellStruct{short(at.X+x),short(at.Y+y)})->IsoTileTypeIndex;
                        at=GetMapCoords();const int overlay=map.GetCellAt(CellStruct{short(at.X+x),short(at.Y+y)})->OverlayTypeIndex;
                        if((tile>=IsometricTileTypeClass::WoodBridgeSet && tile<IsometricTileTypeClass::WoodBridgeSet+16)
                            || (overlay>=74 && overlay<=101))wooden=true;
                    }
                    if(wooden)map.RepairWoodBridgeAt(GetMapCoords());else map.RepairConcreteBridgeAt(GetMapCoords());
                    for(int i=Array.Count-1;i>=0;--i)Array[i]->PointerExpired(building,false);
                    building->GotHijacked();consumed(this);return;
                }
                if(Owner->IsAlliedWith(building) || (building->Owner->Type->MultiplayPassive && building->Type->CanBeOccupied)) {
                    if(building->Health!=building->GetTechnoType()->Strength){building->OnFinishRepair();consumed(this);return;}
                }else {
                    const bool capturable=building->WhatAmI()==AbstractType::Building && building->Type->Capturable;
                    if(multi_engineer() && building->GetHealthPercentage()>RulesClass::Instance->ConditionRed) {
                        const int maximum=std::bit_cast<int>(static_cast<unsigned>(building->Health)
                            -static_cast<unsigned>(truncate(building->GetTechnoType()->Strength*RulesClass::Instance->ConditionRed*0.5)));
                        const double amount=building->GetTechnoType()->Strength*((1.0-RulesClass::Instance->ConditionRed*0.5)*0.5);
                        int damage=truncate(amount>=maximum?double(maximum):building->GetTechnoType()->Strength*((1.0-RulesClass::Instance->ConditionRed*0.5)*0.5));
                        building->ReceiveDamage(&damage,0,RulesClass::Instance->C4Warhead,this,true,false,nullptr);
                        consumed(this);return;
                    }
                    if(!capturable){consumed(this);return;}
                    if(building->CurrentMission!=Mission::Selling && !building->IsBeingWarpedOut()) {
                        entered(building,this);building->Owner->HasBeenThieved=true;
                        if(AttachedTag && AttachedTag->ShouldReplace())building->AttachTrigger(AttachedTag);
                        building->SetOwningHouse(Owner,true);building->HijackerInfantryType=Type->ArrayIndex;building->GotHijacked();
                        consumed(this);return;
                    }
                }
            }else if(Type->Agent){building->SpiedBy(Owner);consumed(this);return;}
            SetDestination(nullptr,true);Scatter(building->GetCoords(),true,true);return;
        }
        if(!Destination) {
            if(CurrentMission!=Mission::Area_Guard)EnterIdleMode(false,true);
            if(map.GetCellAt(Location)->GetBuilding())Scatter(CoordStruct::Empty,true,false);
        }
    }
    // Each fallback re-queries GetCell; virtual getters and callbacks may change it.
    TechnoClass* occupant=nullptr;
    if(Type->VehicleThief) {
        occupant=GetCell()->GetUnit(OnBridge);
        if(!occupant)occupant=GetCell()->GetAircraft(OnBridge);
        if(!occupant)occupant=GetCell()->GetBuilding();
    }else {
        occupant=GetCell()->GetBuilding();
        if(!occupant)occupant=GetCell()->GetAircraft(OnBridge);
        if(!occupant)occupant=GetCell()->GetUnit(OnBridge);
    }
    TechnoClass* at_destination=nullptr;
    if(Destination && Destination->WhatAmI()==AbstractType::Cell) {
        auto* cell=map.GetCellAt(Destination->GetCoords());
        if(Type->VehicleThief) {
            at_destination=cell->GetUnit(true);
            if(!at_destination)at_destination=cell->GetAircraft(true);
            if(!at_destination)at_destination=cell->GetBuilding();
        }else {
            at_destination=cell->GetBuilding();
            if(!at_destination)at_destination=cell->GetAircraft(true);
            if(!at_destination)at_destination=cell->GetUnit(true);
        }
    }
    if(GetCurrentMission()==Mission::Enter && occupant
        && (occupant==Destination || occupant==Target || (at_destination && occupant==at_destination))) {
        entered(occupant,this);
        if(occupant->WhatAmI()==AbstractType::Building) {
            auto* building=static_cast<BuildingClass*>(occupant);
            if(building==GetCell()->GetBuilding() && SendToFirstLink(RadioCommand::RequestCompleteEnter)==RadioCommand::AnswerPositive) {
                MissionAccumulateTime=0;sub_70DE00(0);SetCurrentWeaponStage(0);relinquish_control(this);
                if(building->Type->InfantryAbsorb){VocClass::PlayAt(RulesClass::Instance->EnterBioReactorSound,GetCoords());Absorbed=true;}
                Limbo();
                if(building->IsAbsorbAllowed() && CountedAsOwnedSpecial){--Owner->OwnedInfantry;CountedAsOwnedSpecial=false;}
                if(!building->Type->InfantryAbsorb)SendCommand(RadioCommand::RequestLink,building);
                else if(building->Type->ExtraPowerBonus>0)building->Owner->RecheckPower=true;
                building->Passengers.AddPassenger(this);AbortMotion();return;
            }
        }else if(GetMapCoords()==occupant->GetMapCoords() && occupant==Destination) {
            if(SendCommand(RadioCommand::QueryCanEnter,occupant)==RadioCommand::AnswerPositive) {
                SetArchiveTarget(nullptr);OnBridge=false;MissionAccumulateTime=0;sub_70DE00(0);SetCurrentWeaponStage(0);relinquish_control(this);
                Limbo();if(occupant->GetTechnoType()->OpenTopped)occupant->EnteredOpenTopped(this);
                Transporter=occupant;occupant->AddPassenger(this);Undiscover();
            }else {SetDestination(nullptr,true);QueueMission(Mission::Guard,false);Scatter(CoordStruct::Empty,true,true);}
            return;
        }
    }
    if(GetCurrentMission()==Mission::Sabotage && Type->C4) {
        auto* building=arrival_cell->GetBuilding();
        if(building && building==Destination) {
            entered(building,this);
            if(building->GetCurrentMission()!=Mission::Selling && !building->IsIronCurtained()) {
                if(building->C4Applied){SetDestination(nullptr,true);RearmTimer.Start(GetROF(1));Scatter(building->GetCoords(),true,true);return;}
                building->C4Applied=true;building->Flash(truncate(RulesClass::Instance->C4Delay*900.0*0.5));
                const int frames=truncate(RulesClass::Instance->C4Delay*900.0);
                building->C4AppliedBy=this;building->C4Timer.Start(frames);
            }
            AbortMotion();Uncloak(false);RearmTimer.Start(GetROF(1));Scatter(building->GetCoords(),true,true);return;
        }
        if(map.GetCellAt(GetCoords())==Destination) {
            MapClass::DamageArea(Location,RulesClass::Instance->BridgeStrength,this,RulesClass::Instance->C4Warhead,true,nullptr);
            StopMoving();const unsigned facing=PrimaryFacing.Current().GetValue<3>();
            const auto delta=Unsorted::AdjacentCoord[facing];
            Scatter({Location.X+delta.X,Location.Y+delta.Y,Location.Z},true,true);QueueMission(Mission::Move,false);
            if(!Destination || map.GetCellAt(Destination->GetCoords())->LandType==LandType::Water)Mark(MarkType::Down);
            MapClass::DamageArea(Location,RulesClass::Instance->BridgeStrength,nullptr,RulesClass::Instance->C4Warhead,true,nullptr);
            MapClass::DamageArea(Location,RulesClass::Instance->BridgeStrength,nullptr,RulesClass::Instance->C4Warhead,true,nullptr);
            if(!IsAlive)return;
            Mark(MarkType::Down);
        }
    }
    if(IsTether) {
        SendToFirstLink(RadioCommand::NotifyUnloaded);
        if(Type->VoiceComment.Count>1)VocClass::PlayAt(Type->VoiceComment[1],Location);
        const auto occupied=OnBridge?arrival_cell->AltOccupationFlags:arrival_cell->OccupationFlags;
        if((static_cast<unsigned>(occupied)&0x1Cu)==0x1Cu)arrival_cell->ScatterContent(CoordStruct::Empty,true,true,OnBridge);
    }
    if(QueuedMission==Mission::None && !Destination && !Target && !HasAnyLink())EnterIdleMode(false,true);
    NextMission();
    {
        vt_entry_48C(0,0,0,0);UpdateSight(0,0,0,0,0);
        CoordStruct at=Location;map.RevealArea3(&at,0,LastSightRange+3,false);
    }
    auto* cell=map.GetCellAt(Location);
    if(!Locomotor)std::abort();
    if(!Locomotor->Is_Moving() && !Type->C4 && !HasAbility(Ability::C4)
        && (cell->LandType==LandType::Rock || cell->LandType==LandType::Water)
        && (!OnBridge || !(static_cast<unsigned>(cell->Flags)&0x100u))) {
        int damage=Health;ReceiveDamage(&damage,0,RulesClass::Instance->C4Warhead,nullptr,true,false,nullptr);return;
    }
    // Original factory departure cuts the radio tether at the first completed step.
    if(IsTether&&HasAnyLink()&&GetNthLink()->WhatAmI()==AbstractType::Building
        &&static_cast<BuildingClass*>(GetNthLink())->Type->Factory==AbstractType::InfantryType){
        SendToFirstLink(RadioCommand::NotifyUnloaded);
        if(ArchiveTarget&&ArchiveTarget!=Destination){SetDestination(ArchiveTarget,true);QueueMission(Mission::Move,false);}
    }
    if(IsAlive)FootClass::UpdatePosition(reason);
}
