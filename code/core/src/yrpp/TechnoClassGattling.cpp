// YR-specific Gattling state machine, calibrated to 0x70DE70 / 0x70E000.
// OpenTS 44fac744 techno.cpp has no Gattling counterpart.
#include "yrpp/TechnoClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/Randomizer.h"
#include "yrpp/VocClass.h"
#include "type_resources.hpp"

void TechnoClass::GattlingRateUp(int value) {
    const auto* type=GetTechnoType();
    const int stages=type->WeaponStages;
    if(stages<=0||stages>6)return;
    const auto* thresholds=Veterancy.IsElite()?type->EliteStage:type->WeaponStage;
    // The binary tests the value from BEFORE this call's increment. Crossing
    // a threshold takes effect on the next call, at most one stage per call.
    const int previous=GattlingValue;
    if(previous<thresholds[stages-1])GattlingValue+=value*type->RateUp;
    int stage=CurrentGattlingStage;
    auto* weapon=GetWeapon(2*stage)->WeaponType;
    UnusedGattlingSoundController.Stop();IsUnusedGattlingSoundPlaying=false;
    if(stage>=0&&stage<stages-1&&previous>=thresholds[stage]) {
        SetCurrentWeaponStage(++stage);
        weapon=GetWeapon(2*stage)->WeaponType;
        GattlingSoundController.Stop();IsGattlingSoundPlaying=false;
    }
    if(!IsGattlingSoundPlaying&&weapon&&weapon->Report.Count>0) {
        GattlingSoundController.Stop();
        const auto index=unsigned(Randomizer::Global.Random())%unsigned(weapon->Report.Count);
        if(!game::type_resources().audio_unavailable)
            VocClass::PlayAt(weapon->Report[index],Location,&GattlingSoundController);
        IsGattlingSoundPlaying=true;
    }
}

void TechnoClass::GattlingRateDown(int value) {
    GattlingSoundController.EndLooping();IsGattlingSoundPlaying=false;
    const auto* type=GetTechnoType();
    const int decrease=value*type->RateDown;
    GattlingValue-=decrease;
    if(GattlingValue<0||!decrease)GattlingValue=0;
    const int stage=CurrentGattlingStage;
    if(GattlingValue||stage) {
        const auto* thresholds=Veterancy.IsElite()?type->EliteStage:type->WeaponStage;
        if(stage>0&&stage<=6&&GattlingValue<thresholds[stage-1]) {
            SetCurrentWeaponStage(stage-1);
            UnusedGattlingSoundController.Stop();IsUnusedGattlingSoundPlaying=false;
        }
    }else if(IsUnusedGattlingSoundPlaying||IsGattlingSoundPlaying) {
        GattlingSoundController.Stop();UnusedGattlingSoundController.Stop();
        IsUnusedGattlingSoundPlaying=false;IsGattlingSoundPlaying=false;
    }
}
