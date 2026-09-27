// SPDX-License-Identifier: GPL-3.0-or-later
// Extends the pinned OpenTS 44fac744 cargo path with YR's gunner callbacks.
// YR 0x746420 / 0x7464E0 / 0x70DC70; EA terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/TemporalClass.h"
#include <bit>

namespace {
void set_mode(UnitClass& unit,unsigned mode) {
    // 0x70DC70 accepts all 18 slots, irrespective of WeaponCount, and
    // preserves charge-turret state. Weapon number and model are distinct.
    if(unit.Type->IsChargeTurret)return;
    if(mode>=18)mode=0;
    unit.CurrentWeaponNumber=int(mode);
    unit.CurrentTurretNumber=unit.Type->TurretWeapon[mode];
}
void rearm(TechnoClass& unit,int weapon) {
    unit.CurrentBurstIndex=std::bit_cast<int>(unsigned(unit.CurrentBurstIndex)+1u);
    unit.RearmTimer.Start(unit.GetROF(weapon));
}
}

void UnitClass::ReceiveGunner(FootClass* gunner) {
    if(gunner->TemporalImUsing) {
        const bool armed=gunner->RearmTimer.GetTimeLeft()!=0;
        TemporalImUsing=gunner->TemporalImUsing;TemporalImUsing->Owner=this;
        gunner->TemporalImUsing=nullptr;
        set_mode(*this,7);
        if(armed)rearm(*this,7);
    }
    set_mode(*this,unsigned(gunner->GetTechnoType()->IFVMode));
}

void UnitClass::RemoveGunner(FootClass* gunner) {
    if(gunner&&TemporalImUsing) {
        const bool armed=RearmTimer.GetTimeLeft()!=0;
        gunner->TemporalImUsing=TemporalImUsing;TemporalImUsing->Owner=gunner;
        TemporalImUsing=nullptr;
        if(gunner->TemporalImUsing->Target)gunner->TemporalImUsing->LetGo();
        if(armed)rearm(*gunner,0);
    }
    set_mode(*this,0);
}
