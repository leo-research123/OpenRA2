// Existing YR Game/PlanMgr node registration module, 0x637640 / 0x6378B0.
// Preserve its original three node vectors and original pointer globals.
#include "yrpp/Unsorted.h"
#include "yrpp/PlanningTokenClass.h"
#include <new>
#if !defined(RA2_YRPP_GAME)
namespace {
PlanningNodeClass* node_ac4ccc=nullptr;
PlanningNodeClass* node_ac4c38=nullptr;
PlanningNodeClass* node_ac4bf0=nullptr;
DynamicVectorClass<TechnoClass*> planning_units;
bool deleting_all=false;
}
PlanningNodeClass*& Game::PlanningNodeAtAC4CCC=node_ac4ccc;
PlanningNodeClass*& Game::PlanningNodeAtAC4C38=node_ac4c38;
PlanningNodeClass*& Game::PlanningNodeAtAC4BF0=node_ac4bf0;
DynamicVectorClass<TechnoClass*>& Game::PlanningUnits=planning_units;
bool& Game::PlanningDeletingAll=deleting_all;
void YRPP_FASTCALL Game::PlanningManager_UnregisterNode(PlanningNodeClass* node) noexcept {
    PlanningNodeClass::Unknown3.Remove(node);
    PlanningNodeClass::Unknown1.Remove(node);
    PlanningNodeClass::Unknown2.Remove(node);
    if(PlanningNodeAtAC4CCC==node)PlanningNodeAtAC4CCC=nullptr;
    if(PlanningNodeAtAC4C38==node)PlanningNodeAtAC4C38=nullptr;
    if(PlanningNodeAtAC4BF0==node)PlanningNodeAtAC4BF0=nullptr;
}
void YRPP_FASTCALL Game::PlanningManager_FlagNode(PlanningNodeClass* node) noexcept {
    if(!node || node->field_1C)return;
    node->field_1C=true;
    try { PlanningNodeClass::Unknown3.AddItem(node); }
    catch(const std::bad_alloc&) { PlanningNodeClass::Unknown3.IsInitialized=true; }
}
#endif
