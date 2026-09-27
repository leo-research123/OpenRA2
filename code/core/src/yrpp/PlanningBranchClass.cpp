// Existing YRpp planning branch, calibrated to YR 0x00633AC0. The fixed
// OpenTS baseline has no PlanMgr. Preserve the original embedded Event storage.
#include "yrpp/PlanningTokenClass.h"
#if !defined(RA2_YRPP_GAME)
PlanningBranchClass::PlanningBranchClass() noexcept : Packet(noinit_t{}),MemberCount(0),field_74(-1) {
    Packet.Type=EventType::Empty;
}
#endif
