// YR-specific sidebar pending-object cleanup, 0x00734270; the original
// init.cpp command module owns this state. No independent selection model.
#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/BuildingClass.h"
#if !defined(RA2_YRPP_GAME)
void YRPP_FASTCALL Game::SetSidebarTabObject(TechnoClass* object) noexcept {
    SidebarTabObjects[static_cast<BuildingClass*>(object)->Type->BuildCat == BuildCat::Combat] = object;
}
int YRPP_FASTCALL Game::ClearSidebarTabObject(const TechnoClass* object) noexcept {
    if (!object)
        SidebarTabObjects[0] = SidebarTabObjects[1] = nullptr;
    else if (object == SidebarTabObjects[0])
        SidebarTabObjects[0] = nullptr;
    else if (object == SidebarTabObjects[1])
        SidebarTabObjects[1] = nullptr;
    return 0;
}
#endif
