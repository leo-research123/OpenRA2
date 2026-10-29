// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/SetUnitTabCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"

#if !defined(RA2_YRPP_GAME)
const char* SetUnitTabCommandClass::GetName() const { return "UnitTab"; }

const wchar_t* SetUnitTabCommandClass::GetUIName() const { return StringTable::LoadString("TXT_UNIT_TAB"); }

const wchar_t* SetUnitTabCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_INTERFACE"); }

const wchar_t* SetUnitTabCommandClass::GetUIDescription() const { return StringTable::LoadString("TXT_UNIT_TAB_DESC"); }

void SetUnitTabCommandClass::Execute(WWKey) const {
    auto& sidebar = SidebarClass::Instance;
    if (sidebar.Tabs[3].CameoCount <= 0) return;
    sidebar.SetTab(3);
}
#endif
