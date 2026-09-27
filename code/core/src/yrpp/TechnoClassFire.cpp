// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Fire_At, combat.cpp projectile angle/pitch;
// calibrated to YR 0x6FDD50/0x48A8D0/0x48A9D0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/DiskLaserClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/WaveClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/MapClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>

namespace {
int add(int a,int b){return std::bit_cast<int>(unsigned(a)+unsigned(b));}
int sub(int a,int b){return std::bit_cast<int>(unsigned(a)-unsigned(b));}
int length(const CoordStruct& c,bool spatial=true){return rule_integer(Math::sqrt(double(c.X)*c.X+(spatial?double(c.Z)*c.Z:0.0)+double(c.Y)*c.Y));}
short direction(double radians){return std::bit_cast<short>(static_cast<unsigned short>(rule_integer((radians-1.570796326794897)*-10430.06004058427)));}
double radians(short facing){return (int(facing)-0x3FFF)*-0.00009587672516830327;}
void scatter(CoordStruct& c,int radius) {
    auto& random=ScenarioClass::Instance->Random;
    const double angle=radians(direction(double(random.RandomRanged(0,0x7FFFFFFE))*4.656612877414201e-10*6.283185307179586));
    const int y=rule_integer(double(c.Y)-Math::sin(angle)*radius);
    c.X=rule_integer(Math::cos(angle)*radius+double(c.X));c.Y=y;
}
bool ballistic_angle(bool high,int speed,int distance,int height,double gravity,double& angle) {
    // YR 0x48A9E7/0x48A9ED writes 0x3FE921D9F4D37C12 (about 45
    // degrees), not pi/2. Keep the target's positive-height shortcut,
    // which is absent in the OpenTS implementation.
    if(height>0){angle=0.7853822499999998;return true;}
    const double v=speed,h=height,d=distance;
    const double k=v*v-h*gravity;
    const double discriminant=v*v*v*v-(v*v*h*gravity+v*v*h*gravity)-gravity*gravity*d*d;
    const double divisor=h*h/(d*d)+1.0+h*h/(d*d)+1.0;
    if(discriminant<0.0 || divisor==0.0)return false;
    const double square=(high?k-Math::sqrt(discriminant):Math::sqrt(discriminant)+k)/divisor;
    if(square<0.0)return false;
    angle=1.570796326794897-Math::asin(Math::sqrt(square)/v);return true;
}
bool projectile_pitch(bool high,int speed,int distance,int height,double gravity,short& pitch) {
    double angle;if(!ballistic_angle(high,speed,distance,height,gravity,angle))return false;
    if(!high){double other=angle;ballistic_angle(false,speed,add(distance,1),height,gravity,other);if(other<angle)angle=-angle;}
    pitch=direction(angle);return true;
}
void start_recoil(RecoilData& r) {
    if(r.Turret.Travel){r.State=RecoilData::RecoilState::Compressing;r.TravelFramesLeft=std::max(r.Turret.CompressFrames,1);
        r.TravelPerFrame=float(double(r.Turret.Travel)/r.TravelFramesLeft);}
}
template<class T,class... Args>T* create(Args&&... args) {
    auto* memory=YRMemory::Allocate(sizeof(T));return memory?::new(memory) T(std::forward<Args>(args)...):nullptr;
}
}

