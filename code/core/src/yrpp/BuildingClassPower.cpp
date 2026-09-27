#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SuperClass.h"
#include "x87_integer.hpp"
#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif
bool BuildingClass::IsBeingDrained() const { return DrainingMe!=nullptr; }
int BuildingClass::FirstActiveSWIdx() const {
 const int index=Type->SuperWeapon;
 if(index==-1)return index;
 const auto* auxiliary=SuperClass::Array[index]->Type->AuxBuilding;
 return !auxiliary||Owner->OwnedBuildingTypes.GetItemCount(auxiliary->ArrayIndex)?index:-1;
}
int BuildingClass::SecondActiveSWIdx() const {
 const int index=Type->SuperWeapon2;
 if(index==-1)return index;
 const auto* auxiliary=SuperClass::Array[index]->Type->AuxBuilding;
 return !auxiliary||Owner->OwnedBuildingTypes.GetItemCount(auxiliary->ArrayIndex)?index:-1;
}
// 0x004555D0: the original predicate used by defense targeting and firing.
bool BuildingClass::IsPowerOnline() const {
 if(!Type||(!HasPower&&Overpowerers.Count<2)||EMPLockRemaining>0||Health==0)return false;
 if(Type->Powered&&Type->PowerDrain>0&&Owner&&Owner->GetPowerPercentage()<1.0&&Overpowerers.Count<2)return false;
 if(Type->PoweredSpecial&&Owner&&(Owner->PowerBlackoutTimer.GetTimeLeft()||Owner->IsBeingDrained))return false;
 const auto mission=GetCurrentMission();
 return (!Type->NeedsEngineer||HasEngineer)&&mission!=Mission::Construction&&mission!=Mission::Selling;
}
// 0x0044E7B0 / 0x0044E880, normal building and upgrade paths.
int BuildingClass::GetPowerOutput() const {
 if(!Type||IsBeingWarpedOut())return 0;
 DWORD power=static_cast<DWORD>(Type->PowerBonus);
 if(HasExtraPowerBonus)power+=static_cast<DWORD>(Type->ExtraPowerBonus);
 if((Type->UnitAbsorb||Type->InfantryAbsorb)&&Type->ExtraPowerBonus>0&&Passengers.NumPassengers>0)
  power+=static_cast<DWORD>(Passengers.NumPassengers)*static_cast<DWORD>(Type->ExtraPowerBonus);
 if(UpgradeLevel)for(auto* upgrade:Upgrades)if(upgrade)power+=static_cast<DWORD>(upgrade->PowerBonus);
 const int signedPower=std::bit_cast<int>(power);
 return signedPower>0&&HasPower?game::x87_integer(GetHealthPercentage()*signedPower):0;
}
int BuildingClass::GetPowerDrain() const {
 if(!Type||IsBeingWarpedOut()||!HasPower)return 0;
 DWORD power=static_cast<DWORD>(Type->PowerDrain);
 if(HasExtraPowerDrain)power+=static_cast<DWORD>(Type->ExtraPowerDrain);
 if(UpgradeLevel)for(auto* upgrade:Upgrades)if(upgrade)power+=static_cast<DWORD>(upgrade->PowerDrain);
 return std::bit_cast<int>(power);
}
