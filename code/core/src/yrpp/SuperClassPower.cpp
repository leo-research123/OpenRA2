#include "yrpp/SuperClass.h"

namespace { DynamicVectorClass<SuperClass*> supers,show_timers; }
DynamicVectorClass<SuperClass*>& SuperClass::Array=supers;
DynamicVectorClass<SuperClass*>& SuperClass::ShowTimers=show_timers;

bool SuperClass::SetOnHold(bool hold) {
    if(!IsPresent||IsOneTime||hold==IsSuspended||!CanHold)return false;
    if(hold||Type->ManualControl)RechargeTimer.Pause();
    else RechargeTimer.Resume();
    IsSuspended=hold;
    return true;
}

bool SuperClass::Lose() {
    if(!IsPresent)return false;
    IsReady=false;IsPresent=false;
    ShowTimers.Remove(this);
    return true;
}
