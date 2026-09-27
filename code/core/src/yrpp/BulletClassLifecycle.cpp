// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 bullet.cpp constructor/destructor/Set_Bullet_Data/Detach;
// YR 0x466380/0x466560/0x4664C0/0x4684E0. COM 0x46AFD0/0x46AFF0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BulletClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "target_registry.hpp"
#include "projectile_diagnostics.hpp"
#include <atomic>
#include <cstdlib>
#include <cstring>

namespace {
DynamicVectorClass<BulletClass*> bullets,scalable,nextAnimations;
}
DynamicVectorClass<BulletClass*>& BulletClass::Array=bullets;
DynamicVectorClass<BulletClass*>& BulletClass::ScalableBullets=scalable;
DynamicVectorClass<BulletClass*>& BulletClass::NextAnimBullets=nextAnimations;

BulletClass* BulletClass::Create() noexcept {
    try {
        auto* storage=YRMemory::Allocate(sizeof(BulletClass));
        if(!storage)return nullptr;
        auto* bullet=::new (storage) BulletClass();
        bullet->AddRef();
        return bullet;
    }catch(...){return nullptr;}
}
BulletClass::BulletClass() noexcept
    : ObjectClass(),Type{},Owner{},unknown_B4{},Data{},Bright{},unknown_E4{},Velocity{},
      unknown_100{},unknown_104{true},CourseLock{true},CourseLockCounter{},Target{},Speed{},
      InheritedColor{-1},unknown_118{},unknown_11C{},unknown_120{},WH{},AnimFrame{},AnimRateCounter{},
      WeaponType{},SourceCoords{},TargetCoords{},LastMapCoords{-1,-1},DamageMultiplier{},NextAnim{},SpawnNextAnim{},Range{} {
    // Fuse constructor 0x4E1100 starts both zero-duration timers now.
    Data.UnknownTimer.Start(0);Data.ArmTimer.Start(0);
    Create_ID();
    if(!Array.AddItem(this))std::abort();
    game::register_target_identity(*this);
}
BulletClass::~BulletClass() {
    game::projectile_log_event(*this,"destroyed");
    NotifyObjectExpired(true);
    if(SpawnNextAnim)NextAnimBullets.Remove(this);
    // Object.Limbo returns immediately for an already-limbo object.
    if(Game::IsActive && !InLimbo)ObjectClass::Limbo();
    Type=nullptr;Owner=nullptr;NextAnim=nullptr;
    Array.Remove(this);game::unregister_target_identity(*this);
}
ULONG YRPP_STDCALL BulletClass::AddRef() {
    ++std::atomic_ref<LONG>(Game::COMReferenceCount);
    return static_cast<ULONG>(++std::atomic_ref<LONG>(RefCount));
}
ULONG YRPP_STDCALL BulletClass::Release() {
    --std::atomic_ref<LONG>(Game::COMReferenceCount);
    const LONG remaining=--std::atomic_ref<LONG>(RefCount);
    if(!remaining)GameDelete(this);
    return static_cast<ULONG>(remaining);
}
HRESULT YRPP_STDCALL BulletClass::GetClassID(CLSID* out) {
    if(!out)return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[]{0x0E272DC9,0x11D19C0F,0xA00009B7,0xD1AFDD24};
    std::memcpy(out,words,sizeof(words));return 0;
}
void BulletClass::Construct(BulletTypeClass* type,AbstractClass* target,TechnoClass* owner,
        int damage,WarheadTypeClass* warhead,int speed,bool bright) {
    Target=target;Speed=speed;WH=warhead;Bright=bright;Health=damage;Type=type;Owner=owner;
    AnimFrame=0;AnimRateCounter=type->AnimRate;
    InheritedColor=type->FirersPalette && owner?owner->Owner->ColorSchemeIndex:-1;
    DamageMultiplier=256;NextAnim=nullptr;SpawnNextAnim=false;
    game::projectile_log_created(*this);
}
void BulletClass::PointerExpired(AbstractClass* object,bool removed) {
    ObjectClass::PointerExpired(object,removed);
    if(Owner==object){game::projectile_log_event(*this,removed?"owner_removed":"owner_expired");Owner=nullptr;}
    if(Target==object && Target) {
        game::projectile_log_event(*this,removed?"target_removed":"target_expired");
        const auto cell=CellClass::Coord2Cell(Target->GetCoords());
        Target=Unsorted::ScenarioInit || Target->IsInAir() || cell==CellStruct::Empty
            ?nullptr:MapClass::Instance.GetCellAt(cell);
    }
    if(Type==object)Type=nullptr;
    if(WeaponType==object)WeaponType=nullptr;
    if(NextAnim==object)NextAnim=nullptr;
    if(removed && object && object->WhatAmI()==AbstractType::Bullet) {
        auto* expired=static_cast<BulletClass*>(object);
        if(expired->Type && expired->Type->Scalable)ScalableBullets.Remove(expired);
    }
}
