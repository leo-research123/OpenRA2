// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 OpenTS contributors; adaptations Copyright 2026 RedAlert2Open.
// OpenTS blight.cpp, 44fac744f70235e0d5ddca107364a68f95132ce9.
// YR 0x00435820 / 0x004361D0 / 0x00435BE0; YR beam spread is 5.973333333333333.
#include "yrpp/BuildingLightClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/YRMath.h"
#include <bit>
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/ScenarioClass.h"
#include "building_selection.hpp"
#include "type_drawing.hpp"
#include "map_world.hpp"
#include <cmath>
#include <algorithm>
namespace {
DynamicVectorClass<BuildingLightClass*> lights;
float chop(double value){float result=float(value);if(std::abs(double(result))>std::abs(value))result=std::bit_cast<float>(std::bit_cast<std::uint32_t>(result)-1);return result;}
CoordStruct rotate(CoordStruct origin,CoordStruct target,double angle){
 const float a=chop(angle),c=chop(Math::cos(a)),s=chop(Math::sin(a));
 const float x=float(target.X-origin.X),y=float(target.Y-origin.Y);
 const float rx=chop(double(c)*x-double(s)*y),ry=chop(double(s)*x+double(c)*y);
 return {int(double(origin.X)+rx),int(double(origin.Y)+ry),target.Z};
}
int distance(CoordStruct a,CoordStruct b){return int(Math::sqrt(double(a.X-b.X)*(a.X-b.X)+double(a.Y-b.Y)*(a.Y-b.Y)+double(a.Z-b.Z)*(a.Z-b.Z)));}
CoordStruct move(CoordStruct origin,DirStruct facing,int radius){
 const double a=(short(facing.Raw)-0x3FFF)*-0.00009587672516830327;
 return {int(origin.X+Math::cos(a)*radius),int(origin.Y-Math::sin(a)*radius),origin.Z};
}
}
DynamicVectorClass<BuildingLightClass*>& BuildingLightClass::Array=lights;
BuildingLightClass::BuildingLightClass(ObjectClass* owner) noexcept
 : ObjectClass(),Speed(0),field_B8{},field_C4{},Acceleration(0),Direction(false),BehaviourMode(SpotlightBehaviour::None),FollowingObject(nullptr),OwnerObject(nullptr){
 if(owner&&(owner->WhatAmI()==AbstractType::Building||owner->WhatAmI()==AbstractType::Unit||owner->WhatAmI()==AbstractType::Infantry))OwnerObject=static_cast<TechnoClass*>(owner);
 Array.AddItem(this);
 if(OwnerObject&&RulesClass::Instance){auto&r=*RulesClass::Instance;
  field_C4=move(owner->Location,OwnerObject->PrimaryFacing.Current(),r.SpotlightLocationRadius);
  field_B8=move(owner->Location,OwnerObject->PrimaryFacing.Current(),-r.SpotlightMovementRadius);
  Location=field_C4;InLimbo=false;IsOnMap=true;LogicClass::Instance.AddObject(this,false);
  SetBehaviour(SpotlightBehaviour::Sweep);Direction=(Array.Count-1)%2!=0;
 }
}
BuildingLightClass::~BuildingLightClass(){
 if(OwnerObject&&OwnerObject->WhatAmI()==AbstractType::Building){auto*b=static_cast<BuildingClass*>(OwnerObject);if(b->Spotlight==this)b->Spotlight=nullptr;}
 Array.Remove(this);LogicClass::Instance.RemoveObject(this);
}
AbstractType BuildingLightClass::WhatAmI() const{return AbsID;}
int BuildingLightClass::Size() const{return sizeof(*this);}
Layer BuildingLightClass::InWhichLayer() const{return Layer::Air;}
ObjectTypeClass* BuildingLightClass::GetType() const{return nullptr;}
void BuildingLightClass::PointerExpired(AbstractClass* object,bool){if(object==FollowingObject)FollowingObject=nullptr;if(object==OwnerObject)OwnerObject=nullptr;}
void BuildingLightClass::SetBehaviour(SpotlightBehaviour mode){
 BehaviourMode=mode;Speed=0;
 if(mode!=SpotlightBehaviour::Follow||!OwnerObject)return;
 int closest=9999999;ObjectClass* target=nullptr;
 for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y){auto*cell=MapClass::Instance.TryGetCellAt(CellStruct{short(Location.X/256+x),short(Location.Y/256+y)});if(!cell)continue;
  for(auto*object=cell->FirstObject;object;object=object->NextObject){
   if(object->WhatAmI()!=AbstractType::Unit&&object->WhatAmI()!=AbstractType::Infantry)continue;
   auto*techno=static_cast<TechnoClass*>(object);if(OwnerObject->Owner&&OwnerObject->Owner->IsAlliedWith(techno->Owner))continue;
   const int d=distance(object->GetCenterCoords(),Location);if(d<closest){closest=d;target=object;}
  }
 }
 if(target)FollowingObject=target;
}
void BuildingLightClass::Update(){
 if(!OwnerObject||!OwnerObject->IsAlive){delete this;return;}
 auto*r=RulesClass::Instance;if(!r)return;
 if(BehaviourMode==SpotlightBehaviour::Sweep){
  Speed+=Acceleration;
  if(Direction){if(Speed>r->SpotlightAngle/2){Acceleration-=r->SpotlightAcceleration;if(Acceleration<0){Acceleration=0;Direction=false;}}else if(Acceleration<r->SpotlightSpeed)Acceleration+=r->SpotlightAcceleration;}
  else {if(Speed<r->SpotlightAngle/-2){Acceleration+=r->SpotlightAcceleration;if(Acceleration>0){Acceleration=0;Direction=true;}}else if(-r->SpotlightSpeed<Acceleration)Acceleration-=r->SpotlightAcceleration;}
  Location=rotate(field_B8,field_C4,Speed);
 }else if(BehaviourMode==SpotlightBehaviour::Circle){Speed+=r->SpotlightSpeed*4;if(Speed>6.283185307179586)Speed-=6.283185307179586;Location=rotate(OwnerObject->Location,field_C4,Speed);}
 else if(BehaviourMode==SpotlightBehaviour::Follow){
  if(FollowingObject&&FollowingObject->IsAlive&&distance(FollowingObject->Location,OwnerObject->Location)<r->SpotlightMovementRadius){
   const auto p=FollowingObject->Location;Location={int(Location.X+(p.X-Location.X)*0.25),int(Location.Y+(p.Y-Location.Y)*0.25),int(Location.Z+(p.Z-Location.Z)*0.25)};
  }else SetBehaviour(SpotlightBehaviour::Sweep);
 }
 game::map_object_changed();
}
void BuildingLightClass::DrawIt(Point2D*,RectangleStruct* clip) const{
 auto*context=game::active_type_drawing();auto*tactical=TacticalClass::Instance;auto*r=RulesClass::Instance;
 if(!context||!tactical||!r||!clip||BehaviourMode==SpotlightBehaviour::None||!OwnerObject||OwnerObject->WhatAmI()!=AbstractType::Building)return;
 auto*b=static_cast<BuildingClass*>(OwnerObject);if(!b->IsAlive||!b->IsPowerOnline()||b->IsFogged)return;
 if(ScenarioClass::Instance&&ScenarioClass::Instance->SpecialFlags.FogOfWar){auto*cell=MapClass::Instance.TryGetCellAt(Location);if(!cell||cell->IsFogged())return;}
 const auto center=b->GetCenterCoords();const int dist=distance(Location,center);
 const int step=(r->SpotlightMovementRadius-r->SpotlightLocationRadius)/10;
 const int stage=dist<r->SpotlightLocationRadius||!step?0:(dist-r->SpotlightLocationRadius)/step;
 SpotlightClass pool(Location,16);pool.MovementRadius=BehaviourMode==SpotlightBehaviour::Follow&&dist>r->SpotlightLocationRadius?std::clamp(stage+80,0,89):80;pool.Draw();
 const int detection=int(stage*5.973333333333333+r->SpotlightRadius);
 if(dist<=0||detection<0||dist<detection)return;
 const double angle=Math::asin(double(detection)/dist);
 CoordStruct caster=center;caster.Z+=430;Point2D start{};TacticalClass::CoordsToClient(caster,tactical->TacticalPos,*clip,start);
 for(double a:{angle,-angle}){auto endpoint=rotate(center,Location,a);Point2D end{};TacticalClass::CoordsToClient(endpoint,tactical->TacticalPos,*clip,end);
  game::record_type_drawing_result(game::draw_depth_glow_line(*context,*clip,start,end,-TacticalClass::AdjustForZ(center.Z+400),-TacticalClass::AdjustForZ(Location.Z+250),75-6*stage));
 }
}
