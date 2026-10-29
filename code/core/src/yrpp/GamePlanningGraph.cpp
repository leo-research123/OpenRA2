// Existing YR Game/PlanMgr node registration module, 0x637640 / 0x6378B0.
// Preserve its original three node vectors and original pointer globals.
#include "yrpp/Unsorted.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include <algorithm>
#include <new>
#if !defined(RA2_YRPP_GAME)
namespace {
PlanningNodeClass* node_ac4ccc = nullptr;
PlanningNodeClass* node_ac4c38 = nullptr;
PlanningNodeClass* node_ac4bf0 = nullptr;
DynamicVectorClass<TechnoClass*> planning_units;
DynamicVectorClass<EventClass*> planning_events;
bool deleting_all = false;
bool input_blocked = false;
}
PlanningNodeClass*& Game::PlanningNodeAtAC4CCC = node_ac4ccc;
PlanningNodeClass*& Game::PlanningNodeAtAC4C38 = node_ac4c38;
PlanningNodeClass*& Game::PlanningNodeAtAC4BF0 = node_ac4bf0;
DynamicVectorClass<TechnoClass*>& Game::PlanningUnits = planning_units;
DynamicVectorClass<EventClass*>& Game::PlanningEvents = planning_events;
bool& Game::PlanningDeletingAll = deleting_all;
bool& Game::PlanningInputBlocked = input_blocked;
bool YRPP_STDCALL Game::PlanningManager_UpdateCursor(int x, int y) {
    if (!PlanningNodeClass::PlanningModeActive || PlanningInputBlocked) return false;
    auto* selected = PlanningNodeAtAC4C38;
    auto* hovered = PlanningNodeAtAC4CCC;
    if (hovered) {
        Point2D point;
        hovered->GetScreenCoords(&point);
        if (x < point.X - 8 || x >= point.X + 8 || y < point.Y - 8 || y >= point.Y + 8) hovered = nullptr;
    }
    if (!selected && !hovered) return false;
    MouseClass::Instance.SetCursor(MouseCursorType::Default, !selected || hovered == selected);
    return true;
}
namespace {
// The original's inline vector reset: drop the entries and owned storage,
// keep borrowed Items and IsInitialized.
template <class T>
void clear_slots(DynamicVectorClass<T>& list) noexcept {
    list.Count = 0;
    if (list.Items && list.IsAllocated) {
        DLLDeleteArray(list.Items, static_cast<size_t>(list.Capacity));
        list.Items = nullptr;
    }
    list.IsAllocated = false;
    list.Capacity = 0;
}
}
void YRPP_FASTCALL Game::PlanningManager_DeleteAll() noexcept {
    const int count = PlanningTokenClass::Array.Count;
    PlanningDeletingAll = true;
    PlanningNodeClass::PlanningModeActive = false;
    PlanningSelectionActive = false;
    // Each token clears its owner link and nodes; the registry is reset below.
    for (int i = 0; i < count; ++i)
        if (auto* token = PlanningTokenClass::Array.Items[i]) GameDelete(token);
    clear_slots(PlanningUnits);
    std::fill_n(PlanningMemberCounts, 24, 0);
    clear_slots(PlanningTokenClass::Array);
    clear_slots(PlanningNodeClass::Unknown1);
    clear_slots(PlanningNodeClass::Unknown2);
    std::fill_n(PlanningHelpCounts, 5, 0);
    PlanningNodeAtAC4CCC = nullptr;
    PlanningBlockedNode = nullptr;
    PlanningLoopFeedback = PlanningJoinFeedback = PlanningErrorReported = false;
    PlanningPendingError = -1;
    PlanningDeletingAll = false;
}
void YRPP_FASTCALL Game::PlanningManager_UnregisterNode(PlanningNodeClass* node) noexcept {
    PlanningNodeClass::Unknown3.Remove(node);
    PlanningNodeClass::Unknown1.Remove(node);
    PlanningNodeClass::Unknown2.Remove(node);
    if (PlanningNodeAtAC4CCC == node) PlanningNodeAtAC4CCC = nullptr;
    if (PlanningNodeAtAC4C38 == node) PlanningNodeAtAC4C38 = nullptr;
    if (PlanningNodeAtAC4BF0 == node) PlanningNodeAtAC4BF0 = nullptr;
}
void YRPP_FASTCALL Game::PlanningManager_FlagNode(PlanningNodeClass* node) noexcept {
    if (!node || node->field_1C) return;
    node->field_1C = true;
    try {
        PlanningNodeClass::Unknown3.AddItem(node);
    } catch (const std::bad_alloc&) {
        PlanningNodeClass::Unknown3.IsInitialized = true;
    }
}
#endif
