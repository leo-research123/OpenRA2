#include "presentation/ui/widgets.hpp"
#include "yrpp/ShapeButtonClass.h"
#include "game_ui_runtime.hpp"
#include <algorithm>
namespace game {
bool build_shape_button(GameUiFrame&, const ShapeButtonClass& self, bool forced) {
    if ((!forced && !self.NeedsRedraw) || !self.ShapeData) return false;
    const int frame = self.Disabled ? 2 : self.UseFlash ? (self.IsToFlash ? 3 + int(self.IsOn) : int(self.IsOn)) : int(self.IsPressed);
    // Positions have already been published in whole-canvas coordinates by
    // the original sidebar/tab. Device backends do not know this is a button.
    game::draw_ui_shape(self.ShapeData, {self.X, self.Y}, std::min(frame, int(self.ShapeData->Frames) - 1));
    return true;
}
}
