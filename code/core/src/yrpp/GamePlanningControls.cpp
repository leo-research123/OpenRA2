// YR PlanMgr input state and UI commands. All graph edits use original events.
#include "yrpp/Unsorted.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/WWMouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/StringTable.h"
#include <cstdlib>
#include <new>
#if !defined(RA2_YRPP_GAME)
namespace {
int help_counts[5]{};
PlanningNodeClass* blocked_node = nullptr;
Point2D right_origin{-1, -1};
INoticeSink* notice_sink = nullptr;
bool hit(PlanningNodeClass* node, Point2D point) noexcept {
    if (!node || !TacticalClass::Instance) return false;
    RectangleStruct bounds;
    node->GetScreenBounds(&bounds);
    return point.X >= bounds.X - 4 && point.X < bounds.X + bounds.Width + 4 && point.Y >= bounds.Y - 4 &&
           point.Y < bounds.Y + bounds.Height + 4;
}
PlanningNodeClass* hit_any(Point2D point) noexcept {
    for (int i = 0; i < PlanningNodeClass::Unknown1.Count; ++i)
        if (hit(PlanningNodeClass::Unknown1[i], point)) return PlanningNodeClass::Unknown1[i];
    return nullptr;
}
void message(const char* label, int duration) noexcept {
    Game::ShowMessage(StringTable::LoadString(label, nullptr, "D:\\ra2mdpost\\PlanMgr.cpp", 3233), duration);
}
}
int (&Game::PlanningHelpCounts)[5] = help_counts;
PlanningNodeClass*& Game::PlanningBlockedNode = blocked_node;
void YRPP_FASTCALL Game::PlanningManager_SetNoticeSink(INoticeSink* sink) noexcept { notice_sink = sink; }
void YRPP_CDECL Game::PlanningManager_Begin() noexcept {
    if (PlanningNodeClass::PlanningModeActive) return;
    PlanningNodeClass::PlanningModeActive = true;
    VocClass::PlayGlobal(RulesClass::Instance->StartPlanningModeSound, 0x2000, 1.0f, nullptr);
    if (notice_sink) notice_sink->INoticeSink_Unknown(0x11C7, 0);
}
void YRPP_CDECL Game::PlanningManager_End() noexcept {
    if (!PlanningNodeClass::PlanningModeActive) return;
    EventClass::AddEvent(EventClass(HouseClass::CurrentPlayer->ArrayIndex, EventType::PlanCommit));
    PlanningNodeAtAC4CCC = PlanningNodeAtAC4C38 = PlanningNodeAtAC4BF0 = nullptr;
    PlanningNodeClass::PlanningModeActive = false;
    VocClass::PlayGlobal(RulesClass::Instance->EndPlanningModeSound, 0x2000, 1.0f, nullptr);
    if (notice_sink) notice_sink->INoticeSink_Unknown(0x11C8, 0);
}
void YRPP_FASTCALL Game::PlanningManager_Introduction(bool fromButton, bool enabled) noexcept {
    const int index = enabled ? (fromButton ? 0 : 1) : 2;
    if (fromButton || PlanningHelpCounts[index] < 1) {
        message(enabled ? (fromButton ? "MSG:PlanningModeIntro1Button" : "MSG:PlanningModeIntro1Key")
                        : "MSG:PlanningModeIntro3",
                480);
        ++PlanningHelpCounts[index];
    }
}
void YRPP_FASTCALL Game::PlanningManager_Help(int index) noexcept {
    if (PlanningHelpCounts[index] >= 1) return;
    if (index == 3)
        message("MSG:PlanningModeLoopHelp", 224);
    else if (index == 4)
        message("MSG:PlanningModeDeleteHelp", 224);
    ++PlanningHelpCounts[index];
}
void YRPP_FASTCALL Game::PlanningManager_Report(const char* label) noexcept {
    if (PlanningErrorReported) return;
    PlanningErrorReported = true;
    message(label, 480);
    VocClass::PlayGlobal(RulesClass::Instance->ScoldSound, 0x2000, 1.0f, nullptr);
}
void YRPP_FASTCALL Game::BeginPlanning(bool fromButton) noexcept {
    if (Unsorted::UserInputLocked) return;
    PlanningManager_Begin();
    PlanningManager_Introduction(fromButton, true);
}
void YRPP_FASTCALL Game::EndPlanning(bool fromButton) noexcept {
    PlanningManager_End();
    if (!Unsorted::UserInputLocked) PlanningManager_Introduction(fromButton, false);
}
int YRPP_FASTCALL Game::PlanningManager_CheckNode(PlanningNodeClass* node) noexcept {
    const int count = ObjectClass::CurrentObjects.Count;
    for (int i = 0; i < count; ++i) {
        auto* object = ObjectClass::CurrentObjects[i];
        if (!object || (object->AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None) continue;
        auto* token = static_cast<TechnoClass*>(object)->PlanningToken;
        if (!token) continue;
        if (token->StepsToClosedLoop >= 0) return 1;
        const int index = token->PlanningNodes.FindItemIndex(node);
        if (index == -1) return 5;
        if (index == token->PlanningNodes.Count - 1) return 2;
        if (index == token->PlanningNodes.Count - 2) return 3;
    }
    return -1;
}
void YRPP_CDECL Game::PlanningManager_UpdateHover() noexcept {
    if (PlanningInputBlocked || !WWMouseClass::Instance) return;
    Point2D point{WWMouseClass::Instance->GetX(), WWMouseClass::Instance->GetY()};
    PlanningNodeClass* node = nullptr;
    for (int i = 0; i < PlanningNodeClass::Unknown2.Count; ++i)
        if (hit(PlanningNodeClass::Unknown2[i], point)) {
            node = PlanningNodeClass::Unknown2[i];
            break;
        }
    if (node == PlanningNodeAtAC4CCC) return;
    PlanningBlockedNode = nullptr;
    if (node && (node->GetOwner(0)->Owner != HouseClass::CurrentPlayer ||
                 (PlanningNodeAtAC4C38 && PlanningNodeAtAC4C38 != node) || PlanningManager_CheckNode(node) != -1)) {
        PlanningBlockedNode = node;
        node = nullptr;
    }
    PlanningNodeAtAC4CCC = node;
    // 0x634B20 only queries owners and calls the original no-op observer.
    if (node)
        for (int i = 0; i < node->PlanningMembers.Count; ++i) {
            node->GetOwner(i)->GetType();
            node->GetOwner(i)->GetMapCoords();
        }
}
void YRPP_CDECL Game::PlanningManager_ClickNode() noexcept {
    auto* node = PlanningNodeAtAC4C38;
    PlanningNodeAtAC4C38 = nullptr;
    if (ObjectClass::CurrentObjects.Count <= 0) {
        PlanningNodeAtAC4BF0 = node;
        PlanningManager_Help(4);
        return;
    }
    DynamicVectorClass<TechnoClass*> units;
    try {
        for (int i = 0; i < ObjectClass::CurrentObjects.Count; ++i) {
            auto* object = ObjectClass::CurrentObjects[i];
            if (!object || (object->AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None) continue;
            auto* unit = static_cast<TechnoClass*>(object);
            auto* last = unit->PlanningToken ? unit->PlanningToken->GetLastNode() : nullptr;
            if (last) {
                for (int j = 0; j < last->PlanningMembers.Count; ++j)
                    if (units.FindItemIndex(last->GetOwner(j)) == -1) units.AddItem(last->GetOwner(j));
            } else
                units.AddItem(unit);
        }
    } catch (const std::bad_alloc&) {
        units.IsInitialized = true;
    }
    for (int i = 0; i < units.Count; ++i) {
        const auto& event = *node->PlanningMembers[0]->Packet;
        auto target = event.MegaMission.Target.m_RTTI   ? event.MegaMission.Target
                      : event.MegaMission.Follow.m_RTTI ? event.MegaMission.Follow
                                                        : event.MegaMission.Destination;
        auto* object = target.m_RTTI ? target.As_Abstract() : nullptr;
        if (object)
            EventClass::AddEvent(EventClass(HouseClass::CurrentPlayer->ArrayIndex, EventType::PlanConnect,
                                            TargetClass(units[i]), TargetClass(object)));
    }
    if (units.Count > 0) PlanningManager_Help(3);
}
bool YRPP_STDCALL Game::PlanningManager_LeftDown(Point2D point) noexcept {
    if (!PlanningNodeClass::PlanningModeActive) return false;
    auto* pressed = PlanningNodeAtAC4C38;
    auto* hovered = hit(PlanningNodeAtAC4CCC, point) ? PlanningNodeAtAC4CCC : nullptr;
    if (hovered && hovered != pressed) {
        MouseClass::Instance.SetCursor(MouseCursorType::Default, true);
        PlanningNodeAtAC4C38 = hovered;
        return true;
    }
    if (auto* node = hit_any(point)) {
        if (PlanningManager_CheckNode(node) != -1) {
            PlanningManager_Report("MSG:PlanningModeInvalidNodeX");
            return true;
        }
        return pressed || hovered;
    }
    const int unsupported = PlanningManager_UnsupportedType();
    if (unsupported == -1) return pressed || hovered;
    PlanningManager_Report(unsupported == 0   ? "MSG:PlanningModeNoAircraft"
                           : unsupported == 1 ? "MSG:PlanningModeNoBuildings"
                                              : "MSG:PlanningModeNoUnknowns");
    return true;
}
bool YRPP_STDCALL Game::PlanningManager_LeftUp(Point2D point) noexcept {
    if (!PlanningNodeClass::PlanningModeActive) return false;
    auto* pressed = PlanningNodeAtAC4C38;
    auto* hovered = hit(PlanningNodeAtAC4CCC, point) ? PlanningNodeAtAC4CCC : nullptr;
    if (hovered && hovered == pressed) {
        MouseClass::Instance.SetCursor(MouseCursorType::Default, true);
        PlanningManager_ClickNode();
    } else {
        auto* node = hit_any(point);
        if (node && PlanningManager_CheckNode(node) != -1) return true;
        PlanningNodeAtAC4C38 = nullptr;
    }
    return pressed || hovered;
}
void YRPP_FASTCALL Game::PlanningManager_SubmitDelete(PlanningNodeClass* node) noexcept {
    const auto& event = *node->PlanningMembers[0]->Packet;
    auto target = event.MegaMission.Target.m_RTTI   ? event.MegaMission.Target
                  : event.MegaMission.Follow.m_RTTI ? event.MegaMission.Follow
                                                    : event.MegaMission.Destination;
    auto* object = target.m_RTTI ? target.As_Abstract() : nullptr;
    if (object) {
        TargetClass resolved(object);
        EventClass::AddEvent(EventClass(HouseClass::CurrentPlayer->ArrayIndex, EventType::PlanNodeDelete, resolved.m_ID,
                                        resolved.m_RTTI));
    }
}
void YRPP_STDCALL Game::PlanningManager_RightDown(Point2D point) noexcept {
    right_origin = {-1, -1};
    PlanningInputBlocked = true;
    if (PlanningNodeClass::PlanningModeActive && ObjectClass::CurrentObjects.Count <= 0) right_origin = point;
}
void YRPP_STDCALL Game::PlanningManager_WM_RBUTTONUP_63AB00(Point2D point) {
    PlanningInputBlocked = false;
    if (PlanningNodeClass::PlanningModeActive && ObjectClass::CurrentObjects.Count <= 0 && PlanningNodeAtAC4BF0 &&
        std::abs(point.X - right_origin.X) <= 3 && std::abs(point.Y - right_origin.Y) <= 3)
        PlanningNodeAtAC4BF0 = nullptr;
}
#endif
