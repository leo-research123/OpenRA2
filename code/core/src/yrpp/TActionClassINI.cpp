// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 taction.cpp Read_INI; YR 0x6DD5B0 field interpretation.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TActionClass.h"
#include "scenario_script_ini.hpp"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/ThemeClass.h"
#include "type_resources.hpp"
#include "rules_runtime.hpp"

#if !defined(RA2_YRPP_GAME)
namespace {
// YR serialized parameter codes; these differ from TS ParamCodeType and LogicNeedType.
enum class ActionParameterCode : int {
    Number = 0,
    Team = 1,
    Trigger = 2,
    Tag = 3,
    Text = 4,
    TeamAndNumber = 5,
    Speech = 6,
    Sound = 7,
    Theme = 8,
    TechnoIdAndNumber = 9,
    TechnoIdAndWaypoint = 10,
    NumberAndNumber = 11
};
bool read_fields(TActionClass& action, game::ScriptFieldReader& fields) {
    action.Value = 0;
    int kind, mode;
    const char* data;
    if (!fields.integer(kind)) return false;
    action.ActionKind = TriggerAction(kind);
    if (!fields.integer(mode) || !fields.token(data)) return false;
    const auto code = static_cast<ActionParameterCode>(mode);
    switch (code) {
        case ActionParameterCode::Number:
        case ActionParameterCode::NumberAndNumber:
            if (!fields.number(data, action.Value)) return false;
            break;
        case ActionParameterCode::Team:
        case ActionParameterCode::TeamAndNumber:
            action.TeamType = fields.reference<TeamTypeClass>(data);
            break;
        case ActionParameterCode::Trigger:
            action.TriggerType = fields.reference<TriggerTypeClass>(data);
            break;
        case ActionParameterCode::Tag:
            action.TagType = fields.reference<TagTypeClass>(data);
            break;
        case ActionParameterCode::Text:
            action.Text[0] = 0;
            if (std::atoi(data) != -1) {
                while (*data && static_cast<unsigned char>(*data) <= 0x20u)
                    ++data;
                if (*data) {
                    std::strncpy(action.Text, data, sizeof(action.Text) - 1);
                    action.Text[sizeof(action.Text) - 1] = 0;
                    auto size = std::strlen(action.Text);
                    while (size && static_cast<unsigned char>(action.Text[size - 1]) <= 0x20u)
                        --size;
                    action.Text[size] = 0;
                }
            }
            break;
        case ActionParameterCode::Speech:
            action.Value =
                fields.checked() && game::type_resources().audio_unavailable ? -1 : VoxClass::FindIndex(data);
            break;
        case ActionParameterCode::Sound:
            action.Value = -1;
            if (!fields.checked())
                action.Value = VocClass::FindIndex(data);
            else if (!game::type_resources().audio_unavailable && !game::rules_sound_index(data, action.Value))
                return false;
            break;
        case ActionParameterCode::Theme:
            action.Value = fields.checked() && game::type_resources().audio_unavailable
                               ? -1
                               : ThemeClass::Instance.FindIndex(data);
            break;
        case ActionParameterCode::TechnoIdAndNumber:
        case ActionParameterCode::TechnoIdAndWaypoint:
            std::strncpy(action.TechnoID, data, sizeof(action.TechnoID) - 1);
            action.TechnoID[sizeof(action.TechnoID) - 1] = 0;
            break;
        default:
            if (fields.checked()) return false;
            break;
    }
    for (auto* field : {&action.Param3, &action.Param4, &action.Param5, &action.Param6})
        if (!fields.integer(*field)) return false;
    // 0x006DD5B0 permits a missing final field. The original 512-byte INI
    // line reader truncates the last waypoint of long campaign action lists;
    // the preceding seven fields still form a valid action record.
    if (!fields.token(data)) return true;
    if (code == ActionParameterCode::TeamAndNumber || code == ActionParameterCode::TechnoIdAndNumber)
        return fields.number(data, action.Value);
    if (code == ActionParameterCode::NumberAndNumber) return fields.number(data, action.Value2);
    return fields.waypoint(data, action.Waypoint);
}
}

void TActionClass::LoadFromINI() {
    try {
        game::ScriptFieldReader fields;
        if (!read_fields(*this, fields)) std::abort();
    } catch (...) {
        std::abort();
    }
}
bool TActionClass::LoadFromINI(const char*& cursor) noexcept {
    try {
        game::ScriptFieldReader fields(&cursor);
        return read_fields(*this, fields);
    } catch (...) {
        return false;
    }
}

#endif
