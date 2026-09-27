// Native initialization; original 0x005B2DA0.
#include "yrpp/MissionClass.h"
Mission MissionClass::GetCurrentMission() const {return CurrentMission==Mission::None?QueuedMission:CurrentMission;}
MissionClass::MissionClass() noexcept
 : ObjectClass(),
 CurrentMission{},
 SuspendedMission{},
 QueuedMission{},
 unknown_bool_B8{},
 MissionStatus{},
 CurrentMissionStartTime{},
 MissionAccumulateTime{},
 UpdateTimer{} {CurrentMission=SuspendedMission=QueuedMission=Mission::None;CurrentMissionStartTime=Unsorted::CurrentFrame;}
