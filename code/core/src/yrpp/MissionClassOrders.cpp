// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 mission.cpp Assign/Commence/Set/Override/Restore_Mission.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR adds Wait->Guard guards and timing resets, and does NOT translate QMove.
#include "yrpp/MissionClass.h"

bool MissionClass::QueueMission(Mission mission, bool start) {
    if ((CurrentMission == Mission::Wait && mission == Mission::Guard) ||
        CurrentMission == Mission::Selling)
        return static_cast<unsigned char>(static_cast<int>(mission)) != 0;
    if (mission != Mission::None && (CurrentMission != mission ||
        (QueuedMission != mission && QueuedMission != Mission::None))) {
        QueuedMission = mission;
        unknown_bool_B8 = false;
    }
    return start && ReadyToNextMission() && NextMission();
}

bool MissionClass::NextMission() {
    if (QueuedMission == Mission::None) return false;
    CurrentMission = QueuedMission;
    QueuedMission = Mission::None;
    MissionStatus = 0;
    UpdateTimer.Start(0);
    CurrentMissionStartTime = Unsorted::CurrentFrame;
    MissionAccumulateTime = 0;
    unknown_bool_B8 = false;
    return true;
}

void MissionClass::ForceMission(Mission mission) {
    if (CurrentMission == Mission::Wait && mission == Mission::Guard) return;
    CurrentMission = mission;
    QueuedMission = Mission::None;
    unknown_bool_B8 = false;
    MissionStatus = 0;
    CurrentMissionStartTime = Unsorted::CurrentFrame;
    MissionAccumulateTime = 0;
    UpdateTimer.Start(0);
}

void MissionClass::Override_Mission(Mission mission, AbstractClass*, AbstractClass*) {
    if ((CurrentMission == Mission::Wait && mission == Mission::Guard) ||
        CurrentMission == Mission::Selling) return;
    SuspendedMission = QueuedMission != Mission::None ? QueuedMission : CurrentMission;
    CurrentMission = mission;
    unknown_bool_B8 = false;
}

bool MissionClass::Mission_Revert() {
    if (SuspendedMission == Mission::None) return false;
    CurrentMission = SuspendedMission;
    SuspendedMission = Mission::None;
    unknown_bool_B8 = false;
    return true;
}

bool MissionClass::MissionIsOverriden() const { return SuspendedMission != Mission::None; }
bool YRPP_FASTCALL MissionClass::IsRecruitableMission(Mission mission) {
    if (mission == Mission::None) return true;
    const int index = static_cast<int>(mission);
    return index >= 0 && index < 0x20 && MissionControlClass::Array[index].Recruitable;
}
