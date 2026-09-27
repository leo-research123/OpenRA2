// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 OpenTS contributors; adaptations Copyright 2026 RedAlert2Open.
// OpenTS light.cpp, 44fac744f70235e0d5ddca107364a68f95132ce9.
// YR calibration: 0x00554760, 0x00554A60–0x00554D50.
#include "yrpp/LightSourceClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/GameOptionsClass.h"
#include "map_world.hpp"
#include <algorithm>
#include <cmath>
#include "yrpp/YRMath.h"
namespace {
DynamicVectorClass<LightSourceClass*> lights;
// Original delayed conversion work is presentation scheduling. Native hosts
// recalculate synchronously so no Cell pointer can outlive a map switch.
void recalculate(const LightSourceClass& light,bool ignoreDetail=false) {
 if((!ignoreDetail&&GameOptionsClass::Instance.DetailLevel<light.DetailLevel)||light.LightVisibility<0)return;
 const int radius=std::min(light.LightVisibility/256+1,512);
 const int cx=light.Location.X/256,cy=light.Location.Y/256;
 for(int y=std::max(0,cy-radius);y<=std::min(511,cy+radius);++y)
  for(int x=std::max(0,cx-radius);x<=std::min(511,cx+radius);++x){
   const double dx=x*256+128.0-light.Location.X,dy=y*256+128.0-light.Location.Y;
   if(Math::sqrt(dx*dx+dy*dy)>=double(light.LightVisibility)+1.0)continue;
   if(auto*cell=MapClass::Instance.TryGetCellAt(CellStruct{short(x),short(y)}))
    if(cell->InitializeTerrainLighting())game::map_resource_changed(*cell);
  }
}
}
DynamicVectorClass<LightSourceClass*>& LightSourceClass::Array=lights;
LightSourceClass::LightSourceClass(int x,int y,int z,int visibility,int intensity,int red,int green,int blue) noexcept
 : LightSourceClass(CoordStruct{x,y,z},visibility,intensity,TintStruct{red,green,blue}) {}
LightSourceClass::LightSourceClass(CoordStruct location,int visibility,int intensity,TintStruct tint) noexcept
 : AbstractClass(),LightIntensity(intensity),LightTint(tint),DetailLevel(2),Location(location),LightVisibility(visibility),Activated(false){Array.AddItem(this);}
LightSourceClass::~LightSourceClass(){Array.Remove(this);Deactivate();}
AbstractType LightSourceClass::WhatAmI() const{return AbsID;}
int LightSourceClass::Size() const{return sizeof(*this);}
void LightSourceClass::Activate(DWORD){if(!Activated){Activated=true;recalculate(*this);}}
void LightSourceClass::Deactivate(DWORD){if(Activated){Activated=false;recalculate(*this,true);}}
void LightSourceClass::ChangeLevels(int intensity,TintStruct tint,char){
 // Preserve the target's red-versus-blue comparison, not a corrected RGB test.
 if(LightIntensity!=intensity||LightTint.Red!=tint.Red||LightTint.Red!=tint.Blue){LightIntensity=intensity;LightTint=tint;if(Activated)recalculate(*this);}
}
void YRPP_FASTCALL LightSourceClass::UpdateLightConverts(int,bool force){
 static int previousDetail=2;const int detail=GameOptionsClass::Instance.DetailLevel;
 if(force||previousDetail!=detail){previousDetail=detail;for(auto*light:Array)recalculate(*light,true);}
}
