// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/ScatterCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"

#if !defined(RA2_YRPP_GAME)
const char* ScatterCommandClass::GetName() const { return "ScatterObject"; }

const wchar_t* ScatterCommandClass::GetUIName() const { return StringTable::LoadString("TXT_SCATTER"); }

const wchar_t* ScatterCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_CONTROL"); }

const wchar_t* ScatterCommandClass::GetUIDescription() const { return StringTable::LoadString("TXT_SCATTER_DESC"); }

void ScatterCommandClass::Execute(WWKey) const { Game::ScatterSelected(); }
#endif
