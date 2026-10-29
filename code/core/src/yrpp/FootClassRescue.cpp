// Base-defense recruitment score, YR 0x4D97A0.
#include "yrpp/FootClass.h"
#include "yrpp/TeamClass.h"
#include <bit>
#include <algorithm>
int FootClass::RescueMission(TechnoClass* enemy) {
    if (!enemy) return 0;
    if (Target == enemy) return std::bit_cast<int>(0u - unsigned(GetThreatValue()));
    if (Target && (Target->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None &&
        static_cast<TechnoClass*>(Target)->IsArmed())
        return 0;
    if ((Team && !Team->Type->IsBaseDefense) || GetCurrentMission() == Mission::Harvest || !GetThreatValue()) return 0;
    const int distance = DistanceFrom3D(enemy) - GetWeaponRange(0);
    const int value = std::bit_cast<int>(unsigned(GetThreatValue()) << 10);
    if (distance <= 0) return value;
    const int speed = unsigned(GetDefaultSpeed()) <= 1 ? 1 : GetDefaultSpeed();
    const int divisor = speed <= 0 ? 1 : std::max(distance / speed, 1);
    return std::max(value / divisor, 1);
}
