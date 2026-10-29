// Original infantry cheer gate and sequence dispatch, 0x00522BC0 / 0x00522C00.
#include "yrpp/InfantryClass.h"
#include <cstdlib>

bool InfantryClass::CanCheer() const noexcept {
    if (GetCurrentMission() != Mission::Guard && GetCurrentMission() != Mission::Area_Guard) return false;
    return SequenceAnim == Sequence::Ready || SequenceAnim == Sequence::Idle1 || SequenceAnim == Sequence::Idle2 ||
           SequenceAnim == Sequence::Cheer;
}
void InfantryClass::Cheer(bool force) noexcept {
    try {
        if (force || CanCheer()) PlayAnim(Sequence::Cheer, false, false);
    } catch (...) {
        std::abort();
    }
}
