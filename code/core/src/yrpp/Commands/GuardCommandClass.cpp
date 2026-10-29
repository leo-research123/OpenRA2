// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/GuardCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"

#if !defined(RA2_YRPP_GAME)
const char* GuardCommandClass::GetName() const { return "GuardObject"; }

const wchar_t* GuardCommandClass::GetUIName() const { return StringTable::LoadString("TXT_GUARD"); }

const wchar_t* GuardCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_CONTROL"); }

const wchar_t* GuardCommandClass::GetUIDescription() const { return StringTable::LoadString("TXT_GUARD_DESC"); }

void GuardCommandClass::Execute(WWKey) const { Game::GuardSelected(); }
#endif
