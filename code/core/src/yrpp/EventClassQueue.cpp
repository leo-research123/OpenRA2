// Original event module submission, YR 0x006521C0. Retains the existing
// TS++ QueueClass/EventClass model and the authoritative SystemTimer source.
#include "yrpp/EventClass.h"
// TODO(RADAR-PLAN-EXEC): scheduled front insertion at 0x00652230 is deferred
// by the user. It still needs frame alignment, DoList head insertion and full
// queue handling; the restored OutList tail submission below is not that entry.
#if !defined(RA2_YRPP_GAME)
bool YRPP_STDCALL EventClass::AddEvent(EventClass event) noexcept {
    event.Frame=static_cast<unsigned>(Unsorted::CurrentFrame);
    return OutList.Add(event);
}
#endif
