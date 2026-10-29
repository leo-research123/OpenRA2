// Original event module submission, YR 0x006521C0. Retains the existing
// TS++ QueueClass/EventClass model and the authoritative SystemTimer source.
#include "yrpp/EventClass.h"
#include "runtime_diagnostics.hpp"
#if !defined(RA2_YRPP_GAME)
bool YRPP_STDCALL EventClass::AddEvent(EventClass event) noexcept {
    event.Frame = static_cast<unsigned>(Unsorted::CurrentFrame);
    const bool accepted = OutList.Add(event);
    game::runtime_log_event("event_enqueue", "type=%u house=%d execute_frame=%u accepted=%d",
                            static_cast<unsigned>(event.Type), event.HouseIndex, event.Frame, accepted);
    return accepted;
}
namespace {
int protocol_version = -1;
int frame_send_rate = -1;
}
int& Game::Network::ProtocolVersion = protocol_version;
int& Game::Network::FrameSendRate = frame_send_rate;
void YRPP_FASTCALL EventClass::ScheduleEvent(EventClass* event) noexcept {
    const unsigned current = static_cast<unsigned>(Unsorted::CurrentFrame);
    event->Frame = current + 1u;
    if (Game::Network::ProtocolVersion == 2) {
        const unsigned rate = static_cast<unsigned>(Game::Network::FrameSendRate);
        event->Frame = rate * ((current + rate) / rate);
    }
    // The original campaign/skirmish branch drops a full scheduled queue.
    if (DoList.Count < DO_CAPACITY) DoList.AddFirst(*event, SystemTimer::GetMilliseconds());
}
#endif
