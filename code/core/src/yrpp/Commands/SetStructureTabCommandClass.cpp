// Original YR command metadata and execution; see method VA annotations in the header.
#include "yrpp/Commands/SetStructureTabCommandClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Unsorted.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"

#if !defined(RA2_YRPP_GAME)
const char* SetStructureTabCommandClass::GetName() const { return "StructureTab"; }

const wchar_t* SetStructureTabCommandClass::GetUIName() const { return StringTable::LoadString("TXT_STRUCTURE_TAB"); }

const wchar_t* SetStructureTabCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_INTERFACE"); }

const wchar_t* SetStructureTabCommandClass::GetUIDescription() const {
    return StringTable::LoadString("TXT_STRUCTURE_TAB_DESC");
}

void SetStructureTabCommandClass::Execute(WWKey) const {
    auto& sidebar = SidebarClass::Instance;
    if (sidebar.Tabs[0].CameoCount <= 0) return;
    sidebar.SetTab(0);
    if (auto* product = Game::SidebarTabObjects[0])
        HouseClass::CurrentPlayer->EnterPlacement(product->FindFactory(false, false),
                                                  static_cast<BuildingClass*>(product));
}
#endif
