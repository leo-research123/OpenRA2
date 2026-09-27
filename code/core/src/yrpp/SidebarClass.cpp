// Existing YRpp 9402d7da hierarchy; field initialization calibrated to fixed
// YR 7b8a0685.
#include "yrpp/SidebarClass.h"
SidebarClass::SidebarClass()
    : PowerClass(), Tabs{}, unknown_5394{}, unknown_5398{}, ActiveTabIndex{},
      unknown_53A0{}, HideObjectNameInTooltip{}, IsSidebarActive{}, SidebarNeedsRedraw{},
      SidebarBackgroundNeedsRedraw{}, unknown_bool_53A8{}, DiplomacyHouses{}, DiplomacyKills{},
      DiplomacyOwned{}, DiplomacyPowerDrain{}, DiplomacyColors{}, unknown_544C{},
      unknown_546C{}, unknown_548C{}, unknown_54AC{}, unknown_54CC{},
      unknown_54EC{}, unknown_550C{}, DiplomacyNumHouses{}, unknown_bool_5514{},
      unknown_bool_5515{} {
    SidebarNeedsRedraw = true;
    for (auto& strip : Tabs) {
        strip.Progress.Timer.Start(0);
        strip.NeedsRedraw = true;
        strip.Flasher = -1;
        for (auto& cameo : strip.Cameos) {
            // 6A80A0 clears these after 6AC7C0 constructs the animation.
            cameo.ItemIndex = 0;
            cameo.ItemType = AbstractType::None;
            cameo.Progress.Timer.Start(0);
        }
    }
}
SidebarClass::~SidebarClass() = default;

