// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 script.cpp; YR constructor 0x6913C0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ScriptClass.h"
namespace {
DynamicVectorClass<ScriptClass*> scripts;
}
DynamicVectorClass<ScriptClass*>& ScriptClass::Array = scripts;
ScriptClass::ScriptClass(ScriptTypeClass* type) noexcept
    : AbstractClass(), Type(type), field_28(0), CurrentMission(-1) {
    Array.AddItem(this);
}
ScriptClass::~ScriptClass() {
    NotifyObjectExpired(true);
    Array.Remove(this);
}

ScriptActionNode* ScriptClass::GetNextAction(ScriptActionNode* buffer) const {
    const int next = static_cast<int>(static_cast<unsigned>(CurrentMission) + 1u);
    *buffer = next < Type->ActionsCount ? Type->ScriptActions[next] : ScriptActionNode{TMISSION_NONE, 0};
    return buffer;
}
