// YRpp 9402d7da; supplied 6916B0, 6917F0, 6918A0, 691970, 691C00,
// 691E00, 691F90 and 691FA0. Persistence and TeamType invalidation stay JUMP.
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/CRC.h"
#include <cstdio>
#include <cstring>
#include <new>
namespace { DynamicVectorClass<ScriptTypeClass*> types; }
DynamicVectorClass<ScriptTypeClass*>& ScriptTypeClass::Array = types;
ScriptTypeClass* YRPP_FASTCALL ScriptTypeClass::Find(const char* id) {
    const int index = FindIndex(id);
    return index < 0 ? nullptr : Array[index];
}
int YRPP_FASTCALL ScriptTypeClass::FindIndex(const char* id) {
    if (!id) return -1;
    for (int i = 0; i < Array.Count; ++i)
        if (_strcmpi(Array[i]->ID, id) == 0) return i;
    return -1;
}

ScriptTypeClass::ScriptTypeClass(const char* id) noexcept : AbstractTypeClass(id),
    ArrayIndex(-1), IsGlobal(false), ActionsCount(0), ScriptActions{} {
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
}
ScriptTypeClass::~ScriptTypeClass() { NotifyTypeExpired(); Array.Remove(this); }
int ScriptTypeClass::GetArrayIndex() const { return ArrayIndex; }
ScriptTypeClass* YRPP_FASTCALL ScriptTypeClass::FindOrAllocate(const char* id) {
    if (!id || _strcmpi(id, "none") == 0 || _strcmpi(id, "<none>") == 0) return nullptr;
    if (auto* found = Find(id)) return found;
    void* storage = YRMemory::Allocate(sizeof(ScriptTypeClass));
    return storage ? new (storage) ScriptTypeClass(id) : nullptr;
}
bool ScriptTypeClass::LoadFromINI(CCINIClass* ini) {
    if (!AbstractTypeClass::LoadFromINI(ini)) return false;
    ActionsCount = 0;
    for (int i = 0; i < 50; ++i) {
        char key[16], text[128];
        std::snprintf(key, sizeof(key), "%d", i);
        if (ini->ReadString(ID, key, "", text, sizeof(text)) <= 0) continue;
        ScriptActionNode action{};
        // 723CA0 uses sscanf; malformed input left indeterminate stack values.
        // Reject that undefined-input case rather than fabricate a script action.
        if (std::sscanf(text, "%d,%d", &action.Action, &action.Argument) != 2) return false;
        ScriptActions[ActionsCount++] = action;
    }
    return true;
}
bool ScriptTypeClass::SaveToINI(CCINIClass* ini) {
    if (ActionsCount < 0 || ActionsCount > 50 || !AbstractTypeClass::SaveToINI(ini)) return false;
    for (int i = 0; i < 50; ++i) {
        char key[16], text[128];
        std::snprintf(key, sizeof(key), "%d", i);
        if (i >= ActionsCount) ini->Clear(ID, key);
        else {
            std::snprintf(text, sizeof(text), "%d,%d", ScriptActions[i].Action, ScriptActions[i].Argument);
            if (!ini->WriteString(ID, key, text)) return false;
        }
    }
    return true;
}
void YRPP_FASTCALL ScriptTypeClass::LoadFromINIList(CCINIClass* ini, int scope) noexcept {
    try {
        if (!ini) return;
        for (int i = 0; i < ini->GetKeyCount("ScriptTypes"); ++i) {
            char id[24]{};
            if (ini->ReadString("ScriptTypes", ini->GetKeyName("ScriptTypes", i), "", id, sizeof(id)) <= 0) continue;
            auto* type = FindOrAllocate(id);
            if (!type) std::abort();
            // Original Read_All ignores the virtual reader's return value.
            type->LoadFromINI(ini);
            type->IsGlobal = scope;
        }
    } catch (...) { std::abort(); }
}
void ScriptTypeClass::ComputeCRC(CRCEngine& crc) const {
    AbstractTypeClass::ComputeCRC(crc);
    crc(ActionsCount); // 691E00 hashes the count, not each action.
}

#include "yrpp/TeamTypeClass.h"
void ScriptTypeClass::PointerExpired(AbstractClass* object, bool) {
    // 691E30: repair numeric action 18 indices while the deleted team type is
    // still in its registry. Do not allocate/find a replacement type.
    if (!object || object->WhatAmI() != AbstractType::TeamType) return;
    const int index = TeamTypeClass::Array.FindItemIndex(static_cast<TeamTypeClass*>(object));
    if (index < 0) return;
    for (int i = 0; i < ActionsCount && i < 50; ++i) {
        auto& action = ScriptActions[i];
        if (action.Action != 18) continue;
        if (action.Argument > index) --action.Argument;
        else if (action.Argument == index) action.Argument = 0;
    }
}
