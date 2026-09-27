// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 combat.cpp Explosion_Damage / Modify_Damage /
// Combat_Anim, calibrated to YR 0x489280 / 0x489180 / 0x48A4F0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/AircraftTrackerClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/VoxelAnimClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/GameOptionsClass.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
#include <bit>
#include <vector>

namespace {
int distance_from(const CoordStruct& a,const CoordStruct& b) {
    const auto delta=[](int x,int y){return std::bit_cast<int>(unsigned(x)-unsigned(y));};
    const int x=delta(a.X,b.X),y=delta(a.Y,b.Y),z=delta(a.Z,b.Z);
    return rule_integer(Math::sqrt(double(x)*x+double(y)*y+double(z)*z));
}
template<class T,class... A>T* create_effect(A&&... args) {
    auto* memory=YRMemory::Allocate(sizeof(T));
    return memory?::new(memory) T(std::forward<A>(args)...):nullptr;
}
}

// OpenTS combat.cpp Combat_Lighting; YR 0x0048A620. The native host
// honors DetailLevel; the EXE's adaptive measured-FPS gate is not exposed.
void YRPP_FASTCALL MapClass::FlashbangWarheadAt(int damage,WarheadTypeClass* warhead,
        CoordStruct at,bool force,SpotlightFlags flags){
 if(!GameOptionsClass::Instance.DetailLevel&&(unsigned(flags)&0xFu)==0)return;
 if(!force&&(!warhead||!warhead->Bright))return;
 int size=std::clamp(int(static_cast<signed char>(damage>>2)),21,63);
 if(warhead&&warhead->CombatLightSize>0)size=rule_integer(std::min(double(warhead->CombatLightSize),1.0)*63.0);
 if(auto* light=create_effect<SpotlightClass>(at,size))light->DisableFlags|=flags;
}

