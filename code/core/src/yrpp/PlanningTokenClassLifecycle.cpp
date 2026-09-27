// Original YRpp PlanMgr token ownership, 0x635F80 / 0x636120 / 0x636310.
// The fixed OpenTS revision has no PlanMgr; retain the original object graph,
// registry, owner link and house accounting. No replacement ownership model.
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/Unsorted.h"
#if !defined(RA2_YRPP_GAME)
namespace {
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
void clear_nodes(DynamicVectorClass<PlanningNodeClass*>& nodes) noexcept {
    // Preserve the original embedded vector's virtual Clear dispatch.
    nodes.Clear();
}
}
void PlanningTokenClass::ClearNodes() noexcept {
    const int count=PlanningNodes.Count;
    for(int i=0;i<count;++i)PlanningNodes[i]->RemoveMember(OwnerUnit);
    if(count) {
        auto* owner=OwnerUnit;
        Game::PlanningManager_RemoveHouseMember(unsigned(Game::PlanningManager_OwnerIndex(owner)));
        Game::PlanningUnits.Remove(owner);
    }
    clear_nodes(PlanningNodes);
    field_8C=ClosedLoopNodeCount=StepsToClosedLoop=-1;
    field_98=field_99=false;
}
PlanningTokenClass::~PlanningTokenClass() noexcept {
    if(OwnerUnit)OwnerUnit->PlanningToken=nullptr;
    if(!Game::PlanningDeletingAll)Array.Remove(this);
    ClearNodes();
}
void PlanningTokenClass::Destroy() noexcept {
    ClearNodes();
    OwnerUnit=nullptr;
    GameDelete(this);
}
#endif
