// POWERP.SHP from 63F7C0; segment order/offsets from 63FB20..63FDB1.
// Counts stay in PowerClass's original fields. House production/consumption
// updates are a separate world pass; absent House data displays empty segments.
#include "yrpp/PowerClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Surface.h"
#include "game_ui_runtime.hpp"
#include <bit>

#include "presentation/ui/chrome.hpp"
namespace game {
void build_power(GameUiFrame& context, const PowerClass& self, unsigned force) {

    if (!(force || self.PowerNeedRedraw) || !SidebarClass::Instance.IsSidebarActive) return;
    const int count = (SidebarClass::CameoHeight + 3) / 3;
    const int green = std::bit_cast<int>(self.unknown_152C);
    const int yellow = std::bit_cast<int>(self.unknown_1530);
    const int red = std::bit_cast<int>(self.unknown_1534);
    const bool other = ScenarioClass::Instance && ScenarioClass::Instance->PlayerSideIndex;
    const int x = DSurface::SidebarBounds.X + (other ? 0 : 5);
    int y = DSurface::SidebarBounds.Y + 69;
    const auto segments = [&](int number, int frame) {
        if (number <= 0) return;
        game::draw_ui_shape_strip(game::UiImage::power, {x, y}, {0, 3}, number, frame);
        y += 3 * number;
    };
    segments(count - green - yellow - red, 0);
    const bool flash = std::bit_cast<int>(self.unknown_151C) > 0 && !(self.unknown_151C & 1);
    if (flash) segments(1, 4);
    // The flash replaces the first populated band, including all-red low
    // power. 0x0063FC72 resets the index only when that band is nonempty.
    int skip = int(flash);
    if (green > 0) {
        segments(green - skip, 1);
        skip = 0;
    }
    if (yellow > 0) {
        segments(yellow - skip, 2);
        skip = 0;
    }
    if (red > 0) segments(red - skip, 3);
}

}
