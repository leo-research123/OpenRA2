// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 bullet.cpp Bullet_Explodes; YR 0x468D80.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BulletClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/VoxelAnimClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/BombListClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/TemporalClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/VocClass.h"
#include "type_resources.hpp"
#include "RulesClassReaders.hpp"
#include <bit>
#include <algorithm>

namespace {
template<class T,class... A>T* effect(A&&... args) {
    auto* memory=YRMemory::Allocate(sizeof(T));return memory?::new(memory) T(std::forward<A>(args)...):nullptr;
}
}

void BulletClass::Detonate(const CoordStruct& coords) {
    auto& rules=*RulesClass::Instance;auto& map=MapClass::Instance;
    if(WH->ShakeXlo || WH->ShakeXhi)map.ScreenShakeX=Randomizer::Global.RandomRanged(WH->ShakeXlo,WH->ShakeXhi);
    if(WH->ShakeYlo || WH->ShakeYhi)map.ScreenShakeY=Randomizer::Global.RandomRanged(WH->ShakeYlo,WH->ShakeYhi);
    if(WeaponType && WeaponType->RadLevel>0)
        ApplyRadiationToCell(CellClass::Coord2Cell(coords),rule_integer(WeaponType->Warhead->CellSpread),WeaponType->RadLevel);
    auto* object=Target && (Target->AbstractFlags & AbstractFlags::Object)!=AbstractFlags::None?static_cast<ObjectClass*>(Target):nullptr;
    auto* techno=Target && (Target->AbstractFlags & AbstractFlags::Techno)!=AbstractFlags::None?static_cast<TechnoClass*>(Target):nullptr;
    auto* foot=Target && (Target->AbstractFlags & AbstractFlags::Foot)!=AbstractFlags::None?static_cast<FootClass*>(Target):nullptr;
    auto result=DamageAreaResult::Missed;
    const int damage=std::bit_cast<int>(unsigned(Health)*unsigned(DamageMultiplier))>>8;
    if(WH->MindControl) {
        if(Owner && Owner->CaptureManager) {
            const bool player=techno && techno->Owner->IsControlledByCurrentPlayer();
            if(object && object->IsAlive && object->AttachedTag) {
                object->AttachedTag->RaiseEvent(TriggerEvent::AttackedByAnybody,object,CellStruct::Empty,false,Owner);
                object->AttachedTag->RaiseEvent(TriggerEvent::AttackedByHouse,object,CellStruct::Empty,false,Owner);
            }
            if(Owner->CaptureManager->CaptureUnit(techno) && !game::type_resources().audio_unavailable && rules.YuriMindControlSound!=-1
                && (Owner->Owner->IsControlledByCurrentPlayer() || player))VocClass::PlayAt(rules.YuriMindControlSound,object->Location,nullptr);
        }
    }else if(WH->IvanBomb)BombListClass::Instance.Plant(Owner,techno);
    else if(WH->ElectricAssault) {
        if(Target && Target->WhatAmI()==AbstractType::Building && Owner && Owner->WhatAmI()==AbstractType::Infantry)
            static_cast<BuildingClass*>(Target)->AddOverpowerer(static_cast<InfantryClass*>(Owner));
    }else if(WH->Parasite) {
        if(Owner)static_cast<FootClass*>(Owner)->ParasiteImUsing->TryInfect(foot);
    }else if(WH->Temporal) {
        if(Owner && Target && (!foot || foot->InWhichLayer()==Layer::Ground)) {
            if(techno && techno->BunkerLinkedItem && techno->WhatAmI()==AbstractType::Unit){techno=techno->BunkerLinkedItem;Owner->SetTarget(techno);}
            Owner->TemporalImUsing->Fire(techno);
        }
    }else if(WH->IsLocomotor) {
        if(Owner && Owner->LocomotorTarget!=Target && !Owner->LocomotorSource
            && ((Owner->AbstractFlags & AbstractFlags::Foot)==AbstractFlags::None || !static_cast<FootClass*>(Owner)->IsAttackedByLocomotor)) {
            bool docked=false;
            if(techno && techno->WhatAmI()==AbstractType::Unit) {
                auto* link=techno->GetNthLink();
                if(link && link->WhatAmI()==AbstractType::Building && static_cast<BuildingClass*>(link)->Type->WeaponsFactory
                    && techno->GetCell()->GetBuilding()==link)docked=true;
            }
            if(Owner->LocomotorTarget)Owner->ReleaseLocomotor(false);
            if(Target && !docked && (!foot || foot->InWhichLayer()==Layer::Ground) && foot && !foot->IsIronCurtained()
                && (foot->WhatAmI()==AbstractType::Unit || foot->WhatAmI()==AbstractType::Aircraft)
                && !foot->IsAttackedByLocomotor && double(Health)>foot->GetTechnoType()->Weight)Owner->ImbueLocomotor(foot,WH->Locomotor);
        }
    }else if(WH->Airstrike) {
        if(Owner && techno && Owner->Airstrike && Owner->Airstrike->CanTarget(techno))Owner->Airstrike->StartMission(techno);
    }else if(WH->DirectRocker && Target && Target->WhatAmI()==AbstractType::Unit) {
        if(Owner && techno && !techno->IsIronCurtained()) {
            auto at=techno->Location;
            float x=float(Owner->Location.X-at.X),y=float(Owner->Location.Y-at.Y),z=float(Owner->Location.Z-at.Z);
            const float magnitude=float(Math::sqrt(x*x+y*y+z*z));
            if(magnitude){x/=magnitude;y/=magnitude;z/=magnitude;}
            at.X+=rule_integer(x*10.0);at.Y+=rule_integer(y*10.0);at.Z+=rule_integer(z*10.0);
            techno->vt_entry_3D8(&at,float(std::min(double(damage)*rules.DirectRockingCoefficient/100.0,4.0)),true);
            techno->DirectRockerLinkedUnit=static_cast<FootClass*>(Owner);Owner->DirectRockerLinkedUnit=static_cast<FootClass*>(techno);
        }
    }else if(WH->BombDisarm) {if(object && object->AttachedBomb)object->AttachedBomb->Disarm();}
    else if(WH->MakesDisguise) {if(Owner)Owner->DisguiseAs(Target);}
    else if(WH->NukeMaker)NukeMaker();
    else {
        if(Type->ShrapnelWeapon)Shrapnel();
        result=MapClass::DamageArea(coords,damage,Owner,WH,true,Owner?Owner->Owner:nullptr);
        if(!IsAlive)return;
    }
    auto at=coords;if(Type->Inviso)at=MapClass::GetRandomCoordsNear(at,32,false);
    auto land=static_cast<LandType>(-1);
    if(GetHeight()<2*Unsorted::LevelHeight) {
        auto* cell=map.GetCellAt(Location);land=cell->LandType;
        if(land==LandType::Water && result==DamageAreaResult::Hit && cell->FirstObject && cell->FirstObject->WhatAmI()==AbstractType::Unit) {
            auto* type=static_cast<UnitClass*>(cell->FirstObject)->Type;
            if(type->Naval && !type->Underwater)land=static_cast<LandType>(-1);
        }
    }
    auto* animation=MapClass::SelectDamageAnimation(damage,WH,land,Location);
    if(result==DamageAreaResult::Nullified) {
        if(rules.WeaponNullifyAnim)effect<AnimClass>(rules.WeaponNullifyAnim,at,0,1,0x2600,-15,false);
        return;
    }
    if(Bright) {
        const unsigned flags=(WH->CLDisableRed?2u:0u)|(WH->CLDisableGreen?4u:0u)|(WH->CLDisableBlue?8u:0u);
        MapClass::FlashbangWarheadAt(damage,WH,at,true,static_cast<SpotlightFlags>(flags));
    }
    if(animation && !effect<AnimClass>(animation,at,0,1,0x2600,-15,false) && WH==rules.NukeWarhead) {
        const auto cell=CellClass::Coord2Cell(at);AnimClass::ApplyNukeDamage(nullptr,&cell);
    }
    auto& random=ScenarioClass::Instance->Random;
    if(WH->MaxDebris>0) {
        int remaining=random.RandomRanged(WH->MinDebris,WH->MaxDebris-1);
        for(int index=0;WH->DebrisTypes.Count && remaining>0;index=(index+1)%WH->DebrisTypes.Count) {
            const int value=random.Random();const unsigned absolute=value<0?0u-unsigned(value):unsigned(value);
            const int count=std::min(int(absolute%unsigned(WH->DebrisMaximums[index]+1)),remaining);
            for(int i=0;i<count;++i){auto position=GetCoords();effect<VoxelAnimClass>(WH->DebrisTypes[index],&position,nullptr);}
            remaining-=count;
        }
        if(!WH->DebrisTypes.Count)for(int i=0;i<remaining;++i) {
            auto position=GetCoords();position.Z+=20;
            effect<AnimClass>(rules.MetallicDebris[random.RandomRanged(0,rules.MetallicDebris.Count-1)],position,0,1,0x600,0,false);
        }
    }
    if(Type->Airburst) {
        auto* weapon=Type->AirburstWeapon;auto* center=GetCell();
        const auto spawn=[&](CellClass* target) {
            auto* bullet=weapon->Projectile->CreateBullet(target,Owner,weapon->Damage,weapon->Warhead,50,false);
            if(!bullet)return;
            const auto yaw=static_cast<unsigned short>(random.RandomRanged(0,32)<<8);
            const double angle=(int(std::bit_cast<short>(yaw))-0x3FFF)*-0.00009587672516830327;
            const double magnitude=double(weapon->Speed/10),pitch=4.712436918747274;
            const BulletVelocity velocity{Math::cos(angle)*Math::cos(pitch)*magnitude,
                Math::sin(angle)*Math::cos(pitch)*magnitude,Math::sin(pitch)*magnitude};
            bullet->MoveTo(Location,velocity);
        };
        for(int direction=0;direction<8;++direction)spawn(center->GetNeighbourCell(static_cast<FacingType>(direction)));
        spawn(center);
    }
}

