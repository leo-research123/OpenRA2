// Terrain constructor 0x0071BB90 and animation/update 0x0071C730.
#include "yrpp/TerrainClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/FileFormats/SHP.h"
#include "map_world.hpp"
#include "target_registry.hpp"
#include <algorithm>
namespace {DynamicVectorClass<TerrainClass*> objects;}
DynamicVectorClass<TerrainClass*>& TerrainClass::Array=objects;
TerrainClass::TerrainClass(TerrainTypeClass*tt,CellStruct coords) noexcept
 : ObjectClass(),
 Animation{},
 Type{},
 IsBurning{},
 IsCrumbling{},
 unknown_rect_D0{} {
 Type=tt;Health=EstimatedHealth=tt?std::max(tt->Strength,1):1;Location={int(coords.X)*256+128,int(coords.Y)*256+128,0};Array.AddItem(this);
 game::register_target_identity(*this);
}
TerrainClass::~TerrainClass(){IsAlive=false;NotifyObjectExpired(true);game::detach_map_object(*this);Array.Remove(this);game::unregister_target_identity(*this);}
void TerrainClass::PointerExpired(AbstractClass* object,bool removed){ObjectClass::PointerExpired(object,removed);if(Type==object)Type=nullptr;}
void TerrainClass::Update(){
 if(!Type||!IsAlive||InLimbo||!Type->IsAnimated)return;
 auto*shape=GetImage();if(shape)shape=shape->GetData();if(!shape||shape->Frames<1)return;
 if(Animation.Rate==0){auto*s=ScenarioClass::Instance;if(!s)return;const int sample=s->Random.Random();const auto magnitude=sample<0?0u-static_cast<unsigned>(sample):static_cast<unsigned>(sample);double probability=(magnitude%1000000u)*0.000001;if(probability>=std::clamp(double(Type->AnimationProbability),0.0,1.0))return;Animation.Value=0;Animation.Start(Type->AnimationRate);}
 if(!Animation.Update())return;NeedsRedraw=true;game::map_object_changed();
 if(IsCrumbling){if(Animation.Value>=shape->Frames-1){IsAlive=false;Health=0;game::detach_map_object(*this);}return;}
 if(Type->SpawnsTiberium&&Animation.Value==shape->Frames/2){Animation.Value=0;Animation.Start(0);if(Type->SpawnsTiberium){if(auto*c=GetCell())c->SpreadTiberium(true);}}
}
