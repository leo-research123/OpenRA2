// Original constructor/destructor and pointer expiry: 0x6CAF90/0x6CB120/0x6CDFF0.
#include "yrpp/SuperClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Notifications.h"
#include <cstdlib>

namespace {
DynamicVectorClass<SuperClass*> supers, show_timers;
}
DynamicVectorClass<SuperClass*>& SuperClass::Array = supers;
DynamicVectorClass<SuperClass*>& SuperClass::ShowTimers = show_timers;

SuperClass::SuperClass(SuperWeaponTypeClass* type, HouseClass* owner) noexcept
    : AbstractClass(), CustomChargeTime(-1), Type(type), Owner(owner), RechargeTimer(0), unused_3C{}, BlinkState(false),
      unused_41{}, BlinkTimer{}, SpecialSoundDuration(-1), SpecialSoundLocation{}, CanHold(true), unused_61{},
      ChronoMapCoords(CellStruct::Empty), unused_66{}, Animation(nullptr), AnimationGotInvalid(false), IsPresent(false),
      IsOneTime(false), IsReady(false), IsSuspended(false), unused_71{}, ReadyFrame(-100), CameoChargeState(-1),
      ChargeDrainState(ChargeDrainState::None) {
    Create_ID();
    if (!Array.AddItem(this) || !TypeExpirationListeners.AddItem(this)) std::abort();
    PointerExpiredNotification::NotifyInvalidHouse.Array.AddItem(this);
}
SuperClass::~SuperClass() {
    if (Animation) {
        Animation->RemainingIterations = 0;
        Animation = nullptr;
        SuperClass::AnimExpirationListeners.Remove(this);
    }
    if (AnimationGotInvalid) {
        SuperClass::AnimExpirationListeners.Remove(this);
        AnimationGotInvalid = false;
    }
    ShowTimers.Remove(this);
    Array.Remove(this);
    TypeExpirationListeners.Remove(this);
    PointerExpiredNotification::NotifyInvalidHouse.Remove(this);
}
void SuperClass::PointerExpired(AbstractClass* object, bool) {
    if (Owner == object) Owner = nullptr;
    if (Type == object) Type = nullptr;
    if (Animation == object) {
        Animation = nullptr;
        AnimationGotInvalid = true;
    }
}
