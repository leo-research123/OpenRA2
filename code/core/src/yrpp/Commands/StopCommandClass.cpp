// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/StopCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"

#if !defined(RA2_YRPP_GAME)
const char* StopCommandClass::GetName() const { return "StopObject"; }

const wchar_t* StopCommandClass::GetUIName() const { return StringTable::LoadString("TXT_STOP_OBJECT"); }

const wchar_t* StopCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_CONTROL"); }

const wchar_t* StopCommandClass::GetUIDescription() const { return StringTable::LoadString("TXT_STOP_OBJECT_DESC"); }

void StopCommandClass::Execute(WWKey) const { Game::StopSelected(); }
#endif
