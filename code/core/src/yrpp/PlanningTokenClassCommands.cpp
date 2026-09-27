// Native storage for YR's planning command mode (0xAC4CF4) and original
// node vectors (0xAC4B30 / 0xAC4C18 / 0xAC4C98).
#include "yrpp/PlanningTokenClass.h"
#if !defined(RA2_YRPP_GAME)
namespace {
bool planning_mode=false;
DynamicVectorClass<PlanningNodeClass*> all_nodes,pending_nodes,dirty_nodes;
DynamicVectorClass<PlanningTokenClass*> tokens;
}
bool& PlanningNodeClass::PlanningModeActive=planning_mode;
DynamicVectorClass<PlanningNodeClass*>& PlanningNodeClass::Unknown1=all_nodes;
DynamicVectorClass<PlanningNodeClass*>& PlanningNodeClass::Unknown2=pending_nodes;
DynamicVectorClass<PlanningNodeClass*>& PlanningNodeClass::Unknown3=dirty_nodes;
DynamicVectorClass<PlanningTokenClass*>& PlanningTokenClass::Array=tokens;
#endif
