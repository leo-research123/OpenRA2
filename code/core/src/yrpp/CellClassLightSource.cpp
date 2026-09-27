// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 OpenTS contributors; adaptations Copyright 2026 RedAlert2Open.
// OpenTS cell.cpp/light.cpp at 44fac744f70235e0d5ddca107364a68f95132ce9.
// YR 0x00484180. Arithmetic intentionally wraps at original 32-bit boundaries.
#include "yrpp/CellClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/ConvertClass.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include "yrpp/YRMath.h"
namespace {int wrap(std::uint32_t n){return std::bit_cast<std::int32_t>(n);}}
void CellClass::CalculateLightSourceLighting(int& intensity,int& ambient,int& normal,int& terrain,int& bridge,int& red,int& green,int& blue){
 intensity=0x10000;ambient=0;normal=terrain=bridge=red=green=blue=1000;
 auto*scenario=ScenarioClass::Instance;
 if(!scenario||MapCoords==CellStruct{0,0}||MapCoords==CellStruct{-1,-1})return;
 const auto& base=scenario->NormalLighting;
 normal=wrap(1000u*scenario->AmbientCurrent)/100;
 red=wrap(1000u*base.Tint.Red)/100;green=wrap(1000u*base.Tint.Green)/100;blue=wrap(1000u*base.Tint.Blue)/100;
 for(auto*light:LightSourceClass::Array){
  if(!light->Activated||GameOptionsClass::Instance.DetailLevel<light->DetailLevel||light->LightVisibility<=0)continue;
  const auto dx=std::uint32_t(int(MapCoords.X)*256+128)-std::uint32_t(light->Location.X);
  const auto dy=std::uint32_t(int(MapCoords.Y)*256+128)-std::uint32_t(light->Location.Y);
  const auto radius=std::uint32_t(light->LightVisibility);
  if(dx*dx+dy*dy>radius*radius)continue;
  const auto distance=std::uint32_t(Math::sqrt(double(wrap(dx))*wrap(dx)+double(wrap(dy))*wrap(dy)));
  if(distance>radius)continue;
  const auto factor=(1000u*radius-1000u*distance)/radius;
  ambient=wrap(std::uint32_t(ambient)+std::uint32_t(wrap(factor*light->LightIntensity)/1000));
  red=wrap(std::uint32_t(red)+std::uint32_t(wrap(factor*light->LightTint.Red)/1000));
  green=wrap(std::uint32_t(green)+std::uint32_t(wrap(factor*light->LightTint.Green)/1000));
  blue=wrap(std::uint32_t(blue)+std::uint32_t(wrap(factor*light->LightTint.Blue)/1000));
 }
 normal=wrap(std::uint32_t(normal)+std::uint32_t(ambient));bridge=normal;
 const auto& light=LightningStorm::Active?scenario->IonLighting:PsyDom::Active()?scenario->DominatorLighting:NukeFlash::IsFadingIn()?scenario->NukeLighting:base;
 const int level=PsyDom::Active()&&!LightningStorm::Active?scenario->NukeLighting.Level:light.Level;
 normal=std::min(wrap(std::uint32_t(normal)+std::uint32_t(static_cast<signed char>(Level))*std::uint32_t(level)-std::uint32_t(light.Ground)),2000);
 bridge=wrap(std::uint32_t(bridge)+std::uint32_t(static_cast<signed char>(Level)+4)*std::uint32_t(light.Level)-std::uint32_t(light.Ground));
 terrain=normal;DWORD multiplier=0x10000;LightConvertClass::NormalizeCellLight(multiplier,terrain,red,green,blue);intensity=wrap(multiplier);
 bridge=std::clamp(wrap(std::uint32_t(intensity)*std::uint32_t(bridge))>>16,0,2000);
 normal=std::max(normal,0);terrain=std::max(terrain,0);
}
