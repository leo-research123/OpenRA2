// Building animation management and native combat/factory update admission.
// Originals: 0x004509D0, 0x0043EF90, 0x00451750, 0x00451890, 0x00451E40.
#include "yrpp/BuildingClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/TacticalClass.h"
#include "map_world.hpp"
#include "type_resources.hpp"
#include <algorithm>
#include <cstring>
namespace {
const char* animation_name(const BuildingAnimStruct&s,bool damaged,bool garrisoned){return damaged?s.Damaged:garrisoned?s.Garrisoned:s.Anim;}
bool usable(const char*id){return id&&*id&&_strcmpi(id,"none")&&_strcmpi(id,"<none>");}
CoordStruct animation_position(const BuildingClass& building,int slot){
 // 0x00451913: slot pixel offsets are converted when the animation is
 // created. SetOwnerObject subsequently stores this as a relative coordinate.
 auto at=building.GetRenderCoords();
 const auto delta=TacticalClass::Instance->ApplyMatrix_Pixel(building.Type->BuildingAnim[slot].Position);
 at.X+=delta.X;at.Y+=delta.Y;return at;
}
bool animation_power_off(const BuildingClass& building,const BuildingAnimStruct& slot){
 // 0x00452480 handles manual shutdown; 0x004549B0/0x004547C0 handles
 // automatic power loss only for Powered buildings with positive drain.
 const bool online=building.Type->Powered&&building.Type->PowerDrain>0?
     building.IsPowerOnline():building.HasPower;
 return slot.Powered&&(!building.StuffEnabled||!online);
}
void update_turret_frame(BuildingClass& building) {
 if (!building.Type || !building.Type->Turret || building.Type->TurretAnimIsVoxel) return;
 auto* anim = building.Anims[9];
 if (!anim) return;
 // 0x0045125E reads PrimaryFacing at +0x388; table VA 0x007F4890.
 constexpr int frames[32] = {28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,
                            12,11,10,9,8,7,6,5,4,3,2,1,0,31,30,29};
 const unsigned facing = ((unsigned(building.PrimaryFacing.Current().Raw) >> 10) + 1) >> 1;
 const int frame = frames[facing & 31u];
 if (anim->Animation.Value != frame) {
  anim->Animation.Value = frame;
  building.NeedsRedraw = true;
  game::map_object_changed();
 }
 // The facing frame is absolute, NOT AnimType.Start + facing, and must not
 // advance during AnimClass::Update, which runs after BuildingClass::Update.
 anim->Animation.Rate = 0;
}

}
SHPStruct* BuildingClass::GetImage() const {
 if(!Type)return nullptr;
 // 0x004513D0: unplaced/construction objects use the buildup image.
 return BState!=static_cast<int>(BStateType::Construction)&&ActuallyPlacedOnMap?Type->GetImage():Type->Buildup;
}
int BuildingClass::GetCurrentFrame(){
 if(!Type)return 0;
 // 0x0043EF90: gates reverse their buildup; selling reverses it again.
 if(Type->LaserFence)return LaserFenceFrame;
 if(Type->FirestormWall)return FirestormWallFrame;
 if(BState==static_cast<int>(BStateType::Construction)){
  int frame=Animation.Value;const auto& sequence=Type->BuildingAnimFrame[0];
  const int end=int(sequence.dwUnknown)+sequence.FrameCount-1;
  if(Type->Gate)frame=end-frame;
  if(GetCurrentMission()==Mission::Selling)frame=end-frame;
  return frame;
 }
 if(Type->CanBeOccupied){
  // 0x0043EF90: civilian occupiable buildings use ConditionRed; positive
  // TechLevel also enables the ordinary ConditionYellow damage threshold.
  int frame=GetOccupantCount()>0?2:0;
  if(IsRedHP()||(Type->TechLevel>0&&!IsGreenHP()))++frame;
  return Type->TechLevel==-1&&frame==3?1:frame;
 }
 if(Type->Gate)return IsGreenHP()?0:Type->GateStages+1;
 int frame=Animation.Value;
 if(!IsGreenHP()){
  if(BState==static_cast<int>(BStateType::Idle))++frame;
  else{int offset=0;for(int i:{1,2,4,5})offset=std::max(offset,int(Type->BuildingAnimFrame[i].dwUnknown)+Type->BuildingAnimFrame[i].FrameCount);frame+=offset;}
 }
 return std::max(frame,0);
}
int BuildingClass::GetShapeNumber()const{return const_cast<BuildingClass*>(this)->GetCurrentFrame();}
void BuildingClass::DestroyNthAnim(BuildingAnimSlot slot){int i=static_cast<int>(slot);if(i<0||i>=21)return;auto*a=Anims[i];Anims[i]=nullptr;if(a){a->OwnerObject=nullptr;delete a;game::map_object_changed();}}
void BuildingClass::PlayAnim(const char*name,BuildingAnimSlot slot,bool,bool,int delay){
 int i=static_cast<int>(slot);if(i<0||i>=21||!Type)return;auto*t=usable(name)?AnimTypeClass::Find(name):nullptr;
 const int previous=Anims[i]?Anims[i]->Animation.Value:-1;
 if(Anims[i]&&Anims[i]->Type==t)return;DestroyNthAnim(slot);if(!t||IsDead()||!IsOnMap)return;
 auto*a=new AnimClass(t,animation_position(*this,i),std::max(delay,0),1,0x1600,0,false);Anims[i]=a;a->SetOwnerObject(this);if(previous>=0)a->Animation.Value=previous;
 // 0x00451988/0x004519A2 replace the type defaults with slot values.
 const auto&s=Type->BuildingAnim[i];a->YSortAdjust=s.YSort;a->ZAdjust=s.ZAdjust;
 a->PowerOff=animation_power_off(*this,s);game::map_object_changed();
}
void BuildingClass::PlayNthAnim(BuildingAnimSlot slot,bool damaged,bool garrisoned,int delay){int i=static_cast<int>(slot);if(i<0||i>=21||!Type)return;PlayAnim(animation_name(Type->BuildingAnim[i],damaged,garrisoned),slot,damaged,garrisoned,delay);}
void BuildingClass::SetAnimCoords(){
 if(!Type)return;
 for(int i=0;i<21;++i)if(auto* anim=Anims[i]){
  auto at=animation_position(*this,i);
  // The native lifetime adapter already attaches these animations to the
  // building. Retain its relative-coordinate contract; EXE animations have
  // no OwnerObject here and receive the absolute position.
  if(anim->OwnerObject){const auto owner=anim->OwnerObject->GetCoords();at.X-=owner.X;at.Y-=owner.Y;at.Z-=owner.Z;}
  anim->SetLocation(at);
 }
}
void BuildingClass::UpdateAnimations(){
 // 0x00452410/0x00452480: manual power switching enables/disables the
 // static source. House shortage alone only changes powered animation slots.
 if(LightSource){if(StuffEnabled&&!IsDead()&&IsOnMap)LightSource->Activate();else LightSource->Deactivate();}
 if(!Type)return;const bool damaged=!IsGreenHP(),garrisoned=Occupants.Count>0;
 const bool online=IsPowerOnline();
 const bool power_managed=Type->Powered&&Type->PowerDrain>0;
 const bool power_loss=power_managed&&!online;
 if(power_managed&&online)DestroyNthAnim(static_cast<BuildingAnimSlot>(20));
 // 0x004549B0: slot 19 is PoweredSpecial's blackout/drain animation,
 // not a generic indicator for a player's manual power toggle.
 const bool low_power=Type->PoweredSpecial&&Owner&&!online&&
     (Owner->PowerBlackoutTimer.GetTimeLeft()||Owner->IsBeingDrained);
 if(IsDead()||!IsOnMap){for(int i=0;i<21;++i)DestroyNthAnim(static_cast<BuildingAnimSlot>(i));return;}
 for(int i=0;i<21;++i){const bool ambient=(i>=3&&i<=6)||i==18;const bool low=i==19&&low_power;const bool existing=Anims[i]!=nullptr;
  const bool turret=i==9&&Type->Turret&&!Type->TurretAnimIsVoxel;
  if(i==9&&(!Type->Turret||Type->TurretAnimIsVoxel)){DestroyNthAnim(static_cast<BuildingAnimSlot>(i));continue;}
  if(i==19&&!low_power){DestroyNthAnim(static_cast<BuildingAnimSlot>(i));continue;}
  if(low_power&&Type->BuildingAnim[i].PoweredSpecial){DestroyNthAnim(static_cast<BuildingAnimSlot>(i));continue;}
  const auto& slot=Type->BuildingAnim[i];
  if(power_loss&&!slot.Powered){
   if(slot.PoweredLight){
    if(existing){DestroyNthAnim(static_cast<BuildingAnimSlot>(i));
     if(i==10&&Type->IsAnimDelayedFire&&Type->BuildingAnim[3].PoweredSpecial)PlayNthAnim(static_cast<BuildingAnimSlot>(3),damaged,false);
    }
    continue;
   }
   if(slot.PoweredEffect){
    if(existing){AnimStates[i]=true;DestroyNthAnim(static_cast<BuildingAnimSlot>(i));
     if(i==16)PlayNthAnim(static_cast<BuildingAnimSlot>(20),damaged,false);
    }
    continue;
   }
  }
  const bool restore_light=power_managed&&online&&!slot.Powered&&slot.PoweredLight&&ActuallyPlacedOnMap&&!(Type->IsAnimDelayedFire&&i==10);
  const bool restore_effect=power_managed&&online&&!slot.Powered&&!slot.PoweredLight&&slot.PoweredEffect&&AnimStates[i];
  if(restore_effect)AnimStates[i]=false;
  const bool restore_special=Type->PoweredSpecial&&online&&Type->BuildingAnim[i].PoweredSpecial;
  if(ambient||low||existing||turret||restore_special||restore_light||restore_effect)PlayNthAnim(static_cast<BuildingAnimSlot>(i),damaged,garrisoned);
  if(auto*a=Anims[i]){bool off=animation_power_off(*this,Type->BuildingAnim[i]);if(a->PowerOff!=off){a->PowerOff=off;game::map_object_changed();}
  }
 }
 SetAnimCoords();
 update_turret_frame(*this);
}
void BuildingClass::ToggleDamagedAnims(bool damaged){if(IsDamaged!=damaged){IsDamaged=damaged;game::map_object_changed();}UpdateAnimations();}
// Begin_Mode and body stage progression adapt OpenTS building.cpp at pinned
// 44fac744f70235e0d5ddca107364a68f95132ce9, calibrated to 0x00447780,
// 0x004509D0 and 0x0043FB20. EA/OpenTS copyrights and GPL terms retained in
// code/third_party/opents/LICENSE.md. Local adaptations: RedAlert2Open, 2026.
void BuildingClass::BeginMode(BStateType mode){
 const int state=static_cast<int>(mode);
 if(!Type||state<0||state>=6)return;
 QueueBState=state;
 if(BState==static_cast<int>(BStateType::None)||state==0||Unsorted::ScenarioInit){
  BState=state;QueueBState=static_cast<int>(BStateType::None);
  const auto& sequence=Type->BuildingAnimFrame[state];
  int rate=sequence.FrameDuration;
  if(Type->Normalized&&state!=0)rate=GameOptionsClass::Instance.GetAnimSpeed(rate);
  Animation.Start(rate);Animation.Value=int(sequence.dwUnknown);
  if(Unsorted::ScenarioInit)for(int slot:{18,3,4,5,6})PlayNthAnim(static_cast<BuildingAnimSlot>(slot),!IsGreenHP(),GetOccupantCount()>0);
  NeedsRedraw=true;game::map_object_changed();
 }
}
bool BuildingClass::HasTurret() const {
 if(Type&&Type->Turret)return true;
 for(int i=0;i<std::min(int(UpgradeLevel),3);++i)if(Upgrades[i]&&Upgrades[i]->Turret)return true;
 return false;
}
// OpenTS 44fac744 building.cpp Clicked_As_Target / Update_Anim_Appearance /
// Apparent_Brightness; YR 0x456E00 / 0x452000 / 0x456F80.
void BuildingClass::Flash(int duration){
 if(duration)NeedsRedraw=true;
 TechnoClass::Flash(duration);
 UpdateAnimAppearance();
 game::map_object_changed();
}
int BuildingClass::GetFlashingIntensity(int intensity) const {
 if((Flashing.DurationRemaining&2)==2)return intensity>1500?intensity-500:intensity+500;
 return intensity;
}
void BuildingClass::UpdateAnimAppearance(){
 auto* converter=GetRemapColour();
 int intensity=GetFlashingIntensity(static_cast<short>(GetCell()->Intensity_Normal));
 if(IsIronCurtained()||(Airstrike&&Airstrike->Target==this))intensity=GetEffectTintIntensity(intensity);
 unknown_short_700=static_cast<short>(intensity);
 const auto translucency=[&]{
  char level=Translucency;
  if(level==15&&VisualCharacter(false,nullptr)==VisualType::Hidden)level=16;
  for(auto* anim:Anims)if(anim)anim->TranslucencyLevel=level;
 };
 for(auto* anim:Anims)if(anim){
  if(anim->Type&&anim->Type->ShouldUseCellDrawer){anim->LightConvert=converter;anim->TintColor=static_cast<unsigned short>(intensity);}
  translucency();
 }
 translucency();
}

