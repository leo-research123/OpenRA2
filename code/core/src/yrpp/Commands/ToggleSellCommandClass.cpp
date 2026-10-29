// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/ToggleSellCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"
#include "yrpp/DisplayClass.h"

#if !defined(RA2_YRPP_GAME)
const char* ToggleSellCommandClass::GetName() const { return "ToggleSell"; }

const wchar_t* ToggleSellCommandClass::GetUIName() const { return StringTable::LoadString("TXT_SELL_MODE"); }

const wchar_t* ToggleSellCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_INTERFACE"); }

const wchar_t* ToggleSellCommandClass::GetUIDescription() const {
    return StringTable::LoadString("TXT_SELL_MODE_DESC");
}

void ToggleSellCommandClass::Execute(WWKey) const { DisplayClass::Instance.SetSellMode(-1); }
#endif