BulletClass* TechnoClass::Fire(AbstractClass* target,int index) {
    auto* weapon=GetWeapon(index)->WeaponType;if(!weapon)return nullptr;
    auto* object=target && (target->AbstractFlags & ::AbstractFlags::Object)!=::AbstractFlags::None?static_cast<ObjectClass*>(target):nullptr;
    auto* projectile=weapon->Projectile;
    if(Unsorted::ArmageddonMode || !target || (object && object->InLimbo))return nullptr;
    if(weapon->Suicide){int damage=GetTechnoType()->Strength;ReceiveDamage(&damage,0,RulesClass::Instance->C4Warhead,nullptr,true,false,nullptr);return nullptr;}
    if((weapon->UseFireParticles && FireParticleSystem) || (weapon->IsRailgun && RailgunParticleSystem)
        || (weapon->UseSparkParticles && SparkParticleSystem) || (weapon->IsSonic && Wave))return nullptr;
    auto& map=MapClass::Instance;
    const auto reveal=[&](ObjectClass* victim,bool alwaysFog) {
        if((IsOwnedByCurrentPlayer || DiscoveredByCurrentPlayer)
            && (!map.IsLocationShrouded(GetCoords()) && !map.IsLocationFogged(GetCoords())))return;
        if(WhatAmI()==AbstractType::Aircraft && IsOwnedByCurrentPlayer)return;
        if(!victim)return;
        auto* house=victim->GetOwningHouse();auto at=GetCoords();
        if(house && house->IsControlledByCurrentPlayer() && weapon->RevealOnFire) {
            map.RevealArea1(&at,3,house,0,0,0,1,0);
            if(!alwaysFog){at=GetCoords();map.RevealArea3(&at,0,4,false);}
        }
        if(alwaysFog){at=GetCoords();map.RevealArea3(&at,0,4,false);}
    };
    if(weapon->Spawner){SpawnManager->SetTarget(target);reveal(object,true);return nullptr;}
    if(weapon->DrainWeapon) {
        if((target->AbstractFlags & ::AbstractFlags::Techno)!=::AbstractFlags::None && static_cast<TechnoClass*>(target)->GetTechnoType()->Drainable) {
            StartDrain(static_cast<TechnoClass*>(target));SetTarget(nullptr);
        }
        return nullptr;
    }
    CoordStruct targetAt;
    auto* type=GetTechnoType();
    if(type->SprayAttack) {
        constexpr CoordStruct offsets[]{{256,0,0},{180,180,0},{0,256,0},{-180,180,0},{-256,0,0},{-180,-180,0},{0,-256,0},{180,-180,0}};
        SprayOffsetIndex=CurrentBurstIndex?(8/weapon->Burst+SprayOffsetIndex)%8:ScenarioClass::Instance->Random.RandomRanged(0,7);
        const auto offset=offsets[SprayOffsetIndex];targetAt={add(Location.X,offset.X),add(Location.Y,offset.Y),add(Location.Z,offset.Z)};
        target=map.GetCellAt(targetAt);
    }else if(weapon->AreaFire){target=map.GetCellAt(GetCoords());targetAt=target->GetCoords();}
    else if(object)object->GetTargetCoords(&targetAt);
    else targetAt=target->GetCenterCoords();
    CoordStruct fireAt;GetFLH(&fireAt,index,CoordStruct::Empty);
    // These original pre-creation queries are intentionally retained.
    if(projectile->ROT || projectile->Dropping){GetRealFacing();if(projectile->Dropping)fireAt=GetCoords();}
    else Math::atan2(double(fireAt.Y)-targetAt.Y,double(targetAt.X)-fireAt.X);
    int damage=weapon->Damage;
    if(weapon->IsSonic || weapon->UseFireParticles)damage=0;
    else if(damage>0) {
        damage=rule_integer(Owner->FirepowerMultiplier*FirepowerMultiplier*damage);
        if((Veterancy.IsVeteran() && type->VeteranAbilities.FIREPOWER)
            || (Veterancy.IsElite() && (type->VeteranAbilities.FIREPOWER || type->EliteAbilities.FIREPOWER)))
            damage=rule_integer(double(damage)*RulesClass::Instance->VeteranCombat);
    }
    if(CanOccupyFire())damage=rule_integer(double(damage)*RulesClass::Instance->OccupyDamageMultiplier);
    if(BunkerLinkedItem && WhatAmI()!=AbstractType::Building)damage=rule_integer(double(damage)*RulesClass::Instance->BunkerDamageMultiplier);
    if(InOpenToppedTransport)damage=rule_integer(double(damage)*RulesClass::Instance->OpenToppedDamageMultiplier);
    if(weapon->DiskLaser)if(auto* disk=create<DiskLaserClass>()) {
        ++CurrentBurstIndex;ChargeTurretDelay=GetROF(index);RearmTimer.Start(ChargeTurretDelay);CurrentBurstIndex%=weapon->Burst;
        disk->Fire(this,reinterpret_cast<TechnoClass*>(target),weapon,damage);return nullptr;
    }
    const int dx=sub(targetAt.X,fireAt.X),dy=sub(targetAt.Y,fireAt.Y);
    int speed=weapon->GetSpeed(rule_integer(Math::sqrt(double(std::bit_cast<int>(unsigned(dx)*unsigned(dx)+unsigned(dy)*unsigned(dy))))));
    auto* bullet=projectile->CreateBullet(target,this,damage,weapon->Warhead,speed,weapon->Bright);
    BulletVelocity velocity{};
    if(bullet) {
        bullet->SetWeaponType(weapon);bullet->Limbo();
        auto* foot=(AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None?static_cast<FootClass*>(this):nullptr;
        if(foot){if(!foot->Locomotor)std::abort();if(foot->Locomotor->Is_Moving() && !type->JumpJet)bullet->unknown_B4=true;}
        if(!bullet->unknown_B4 && !projectile->Inaccurate && Target && (Target->AbstractFlags & ::AbstractFlags::Techno)!=::AbstractFlags::None) {
            auto* victim=static_cast<TechnoClass*>(Target);victim->EstimatedHealth=sub(victim->EstimatedHealth,EstimateDamage(victim,bullet->GetWeaponType()));
        }
        CoordStruct predicted;PredictTargetCoords(&predicted);
        CoordStruct delta{sub(predicted.X,fireAt.X),sub(predicted.Y,fireAt.Y),sub(predicted.Z,fireAt.Z)};
        if(projectile->Inaccurate && projectile->Arcing) {
            auto& random=ScenarioClass::Instance->Random;int radius;
            if(!projectile->FlakScatter || projectile->Inviso)radius=random.RandomRanged(RulesClass::Instance->BallisticScatter/2,RulesClass::Instance->BallisticScatter);
            else {
                const float x=float(delta.X),y=float(delta.Y),d=float(Math::sqrt(double(x)*x+double(y)*y+double(delta.Z)*delta.Z));
                const int randomRadius=random.RandomRanged(0,RulesClass::Instance->BallisticScatter);
                radius=std::bit_cast<int>(unsigned(rule_integer(d))*unsigned(randomRadius))/GetWeaponRange(index);
            }
            scatter(delta,radius);
        }
        short facing;
        if(projectile->ROT || projectile->Dropping){facing=std::bit_cast<short>(GetRealFacing().Raw);if(projectile->Dropping)fireAt=GetCoords();}
        else facing=direction(Math::atan2(-double(delta.Y),double(delta.X)));
        if(speed>length(delta)/2)speed=length(delta)/2;
        if(projectile->ROT>0 || projectile->Vertical){if(!type->RadialFireSegments)speed=1;bullet->Speed=weapon->Speed;}
        double planarSpeed=double(speed)/Math::sqrt(10000.0)*100.0;
        if(planarSpeed==0.0)planarSpeed=100.0;
        planarSpeed=Math::sqrt(planarSpeed*planarSpeed);
        if(type->RadialFireSegments>0) {
            const double radial=3.141592653589793/type->RadialFireSegments*unknown_43C-1.570796326794897;
            facing=direction(radians(std::bit_cast<short>(PrimaryFacing.Current().Raw))+radial);
            if(++unknown_43C>=unsigned(type->RadialFireSegments))unknown_43C=0;
        }
        velocity={Math::cos(radians(facing))*planarSpeed,-Math::sin(radians(facing))*planarSpeed,0.0};
        short pitch=0x3FFF;bool valid=true;
        if(projectile->Arcing) {
            bool high=weapon->Lobber;
            if(!high && Target){const int z=sub(Target->GetCoords().Z,Location.Z);if(z>0){auto a=GetCoords(),b=Target->GetCoords();high=length({sub(a.X,b.X),sub(a.Y,b.Y),0},false)<z;}}
            valid=projectile_pitch(high,speed,length(delta,false),delta.Z,RulesClass::Instance->Gravity*(projectile->Floater?0.5:1.0),pitch);
        }else if(projectile->Voxel)pitch=target->GetCoords().Z>=GetCoords().Z?0x4000:std::bit_cast<short>(static_cast<unsigned short>(0x8000));
        else if(std::abs(delta.Z)>200) {
            double adjustment=20.0;
            if(Target && Target->WhatAmI()==AbstractType::Building){CoordStruct turret;vt_entry_300(&turret,index);delta.Z=sub(200*static_cast<BuildingClass*>(Target)->Type->Height,turret.Z);if(std::abs(delta.Z)<20)adjustment=0.0;}
            double angle=std::atan2(float(std::abs(delta.Z)-adjustment),float(std::max(double(length(delta,false)),0.05)));
            if(delta.Z<0)angle=-angle;pitch=direction(angle);
        }
        const double oldPitch=radians(direction(Math::atan2(velocity.Z,Math::sqrt(velocity.X*velocity.X+velocity.Y*velocity.Y))));
        const double magnitude=Math::sqrt(velocity.X*velocity.X+velocity.Y*velocity.Y+velocity.Z*velocity.Z);
        if(oldPitch!=0.0){velocity.X/=Math::cos(oldPitch);velocity.Y/=Math::cos(oldPitch);}
        velocity.X*=Math::cos(radians(pitch));velocity.Y*=Math::cos(radians(pitch));velocity.Z=Math::sin(radians(pitch))*magnitude;
        if(!valid || !bullet->MoveTo(fireAt,velocity)){bullet->Release();bullet=nullptr;}
        else {
            if(CanOccupyFire() && WhatAmI()==AbstractType::Building){auto* building=static_cast<BuildingClass*>(this);building->FiringOccupantIndex=(building->FiringOccupantIndex+1)%building->GetOccupantCount();}
            if(projectile->Inviso && object && object->OnBridge)bullet->OnBridge=true;
            if(HasTurret() && type->TurretRecoil){start_recoil(TurretRecoil);start_recoil(BarrelRecoil);}
            if(weapon->UseFireParticles && !FireParticleSystem)FireParticleSystem=create<ParticleSystemClass>(weapon->AttachedParticleSystem,fireAt,target,this,CoordStruct::Empty,nullptr);
            if(weapon->UseSparkParticles && !SparkParticleSystem)SparkParticleSystem=create<ParticleSystemClass>(weapon->AttachedParticleSystem,fireAt,target,this,CoordStruct::Empty,nullptr);
            if(weapon->IsRailgun && !RailgunParticleSystem){CoordStruct end;RailgunBeamDamage(&end,fireAt,target,weapon);RailgunParticleSystem=create<ParticleSystemClass>(weapon->AttachedParticleSystem,fireAt,nullptr,this,end,nullptr);}
            ++CurrentBurstIndex;int delay=GetROF(index);if(Berzerk)delay/=2;ChargeTurretDelay=delay;RearmTimer.Start(delay);CurrentBurstIndex%=weapon->Burst;
            AnimTypeClass* animType=nullptr;
            if(weapon->Anim.Count==8)animType=weapon->Anim[(GetRealFacing().GetValue<3>()+1)%8];
            else if(weapon->Anim.Count>0)animType=weapon->Anim[0];
            if(CanOccupyFire())animType=weapon->OccupantAnim;
            if(!animType && InOpenToppedTransport && weapon->OpenToppedAnim)animType=weapon->OpenToppedAnim;
            if(weapon->Report.Count>0 && !type->IsGattling)VocClass::PlayAt(weapon->Report[static_cast<unsigned short>(unknown_short_3C8)%weapon->Report.Count],fireAt,nullptr);
            if(animType)if(auto* anim=create<AnimClass>(animType,fireAt,0,1,0x600,0,false)) {
                if(WhatAmI()==AbstractType::Building){anim->ZAdjust=std::min((fireAt.Y-GetRenderCoords().Y)/-4,0);if(GetOccupantCount()>0)anim->ZAdjust=-200;}
                else anim->SetOwnerObject(this);
            }
            if(weapon->IsSonic)Wave=create<WaveClass>(targetAt,fireAt,this,WaveType::Sonic,target);
            if(type->TargetLaser && Owner->IsControlledByCurrentPlayer())TargetLaserTimer.Start(15);
            if(weapon->IsLaser) {
                auto* laser=CreateLaser(reinterpret_cast<ObjectClass*>(target),index,WhatAmI()==AbstractType::Building?GetTurretWeapon()->WeaponType:weapon,CoordStruct::Empty);
                if(laser && weapon->IsBigLaser && WhatAmI()!=AbstractType::Building)laser->Thickness=2;
                if(laser && WhatAmI()==AbstractType::Building && GetTechnoType()==RulesClass::Instance->PrismType){laser->Thickness=3;if(static_cast<BuildingClass*>(this)->SupportingPrisms>0){laser->Thickness=5;laser->IsSupported=true;}}
            }else if(weapon->IsElectricBolt)FireElectricBolt(target);
            else if(weapon->IsRadBeam)FireRadiationBeam(target,weapon->Warhead && weapon->Warhead->Temporal);
            else if(weapon->IsRadEruption)FireRadiationEruption(static_cast<short>(rule_integer(weapon->Warhead->CellSpread)));
            else if(weapon->IsMagBeam && !Wave)Wave=create<WaveClass>(targetAt,fireAt,this,WaveType::Magnetron,target);
            DecreaseAmmo();Mark(MarkType::Change);reveal((target->AbstractFlags & ::AbstractFlags::Object)!=::AbstractFlags::None?static_cast<ObjectClass*>(target):nullptr,false);
            LastFireBulletFrame=Unsorted::CurrentFrame;
        }
    }
    if(weapon->LimboLaunch) {
        if(type->ReselectIfLimboed && IsSelected && Owner->IsControlledByCurrentPlayer() && target->WhatAmI()==AbstractType::Infantry)ShouldBeReselectOnUnlimbo=true;
        auto* foot=(AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None?static_cast<FootClass*>(this):nullptr;
        if(type->RejoinTeamIfLimboed && foot && foot->Team && target->WhatAmI()==AbstractType::Infantry)OldTeam=foot->Team;
        Limbo();
        if(weapon->Warhead->Parasite) {
            if((target->AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None)static_cast<FootClass*>(target)->LastBeParasitedStartFrame=Unsorted::CurrentFrame+10;
            bullet->Limbo();bullet->Construct(projectile,target,this,damage,weapon->Warhead,weapon->Speed,weapon->Bright);bullet->MoveTo(fireAt,velocity);
        }
    }
    if(type->DistributedFire && (CurrentMission!=Mission::Attack || WhatAmI()==AbstractType::Building)){AttackedTargets.AddItem(target);SetTarget(nullptr);}
    if(weapon->FireOnce){if((AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None)if(auto* team=static_cast<FootClass*>(this)->Team){team->AssignMissionTarget(nullptr);team->StepCompleted=true;}SetTarget(nullptr);}
    return bullet;
}
