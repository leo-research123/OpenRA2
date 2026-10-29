// YR 0x00454260. Gate/presentation state not listed here is intentionally omitted.
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/CRC.h"
#include "checksum_values.hpp"
#if !defined(RA2_YRPP_GAME)
void BuildingClass::ComputeCRC(CRCEngine& crc) const {
    TechnoClass::ComputeCRC(crc);
    crc(game::checksum_timer_remaining(C4Timer));
    crc(game::checksum_timer_remaining(CashProductionTimer));
    crc(BState);
    crc(QueueBState);
    crc(int(OwnerCountryIndex));
    if (C4AppliedBy) crc(C4AppliedBy->Fetch_ID());
    crc(int(unknown_544));
    if (FirestormAnim) crc(FirestormAnim->Fetch_ID());
    crc(game::checksum_timer_remaining(FactoryRetryTimer));
    crc(int(unknown_5FC));
    crc(int(LaserFenceFrame));
    crc(int(FirestormWallFrame));
    crc(HasPower);
    crc(StuffEnabled);
    crc(BeingProduced);
    crc(ShouldRebuild);
    crc(AI_Sellable);
    crc(IsReadyToCommence);
    crc(IsBeingRepaired);
    crc(NeedsRepairs);
    crc(C4Applied);
    crc(NoCrew);
    crc(unknown_bool_6E1);
    crc(unknown_bool_6E2);
    crc(HasBeenCaptured);
    crc(ActuallyPlacedOnMap);
    crc(HasEngineer);
    crc(unknown_bool_6E5);
    crc(IsDamaged);
    crc(int(static_cast<unsigned short>(unknown_short_700)));
    crc(static_cast<char>(UpgradeLevel));
}
#endif
