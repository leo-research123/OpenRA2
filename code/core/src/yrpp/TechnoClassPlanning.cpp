// Original Techno planning interface, YR 0x0070F070. Delegates through both
// original virtual slots; capability belongs to the TechnoType instance.
#include "yrpp/TechnoClass.h"
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/Unsorted.h"
bool TechnoClass::CanUseWaypoint() const { return GetTechnoType()->CanUseWaypoint(); }

bool TechnoClass::TryNextPlanningTokenNode() {
    auto* token = PlanningToken;
    if (!token || token->PlanningNodes.Count <= 0 || (token->field_8C != -1 && token->field_8C <= 0) || token->field_1C)
        return false;
    QueueMission(Mission::Wait, true);
    EventClass event(noinit_t{});
    token->ConsumeNextEvent(&event);
    Game::PlanningManager_QueueEvent(&event);
    token->field_1C = true;
    return true;
}
