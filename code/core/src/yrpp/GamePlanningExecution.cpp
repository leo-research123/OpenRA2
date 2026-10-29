// Original PlanMgr admission, editing, dispatch and maintenance (YR 0x637270..0x639CB0).
#include "yrpp/Unsorted.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include <cstdlib>
#include <new>
#if !defined(RA2_YRPP_GAME)
namespace {
int pending_error = -1;
bool loop_feedback = false, join_feedback = false;
template <class T>
void append(DynamicVectorClass<T*>& list, T* item) noexcept {
    try {
        list.AddItem(item);
    } catch (const std::bad_alloc&) {
        list.IsInitialized = true;
    }
}
PlanningTokenClass* ensure_token(TechnoClass* unit) noexcept {
    if (!unit->PlanningToken) {
        void* memory = YRMemory::Allocate(sizeof(PlanningTokenClass));
        if (!memory) std::abort();
        auto* token = ::new (memory) PlanningTokenClass(unit);
        append(PlanningTokenClass::Array, token);
        unit->PlanningToken = token;
    }
    return unit->PlanningToken;
}
PlanningNodeClass* find_node(CoordStruct coords, int house, unsigned frame = ~0u) noexcept {
    for (int i = 0; i < PlanningNodeClass::Unknown1.Count; ++i) {
        auto* node = PlanningNodeClass::Unknown1[i];
        if (node->IsAt(coords) && (frame == ~0u || frame == unsigned(node->field_18)) &&
            (house == -1 || house == Game::PlanningManager_OwnerIndex(node->GetOwner(0))))
            return node;
    }
    return nullptr;
}
bool continual(Mission mission, TargetClass target) noexcept {
    return (mission == Mission::Attack || mission == Mission::Area_Guard) &&
           (target.m_RTTI == static_cast<char>(AbstractType::Cell) ||
            (mission == Mission::Area_Guard && !target.m_RTTI));
}
void rebuild(PlanningNodeClass* node) noexcept {
    node->ClearBranches();
    for (int i = 0; i < node->PlanningMembers.Count; ++i)
        node->AddBranch(node->PlanningMembers[i]);
    node->UpdateLoopBranch();
    node->field_1C = false;
}
void deselect_members(PlanningNodeClass* node) noexcept {
    for (int i = 0; i < node->PlanningMembers.Count; ++i) {
        auto* owner = node->GetOwner(i);
        if (owner->IsSelected) owner->Deselect();
    }
}
}
int& Game::PlanningPendingError = pending_error;
bool& Game::PlanningLoopFeedback = loop_feedback;
bool& Game::PlanningJoinFeedback = join_feedback;

