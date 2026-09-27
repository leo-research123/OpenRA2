// YR PlanMgr 0x00639040/0x00639130/0x00639DA0/0x00639E30. The fixed OpenTS
// 44fac744 baseline has no PlanMgr module; retain the existing YRpp original
// token/node/member structures and Game's existing PlanMgr entry-point scope.
#include "yrpp/Unsorted.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/VocClass.h"
#include <bit>
#if !defined(RA2_YRPP_GAME)
namespace {
bool error_reported=false;
int member_counts[24]{};
TechnoClass* techno(ObjectClass* object) noexcept {
    return object && (object->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None
        ?static_cast<TechnoClass*>(object):nullptr;
}
void error(const char* label) noexcept {
    if(Game::PlanningErrorReported)return;
    // Mark before querying the CSF or audio services, as the original does.
    Game::PlanningErrorReported=true;
    Game::ShowMessage(StringTable::LoadString(label,nullptr,"D:\\ra2mdpost\\PlanMgr.cpp",3233),480);
    VocClass::PlayGlobal(RulesClass::Instance->ScoldSound,0x2000,1.0f,nullptr);
}
bool includes(const PlanningNodeClass* node,const TechnoClass* owner) noexcept {
    for(int i=0;i<node->PlanningMembers.Count;++i)
        if(node->PlanningMembers[i]->Owner==owner)return true;
    return false;
}
}
bool& Game::PlanningErrorReported=error_reported;
int (&Game::PlanningMemberCounts)[24]=member_counts;

bool YRPP_FASTCALL Game::PlanningManager_CompatibleTokens(const PlanningTokenClass* first,const PlanningTokenClass* second) noexcept {
    const int first_count=first?first->PlanningNodes.Count:0;
    const int second_count=second?second->PlanningNodes.Count:0;
    if(!first_count)return !second_count;
    if(!second_count)return false;
    // For initialized original vectors count-1 is always in range. Token loop
    // wrapping is irrelevant for this particular last-node query.
    const auto* first_node=first->PlanningNodes[first_count-1];
    const auto* second_node=second->PlanningNodes[second_count-1];
    return includes(first_node,second->OwnerUnit) && includes(second_node,first->OwnerUnit);
}
bool YRPP_CDECL Game::PlanningManager_CheckSelection() noexcept {
    if(!PlanningNodeClass::PlanningModeActive)return true;
    auto& selected=ObjectClass::CurrentObjects;
    const int count=selected.Count;
    if(!count)return true;
    TechnoClass* first=nullptr;
    for(int i=0;i<count;++i)if((first=techno(selected[i])))break;
    // No Techno produces false without feedback in the original.
    if(!first)return false;
    auto* token=first->PlanningToken;
    for(int i=0;i<count;++i){
        auto* unit=techno(selected[i]);
        if(!unit || !PlanningManager_CompatibleTokens(token,unit->PlanningToken)){
            error("MSG:PlanningModeHeteroSel");return false;
        }
    }
    return true;
}
bool YRPP_CDECL Game::PlanningManager_CheckCapacity() noexcept {
    if(!PlanningNodeClass::PlanningModeActive)return true;
    const unsigned house=unsigned(HouseClass::CurrentPlayer->ArrayIndex);
    const int existing=house<24?PlanningMemberCounts[house]:0;
    auto& selected=ObjectClass::CurrentObjects;
    const int count=selected.Count;
    unsigned added=0;
    for(int i=0;i<count;++i)if(auto* unit=techno(selected[i]))
        if(!unit->PlanningToken || !unit->PlanningToken->PlanningNodes.Count)++added;
    const int total=std::bit_cast<int>(unsigned(existing)+added);
    if(total<=128)return true;
    error("MSG:PlannerMaximum");return false;
}
int YRPP_CDECL Game::PlanningManager_UnsupportedType() noexcept {
    auto& selected=ObjectClass::CurrentObjects;
    const int count=selected.Count;
    for(int i=0;i<count;++i){
        auto* unit=techno(selected[i]);
        if(!unit)return 2;
        if(!unit->CanUseWaypoint()){
            if(unit->WhatAmI()==AbstractType::Aircraft)return 0;
            // Deliberately preserve the second virtual query for non-aircraft.
            if(unit->WhatAmI()==AbstractType::Building)return 1;
            return 2;
        }
    }
    return -1;
}
#endif
