#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "map_world.hpp"
#include "scenario_runtime.hpp"
#include "yrpp/RadarClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/FactoryClass.h"
#include <algorithm>
#include <limits>
long YRPP_STDCALL HouseClass::Power_Output() const { return PowerOutput; }
long YRPP_STDCALL HouseClass::Power_Drain() const { return PowerDrain; }
double HouseClass::GetPowerPercentage() const {
 if(PowerOutput>=PowerDrain||!PowerDrain)return 1.0;
 return PowerOutput?double(PowerOutput)/PowerDrain:0.0;
}
void HouseClass::CreatePowerOutage(int duration) {
 RecheckPower=true;
 PowerBlackoutTimer.Start(duration);
}
void HouseClass::CreateRadarOutage(int duration) {
 RecheckRadar=true;
 RadarBlackoutTimer.Start(duration);
}
// The existing native placement path has no COM power-event subscribers.
// Its gain/loss invalidation is separate from the recovered UpdatePower body.
void HouseClass::AddPowerDrain(int amount){PowerDrain+=amount;RecheckPower=true;}
// 0x00508C30: accounting, production-rate refresh, superweapon transition,
// then radar invalidation. The original 0x00454CE0 callee is a nullsub.
void HouseClass::UpdatePower(){
 const int old_output=PowerOutput,old_drain=PowerDrain;
 const bool wasLow=GetPowerPercentage()<1.0;
 RecheckPower=false;PowerOutput=PowerDrain=0;
 bool drained=false;
 const auto& runtime=game::scenario_runtime();
 const bool campaign=runtime.session_mode(runtime.context)==int(GameMode::Campaign);
 const int count=Buildings.Count;
 for(int i=0;i<count;++i)if(auto* building=Buildings[i];building&&!building->InLimbo&&building->IsOnMap){
  if(campaign&&(IsHumanPlayer||IsInPlayerControl)&&!building->DiscoveredByCurrentPlayer)continue;
  PowerOutput=std::bit_cast<int>(static_cast<DWORD>(PowerOutput)+static_cast<DWORD>(building->GetPowerOutput()));
  PowerDrain=std::bit_cast<int>(static_cast<DWORD>(PowerDrain)+static_cast<DWORD>(building->GetPowerDrain()));
  if(building->IsBeingDrained()&&building->GetPowerOutput()>0)drained=true;
 }
 IsBeingDrained=drained;
 if(PowerBlackoutTimer.GetTimeLeft()||drained)PowerOutput=0;
 const int bonus=game::active_map_player_power_bonus(this);
 if(bonus>0)PowerOutput=static_cast<int>(std::clamp(std::int64_t(PowerOutput)+bonus,
                                          std::int64_t(0),std::int64_t(std::numeric_limits<int>::max())));
 FactoryClass::UpdateBuildSpeed(this);
 if(wasLow!=(GetPowerPercentage()<1.0))UpdateSuperWeapons();
 RecheckRadar=true;
 if(old_output!=PowerOutput||old_drain!=PowerDrain)game::map_object_changed();
}

void HouseClass::UpdateRadarAvailability() {
 RecheckRadar=false;
 if(this!=CurrentPlayer)return;
 bool available=false;
 if(!RadarBlackoutTimer.GetTimeLeft()) {
  if(ScenarioClass::Instance->FreeRadar)available=true;
  else if(GetPowerPercentage()>=1.0) {
   const auto& runtime=game::scenario_runtime();
   const bool campaign=runtime.session_mode(runtime.context)==int(GameMode::Campaign);
   for(auto* building:Buildings) {
    if(!building||!building->HasPower||!building->Type->Radar||building->InLimbo||!building->IsOnMap)continue;
    if(campaign&&(IsHumanPlayer||IsInPlayerControl)&&!building->DiscoveredByCurrentPlayer)continue;
    if(building->CurrentMission==Mission::Selling||building->QueuedMission==Mission::Selling)continue;
    // 0x00508F12 tests EMP and warp after selecting the first candidate.
    // An EMP'd first radar does not fall through to another radar building.
    available=!building->EMPLockRemaining&&!building->IsBeingWarpedOut();
    break;
   }
  }
 }
 if(RadarClass::Instance.IsAvailableNow!=available)RadarClass::Instance.SetRadarAvailability(available);
}
