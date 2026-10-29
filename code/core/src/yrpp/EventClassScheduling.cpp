// YR Queue.CPP: local 0x00647260, command dispatch 0x0064C380, cleanup 0x0064D990.
#include "yrpp/EventClass.h"
#include "yrpp/HouseClass.h"
#include <bit>
#include <exception>

namespace {
BYTE& execution_flags(EventClass& event) noexcept { return reinterpret_cast<BYTE*>(&event)[1]; }
bool same_target(const TargetClass& a, const TargetClass& b) noexcept {
    return a.m_ID == b.m_ID && a.m_RTTI == b.m_RTTI;
}
bool same_mission(const EventClass& a, const EventClass& b) noexcept {
    return a.MegaMission.Mission == b.MegaMission.Mission && same_target(a.MegaMission.Target, b.MegaMission.Target) &&
           same_target(a.MegaMission.Destination, b.MegaMission.Destination);
}
void execute_mega_missions() {
    int first = 0;
    for (int i = 0; i < EventClass::MegaMissionList.Count; ++i) {
        auto& queue = EventClass::MegaMissionList;
        if (i && !same_mission(queue[i - 1], queue[i])) {
            if (i - first > 1) EventClass::AdjustMegaMissionDestinations(first, i - first);
            first = i;
        } else if (i == queue.Count - 1 && i - first + 1 > 1) {
            EventClass::AdjustMegaMissionDestinations(first, i - first + 1);
        }
    }
    while (EventClass::MegaMissionList.Count) {
        // Execute in-place: original callbacks can observe and change the queue.
        EventClass::MegaMissionList.First().Execute();
        EventClass::MegaMissionList.Next();
    }
}
}

void EventClass::ProcessLocalQueues() noexcept {
    try {
        while (OutList.Count) {
            execution_flags(OutList.First()) &= static_cast<BYTE>(~1u);
            // The original local branch consumes the source even if DoList is full.
            DoList.Add(OutList.First());
            OutList.Next();
        }
        ExecuteLocalDoList();
        CleanDoList();
    } catch (...) {
        std::terminate();
    }
}

void EventClass::ExecuteLocalDoList() noexcept {
    try {
        const int houses = HouseClass::Array.Count;
        const int events = DoList.Count;
        for (int h = 0; h < houses; ++h) {
            auto* house = HouseClass::Array[h];
            if (!house || (!house->IsHumanPlayer && !house->IsInPlayerControl)) continue;
            for (int i = 0; i < events; ++i) {
                auto& event = DoList[i];
                if (event.HouseIndex != house->ArrayIndex ||
                    Unsorted::CurrentFrame < std::bit_cast<int>(static_cast<unsigned>(event.Frame)) ||
                    (execution_flags(event) & 1))
                    continue;
                // Network frame checks, disconnect handling and recording are
                // not produced by the standalone campaign/skirmish session.
                if (event.Type == EventType::FrameInfo || event.Type == EventType::Exit) std::terminate();
                LastExecutionDelay =
                    std::bit_cast<int>(SystemTimer::GetMilliseconds() - static_cast<unsigned>(DoList.GetTiming(i)));
                if (event.Type == EventType::MegaMission)
                    MegaMissionList.Add(event);
                else
                    event.Execute();
                execution_flags(DoList[i]) |= 1;
            }
            execute_mega_missions();
        }
    } catch (...) {
        std::terminate();
    }
}

void EventClass::CleanDoList() noexcept {
    while (DoList.Count) {
        auto& event = DoList.First();
        // EventClass is packed; materialize the field before bit_cast binds a reference.
        if (!(execution_flags(event) & 1) &&
            Unsorted::CurrentFrame <= std::bit_cast<int>(static_cast<unsigned>(event.Frame)))
            break;
        DoList.Next();
    }
}
