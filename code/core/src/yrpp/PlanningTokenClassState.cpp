// Existing YRpp PlanMgr token, YR 0x00635F20 / 0x00636570 /
// 0x00636CE0 / 0x00636E60 / 0x00636EB0 / 0x00636F00.
// No corresponding PlanMgr exists in the pinned OpenTS revision.
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/Unsorted.h"
#include <bit>
#include <new>
#if !defined(RA2_YRPP_GAME)
PlanningTokenClass::PlanningTokenClass(TechnoClass* owner) noexcept
    : OwnerUnit(owner), field_1C(false), CurrentEvent(noinit_t{}), field_8C(-1), ClosedLoopNodeCount(-1),
      StepsToClosedLoop(-1), field_98(false), field_99(false) {
    CurrentEvent.Type = EventType::Empty;
}

BOOL PlanningTokenClass::HasCommittedNodes() const noexcept {
    return PlanningNodes.Count > 0 && (field_8C == -1 || field_8C > 0);
}
PlanningNodeClass* PlanningTokenClass::GetNode(int index) const noexcept {
    const int count = PlanningNodes.Count;
    if (!count) return nullptr;
    if (index >= count) {
        if (StepsToClosedLoop < 0) return nullptr;
        const int distance = std::bit_cast<int>(unsigned(index) - unsigned(StepsToClosedLoop));
        index = std::bit_cast<int>(unsigned(StepsToClosedLoop) + unsigned(distance % ClosedLoopNodeCount));
    }
    return index >= 0 && index < count ? PlanningNodes[index] : nullptr;
}
PlanningNodeClass* PlanningTokenClass::GetLastNode() const noexcept {
    const int index = PlanningNodes.Count - 1;
    return index >= 0 ? PlanningNodes[index] : nullptr;
}
EventClass* PlanningTokenClass::GetEvent(EventClass* output, int index) const noexcept {
    *output = *PlanningNodes[index]->FindMember(OwnerUnit)->Packet;
    return output;
}
EventClass* PlanningTokenClass::ConsumeNextEvent(EventClass* output) noexcept {
    auto* node = PlanningNodes[0];
    const EventClass event(*node->FindMember(OwnerUnit)->Packet);
    CurrentEvent = event;
    PlanningNodes.RemoveItem(0);
    if (StepsToClosedLoop) {
        node->RemoveMember(OwnerUnit);
        if (StepsToClosedLoop > 0) --StepsToClosedLoop;
    } else {
        field_99 = true;
        try {
            PlanningNodes.AddItem(node);
        } catch (const std::bad_alloc&) {
            PlanningNodes.IsInitialized = true;
        }
        Game::PlanningManager_FlagNode(node);
    }
    const int remaining = std::bit_cast<int>(unsigned(field_8C) - 1u);
    field_8C = remaining < -1 ? -1 : remaining;
    if (!PlanningNodes.Count) {
        Game::PlanningManager_RemoveHouseMember(unsigned(Game::PlanningManager_OwnerIndex(OwnerUnit)));
        Game::PlanningUnits.Remove(OwnerUnit);
    }
    *output = event;
    return output;
}
void PlanningTokenClass::Commit() noexcept {
    const auto invalidate = [](PlanningNodeClass* node) noexcept {
        if (!node) return;
        node->field_B4 = -1;
        const int count = node->PlanningBranches.Count;
        for (int i = 0; i < count; ++i)
            node->PlanningBranches[i]->field_74 = -1;
        node->CommonBranch.field_74 = -1;
    };
    if (field_98) {
        if (StepsToClosedLoop != -1) invalidate(PlanningNodes[StepsToClosedLoop]);
        field_98 = false;
    }
    const int count = PlanningNodes.Count;
    for (int i = field_8C == -1 ? 0 : field_8C; i < count; ++i)
        invalidate(PlanningNodes[i]);
    field_8C = -1;
}
#endif
