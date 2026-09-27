// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 partsys.cpp.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// YR 0x62DC50 / 0x62E070 / 0x62FE90; EA terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/MapClass.h"
#include <cstdlib>

namespace { DynamicVectorClass<ParticleSystemClass*> systems;ParticleSystemClass* default_system=nullptr; }
DynamicVectorClass<ParticleSystemClass*>& ParticleSystemClass::Array=systems;
ParticleSystemClass*& ParticleSystemClass::DefaultSystem=default_system;

ParticleSystemClass::ParticleSystemClass(ParticleSystemTypeClass* type,const CoordStruct& at,AbstractClass* target,ObjectClass* owner,const CoordStruct& targetAt,HouseClass* house) noexcept
 : ObjectClass(),Type(type),SpawnDistanceToOwner{},Particles(),TargetCoords(CoordStruct::Empty),Owner(nullptr),Target(nullptr),
 SpawnFrames(float(type->SpawnFrames)),Lifetime(type->Lifetime),SparkSpawnFrames(type->SparkSpawnFrames),SpotlightRadius(29),TimeToDie(false),unknown_bool_F9(false),OwnerHouse(house){
 Create_ID();if(!Array.AddItem(this))std::abort();Particles.Clear();
 if(target){
  TargetCoords=target->GetCoords();
  if((unsigned(MapClass::Instance.GetCellAt(target->GetCoords())->Flags)&0x100u)&&target->WhatAmI()==AbstractType::Cell)TargetCoords.Z+=CellClass::BridgeHeight;
 }else TargetCoords=targetAt;
 Owner=owner;
 if(owner){if((owner->AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None)Target=static_cast<TechnoClass*>(owner)->Target;}
 else Target=target;
 ObjectClass::Unlimbo(at,DirType::North);
 if(Owner){const auto origin=Owner->GetCoords();SpawnDistanceToOwner={Location.X-origin.X,Location.Y-origin.Y,Location.Z-origin.Z};}
}
ParticleSystemClass::~ParticleSystemClass(){
 NotifyObjectExpired(true);ObjectClass::Limbo();
 while(Particles.Count){auto* particle=Particles[0];Particles.Remove(particle);delete particle;}
 while(PendingDeletes.Remove(this)){}Array.Remove(this);if(DefaultSystem==this)DefaultSystem=nullptr;Type=nullptr;
 // DECLARE_PROPERTY stores this vector in a union, so it has no automatic destruction.
 Particles.~DynamicVectorClass();
}
void ParticleSystemClass::PointerExpired(AbstractClass* object,bool removed){
 ObjectClass::PointerExpired(object,removed);
 for(int i=Particles.Count-1;i>=0;--i)if(Particles[i]==object)Particles.RemoveItem(i);
 if(Type==object)Type=nullptr;
 if(Target==object)Target=nullptr;
 if(Owner==object){TimeToDie=true;Owner=nullptr;}
}
ParticleClass* ParticleSystemClass::SpawnParticle(const CoordStruct& from,const CoordStruct& to){
 if(Type->HoldsWhat==-1)return nullptr;
 auto a=from,b=to;auto* type=ParticleTypeClass::Array.GetItemOrDefault(Type->HoldsWhat);
 if(!type)return nullptr;
 auto* particle=GameCreate<ParticleClass>(type,&a,&b,this);
 if(particle&&!Particles.AddItem(particle)){delete particle;return nullptr;}
 return particle;
}
ParticleClass* ParticleSystemClass::SpawnParticle(ParticleTypeClass* type,const CoordStruct& at){
 auto a=at,b=CoordStruct::Empty;auto* particle=GameCreate<ParticleClass>(type,&a,&b,nullptr);
 if(particle&&!Particles.AddItem(particle)){delete particle;return nullptr;}
 return particle;
}
