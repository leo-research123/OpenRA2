// YR planning node owner lookup, 0x00634290. Retains the original member
// vector and first-match order; fixed OpenTS has no corresponding planner.
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#if !defined(RA2_YRPP_GAME)
namespace {
// An original embedded vector may have an EXE-owned virtual implementation.
// Keep its reference opaque so the compiler cannot replace the original call
// with the native DynamicVectorClass body based on the declared member type.
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
int find_node(const DynamicVectorClass<PlanningNodeClass*>& nodes,PlanningNodeClass* node) noexcept {
    return nodes.FindItemIndex(node);
}
}
PlanningNodeClass* PlanningNodeClass::GetPrevious(TechnoClass* owner) const noexcept {
    const auto* token=owner->PlanningToken;
    auto* self=const_cast<PlanningNodeClass*>(this);
    const int index=find_node(token->PlanningNodes,self);
    if(index==-1)return nullptr;
    const int count=token->PlanningNodes.Count;
    if(count==1)return self;
    return token->GetNode(index-1<0 ? count-1 : index-1);
}
PlanningNodeClass* PlanningNodeClass::GetNext(TechnoClass* owner) const noexcept {
    const auto* token=owner->PlanningToken;
    auto* self=const_cast<PlanningNodeClass*>(this);
    const int index=find_node(token->PlanningNodes,self);
    if(index==-1)return nullptr;
    const int count=token->PlanningNodes.Count;
    if(count==1)return self;
    return token->GetNode(index+1>=count ? 0 : index+1);
}
int PlanningNodeClass::FindMemberIndex(const TechnoClass* owner) const noexcept {
    const int count=PlanningMembers.Count;
    for(int i=0;i<count;++i)if(PlanningMembers[i]->Owner==owner)return i;
    return -1;
}
TechnoClass* PlanningNodeClass::GetOwner(int index) const noexcept {
    return PlanningMembers[index]->Owner;
}
PlanningMemberClass* PlanningNodeClass::FindMember(const TechnoClass* owner) const noexcept {
    const int index=FindMemberIndex(owner);
    return index==-1 ? nullptr : PlanningMembers[index];
}
CoordStruct* PlanningNodeClass::GetCoords(CoordStruct* output) const noexcept {
    CoordStruct coords;
    *output=*Game::PlanningManager_EventCoords(&coords,PlanningMembers[0]->Packet);
    return output;
}
BOOL PlanningNodeClass::IsAt(CoordStruct coords) const noexcept {
    CoordStruct location;
    return *Game::PlanningManager_EventCoords(&location,PlanningMembers[0]->Packet)==coords;
}
#endif