void BuildingClass::Update(){
 UpdateDamageFires();
 if(!Type||!IsAlive||!IsOnMap)return;
 // 0x0044003F..0x0044006C compares Health with the building's own cached
 // health at 0x544, independently of the generic EstimatedHealth field.
 if(unknown_544!=Health) {
  if(Owner)Owner->RecheckPower=Owner->RecheckRadar=true;
  unknown_544=Health;
 }
 if(Health<=0){
  if(!C4Timer.GetTimeLeft()){if(Owner)Owner->RegisterLoss(this,false);game::detach_map_object(*this);LeaveRubble();IsAlive=false;ActuallyPlacedOnMap=false;game::map_object_changed();delete this;}
  return;
 }
 if(GetCurrentMission()==Mission::Selling) {
  Mission_Selling();
  // Retire after the mission has returned, without a timer write through a
  // deleted this pointer in the generic mission update.
  if(!IsAlive){delete this;return;}
 }
 UpdateRepair();
 // Armed buildings need the same Techno mission/target update as garrisons.
 // Unarmed presentation-only buildings still advance their flash timer here.
 const bool technoLogic=GetCurrentMission()!=Mission::Selling && (IsArmed()||(Type->CanBeOccupied&&(Occupants.Count>0||GetCurrentMission()==Mission::Unload))||
     (Type->WeaponsFactory&&(GetCurrentMission()==Mission::Unload||QueuedMission==Mission::Unload)));
 if(technoLogic){if(ReadyToNextMission()&&BState&&NextMission())IsReadyToCommence=false;TechnoClass::Update();if(!IsAlive)return;}
 const int oldFlash=Flashing.DurationRemaining;
 if(!technoLogic&&Flashing.Update()){
  NeedsRedraw=true;
  if((oldFlash&2)!=(Flashing.DurationRemaining&2))UpdateAnimAppearance();
  game::map_object_changed();
 }
 const bool damaged=!IsGreenHP(),online=IsPowerOnline();
 if(IsDamaged!=damaged||WasOnline!=online){IsDamaged=damaged;WasOnline=online;UpdateAnimations();game::map_object_changed();}
 // Construction and selling are intentionally offline to combat queries,
 // but their animation timer still advances in the original Animation_AI.
 const bool stage_changed=Animation.Update();
 const auto mission=GetCurrentMission();
 if(!HasTurret()||mission==Mission::Construction||mission==Mission::Selling){
  if(stage_changed&&BState>=0&&BState<6){
   const auto& sequence=Type->BuildingAnimFrame[BState];const int start=int(sequence.dwUnknown);
   if(Animation.Value==start+sequence.FrameCount-1||(!ArchiveTarget&&Type->UndeploysInto&&mission==Mission::Selling&&Animation.Value==23))IsReadyToCommence=true;
   if(Animation.Value>=start+sequence.FrameCount){
    Animation.Start(BState<2?GameOptionsClass::Instance.GetAnimSpeed(sequence.FrameDuration):sequence.FrameDuration);
    Animation.Value=start;
   }
   NeedsRedraw=true;game::map_object_changed();
  }else if(BState==-1||!Animation.Rate)IsReadyToCommence=true;
 }
 if(QueueBState>=0&&QueueBState<6){
  if(BState!=QueueBState){
   BState=QueueBState;const auto& sequence=Type->BuildingAnimFrame[BState];
   Animation.Start(BState<2?GameOptionsClass::Instance.GetAnimSpeed(sequence.FrameDuration):sequence.FrameDuration);
   Animation.Value=int(sequence.dwUnknown);NeedsRedraw=true;game::map_object_changed();
  }
  QueueBState=-1;
 }
 if(UnloadTimer.State1){
  if(UnloadTimer.IsTimerFinished())UnloadTimer.SetToDone();
  NeedsRedraw=true;game::map_object_changed();
 }
 update_turret_frame(*this);
 if(Type->CanBeOccupied)UpdateGarrison();
 EstimatedHealth=Health;
}
