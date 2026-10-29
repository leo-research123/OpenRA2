// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tevent.cpp Read_INI; YR 0x71F4E0 field interpretation.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TEventClass.h"
#include "scenario_script_ini.hpp"
#include "yrpp/TeamTypeClass.h"

#if !defined(RA2_YRPP_GAME)
namespace {
bool read_fields(TEventClass& event, game::ScriptFieldReader& fields) {
    event.Value = 0;
    int kind, mode;
    const char* data;
    if (!fields.integer(kind)) return false;
    event.EventKind = TriggerEvent(kind);
    if (!fields.integer(mode) || !fields.token(data)) return false;
    if (mode == 1) {
        event.TeamType = TeamTypeClass::FindByNameOrID(data);
        return true;
    }
    if (mode != 0 && mode != 2) return !fields.checked();
    if (!fields.number(data, event.Value)) return false;
    if (mode == 2) {
        if (!fields.token(data)) return false;
        std::strncpy(event.String, data, 24);
        event.String[24] = 0;
    }
    return true;
}
}

void TEventClass::LoadFromINI() {
    try {
        game::ScriptFieldReader fields;
        if (!read_fields(*this, fields)) std::abort();
    } catch (...) {
        std::abort();
    }
}
bool TEventClass::LoadFromINI(const char*& cursor) noexcept {
    try {
        game::ScriptFieldReader fields(&cursor);
        return read_fields(*this, fields);
    } catch (...) {
        return false;
    }
}

#endif
