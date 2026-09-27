// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// Additional terms: code/third_party/opents/LICENSE.md. Adaptations: RedAlert2Open 2026.
// OpenTS building.cpp 44fac744f70235e0d5ddca107364a68f95132ce9.
// YR 0x004415F0, 0x00441F60. YR uses Explosion/DestroyAnim, not TS random fires.
#include "yrpp/BuildingClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "map_world.hpp"
#include <new>
#include <cmath>
#include "yrpp/YRMath.h"
namespace {
CoordStruct scatter(CoordStruct p,int radius,Randomizer& random){
 const short facing=short((random.Random()&255)<<8);
 const double a=(facing-0x3FFF)*-0.00009587672516830327;
 CoordStruct result{int(p.X+Math::cos(a)*radius),int(p.Y-Math::sin(a)*radius),p.Z};
 return result.X/256<0||result.X/256>=512||result.Y/256<0||result.Y/256>=512?p:result;
}
}
void BuildingClass::Destory(TechnoClass*,TechnoClass*,bool noSurvivor,const CellStruct* footprint){
 if(!Type||!IsAlive)return;
 // Native host calls may repeat before deferred removal. C4Applied marks
 // a live planted charge; a zero-health, cleared charge with a ticking
 // destruction timer has already emitted its independent effects.
 if(Health<=0&&!C4Applied&&C4Timer.IsTicking())return;
 auto*scenario=ScenarioClass::Instance;
 Deselect();IsBeingRepaired=false;NeedsRepairs=false;C4Applied=false;C4AppliedBy=nullptr;
 for(int i=0;i<21;++i)DestroyNthAnim(static_cast<BuildingAnimSlot>(i));
 for(auto*&fire:DamageFireAnims){delete fire;fire=nullptr;}
 delete Spotlight;Spotlight=nullptr;delete LightSource;LightSource=nullptr;
 // These effects are independent AnimClass objects: their lifetimes continue
 // after the building leaves Logic and its foundation becomes rubble.
 if(scenario){auto&random=scenario->Random;
  const int width=Type->GetFoundationWidth(),height=Type->GetFoundationHeight(false);
  if(width>=2&&height>=2){
   if(width>2)random.RandomRanged(0,width-2);if(height>2)random.RandomRanged(0,height-2);
   if(random.RandomRanged(0,99)<50)SmudgeTypeClass::ScorchTheGround(Location,100,100,true);
   else SmudgeTypeClass::CraterTheGround(Location,100,100,true);
  }
  if(footprint&&Type->Explosion.Count>0){const auto origin=GetRenderCoords();
   for(int i=0;i<512&&footprint[i]!=CellStruct{0x7FFF,0x7FFF};++i){
    const auto offset=footprint[i];auto location=scatter({(origin.X/256+offset.X)*256+128,(origin.Y/256+offset.Y)*256+128,Location.Z},64,random);
    // 0x00441A13 is unsigned DIV, not signed C++ remainder.
    const int delay=random.RandomRanged(0,3);auto*type=Type->Explosion[unsigned(random.Random())%unsigned(Type->Explosion.Count)];
    if(type){auto*anim=new(std::nothrow) AnimClass(type,location,delay,1,0x600);if(anim)anim->Owner=Owner;}
   }
  }
  if(Type->Explodes){
   static constexpr CellStruct offsets[]={{0,-1},{1,0},{0,1},{-1,0}};
   for(auto offset:offsets){auto*cell=MapClass::Instance.TryGetCellAt(CellStruct{short(Location.X/256+offset.X),short(Location.Y/256+offset.Y)});
    auto*overlay=cell?OverlayTypeClass::Array.GetItemOrDefault(cell->OverlayTypeIndex):nullptr;
    if(overlay&&overlay->Explodes){const int delay=random.RandomRanged(1,3)+3;if(auto*type=AnimTypeClass::Find("FIRE3"))new(std::nothrow) AnimClass(type,{cell->MapCoords.X*256+128,cell->MapCoords.Y*256+128,Location.Z},delay,1,0x600);}
   }
  }
  if(Type->DestroyAnim.Count>0){auto*type=Type->DestroyAnim[unsigned(random.Random())%unsigned(Type->DestroyAnim.Count)];if(type){auto*anim=new(std::nothrow) AnimClass(type,GetRenderCoords(),0,1,0x600);if(anim)anim->Owner=Owner;}}
 }
 Health=EstimatedHealth=0;NoCrew=NoCrew||noSurvivor;
 const int delay=GetCurrentMission()==Mission::Selling||Type->Explodes?0:8;
 C4Timer.Start(delay);if(!delay)Animation.Start(0);
 // A non-logic building still needs its original delayed removal callback.
 LogicClass::Instance.AddObject(this,false);
 if(Owner)Owner->RecheckPower=true;
 NeedsRedraw=true;game::map_object_changed();
}
void BuildingClass::LeaveRubble(){
 if(!Type||!Type->LeaveRubble)return;
 auto*image=Type->Rubble?Type->Rubble:Type->Image;if(!image)return;
 if(auto*ref=image->AsReference()){ref->Load();image=ref->Data;}
 if(!image||(!Type->Rubble&&image->Frames/2<=3))return;
 const auto origin=GetRenderCoords();const CellStruct anchor{short(origin.X/256),short(origin.Y/256)};
 if(auto*offsets=Type->FoundationData)for(int i=0;i<512&&offsets[i]!=CellStruct{0x7FFF,0x7FFF};++i){
  if(auto*cell=MapClass::Instance.TryGetCellAt(CellStruct{short(anchor.X+offsets[i].X),short(anchor.Y+offsets[i].Y)})){
   cell->OverlayTypeIndex=239;cell->OverlayData=0;cell->Rubble=nullptr;game::map_resource_changed(*cell);
  }
 }
 if(auto*cell=MapClass::Instance.TryGetCellAt(anchor)){cell->Rubble=Type;game::map_resource_changed(*cell);}
}
