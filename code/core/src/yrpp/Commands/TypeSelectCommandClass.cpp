// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/TypeSelectCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"

#if !defined(RA2_YRPP_GAME)
const char* TypeSelectCommandClass::GetName() const { return "TypeSelect"; }

const wchar_t* TypeSelectCommandClass::GetUIName() const { return StringTable::LoadString("Cmnd:TypeSelect"); }

const wchar_t* TypeSelectCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_SELECTION"); }

const wchar_t* TypeSelectCommandClass::GetUIDescription() const {
    return StringTable::LoadString("Cmnd:TypeSelectDesc");
}

bool TypeSelectCommandClass::PreventCombinationOverride(WWKey input) const { return !!(input & WWKey::Shift); }

bool TypeSelectCommandClass::ExtraTriggerCondition(WWKey) const { return true; }

bool TypeSelectCommandClass::CheckLoop55E020(WWKey input) const { return !(input & WWKey::Release); }

void TypeSelectCommandClass::Execute(WWKey input) const {
    if (input & WWKey::Release)
        Game::EndTypeSelection();
    else
        Game::BeginTypeSelection();
}
#endif
