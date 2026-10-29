// Original production-pool admission, YR 0x004DA230.
#include "yrpp/FootClass.h"

bool FootClass::CanBeRecruited(HouseClass* house) const {
    if (!house || house != Owner || InLimbo || Team) return false;
    // This predicate is intentionally narrower than TeamClass::CanAddMember:
    // health, aircraft ammo and radio links are not gates in 0x004DA230.
    return MissionClass::IsRecruitableMission(GetCurrentMission()) && RecruitableA;
}
