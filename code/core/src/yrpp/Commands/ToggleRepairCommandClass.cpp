// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/ToggleRepairCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"
#include "yrpp/DisplayClass.h"

#if !defined(RA2_YRPP_GAME)
const char* ToggleRepairCommandClass::GetName() const { return "ToggleRepair"; }

const wchar_t* ToggleRepairCommandClass::GetUIName() const { return StringTable::LoadString("TXT_REPAIR_MODE"); }

const wchar_t* ToggleRepairCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_INTERFACE"); }

const wchar_t* ToggleRepairCommandClass::GetUIDescription() const {
    return StringTable::LoadString("TXT_REPAIR_MODE_DESC");
}

void ToggleRepairCommandClass::Execute(WWKey) const { DisplayClass::Instance.SetRepairMode(-1); }
#endif
