// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/SetDefenseTabCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"

#if !defined(RA2_YRPP_GAME)
const char* SetDefenseTabCommandClass::GetName() const { return "DefenseTab"; }

const wchar_t* SetDefenseTabCommandClass::GetUIName() const { return StringTable::LoadString("TXT_DEFENSE_TAB"); }

const wchar_t* SetDefenseTabCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_INTERFACE"); }

const wchar_t* SetDefenseTabCommandClass::GetUIDescription() const {
    return StringTable::LoadString("TXT_DEFENSE_TAB_DESC");
}

void SetDefenseTabCommandClass::Execute(WWKey) const {
    auto& sidebar = SidebarClass::Instance;
    if (sidebar.Tabs[1].CameoCount <= 0) return;
    sidebar.SetTab(1);
    if (auto* product = Game::SidebarTabObjects[1])
        HouseClass::CurrentPlayer->EnterPlacement(product->FindFactory(false, false),
                                                  static_cast<BuildingClass*>(product));
}
#endif
