// OpenTS TeamClass::Is_Leaving_Map, YR 0x006EC300. Read-only script query;
// does not execute a team script or autonomously issue a movement order.
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TeamClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"

int TeamClass::GetStrayDistance() const {
    ScriptActionNode action;CurrentScript->GetCurrentAction(&action);
    return action.Action==53 || action.Action==54?RulesClass::Instance->RelaxedStray:RulesClass::Instance->Stray;
}

bool TeamClass::IsLeavingMapNow() const {
    if (!IsMoving || !CurrentScript->HasCurrentMission()) return false;
    ScriptActionNode action;
    CurrentScript->GetCurrentAction(&action);
    return action.Action == 3 && !MapClass::Instance.IsWithinUsableArea(
        ScenarioClass::Instance->GetWaypointCoords(action.Argument),true);
}