DamageAreaResult YRPP_FASTCALL MapClass::DamageArea(const CoordStruct& at,int damage,TechnoClass* source,
        WarheadTypeClass* warhead,bool affectsTiberium,HouseClass* sourceHouse) {
    auto& scenario=*ScenarioClass::Instance;auto& rules=*RulesClass::Instance;auto& map=Instance;
    if(!damage || scenario.SpecialFlags.Inert || !warhead)return DamageAreaResult::Missed;
    const int spread=rule_integer(double(warhead->CellSpread)*256.0);
    const auto center=CellClass::Coord2Cell(at);
    auto* impact=map.GetCellAt(center);
    const bool small_spread=warhead->CellSpread<=0.5f;
    bool nullified=false,hit=false;
    struct DamageGroup { ObjectClass* object;int distance; };
    std::vector<DamageGroup> groups;
    const auto iron=[](ObjectClass* object) {
        return (object->AbstractFlags & AbstractFlags::Techno)!=AbstractFlags::None && object->IsIronCurtained();
    };
    auto ground=impact->GetCoords();ground.Z=0;
    if(map.GetCellFloorHeight(ground)<at.Z) {
        auto& tracker=AircraftTrackerClass::Instance;
        tracker.FillCurrentVector(impact,rule_integer(warhead->CellSpread));
        while(auto* object=tracker.Get())if(object->IsAlive && object->IsOnMap && object->Health>0) {
            const int distance=distance_from(at,object->Location);
            if(distance>spread)continue;
            if(small_spread && distance<85 && object->IsIronCurtained() && !object->ForceShielded)nullified=true;
            groups.push_back({object,distance});
        }
    }
    const bool bridge=impact->ContainsBridgeEx() && at.Z>map.GetCellFloorHeight(at)+CellClass::BridgeHeight/2;
    const auto count=CellSpread::NumCells(rule_integer(double(warhead->CellSpread)+0.99));
    for(std::size_t index=0;index<count;++index) {
        const auto delta=CellSpread::GetCell(index);
        auto* cell=map.GetCellAt(CellStruct{short(center.X+delta.X),short(center.Y+delta.Y)});
        if(cell->OverlayTypeIndex!=-1) {
            auto* overlay=OverlayTypeClass::Array[cell->OverlayTypeIndex];
            if(overlay->ChainReaction && (!overlay->Tiberium || warhead->Tiberium) && affectsTiberium)cell->ReduceTiberium(damage/10);
            if(overlay->Wall) {
                if(warhead->WallAbsoluteDestroyer)cell->DamageWall(-1);
                else if(warhead->Wall || (warhead->Wood && overlay->Armor==Armor::Wood))cell->DamageWall(damage);
            }
            if(cell->OverlayTypeIndex==-1)cell->BecomeUntargetable();
        }
        for(auto* object=bridge?cell->AltObject:cell->FirstObject;object;object=object->NextObject) {
            if(!object->IsAlive || (object==source && !source->GetTechnoType()->DamageSelf && warhead!=rules.CrushWarhead))continue;
            if(object->WhatAmI()==AbstractType::Unit && scenario.SpecialFlags.HarvesterImmune) {
                bool immune=false;
                for(auto* type:rules.HarvesterUnit)if(type==object->GetType()){immune=true;break;}
                if(immune)continue;
            }
            int distance;
            if(object->WhatAmI()==AbstractType::Building) {
                const auto cellCenter=cell->GetCoords();
                distance=index?distance_from(at,cellCenter):at.Z-cellCenter.Z<=2*Unsorted::LevelHeight
                    ?0:distance_from(at,cellCenter)-2*Unsorted::LevelHeight;
            }else distance=distance_from(at,object->GetTargetCoords());
            if(small_spread && !index && distance<85 && iron(object) && !static_cast<TechnoClass*>(object)->ForceShielded)nullified=true;
            groups.push_back({object,distance});
        }
    }
    // The original collects all recipients before invoking any virtual damage
    // handler. Building entries are intentionally not deduplicated by identity.
    for(const auto& group:groups) {
        auto* object=group.object;
        if(!object->IsAlive || (object->WhatAmI()==AbstractType::Building && static_cast<BuildingClass*>(object)->Type->InvisibleInGame)
            || (nullified && !iron(object)))continue;
        int distance=group.distance;
        if(object->WhatAmI()==AbstractType::Aircraft && object->IsInAir())distance/=2;
        if(object->Health>0 && object->IsOnMap && !object->InLimbo && distance<=spread) {
            int amount=damage;
            object->ReceiveDamage(&amount,distance,warhead,source,false,false,sourceHouse);hit=true;
        }
    }
    if(nullified)return DamageAreaResult::Nullified;
    const float amplitude=float(std::min(double(damage)*0.01,4.0));
    if(warhead->Rocker && amplitude>0.3)for(int x=center.X-3;x<=center.X+3;++x)for(int y=center.Y-3;y<=center.Y+3;++y) {
        auto* cell=map.GetCellAt(CellStruct{short(x),short(y)});
        for(auto* object=bridge?cell->AltObject:cell->FirstObject;object;object=object->NextObject) {
            if((object->AbstractFlags & AbstractFlags::Foot)==AbstractFlags::None)continue;
            auto* foot=static_cast<TechnoClass*>(object);
            if(x==center.X && y==center.Y && source) {
                auto location=object->Location;
                float dx=float(source->Location.X-location.X),dy=float(source->Location.Y-location.Y),dz=float(source->Location.Z-location.Z);
                const float length=float(Math::sqrt(dx*dx+dy*dy+dz*dz));
                if(length){dx/=length;dy/=length;dz/=length;}
                location.X+=rule_integer(dx*10.0);location.Y+=rule_integer(dy*10.0);location.Z+=rule_integer(dz*10.0);
                foot->vt_entry_3D8(&location,amplitude,false);
            }else if(warhead->CellSpread>0)foot->vt_entry_3D8(&at,amplitude,false);
        }
    }
    impact=map.GetCellAt(center);
    if(scenario.SpecialFlags.DestroyableBridges && warhead->Wall) {
        const bool ion=warhead==rules.IonCannonWarhead;
        auto* owner=impact->ContainsBridgeEx()?(impact->ContainsBridge()?impact:impact->BridgeOwnerCell):nullptr;
        const auto middle=[](int tile) {
            return (tile>=IsometricTileTypeClass::BridgeMiddle1 && tile<=IsometricTileTypeClass::BridgeMiddle1+3)
                || (tile>=IsometricTileTypeClass::BridgeMiddle2 && tile<=IsometricTileTypeClass::BridgeMiddle2+3);
        };
        const bool height=!impact->ContainsBridgeEx() || (at.Z<=CellClass::BridgeHeight+Unsorted::LevelHeight*(int(impact->Level)+1)
            && at.Z>CellClass::BridgeHeight+Unsorted::LevelHeight*(int(impact->Level)-2));
        const auto damageHigh=[&](bool applicable,int dirty) {
            if(!applicable || !height || (!ion && scenario.Random.RandomRanged(1,rules.BridgeStrength)>=damage))return;
            bool destroyed=map.DamageBridgeAt(center);
            for(int i=0;!destroyed && ion && i<3;++i)destroyed=map.DamageBridgeAt(center);
            if(destroyed)impact->BecomeUntargetable();
            Point2D point;TacticalClass::Instance->CoordsToClient(&at,&point);
            TacticalClass::Instance->RegisterDirtyArea({point.X-dirty/2,point.Y-dirty/2,dirty,dirty},false);
        };
        damageHigh((owner && (owner->OverlayTypeIndex==24 || owner->OverlayTypeIndex==25))
            || middle(impact->IsoTileTypeIndex-IsometricTileTypeClass::BridgeSet+1),256);
        damageHigh((owner && (owner->OverlayTypeIndex==237 || owner->OverlayTypeIndex==238))
            || middle(impact->IsoTileTypeIndex-IsometricTileTypeClass::WoodBridgeSet+1),192);
        if(impact->OverlayTypeIndex>=74 && impact->OverlayTypeIndex<=99
            && (ion || scenario.Random.RandomRanged(1,rules.BridgeStrength)<damage) && map.DamageLowBridgeAt(center))impact->BecomeUntargetable();
        if(impact->OverlayTypeIndex>=205 && impact->OverlayTypeIndex<=230
            && (ion || scenario.Random.RandomRanged(1,rules.BridgeStrength)<damage) && map.DamageLowWoodBridgeAt(center))impact->BecomeUntargetable();
    }
    if(impact->OverlayTypeIndex!=-1 && OverlayTypeClass::Array[impact->OverlayTypeIndex]->Explodes) {
        impact->MarkForRedraw();impact->OverlayTypeIndex=-1;impact->RecalcAttributes(-1);
        map.ResetZones(center);map.RecalculateSubZones(center);impact->BecomeUntargetable();
        create_effect<AnimClass>(rules.BarrelExplode,at,0,1,0x600,0,false);
        DamageArea(at,rules.AmmoCrateDamage,nullptr,rules.C4Warhead,true,sourceHouse);
        for(auto* debris:rules.BarrelDebris)if(scenario.Random.RandomRanged(0,99)<15) {
            auto location=at;create_effect<VoxelAnimClass>(debris,&location,nullptr);break;
        }
        if(scenario.Random.RandomRanged(0,99)<25)if(auto* particle=create_effect<ParticleSystemClass>(rules.BarrelParticle,at,nullptr,nullptr,CoordStruct::Empty,nullptr))
            particle->SpawnParticle(at,at);
    }
    if(warhead->Particle)if(auto* particle=create_effect<ParticleSystemClass>(warhead->Particle,at,nullptr,nullptr,CoordStruct::Empty,sourceHouse))
        particle->SpawnParticle(at,at);
    return hit?DamageAreaResult::Hit:DamageAreaResult::Missed;
}

