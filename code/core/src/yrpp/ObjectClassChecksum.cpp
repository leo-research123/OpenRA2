// YR 0x005F6250: ordered scalar contributions, with local UI flags only offline.
#include "yrpp/ObjectClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/CRC.h"
#if !defined(RA2_YRPP_GAME)
void ObjectClass::ComputeCRC(CRCEngine& crc) const {
    AbstractClass::ComputeCRC(crc);
    if (NextObject) crc(NextObject->Fetch_ID());
    if (AttachedTag) crc(AttachedTag->Fetch_ID());
    crc(Health);
    crc(IsOnMap);
    if (int(SessionClass::Instance.GameMode) == 0 || int(SessionClass::Instance.GameMode) == 5) {
        crc(NeedsRedraw);
        crc(IsSelected);
    }
    crc(InLimbo);
    crc(HasParachute);
    crc(OnBridge);
    crc(IsFallingDown);
    crc(IsABomb);
    crc(IsAlive);
    crc(Location.X);
    crc(Location.Y);
    crc(Location.Z);
}
#endif
