// YR CaptureManagerClass, calibrated to 0x4717D0..0x4726C0.
// Fixed OpenTS 44fac744 has no RA2 mind-control manager; retain the existing
// YRpp object and ControlNode layout and use original ownership operations.
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/TemporalClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/ScenarioClass.h"
#include "type_resources.hpp"
#include <cstdlib>
#include <cstring>

namespace { DynamicVectorClass<CaptureManagerClass*> managers; }
DynamicVectorClass<CaptureManagerClass*>& CaptureManagerClass::Array=managers;

CaptureManagerClass::CaptureManagerClass(TechnoClass* owner,int maximum,bool infinite) noexcept
 :AbstractClass(),ControlNodes(),MaxControlNodes(maximum),InfiniteMindControl(infinite),
 OverloadDeathSoundPlayed(false),OverloadPipState(0),Owner(owner),OverloadDamageDelay(30){
 if(!Array.AddItem(this))std::abort();
}
CaptureManagerClass::~CaptureManagerClass(){
 // Normal release happens before destruction. At scenario teardown there can
 // still be nodes; dispose their storage without changing dying world objects.
 for(auto* node:ControlNodes)GameDelete(node);
 Array.Remove(this);
}
HRESULT YRPP_STDCALL CaptureManagerClass::GetClassID(CLSID* output){
 if(!output)return static_cast<HRESULT>(0x80004003u);
 constexpr unsigned words[]{0x0679E982,0x11D3AD9D,0x100016BE,0x6CA1624B};
 std::memcpy(output,words,sizeof(words));return 0;
}
bool CaptureManagerClass::CanCapture(TechnoClass* target) const {
 if(!target||target->GetOwningHouse()==Owner->GetOwningHouse()||target->GetTechnoType()->ImmuneToPsionics
  ||(target->BunkerLinkedItem&&target->WhatAmI()==AbstractType::Unit)||target->IsMindControlled()
  ||target->MindControlledByHouse||target->IsIronCurtained())return false;
 if(!InfiniteMindControl&&ControlNodes.Count>=MaxControlNodes&&MaxControlNodes!=1)return false;
 return target->CurrentMission!=Mission::Selling&&target->CurrentMission!=Mission::Construction;
}
bool CaptureManagerClass::CannotControlAnyMore() const {return !InfiniteMindControl&&ControlNodes.Count>=MaxControlNodes;}
bool CaptureManagerClass::IsControllingSomething() const {return ControlNodes.Count>0;}
int CaptureManagerClass::GetControlledCount(){return ControlNodes.Count;}
HouseClass* CaptureManagerClass::GetOriginalOwner(TechnoClass* unit) const {
 for(int i=ControlNodes.Count-1;i>=0;--i)if(ControlNodes[i]->Unit==unit)return ControlNodes[i]->OriginalOwner;
 return nullptr;
}
bool CaptureManagerClass::CaptureUnit(TechnoClass* target){
 if(!target||(target->AbstractFlags&::AbstractFlags::Techno)==::AbstractFlags::None||!CanCapture(target))return false;
 if(MaxControlNodes==1)FreeAll();
 auto* house=target->GetOwningHouse();
 if(!target->SetOwningHouse(Owner->GetOwningHouse(),true))return false;
 auto* node=GameCreate<ControlNode>();node->Unit=target;node->OriginalOwner=house;
 if(!ControlNodes.AddItem(node))std::abort();
 target->MindControlledBy=Owner;node->LinkDrawTimer.Start(RulesClass::Instance->MindControlAttackLineFrames);
 const auto* unit=target->WhatAmI()==AbstractType::Unit?static_cast<UnitClass*>(target):nullptr;
 if((!unit||!unit->Type->Harvester||unit->CurrentMission!=Mission::Unload)
  &&target->CurrentMission!=Mission::Selling&&target->CurrentMission!=Mission::Construction)target->Guard();
 DecideUnitFate(target);
 const bool building=target->WhatAmI()==AbstractType::Building;
 auto at=target->GetCoords();at.Z+=building?Unsorted::LevelHeight*static_cast<BuildingClass*>(target)->Type->Height
  :target->GetTechnoType()->LeptonMindControlOffset;
 if(void* memory=YRMemory::Allocate(sizeof(AnimClass)))
  target->MindControlRingAnim=::new(memory) AnimClass(RulesClass::Instance->ControlledAnimationType,at,0,1,0x600,0,false);
 if(target->MindControlRingAnim){target->MindControlRingAnim->SetOwnerObject(target);if(building)target->MindControlRingAnim->ZAdjust=-1024;}
 return true;
}
bool CaptureManagerClass::FreeUnit(TechnoClass* target){
 if(!target)return false;
 for(int i=ControlNodes.Count-1;i>=0;--i){
  auto* node=ControlNodes[i];if(node->Unit!=target)continue;
  if(target->MindControlRingAnim){target->MindControlRingAnim->UnInit();target->MindControlRingAnim=nullptr;}
  const int sound=target->GetTechnoType()->MindClearedSound==-1?RulesClass::Instance->MindClearedSound:target->GetTechnoType()->MindClearedSound;
  if(sound!=-1&&!game::type_resources().audio_unavailable)VocClass::PlayAt(sound,target->Location,nullptr);
  target->SetOwningHouse(node->OriginalOwner,true);DecideUnitFate(target);target->MindControlledBy=nullptr;
  GameDelete(node);ControlNodes.RemoveItem(i);
 }
 return true;
}
void CaptureManagerClass::FreeAll(){for(int i=ControlNodes.Count-1;i>=0;--i)FreeUnit(ControlNodes[i]->Unit);}
void CaptureManagerClass::DecideUnitFate(TechnoClass* target){
 if(!target)return;
 auto* foot=(target->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None?static_cast<FootClass*>(target):nullptr;
 if(foot&&foot->Team)foot->Team->LiberateMember(foot,-1,0);
 if(target->TemporalImUsing&&target->TemporalImUsing->Target)target->TemporalImUsing->LetGo();
 if(target->GetTechnoType()->OpenTopped)for(auto* passenger=target->Passengers.FirstPassenger;passenger;passenger=static_cast<FootClass*>(passenger->NextObject))passenger->SetTarget(nullptr);
 if(target->Owner->IsHumanPlayer)return;
 auto& rules=*RulesClass::Instance;auto* controllerHouse=Owner->Owner;
 auto* weights=&rules.AICaptureLowMoney;
 if(controllerHouse->Available_Money()>=rules.AICaptureLowMoneyMark){
  if(controllerHouse->GetPowerPercentage()<1.0)weights=&rules.AICaptureLowPower;
  else weights=float(target->Health)/double(target->GetTechnoType()->Strength)>=rules.AICaptureWoundedMark?&rules.AICaptureNormal:&rules.AICaptureWounded;
 }
 const int roll=ScenarioClass::Instance->Random.RandomRanged(1,100);int decision=0,sum=0;
 while(decision!=6&&decision!=weights->Count){sum+=(*weights)[decision++];if(roll<=sum)break;}
 auto* ownerFoot=(Owner->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None?static_cast<FootClass*>(Owner):nullptr;
 if(ownerFoot&&ownerFoot->Team&&ownerFoot->Team->Type->MindControlDecision)decision=ownerFoot->Team->Type->MindControlDecision;
 switch(decision){
 case 1:if(target->Owner==Owner->Owner&&ownerFoot&&ownerFoot->Team&&ownerFoot->Team->AddMember(foot,false))return;break;
 case 2:if(target->EnterGrinder())return;break;
 case 3:if(target->EnterBioReactor())return;break;
 case 5:return;
 default:break;
 }
 target->QueueMission(Mission::Hunt,false);
}
bool CaptureManagerClass::NeedsToDrawLinks() const {
 if(Owner->IsSelected||(Owner->Transporter&&Owner->Transporter->IsSelected))return true;
 for(int i=ControlNodes.Count-1;i>=0;--i)if(ControlNodes[i]->Unit->IsSelected||ControlNodes[i]->LinkDrawTimer.GetTimeLeft()>0)return true;
 return false;
}
bool CaptureManagerClass::IsOverloading(bool* applied) const {
 if(!InfiniteMindControl||ControlNodes.Count<=MaxControlNodes)return false;
 *applied=OverloadPipState>0;return true;
}
void CaptureManagerClass::HandleOverload(){
 if(!InfiniteMindControl)return;
 if(OverloadPipState>0)--OverloadPipState;
 if(OverloadDamageDelay>0){--OverloadDamageDelay;return;}
 auto& rules=*RulesClass::Instance;int band=0;
 while(ControlNodes.Count>rules.OverloadCount[band]&&band<rules.OverloadCount.Count-1)++band;
 OverloadDamageDelay=rules.OverloadFrames[band];int damage=rules.OverloadDamage[band];
 if(damage<=0){OverloadDeathSoundPlayed=false;return;}
 OverloadPipState=10;Owner->ReceiveDamage(&damage,0,rules.C4Warhead,nullptr,false,false,nullptr);
 if(!OverloadDeathSoundPlayed){
  if(!game::type_resources().audio_unavailable)VocClass::PlayAt(rules.MasterMindOverloadDeathSound,Owner->Location,nullptr);
  OverloadDeathSoundPlayed=true;
 }
 for(int i=0;i<5;++i){auto at=Owner->Location;at.Y+=ScenarioClass::Instance->Random.RandomRanged(-200,200);
  at.X+=ScenarioClass::Instance->Random.RandomRanged(-200,200);at.Z+=100;
  if(void* memory=YRMemory::Allocate(sizeof(ParticleSystemClass)))::new(memory) ParticleSystemClass(rules.DefaultSparkSystem,at,nullptr,nullptr,CoordStruct::Empty,nullptr);
 }
 if(band>0&&Owner->IsAlive){const float tilt=band==1?0.015f:0.03f;Owner->AngleRotatedSideways=ScenarioClass::Instance->Random.RandomRanged(0,100)<50?-tilt:tilt;}
}
