// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/OptionsCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"

#if !defined(RA2_YRPP_GAME)
const char* OptionsCommandClass::GetName() const { return "Options"; }

const wchar_t* OptionsCommandClass::GetUIName() const { return StringTable::LoadString("TXT_OPTIONS"); }

const wchar_t* OptionsCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_INTERFACE"); }

const wchar_t* OptionsCommandClass::GetUIDescription() const { return StringTable::LoadString("TXT_OPTIONS_DESC"); }

void OptionsCommandClass::Execute(WWKey) const { Game::QueueOptions(); }
#endif
