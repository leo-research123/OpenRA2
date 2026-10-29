// YR 0x005B3970.
#include "yrpp/MissionClass.h"
#include "yrpp/CRC.h"
#include "checksum_values.hpp"
#if !defined(RA2_YRPP_GAME)
void MissionClass::ComputeCRC(CRCEngine& crc) const {
    ObjectClass::ComputeCRC(crc);
    crc(int(CurrentMission));
    crc(int(SuspendedMission));
    crc(int(QueuedMission));
    crc(MissionStatus);
    crc(game::checksum_timer_remaining(UpdateTimer));
    crc(CurrentMissionStartTime);
}
#endif
