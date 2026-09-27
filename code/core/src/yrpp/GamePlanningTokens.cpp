// Original Game/PlanMgr token accounting and command clear, YR
// 0x6339B0 / 0x639F80 / 0x6386E0. Uses existing original objects and lists.
#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/EventClass.h"
#include <bit>
#if !defined(RA2_YRPP_GAME)
int YRPP_FASTCALL Game::PlanningManager_OwnerIndex(const TechnoClass* unit) noexcept {
    const auto* house=unit->Owner;
    const int index=house->ArrayIndex;
    if(house->IsControlledByCurrentPlayer() && HouseClass::CurrentPlayer)
        return HouseClass::CurrentPlayer->ArrayIndex;
    return index;
}
void YRPP_FASTCALL Game::PlanningManager_RemoveHouseMember(unsigned house) noexcept {
    if(house<24) {
        const int remaining=std::bit_cast<int>(unsigned(PlanningMemberCounts[house])-1u);
        PlanningMemberCounts[house]=remaining<0 ? 0 : remaining;
    }
}
void YRPP_FASTCALL Game::PlanningManager_ClearToken(TechnoClass* unit,const EventClass* event) noexcept {
    if(!unit)return;
    auto* token=unit->PlanningToken;
    if(!token)return;
    if((!event || !event->MegaMission.IsPlanningEvent) && token->PlanningNodes.Count>0)
        token->ClearNodes();
    token->field_1C=false;
}
#endif