void BulletClass::Explode(bool forced) {
    auto at=Location;
    auto* airborne=Target && Target->IsInAir()?static_cast<ObjectClass*>(Target):nullptr;
    if(!Type->Inaccurate) {
        if(Target) {
            const auto target=Target->GetCoords();
            const auto delta=[](int a,int b){return std::bit_cast<int>(unsigned(a)-unsigned(b));};
            const int x=delta(at.X,target.X),y=delta(at.Y,target.Y),z=delta(at.Z,target.Z);
            if(rule_integer(Math::sqrt(double(x)*x+double(z)*z+double(y)*y))<32 && !Type->Airburst && !Type->Inaccurate)
                at=Target->GetCoords();
        }
        if(!WH->EMEffect && !Type->Airburst) {
            if(!forced && !Type->Arcing && !IsHoming() && Data.Location!=CoordStruct::Empty)at=Data.Location;
            if(airborne && airborne->InWhichLayer()!=Layer::Ground) {
                if(DistanceFrom3D(Target)<128)airborne->GetTargetCoords(&at);
            }else if(Target && DistanceFrom3D(Target)<42) {
                at=Target->GetCenterCoords();
                if(Target->WhatAmI()==AbstractType::Building) {
                    auto* building=static_cast<BuildingClass*>(Target);
                    const auto offset=building->Type->TargetCoordOffset;
                    if(offset.X || offset.Y || offset.Z)building->GetTargetCoords(&at);
                }
            }
        }
    }
    if(Type->Airburst){Detonate(at);return;}
    const auto origin=at;
    for(int i=0;i<Type->Cluster;++i) {
        Detonate(at);
        if(!IsAlive)break;
        const int scatter=ScenarioClass::Instance->Random.RandomRanged(256,512);
        at=MapClass::GetRandomCoordsNear(origin,scatter,false);
    }
}