bool YRPP_FASTCALL Game::PlanningManager_IsReady(TechnoClass* unit) noexcept {
    if (!unit) return false;
    const auto mission = unit->GetCurrentMission();
    if (mission == Mission::Guard) return true;
    return continual(mission, TargetClass(unit->Target));
}
bool YRPP_FASTCALL Game::PlanningManager_AddCommand(TechnoClass* unit, const EventClass* event) noexcept {
    if (!unit) return false;
    const bool local = HouseClass::CurrentPlayer && event->HouseIndex == HouseClass::CurrentPlayer->ArrayIndex;
    CoordStruct coords;
    PlanningManager_EventCoords(&coords, event);
    if (coords == CoordStruct{-1, -1, -1}) return false;
    auto* node = find_node(coords, event->HouseIndex, event->Frame);
    if (node && node->FindMemberIndex(unit) != -1) return true;
    const int error = PlanningManager_CheckCommand(unit, event);
    if (error != -1) {
        if (local) {
            if (error == 1)
                PlanningManager_Report("MSG:PostTerminatingCommand");
            else if (error == 2)
                PlanningManager_Report("MSG:PostContinualCommand");
            else if (event->Type == EventType::Deploy)
                PlanningManager_Report("MSG:PlanningModeNoDeploy");
            else if (event->Type == EventType::Idle)
                PlanningManager_Report("MSG:PlanningModeNoStop");
            else if (event->Type == EventType::Scatter)
                PlanningManager_Report("MSG:PlanningModeNoScatter");
        }
        return false;
    }
    auto* token = ensure_token(unit);
    const auto mission = static_cast<Mission>(event->MegaMission.Mission);
    if (local) {
        if (mission == Mission::Enter || mission == Mission::Capture || mission == Mission::Harvest ||
            continual(mission, event->MegaMission.Target))
            PlanningPendingError = 0;
        else if (token->PlanningNodes.Count == 15)
            PlanningPendingError = 2;
    }
    if (token->StepsToClosedLoop >= 0) {
        if (local) PlanningManager_Report("MSG:PostLoop");
        return false;
    }
    if (token->PlanningNodes.Count == 16) {
        if (local) PlanningManager_Report("MSG:NodeMaximum");
        return false;
    }
    if (!token->PlanningNodes.Count) {
        append(PlanningUnits, unit);
        if (unsigned(event->HouseIndex) < 24) ++PlanningMemberCounts[unsigned(event->HouseIndex)];
    }
    if (!node) {
        void* memory = YRMemory::Allocate(sizeof(PlanningNodeClass));
        if (!memory) std::abort();
        node = ::new (memory) PlanningNodeClass;
        node->field_18 = static_cast<int>(event->Frame);
        node->field_1C = false;
        node->field_B0 = node->field_B4 = -1;
        append(PlanningNodeClass::Unknown1, node);
    }
    if (auto* previous = token->GetLastNode()) {
        const int count = previous->PlanningMembers.Count;
        for (int i = 0; i < count; ++i) {
            auto* member = previous->GetOwner(i);
            EventClass copy(*event);
            copy.MegaMission.Whom = TargetClass(member);
            node->AddMember(member, &copy);
        }
    } else
        node->AddMember(unit, event);
    if (local) VocClass::PlayGlobal(RulesClass::Instance->AddPlanningModeCommandSound, 0x2000, 1.0f, nullptr);
    return true;
}
void YRPP_FASTCALL Game::PlanningManager_AdjustLoop(TechnoClass* unit) noexcept {
    auto* token = unit->PlanningToken;
    auto* first = token->GetNode(0);
    bool attack = false;
    for (int i = 0; i < first->PlanningMembers.Count; ++i)
        if (first->GetOwner(i)->CanAttackOnTheMove()) {
            attack = true;
            break;
        }
    if (!attack) return;
    for (int i = token->StepsToClosedLoop; i < token->PlanningNodes.Count; ++i) {
        auto* node = token->GetNode(i);
        for (int j = 0; j < node->PlanningMembers.Count; ++j) {
            auto* event = node->PlanningMembers[j]->Packet;
            if (event->MegaMission.Mission == static_cast<BYTE>(Mission::Move))
                event->MegaMission.Mission = static_cast<BYTE>(Mission::AttackMove);
        }
    }
}
void YRPP_FASTCALL Game::PlanningManager_MakeLoop(TechnoClass* unit, PlanningNodeClass* node) noexcept {
    auto* token = unit->PlanningToken;
    token->StepsToClosedLoop = token->PlanningNodes.FindItemIndex(node);
    token->field_98 = true;
    token->ClosedLoopNodeCount = token->PlanningNodes.Count - token->StepsToClosedLoop;
    if (!PlanningLoopFeedback) {
        if (unit->Owner->IsControlledByCurrentPlayer()) {
            PlanningLoopFeedback = true;
            deselect_members(token->GetLastNode());
        }
        PlanningManager_AdjustLoop(unit);
    }
    node->UpdateLoopBranch();
}
void YRPP_FASTCALL Game::PlanningManager_Join(TechnoClass* unit, PlanningNodeClass* node) noexcept {
    auto* source = node->GetOwner(0)->PlanningToken;
    const int index = source->PlanningNodes.FindItemIndex(node);
    source->CopyNodesTo(unit, index, source->PlanningNodes.Count - index);
    auto* token = unit->PlanningToken;
    int loop_start = -1, loop_count = -1;
    PlanningNodeClass* loop = nullptr;
    if (source->StepsToClosedLoop >= 0) {
        loop_count = source->ClosedLoopNodeCount;
        if (source->StepsToClosedLoop > 0 && index < source->StepsToClosedLoop) {
            loop = source->GetNode(source->StepsToClosedLoop);
        } else {
            source->CopyNodesTo(unit, source->StepsToClosedLoop, index - source->StepsToClosedLoop);
            loop = node;
        }
        loop_start = token->PlanningNodes.FindItemIndex(loop);
    }
    token->ClosedLoopNodeCount = loop_count;
    token->StepsToClosedLoop = loop_start;
    token->field_98 = true;
    if (loop) loop->UpdateLoopBranch();
    if (!PlanningJoinFeedback && unit->Owner->IsControlledByCurrentPlayer()) {
        PlanningJoinFeedback = true;
        auto* last = token->GetLastNode();
        if (loop)
            deselect_members(last);
        else
            for (int i = 0; i < last->PlanningMembers.Count; ++i)
                if (!last->GetOwner(i)->IsSelected) last->GetOwner(i)->Select();
    }
}
void YRPP_FASTCALL Game::PlanningManager_DeleteNode(PlanningNodeClass* node) noexcept {
    auto* next = node->GetNext(node->GetOwner(0));
    const bool rebuild_next = next && next != node;
    for (int i = node->PlanningMembers.Count - 1; i >= 0; --i) {
        auto* token = node->GetOwner(i)->PlanningToken;
        token->RemoveNode(token->PlanningNodes.FindItemIndex(node));
    }
    if (rebuild_next) rebuild(next);
}
void YRPP_FASTCALL Game::PlanningManager_Execute(const EventClass* event) noexcept {
    const int house = event->HouseIndex;
    if (event->Type == EventType::PlanCommit) {
        const int count = TechnoClass::Array.Count;
        for (int i = 0; i < count; ++i) {
            auto* unit = TechnoClass::Array[i];
            if (PlanningManager_OwnerIndex(unit) != house || !unit->PlanningToken) continue;
            auto* token = unit->PlanningToken;
            const bool committed = token->HasCommittedNodes();
            token->Commit();
            const bool ready = PlanningManager_IsReady(unit);
            if (!committed && ready) unit->TryNextPlanningTokenNode();
        }
    } else if (event->Type == EventType::PlanConnect) {
        auto* unit = TargetClass(event->PlanConnect.Target1).As_Techno();
        if (!unit) return;
        auto* target = TargetClass(event->PlanConnect.Target2).As_Abstract();
        if (!target) return;
        auto* node = find_node(target->GetCenterCoords(), house);
        if (!node) return;
        auto* token = ensure_token(unit);
        if (token->PlanningNodes.FindItemIndex(node) == -1)
            PlanningManager_Join(unit, node);
        else
            PlanningManager_MakeLoop(unit, node);
    } else if (event->Type == EventType::PlanNodeDelete) {
        auto* target = TargetClass(event->PlanNodeDelete.Target).As_Abstract();
        if (!target) return;
        auto* node = find_node(target->GetCenterCoords(), house);
        if (!node) return;
        auto* token = node->GetOwner(0)->PlanningToken;
        auto* next = token->GetNode(token->PlanningNodes.FindItemIndex(node) + 1);
        PlanningManager_DeleteNode(node);
        if (next && HouseClass::CurrentPlayer && house == HouseClass::CurrentPlayer->ArrayIndex)
            PlanningNodeAtAC4BF0 = next;
    } else
        PlanningManager_AddCommand(TargetClass(event->MegaMission.Whom).As_Techno(), event);
}
void YRPP_CDECL Game::PlanningManager_FlushEvents() noexcept {
    for (int i = PlanningEvents.Count - 1; i >= 0; --i) {
        auto* event = PlanningEvents[i];
        EventClass::ScheduleEvent(event);
        PlanningEvents.RemoveItem(i);
        GameDelete(event);
    }
}
void YRPP_CDECL Game::PlanningManager_Update() noexcept {
    PlanningManager_UpdateHover();
    if (PlanningNodeAtAC4BF0 && ObjectClass::CurrentObjects.Count > 0) PlanningNodeAtAC4BF0 = nullptr;
    const int count = PlanningTokenClass::Array.Count;
    for (int t = 0; t < count; ++t) {
        auto* token = PlanningTokenClass::Array[t];
        for (int i = token->PlanningNodes.Count - 1; i >= 0; --i) {
            auto* node = token->PlanningNodes[i];
            const auto* event = node->FindMember(token->OwnerUnit)->Packet;
            auto target = event->MegaMission.Target.m_RTTI ? event->MegaMission.Target : event->MegaMission.Destination;
            if (event->Type == EventType::MegaMission && target.m_RTTI && target.As_Abstract()) continue;
            PlanningManager_FlagNode(node->GetNext(token->OwnerUnit));
            token->RemoveNode(i);
        }
    }
    const int dirty = PlanningNodeClass::Unknown3.Count;
    for (int i = 0; i < dirty; ++i)
        rebuild(PlanningNodeClass::Unknown3[i]);
    PlanningNodeClass::Unknown3.Clear();
    if (PlanningPendingError != -1) {
        while (ObjectClass::CurrentObjects.Count)
            ObjectClass::CurrentObjects[0]->Deselect();
        SetSelectionCommandMode(0);
        PlanningManager_Report(PlanningPendingError == 0   ? "MSG:PostTerminatingCommand"
                               : PlanningPendingError == 1 ? "MSG:PostContinualCommand"
                                                           : "MSG:NodeMaximum");
        PlanningPendingError = -1;
    }
    PlanningJoinFeedback = PlanningLoopFeedback = PlanningErrorReported = false;
}
#endif