AnimTypeClass* YRPP_FASTCALL MapClass::SelectDamageAnimation(int damage,WarheadTypeClass* warhead,LandType land,const CoordStruct& at) {
    if(!damage || !warhead)return nullptr;
    auto& rules=*RulesClass::Instance;
    if(land==LandType::Water && warhead->Conventional && !Instance.GetCellAt(at)->ContainsBridgeEx()
        && at.Z<Instance.GetCellFloorHeight(at)+2*Unsorted::LevelHeight) {
        return rules.SplashList.Count?rules.SplashList[std::min(damage,35*rules.SplashList.Count-1)/35]:nullptr;
    }
    if(warhead==rules.LightningWarhead)return rules.WeatherConBoltExplosion;
    if(!warhead->AnimList.Count)return nullptr;
    const int index=warhead->EMEffect?ScenarioClass::Instance->Random.RandomRanged(0,warhead->AnimList.Count-1)
        :std::min(damage,25*warhead->AnimList.Count-1)/25;
    return warhead->AnimList[index];
}
int YRPP_FASTCALL MapClass::GetTotalDamage(int damage,const WarheadTypeClass* warhead,Armor armor,int distance) {
    if(!damage || ScenarioClass::Instance->SpecialFlags.Inert || !warhead)return 0;
    if(damage<0)return distance>=8?0:damage;
    const float full=float(damage),edge=float(double(damage)*warhead->PercentAtMax);
    const int spread=rule_integer(double(warhead->CellSpread)*256.0);
    if(edge!=full && spread)damage=rule_integer(double(full-edge)*double(spread-distance)/spread+edge);
    return std::min(rule_integer(double(std::max(damage,0))*warhead->Verses[int(armor)]),RulesClass::Instance->MaxDamage);
}
