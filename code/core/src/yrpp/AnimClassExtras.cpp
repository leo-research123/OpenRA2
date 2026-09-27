// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 anim.cpp Bounce_AI; YR 0x00423930.
// Copyright 2026 OpenTS contributors. See third_party/opents/LICENSE.md.
#include "yrpp/AnimClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "map_world.hpp"
#include <cstdlib>
#include <new>

int AnimClass::AnimExtras(){
 const auto result=Bounce.Update();
 if(Type->IsMeteor)Bounce.Velocity.Z=float(Bounce.Velocity.Z+Bounce.Gravity);
 if(result==BounceClass::Status::Bounce){
  if(Type->BounceAnim)new(std::nothrow) AnimClass(Type->BounceAnim,GetCoords());
  const auto at=Bounce.GetCoords();
  if(auto* cell=MapClass::Instance.TryGetCellAt(at)){
   for(auto* object=cell->FirstObject;object;){
    auto* next=object->NextObject;const auto target=object->GetCoords();
    const int distance=std::abs(at.X-target.X)+std::abs(at.Y-target.Y);
    if(distance<=Type->DamageRadius&&Type->Warhead&&Type->Damage>0){
     int damage=int(Type->Damage);
     object->ReceiveDamage(&damage,TacticalClass::AdjustForZ(distance),Type->Warhead,nullptr,false,false,nullptr);
    }
    object=next;
   }
  }
 }
 Location=Bounce.GetCoords();NeedsRedraw=true;game::map_object_changed();
 return int(result);
}
