// YR 0x4DBAD0: append the mobile state after the complete Techno checksum.
#include "yrpp/FootClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/CRC.h"
#include "checksum_values.hpp"
#include "x87_integer.hpp"
#if !defined(RA2_YRPP_GAME)
void FootClass::ComputeCRC(CRCEngine& crc) const {
    TechnoClass::ComputeCRC(crc);
    crc(ThreatAvoidanceCoefficient);
    const double x = CurrentMapCoords.X, y = CurrentMapCoords.Y;
    crc(static_cast<short>(game::x87_integer(Math::sqrt(x * x + y * y))));
    const auto magnitude = [](const CoordStruct& at) {
        const double x = at.X, y = at.Y, z = at.Z;
        return game::x87_integer(Math::sqrt(x * x + y * y + z * z));
    };
    crc(magnitude(CurrentTunnelCoords));
    crc(SpeedPercentage);
    crc(SpeedMultiplier);
    if (Destination) crc(Destination->Fetch_ID());
    if (LastDestination) crc(LastDestination->Fetch_ID());
    if (Team) crc(Team->Fetch_ID());
    if (NextTeamMember) crc(NextTeamMember->Fetch_ID());
    crc(game::checksum_timer_remaining(PathDelayTimer));
    crc(PathWaitTimes);
    crc(game::checksum_timer_remaining(unknown_timer_650));
    crc(magnitude(unknown_point3d_678));
    crc(static_cast<char>(TubeIndex));
    crc(static_cast<char>(TubeFaceIndex));
    crc(IsScanLimited);
    crc(IsInitiated);
    crc(ShouldScanForTarget);
    crc(unknown_bool_68B);
    crc(IsDeploying);
    crc(IsFiring);
    crc(unknown_bool_6AF);
    crc(unknown_bool_6B0);
    crc(unknown_bool_6B1);
    crc(unknown_bool_6B2);
    crc(unknown_bool_6B3);
    crc(static_cast<char>(DrawingYOffset));
}
#endif
