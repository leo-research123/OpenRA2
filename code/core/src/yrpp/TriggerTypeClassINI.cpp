// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 trigtype.cpp Read_INI; YR 0x727240.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "scenario_script_ini.hpp"
#include "yrpp/TActionClass.h"
#include "yrpp/TEventClass.h"
#include "yrpp/TriggerTypeClass.h"
#include <cstdio>
#include <memory>
bool TriggerTypeClass::LoadFromINI(CCINIClass *ini) {
  if (!ini)
    return false;
  try {
    const auto row = game::scenario_fields(
        game::scenario_text(*ini, "Triggers", ID).c_str());
    if (row.size() < 7)
      return false;
    House =
        INIClass::IsBlankValue(row[0].c_str())
            ? (HouseTypeClass::Array.Count ? HouseTypeClass::Array[0] : nullptr)
            : HouseTypeClass::Find(row[0].c_str());
    NextTrigger = TriggerTypeClass::FindOrAllocate(row[1].c_str());
    std::snprintf(Name, sizeof(Name), "%s", row[2].c_str());
    Enabled = game::scenario_integer(row, 3, 1) == 0;
    for (int i = 0; i < 3; ++i)
      Difficulty[i] = game::scenario_integer(row, 4 + i, 0) != 0;
    MustTransfer = game::scenario_integer(row, 7, 0) != 0;
    while (FirstEvent) {
      auto *next = FirstEvent->NextEvent;
      delete FirstEvent;
      FirstEvent = next;
    }
    while (FirstAction) {
      auto *next = FirstAction->NextAction;
      delete FirstAction;
      FirstAction = next;
    }
    const auto events = game::scenario_text(*ini, "Events", ID);
    const char *cursor = events.c_str();
    int count = 0;
    if (*cursor) {
      if (!game::script_integer(cursor, count) || count < 0 || count > 32)
        return false;
      for (int i = 0; i < count; ++i) {
        std::unique_ptr<TEventClass> event(new TEventClass);
        if (!game::read_trigger_event_fields(*event, cursor))
          return false;
        // The target prepends events but preserves action order.
        event->NextEvent = FirstEvent;
        FirstEvent = event.release();
      }
      if (*cursor)
        return false;
    }
    const auto actions = game::scenario_text(*ini, "Actions", ID);
    cursor = actions.c_str();
    TActionClass *last = nullptr;
    if (*cursor) {
      if (!game::script_integer(cursor, count) || count < 0 || count > 256)
        return false;
      for (int i = 0; i < count; ++i) {
        std::unique_ptr<TActionClass> action(new TActionClass);
        if (!game::read_trigger_action_fields(*action, cursor))
          return false;
        if (last)
          last->NextAction = action.get();
        else
          FirstAction = action.get();
        last = action.release();
      }
      if (*cursor)
        return false;
    }
    return House != nullptr;
  } catch (...) {
    return false;
  }
}

TriggerTypeClass *YRPP_FASTCALL
TriggerTypeClass::FindOrAllocate(const char *id) noexcept {
  try {
    return game::allocate_type<TriggerTypeClass>(id);
  } catch (...) {
    return nullptr;
  }
}

void YRPP_FASTCALL TriggerTypeClass::LoadFromINIList(CCINIClass *ini) noexcept {
  try {
    if (!ini)
      return;
    for (int i = 0; i < ini->GetKeyCount("Triggers"); ++i) {
      auto *type = FindOrAllocate(ini->GetKeyName("Triggers", i));
      if (!type)
        std::abort();
      type->LoadFromINI(ini);
    }
  } catch (...) {
    std::abort();
  }
}
