// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp::Assign_Target; YR 0x0051B1F0 / 0x00522CB0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include <cmath>

AbstractClass* InfantryClass::GreatestThreat(ThreatType threat,CoordStruct* origin,bool onlyEnemy) {
    unsigned flags=unsigned(threat);
    const auto close=[&](AbstractClass* object) {
        auto a=GetCoords(),b=object->GetCoords();
        const double x=double(a.X)-b.X,y=double(a.Y)-b.Y,z=double(a.Z)-b.Z;
        return int(std::sqrt(x*x+y*y+z*z))<3840;
    };
    if(!Owner->IsControlledByHuman()&&Type->Engineer) {
        if(Owner->ToCapture&&close(Owner->ToCapture))return Owner->ToCapture;
        flags|=0x200;
    }
    if(Type->VehicleThief&&Destination&&(Destination->AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None) {
        auto* object=static_cast<TechnoClass*>(Destination);
        if(object->IsStrange()&&!object->GetTechnoType()->IsTrain&&close(object))return object;
    }
    if(!IsArmed()) {
        if(!Type->Infiltrate&&!Type->VehicleThief)return nullptr;
        if(Type->VehicleThief)flags=(flags&0xFFFFFF4B)|0x10;
    }
    if(!(flags&0x1B978))for(int i=0;i<2;++i)if(auto* weapon=GetWeapon(i)->WeaponType)flags|=unsigned(weapon->AllowedThreats());
    if(IsArmed()&&GetWeapon(0)->WeaponType->Warhead->unknown_bool_149)flags&=0xFFFFFF4B;
    if((Type->C4||HasAbility(Ability::C4))&&!Owner->IsControlledByHuman())flags|=0x20;
    if(Type->Thief)flags|=0x240;
    return FootClass::GreatestThreat(ThreatType(flags),origin,onlyEnemy);
}
int InfantryClass::SelectWeapon(AbstractClass* target) const {
    if(!Type->DeployFire)return TechnoClass::SelectWeapon(target);
    if(SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle)return Type->DeployFireWeapon;
    if(InOpenToppedTransport && GetTechnoType()->OpenTransportWeapon!=-1)return GetTechnoType()->OpenTransportWeapon;
    return 0;
}
bool InfantryClass::IsDisguisedAs(HouseClass* house) const {
    if (!IsDisguised()) return false;
    const auto coord = GetCoords();
    const CellStruct cell{static_cast<short>(coord.X / 256),static_cast<short>(coord.Y / 256)};
    if (Owner->IsAlliedWith(house) || MapClass::Instance.GetCellAt(cell)->DisguiseSensors_InclHouse(house->ArrayIndex))
        return false;
    return DisguisedAsHouse == house || !DisguisedAsHouse || house->IsAlliedWith(DisguisedAsHouse);
}
bool InfantryClass::IsPlayingDeathSequence() const {
    const int action = static_cast<int>(SequenceAnim);
    return (action >= 11 && action <= 15) || (action >= 34 && action <= 36) || action == 20 || action == 21;
}
void InfantryClass::SetTarget(AbstractClass* target) {
    auto deployed = [&] { const int action = static_cast<int>(SequenceAnim); return action >= 27 && action <= 30; };
    if (target != Target) {
        if (Health > 0) {
            IsFiring = false;
            PlayAnim(deployed() ? Sequence::Deployed : Crawling ? Sequence::Prone : Sequence::Ready);
        }
        if (target != Target && DirectRockerLinkedUnit) {
            DirectRockerLinkedUnit->DirectRockerLinkedUnit = nullptr;
            DirectRockerLinkedUnit = nullptr;
        }
    }
    if (!deployed() || (Type && Type->DeployFire)) {
        PathDirections[0] = -1;
        TechnoClass::SetTarget(target);
        if (!Destination && Type && Type->Infiltrate && target && target->WhatAmI() == AbstractType::Building) {
            auto* building = static_cast<BuildingClass*>(target);
            if (building->Type && (Type->Agent ? building->Type->Spyable : building->Type->Capturable))
                SetDestination(target, true);
        }
        unknown_bool_68E = false;
    }
}
