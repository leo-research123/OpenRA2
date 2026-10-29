// Original UI control groups, 0x00730990..0x007313A0. Group is one-based at
// these command entries and zero-based in TechnoClass::Group.
#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/TacticalClass.h"

#if !defined(RA2_YRPP_GAME)
namespace {
DWORD selected_at = DWORD(-100);
int last_group = -1;
bool controlled(const TechnoClass* object) {
    return SessionClass::Instance.GameMode != GameMode::Campaign ? object->Owner->IsControlledByCurrentPlayer()
                                                                 : object->Owner->IsInPlayerControl;
}
}
DWORD& Game::ControlGroupSelectedAt = selected_at;
int& Game::LastControlGroup = last_group;
bool YRPP_FASTCALL Game::ControlGroupIsSelected(int group) noexcept {
    const int count = TechnoClass::Array.Count;
    for (int i = 0; i < count; ++i) {
        auto* object = TechnoClass::Array[i];
        if (object && !object->InLimbo && controlled(object) && !object->IsSelected && object->Group == group - 1)
            return false;
    }
    return true;
}
int YRPP_FASTCALL Game::ControlGroupCount(int group) noexcept {
    int members = 0;
    const int count = TechnoClass::Array.Count;
    for (int i = 0; i < count; ++i) {
        auto* object = TechnoClass::Array[i];
        if (object && !object->InLimbo && controlled(object) && object->Group == group - 1) ++members;
    }
    return members;
}
void YRPP_FASTCALL Game::AssignControlGroup(int group) noexcept {
    const int count = TechnoClass::Array.Count;
    for (int i = 0; i < count; ++i) {
        auto* object = TechnoClass::Array[i];
        if (!object || !controlled(object)) continue;
        if (object->Group == group - 1) object->Group = -1;
        if (object->IsSelected) object->Group = group - 1;
    }
}
void YRPP_FASTCALL Game::ClearControlGroup(int group) noexcept {
    const int count = TechnoClass::Array.Count;
    for (int i = 0; i < count; ++i) {
        auto* object = TechnoClass::Array[i];
        if (object && controlled(object) && object->Group == group - 1) object->Group = -1;
    }
}
void YRPP_FASTCALL Game::SelectControlGroup(int group) noexcept {
    DisplayClass::Instance.CancelSelectionModes();
    if (SystemTimer::GetMilliseconds() - ControlGroupSelectedAt < 800u && group == LastControlGroup) {
        bool exact = true;
        for (int i = TechnoClass::Array.Count - 1; i >= 0; --i) {
            auto* object = TechnoClass::Array[i];
            if (object && !object->InLimbo && controlled(object) &&
                object->IsSelected != (object->Group == group - 1)) {
                exact = false;
                break;
            }
        }
        if (exact) {
            CenterControlGroup(group);
            return;
        }
    }
    ControlGroupSelectedAt = SystemTimer::GetMilliseconds();
    LastControlGroup = group;
    MapClass::UnselectAll();
    Unsorted::MoveFeedback = true;
    for (int i = 0; i < TechnoClass::Array.Count; ++i) {
        auto* object = TechnoClass::Array[i];
        if (object && !object->InLimbo && object->Group == group - 1 && controlled(object) && !object->IsSelected) {
            object->Select();
            object->RadarFlashTimer.Start(RulesClass::Instance->RadarCombatFlashTime);
            Unsorted::MoveFeedback = false;
        }
    }
    Unsorted::MoveFeedback = true;
    TacticalClass::StartDrawActionLineTimer();
}
void YRPP_FASTCALL Game::CenterControlGroup(int group) noexcept {
    DisplayClass::Instance.CancelSelectionModes();
    if (ObjectClass::CurrentObjects.Count) {
        auto* first = ObjectClass::CurrentObjects[0];
        if (!first || (first->AbstractFlags & AbstractFlags::Foot) == AbstractFlags::None ||
            static_cast<TechnoClass*>(first)->Group != group - 1)
            MapClass::UnselectAll();
    }
    for (int i = 0; i < TechnoClass::Array.Count; ++i) {
        auto* object = TechnoClass::Array[i];
        if (object && !object->InLimbo && object->Group == group - 1 && controlled(object) && !object->IsSelected) {
            object->Select();
            Unsorted::MoveFeedback = false;
        }
    }
    DisplayClass::CenterOnSelection();
    DisplayClass::Instance.GScreenClass::MarkNeedsRedraw(1);
    Unsorted::MoveFeedback = true;
}
#endif
