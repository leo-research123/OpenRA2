// YR TiberiumClass / TiberiumLogic, 0x007221B0–0x007236C2.
// Uses existing YRpp queues/objects; no host cell schedules or extra RNG.
// Growth_AI/Spread_AI/Queue_Growth/Queue_Spread: OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9, code/tiberium.cpp,
// GPL-3.0-or-later (code/third_party/opents/LICENSE.md), calibrated to YR.
#include "yrpp/TiberiumClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif
namespace {
using ResourceNode=PriorityQueueClassNode;
int random_mod(int divisor){
 const int r=ScenarioClass::Instance->Random.Random();
 const unsigned magnitude=r<0?0u-static_cast<unsigned>(r):static_cast<unsigned>(r);
 // The EXE wraps abs(INT_MIN), then uses signed IDIV. Keep that result
 // without signed-overflow UB; unsigned remainder changes its queue budget.
 return std::bit_cast<int>(magnitude)%divisor;
}
float frame_score(int offset){return float(std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame)+unsigned(offset)));}
float random_score(){return frame_score(random_mod(50));}
float growth_score(){
 // 0x72303C..0x72307C takes the remainder BEFORE abs, unlike registration.
 const int remainder=ScenarioClass::Instance->Random.Random()%50;
 return frame_score(remainder<0?-remainder:remainder);
}
int index(const CellStruct& c){return ResourceNode::ToSurfaceIndex(c);}
void set_membership(TiberiumLogic& l,const CellStruct& c,bool value){
 const int i=index(c);if(i>=0&&i<ResourceNode::SurfaceDataCount())l.CellIndexesWithTiberium[i]=value;
}
bool member(TiberiumLogic& l,const CellStruct& c){const int i=index(c);return i>=0&&i<ResourceNode::SurfaceDataCount()&&l.CellIndexesWithTiberium[i];}
void enqueue(TiberiumLogic& l,const CellStruct& c,float score){
 // Original Nodes is append-only until rebuild. Reject malformed/overflowing
 // maps instead of corrupting the adjacent allocation.
 if(l.Count>=ResourceNode::SurfaceDataCount())throw std::runtime_error("TiberiumLogic node capacity exhausted");
 auto* n=&l.Nodes[l.Count++];n->MapCoord=c;n->Score=score;
 if(l.Queue->Count+1<l.Queue->Capacity){
  l.Queue->Push(n);
  // Original +0x0C tracks the greatest address, +0x10 the least (YRpp's
  // LMost/RMost names are reversed). Compare addresses without pointer UB.
  const auto address=reinterpret_cast<std::uintptr_t>(n);
  if(address>reinterpret_cast<std::uintptr_t>(l.Queue->LMost))l.Queue->LMost=n;
  if(reinterpret_cast<std::uintptr_t>(l.Queue->RMost)==0xFFFFFFFFu||
     address<reinterpret_cast<std::uintptr_t>(l.Queue->RMost))l.Queue->RMost=n;
 }
 set_membership(l,c,true);
}
ResourceNode* pop(TiberiumLogic& l){auto* n=l.Queue->Top();if(n){l.Queue->Pop();l.Queue->Nodes[l.Queue->Count+1]=nullptr;}return n;}
void rebuild(TiberiumClass& type,TiberiumLogic& l,bool growth){
 const int count=ResourceNode::SurfaceDataCount();if(count<=0)return;
 if(!l.Queue)l.Construct(count);
 l.Count=0;l.Queue->Clear();l.Queue->Count=0;
 std::fill_n(l.CellIndexesWithTiberium,count,false);
 auto& map=MapClass::Instance;map.CellIteratorReset();
 while(auto* c=map.CellIteratorNext())
  if(c->GetContainedTiberiumIndex()==type.ArrayIndex&&(growth?c->CanTiberiumGrow():c->CanTiberiumSpread()))enqueue(l,c->MapCoords,0);
}
CellClass* adjacent(CellClass& c,int n){
 constexpr CellStruct delta[]={{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};
 return MapClass::Instance.TryGetCellAt(CellStruct{short(c.MapCoords.X+delta[n].X),short(c.MapCoords.Y+delta[n].Y)});
}
}
void TiberiumClass::RebuildGrowth(){rebuild(*this,GrowthLogic,true);}
void TiberiumClass::RebuildSpread(){rebuild(*this,SpreadLogic,false);}
void TiberiumClass::RegisterForGrowth(CellStruct* c){
 auto* cell=c?MapClass::Instance.TryGetCellAt(*c):nullptr;
 if(!cell||!GrowthLogic.Queue||cell->OverlayData>=11)return;
 if(GrowthLogic.Count>ResourceNode::SurfaceDataCount()-10)RebuildGrowth();
 enqueue(GrowthLogic,*c,random_score());
}
void TiberiumClass::RegisterForSpread(CellStruct* c){
 auto* cell=c?MapClass::Instance.TryGetCellAt(*c):nullptr;
 if(!cell||!SpreadLogic.Queue||!cell->CanTiberiumSpread()||member(SpreadLogic,*c))return;
 if(SpreadLogic.Count>=ResourceNode::SurfaceDataCount()-20)RebuildSpread();
 enqueue(SpreadLogic,*c,random_score());
}
void TiberiumClass::Grow(){
 auto& l=GrowthLogic;if(!l.Queue||!l.Queue->Count||!(GrowthPercentage>0.00001))return;
 const int limit=std::clamp(game::x87_integer(double(l.Queue->Count)*GrowthPercentage),5,50);
 const int budget=random_mod(limit)+1;
 // Reuse the original rebuild for append-storage exhaustion as well. The
 // EXE checks only the heap count here; exhausted append storage is outside
 // the instruction-parity cases, and must not become a native memory write.
 if(l.Queue->Count>ResourceNode::SurfaceDataCount()-2*budget||l.Count+budget>=ResourceNode::SurfaceDataCount())RebuildGrowth();
 // 0x722FB4..0x722FEC pops once even when abs(INT_MIN) made budget <= 0.
 if(budget<=0){pop(l);return;}
 for(int i=0;i<budget;++i){
  auto* node=pop(l);if(!node)return;auto coords=node->MapCoord;
  auto* cell=MapClass::Instance.TryGetCellAt(coords);
  if(!cell||cell->GetContainedTiberiumIndex()!=ArrayIndex)continue;
  cell->GrowTiberium();
  if(cell->OverlayData>=11)set_membership(l,coords,false);
  else{enqueue(l,coords,growth_score());RegisterForSpread(&coords);}
 }
}
void TiberiumClass::SpreadCells(){
 auto& l=SpreadLogic;if(!l.Queue||!l.Queue->Count||!(SpreadPercentage>0.00001))return;
 const int limit=std::clamp(game::x87_integer(double(l.Queue->Count)*SpreadPercentage),5,25);
 const int budget=random_mod(limit)+1;
 if(l.Queue->Count>ResourceNode::SurfaceDataCount()-20)RebuildSpread();
 if(budget<=0){pop(l);return;}
 int processed=0;
 while(processed<budget){
  if(l.Count>=ResourceNode::SurfaceDataCount()-1)RebuildSpread();
  auto* node=pop(l);if(!node)return;
  auto* cell=MapClass::Instance.TryGetCellAt(node->MapCoord);if(!cell)continue;
  int neighbours=0;for(int i=0;i<8;++i)if(auto* c=adjacent(*cell,i))if(c->CanTiberiumGerminate(this))++neighbours;
  if(neighbours){cell->SpreadTiberium(false);++processed;if(neighbours>1)enqueue(l,cell->MapCoords,0);}
  else set_membership(l,cell->MapCoords,false);
 }
}
void TiberiumClass::UpdateGrowth(){
 auto* s=ScenarioClass::Instance;if(!s||!s->TiberiumGrowthEnabled)return;
 // Original tests remaining time, including a paused timer's saved delay.
 for(auto* type:Array)if(type->GrowthLogic.Timer.GetTimeLeft()==0){
  type->Grow();
  // OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9, code/tiberium.cpp,
  // Tiberium_Growth (GPL-3.0-or-later; code/third_party/opents/LICENSE.md).
  // YR 0x722CC6..0x722CCE multiplies Growth before _ftol; the decompiler
  // loses that x87 operand. Preserve the caller's rounding for the product.
  type->GrowthLogic.Timer.Start(game::x87_integer(double(type->Growth)*(s->SpecialFlags.TiberiumGrows?0.3:1.0)));
 }
}
void TiberiumClass::UpdateSpread(){
 auto* s=ScenarioClass::Instance;if(!s||!s->TiberiumGrowthEnabled)return;
 for(auto* type:Array)if(type->SpreadLogic.Timer.GetTimeLeft()==0){
  type->SpreadCells();type->SpreadLogic.Timer.Start(type->Spread);
 }
}
