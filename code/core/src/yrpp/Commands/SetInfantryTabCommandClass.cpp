// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/SetInfantryTabCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"

#if !defined(RA2_YRPP_GAME)
const char* SetInfantryTabCommandClass::GetName() const { return "InfantryTab"; }

const wchar_t* SetInfantryTabCommandClass::GetUIName() const { return StringTable::LoadString("TXT_INFANTRY_TAB"); }

const wchar_t* SetInfantryTabCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_INTERFACE"); }

const wchar_t* SetInfantryTabCommandClass::GetUIDescription() const {
    return StringTable::LoadString("TXT_INFANTRY_TAB_DESC");
}

void SetInfantryTabCommandClass::Execute(WWKey) const {
    auto& sidebar = SidebarClass::Instance;
    if (sidebar.Tabs[2].CameoCount <= 0) return;
    sidebar.SetTab(2);
}
#endif
