// Native cell resource transitions. Displayed value uses OverlayData+1, but
// clearing the terminal frame credits zero. Empty cells have OverlayTypeIndex=-1.
// Originals: 0x00485010, 0x00485020, 0x004838E0, 0x00487190,
// 0x00480A80, 0x00483780.
#include "yrpp/CellClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Unsorted.h"
#include "map_world.hpp"
#include <algorithm>
#include <cstdint>
#include <cmath>
bool CellClass::IsCovered() const {
 // 0x487E00 scans this exact content chain, not GetBuilding's native
 // foundation fallback or its alive/on-map filter.
 if(Game::IsActive)for(auto* object=FirstObject;object;object=object->NextObject)
  if(object->WhatAmI()==AbstractType::Building)
   return OccupyHeightsCoveringMe!=0 && OccupyHeightsCoveringMe!=1;
 return OccupyHeightsCoveringMe!=0;
}
int CellClass::GetRadLevel() const {
 const double maximum=RulesClass::Instance->RadLevelMax;
 const double level=RadLevel<maximum?RadLevel:maximum;
 return std::isfinite(level) && level>=-2147483648.0 && level<2147483648.0?static_cast<int>(level):INT32_MIN;
}
ObjectClass* CellClass::FindObjectOfType(AbstractType type,bool)const{
 for(auto*p=FirstObject;p;p=p->NextObject)if(p->WhatAmI()==type&&p->IsOnMap&&!p->IsDead())return p;
 if(type==AbstractType::Building){for(int i=0;i<BuildingClass::Array.Count;++i){auto*p=BuildingClass::Array[i];if(p->IsOnMap&&!p->IsDead()&&game::foundation_contains(*p,MapCoords))return p;}}
 if(type==AbstractType::Terrain){for(int i=0;i<TerrainClass::Array.Count;++i){auto*p=TerrainClass::Array[i];if(p->IsOnMap&&!p->IsDead()&&game::foundation_contains(*p,MapCoords))return p;}}
 return nullptr;
}
BuildingClass*CellClass::GetBuilding()const{return static_cast<BuildingClass*>(FindObjectOfType(AbstractType::Building,false));}
TerrainClass*CellClass::GetTerrain(bool alt)const{return static_cast<TerrainClass*>(FindObjectOfType(AbstractType::Terrain,alt));}
int CellClass::GetContainedTiberiumIndex()const{return TiberiumClass::FindIndex(OverlayTypeIndex);}
int CellClass::GetContainedTiberiumValue()const{auto*t=TiberiumClass::Find(OverlayTypeIndex);return t?int(std::min<std::int64_t>(std::int64_t(OverlayData+1)*std::max(t->Value,0),INT32_MAX)):0;}
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 cell.cpp,
// Can_Tiberium_Germinate / Place_Tiberium (GPL-3.0-or-later;
// code/third_party/opents/LICENSE.md). YR 0x4838E0 ignores the type argument
// and rejects every slope; its ground/building/tile gates remain original.
bool CellClass::CanTiberiumGerminate(TiberiumClass*){
 if(!MapClass::Instance.IsWithinUsableArea(MapCoords,true)||(static_cast<unsigned>(Flags)&0x500u))return false;
 if(Game::IsActive){
  for(auto* object=FirstObject;object;object=object->NextObject)if(object->WhatAmI()==AbstractType::Building){
   auto* building=static_cast<BuildingClass*>(object);
   if(building->Health>0&&!building->Type->Invisible&&!building->Type->InvisibleInGame)return false;
   break;
  }
  for(auto* object=FirstObject;object;object=object->NextObject)if(object->WhatAmI()==AbstractType::Terrain){
   if(static_cast<TerrainClass*>(object)->Type->SpawnsTiberium)return false;
   break;
  }
 }
 const int land=static_cast<int>(LandType);
 if(land<0||land>=12||!GroundType::Array[land].Buildable||OverlayTypeIndex!=-1||SlopeIndex)return false;
 const auto* tile=IsometricTileTypeClass::Array.GetItemOrDefault(IsoTileTypeIndex);
 return !tile||tile->AllowTiberium;
}
bool CellClass::IncreaseTiberium(int index,int amount){
 auto*t=TiberiumClass::Array.GetItemOrDefault(index);if(!t||!t->Image||t->NumFrames<=0||amount<=0||amount>=t->NumFrames)return false;
 int existing=GetContainedTiberiumIndex();
 if(existing<0){if(!CanTiberiumGerminate(t))return false;int r=ScenarioClass::Instance?ScenarioClass::Instance->Random.RandomRanged(0,11):0;OverlayTypeIndex=t->Image->ArrayIndex+r;
 if(t->GrowthLogic.Queue)t->RegisterForGrowth(&MapCoords);
 OverlayData=BYTE(amount);}
 else{if(existing!=index||!CanTiberiumGrow())return false;int frame=int(std::min<std::int64_t>(std::min(t->NumFrames-1,255),std::int64_t(OverlayData)+amount));if(frame<=OverlayData)return false;OverlayData=BYTE(frame);}
 game::map_resource_changed(*this);
 // 0x487190 registers a thickened patch for spreading before Grow enqueues
 // its next growth score. Reversing these calls consumes RNG in another order.
 if(existing>=0&&t->SpreadLogic.Queue)t->RegisterForSpread(&MapCoords);
 return true;
}
int CellClass::ReduceTiberium(int amount){
 if(amount<=0||GetContainedTiberiumIndex()<0)return 0;
 if(amount>=int(OverlayData)+1){
  // 0x00480BC8 returns the old frame, not frame+1. With frame zero the
  // overlay disappears without another load or a restarted harvest stage.
  amount=OverlayData;OverlayTypeIndex=-1;OverlayData=0;RefreshTerrainGeometry();
 }else OverlayData=BYTE(int(OverlayData)-amount);
 game::map_resource_changed(*this);return amount;
}
bool CellClass::SpreadTiberium(bool forced){
 auto*s=ScenarioClass::Instance;if(!s)return false;if(!forced&&!CanTiberiumSpread())return false;
 int index=GetContainedTiberiumIndex();if(index<0){if(!forced)return false;index=0;}auto*t=TiberiumClass::Array.GetItemOrDefault(index);if(!t)return false;
 constexpr int dx[]={0,1,1,1,0,-1,-1,-1},dy[]={-1,-1,0,1,1,1,0,-1};unsigned start=unsigned(s->Random.RandomRanged(0,7));
 for(int i=0;i<8;++i){int n=(start+unsigned(i))%8;CellStruct p{short(MapCoords.X+dx[n]),short(MapCoords.Y+dy[n])};auto*c=MapClass::Instance.TryGetCellAt(p);if(c&&c->CanTiberiumGerminate(t)&&c->IncreaseTiberium(index,3))return true;}return false;
}

// Original predicates used by the TiberiumLogic queues.
bool CellClass::CanTiberiumGrow() const {
 auto* s=ScenarioClass::Instance;auto* t=TiberiumClass::Find(OverlayTypeIndex);
 return s&&s->TiberiumGrowthEnabled&&t&&!SlopeIndex&&OverlayData<t->NumFrames-1&&t->GrowthPercentage>=0.00001;
}
bool CellClass::CanTiberiumSpread() const {
 auto* s=ScenarioClass::Instance;auto* t=TiberiumClass::Find(OverlayTypeIndex);
 return s&&s->SpecialFlags.TiberiumSpreads&&t&&OverlayData>t->ArrayIndex/2&&!SlopeIndex&&t->SpreadPercentage>=0.00001&&!FirstObject;
}
bool CellClass::GrowTiberium(){return CanTiberiumGrow()&&IncreaseTiberium(GetContainedTiberiumIndex(),1);}
