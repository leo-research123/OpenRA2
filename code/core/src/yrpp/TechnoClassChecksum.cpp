// YR 0x0070C270: keep widths/order and conditional campaign UI contributions.
#include "yrpp/TechnoClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/CRC.h"
#include "checksum_values.hpp"
#if !defined(RA2_YRPP_GAME)
void TechnoClass::ComputeCRC(CRCEngine& crc) const {
    RadioClass::ComputeCRC(crc);
    crc(Group);
    crc(ArmorMultiplier);
    crc(FirepowerMultiplier);
    crc(game::checksum_timer_remaining(IdleActionTimer));
    crc(int(DisplayProductionTo.data));
    crc(int(CloakState));
    crc(game::checksum_timer_remaining(CloakDelayTimer));
    crc(WarpFactor);
    crc(unknown_bool_250);
    crc(LastSightRange);
    crc(PitchAngle);
    crc(game::checksum_timer_remaining(RearmTimer));
    crc(Ammo);
    crc(game::checksum_timer_remaining(ReloadTimer));
    crc(Value);
    crc(AngleRotatedSideways);
    crc(AngleRotatedForwards);
    crc(RockingSidewaysPerFrame);
    crc(RockingForwardsPerFrame);
    crc(IsSinking);
    crc(unknown_bool_3CF);
    crc(IsUseless);
    crc(HasBeenAttacked);
    crc(Cloakable);
    crc(IsPrimaryFactory);
    crc(IsALoaner);
    crc(IsInPlayfield);
    crc(IsTether);
    crc(IsAlternativeTether);
    if (int(SessionClass::Instance.GameMode) == 0 || int(SessionClass::Instance.GameMode) == 5) {
        crc(IsOwnedByCurrentPlayer);
        crc(DiscoveredByCurrentPlayer);
        crc(DiscoveredByComputer);
    }
    crc(unknown_bool_41D);
    crc(int(EMPLockRemaining));
    crc(int(unknown_bool_41E));
    crc(int(unknown_bool_41F));
    crc(SightIncrease);
    crc(CurrentTargets.Count);
    for (int i = 0; i < CurrentTargets.Count; ++i) {
        crc(CurrentTargets[i]->Fetch_ID());
        crc(CurrentTargetThreatValues[i]);
    }
    crc(AttackedTargets.Count);
    for (int i = 0; i < AttackedTargets.Count; ++i)
        crc(AttackedTargets[i]->Fetch_ID());
}
#endif
