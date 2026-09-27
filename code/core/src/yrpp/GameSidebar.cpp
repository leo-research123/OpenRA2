// YR-specific sidebar pending-object cleanup, 0x00734270; the original
// init.cpp command module owns this state. No independent selection model.
#include "yrpp/Unsorted.h"
#if !defined(RA2_YRPP_GAME)
int YRPP_FASTCALL Game::ClearSidebarTabObject(const TechnoClass* object) noexcept {
    if(!object)SidebarTabObjects[0]=SidebarTabObjects[1]=nullptr;
    else if(object==SidebarTabObjects[0])SidebarTabObjects[0]=nullptr;
    else if(object==SidebarTabObjects[1])SidebarTabObjects[1]=nullptr;
    return 0;
}
#endif
