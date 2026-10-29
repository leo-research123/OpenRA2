// Original PlanMgr token editing, YR 0x6367B0 / 0x636B50.
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/TechnoClass.h"
#if !defined(RA2_YRPP_GAME)
void PlanningTokenClass::RemoveNode(int index) noexcept {
    if (PlanningNodes.Count == 1)
        field_8C = -1;
    else if (field_8C > 0 && index < field_8C)
        --field_8C;
    if (StepsToClosedLoop >= 0) {
        if (index >= StepsToClosedLoop) {
            if (--ClosedLoopNodeCount < 3) {
                StepsToClosedLoop = -1;
                ClosedLoopNodeCount = 0;
            }
        } else
            --StepsToClosedLoop;
    }
    PlanningNodes[index]->RemoveMember(OwnerUnit);
    PlanningNodes.RemoveItem(index);
}
void PlanningTokenClass::CopyNodesTo(TechnoClass* unit, int start, int count) noexcept {
    if (!unit->PlanningToken) return;
    for (int i = start; i < start + count; ++i) {
        auto* node = PlanningNodes[i];
        EventClass event(*node->FindMember(OwnerUnit)->Packet);
        event.MegaMission.Whom = TargetClass(unit);
        node->AddMember(unit, &event);
    }
}
#endif
