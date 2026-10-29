#include "yrpp/HouseClass.h"
#include "yrpp/SidebarClass.h"

void HouseClass::UpdateTechTree() noexcept {
    // House AI 0x4F9265..0x4F9302 clears the flag before callbacks. A callback
    // can request another pass; do not clear its request at the end.
    if (!RecheckTechTree) return;
    RecheckTechTree = false;
    if (this == CurrentPlayer) {
        const int count = Buildings.Count;
        for (int i = 0; i < count; ++i) {
            auto* building = Buildings[i];
            if (building && building->IsAlive && building->Health > 0 &&
                building->GetCurrentMission() != Mission::Selling && building->QueuedMission != Mission::Selling &&
                building->Owner == CurrentPlayer)
                building->UpdateConstructionOptions();
        }
        SidebarClass::Instance.RefreshBuildables();
    }
    UpdateSuperWeapons();
    AISupers();
}
