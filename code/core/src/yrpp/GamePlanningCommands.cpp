// YR PlanMgr command queries 0x00633BC0 / 0x00638B70 / 0x00638CE0.
// The fixed OpenTS revision has no PlanMgr module; use original Event/Target,
// token/node/member storage and virtual queries, without a replacement graph.
#include "yrpp/Unsorted.h"
#include "yrpp/EventClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/PlanningTokenClass.h"
#include <cstdlib>
#if defined(RA2_YRPP_GAME)
__declspec(noinline) bool YRPP_FASTCALL Game::PlanningManager_IsLocalGuard(TechnoClass* unit,Mission mission,TargetClass target) { JMP_STD(0x638B70); }
#else
CoordStruct* YRPP_FASTCALL Game::PlanningManager_EventCoords(CoordStruct* output,const EventClass* event) noexcept {
    if(event->Type==EventType::MegaMission) {
        // Resolve exactly one preferred target. A stale target does not fall
        // through to another field that happens to contain a valid object.
        auto target=event->MegaMission.Target.m_RTTI ? event->MegaMission.Target :
            event->MegaMission.Follow.m_RTTI ? event->MegaMission.Follow : event->MegaMission.Destination;
        if(target.m_RTTI)if(auto* object=target.As_Abstract()) {
            CoordStruct coords;*output=*object->GetCenterCoords(&coords);return output;
        }
    }
    *output={-1,-1,-1};return output;
}
bool YRPP_FASTCALL Game::PlanningManager_IsLocalGuard(TechnoClass* unit,Mission mission,TargetClass target) noexcept {
    if(mission!=Mission::Area_Guard)return false;
    if(!target.m_RTTI)return true;
    if(!unit)return false;
    CellStruct a,b;
    const auto first=*unit->GetMapCoords(&a);
    auto second=*unit->GetMapCoords(&b);
    if(auto* cell=target.As_Cell())second=cell->MapCoords;
    else if(auto* other=target.As_Techno())second=*other->GetMapCoords(&b);
    return first==second;
}
int YRPP_FASTCALL Game::PlanningManager_CheckCommand(TechnoClass* unit,const EventClass* event) noexcept {
    if(event->Type==EventType::Deploy || event->Type==EventType::Idle || event->Type==EventType::Scatter)return 0;
    const auto mission=static_cast<Mission>(static_cast<signed char>(event->MegaMission.Mission));
    if(PlanningManager_IsLocalGuard(unit,mission,event->MegaMission.Target))return 0;
    const auto* token=unit->PlanningToken;
    if(!token || token->PlanningNodes.Count<=0)return -1;
    const auto* node=token->PlanningNodes[token->PlanningNodes.Count-1];
    const int index=node->FindMemberIndex(token->OwnerUnit);
    if(index<0)std::abort(); // The original immediately dereferences the missing member.
    const auto& previous=*node->PlanningMembers[index]->Packet;
    const int old_mission=previous.MegaMission.Mission;
    if(old_mission==7 || old_mission==8 || old_mission==9)return 1;
    if((old_mission==1 || old_mission==11) &&
        (previous.MegaMission.Target.m_RTTI==11 || (old_mission==11 && !previous.MegaMission.Target.m_RTTI)))return 2;
    return -1;
}
#endif
