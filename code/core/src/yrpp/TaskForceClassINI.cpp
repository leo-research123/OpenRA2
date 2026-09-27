// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 taskforc.cpp Read_All / Needed_Tech_Level, calibrated to YR.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Supplied 006E8420/006E8510 and TaskForceEntry helpers 004C4EF0/004C4F90.
#include "yrpp/TaskForceClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/AircraftTypeClass.h"
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include "scenario_script_ini.hpp"
#include "scenario_runtime.hpp"
#include "yrpp/SwizzleManagerClass.h"
namespace {
bool parse_entry(const char* text, TaskForceEntryStruct& entry) {
    entry = {};
    char* end = nullptr;
    errno = 0;
    const long amount = std::strtol(text, &end, 10);
    if (end == text || !end || *end != ',' || errno == ERANGE || amount < INT_MIN || amount > INT_MAX)
        return false; // original malformed sscanf fields are indeterminate
    text = end + 1;
    while (std::isspace(static_cast<unsigned char>(*text))) ++text;
    char id[64]{}; std::size_t length = 0;
    while (*text && !std::isspace(static_cast<unsigned char>(*text))) {
        if (length + 1 >= sizeof(id)) return false;
        id[length++] = *text++;
    }
    if (!length) return false;
    entry.Amount = static_cast<int>(amount);
    entry.Type = InfantryTypeClass::Find(id);
    if (!entry.Type) entry.Type = UnitTypeClass::Find(id);
    if (!entry.Type) entry.Type = AircraftTypeClass::Find(id);
    return true;
}
}
bool TaskForceClass::LoadFromINI(CCINIClass* ini) {
    if (!ini) return false;
    try {
        if (!AbstractTypeClass::LoadFromINI(ini)) return false;
        CountEntries = 0;
        for (int i = 0; i < 6; ++i) {
            char key[32], text[128]{};
            std::snprintf(key, sizeof(key), "%d", i);
            if (ini->ReadString(ID, key, "", text, sizeof(text))) {
                if (!parse_entry(text, Entries[CountEntries])) return false;
                if (Entries[CountEntries].Type) ++CountEntries;
            }
        }
        Group = ini->ReadInteger(ID, "Group", Group);
        return true;
    } catch (...) { return false; }
}
bool TaskForceClass::SaveToINI(CCINIClass* ini) {
    if (!ini || CountEntries < 0 || CountEntries > 6) return false;
    for (int i = 0; i < CountEntries; ++i) if (!Entries[i].Type) return false;
    try {
        if (!AbstractTypeClass::SaveToINI(ini)) return false;
        for (int i = 0; i < 6; ++i) {
            char key[32]; std::snprintf(key, sizeof(key), "%d", i);
            if (i < CountEntries) {
                char text[128];
                std::snprintf(text, sizeof(text), "%d,%s", Entries[i].Amount, Entries[i].Type->ID);
                if (!ini->WriteString(ID, key, text)) return false;
            } else ini->Clear(ID, key);
        }
        return ini->WriteInteger(ID, "Group", Group);
    } catch (...) { return false; }
}

TaskForceClass* YRPP_FASTCALL TaskForceClass::FindOrAllocate(const char* id) noexcept {
  try { return game::allocate_type<TaskForceClass>(id); }
  catch (...) { return nullptr; }
}
void YRPP_FASTCALL TaskForceClass::LoadFromINIList(CCINIClass* ini,int scope) noexcept {
 try {
  if(!ini)return;
  for(int i=0;i<ini->GetKeyCount("TaskForces");++i){
   char id[24]{};
   if(!ini->ReadString("TaskForces",ini->GetKeyName("TaskForces",i),"",id,sizeof(id)))continue;
   auto* type=FindOrAllocate(id);if(!type)std::abort();
   SwizzleManagerClass::Instance.Here_I_Am(game::scenario_hex_identity(id),type);
   type->LoadFromINI(ini);type->IsGlobal=scope;
  }
 }catch(...){std::abort();}
}
int TaskForceClass::GetRequiredTechLevel() const {
 const auto& runtime=game::scenario_runtime();
 const bool campaign=runtime.session_mode(runtime.context)==0;
 int required=0;
 for(int i=0;i<CountEntries;++i){
  const int level=Entries[i].Type->TechLevel;
  if(level>required)required=level;
  else if(level==-1&&!campaign)required=11;
 }
 return required;
}
