// YRpp 9402d7da Sidebar/Strip/Select declarations. YR 7b8a0685 layout:
// 6A5090, 6A5130, 6A8220, 6AC430, and positioning portion of 6ABD30.
#include "yrpp/SidebarClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Surface.h"
#include <algorithm>

int SelectClass::CameoPitchX() noexcept { return SidebarClass::CameoPitch.X; }
int SelectClass::CameoPitchY() noexcept { return SidebarClass::CameoPitch.Y; }
int SidebarClass::GetVisibleCameoCount() {
    const int bottom = ScenarioClass::Instance && ScenarioClass::Instance->PlayerSideIndex ? 18 : 26;
    return 2 * ((DSurface::SidebarBounds.Height - CameoPosition.Y - bottom + DSurface::SidebarBounds.Y - 7) / 50);
}
int SidebarClass::GetUsableCameoCount() noexcept {
    // Modern high-resolution host extension: keep the original 60-slot ABI.
    // Raw original arithmetic remains available above for binary comparison.
#ifdef RA2_YRPP_GAME
    return GetVisibleCameoCount();
#else
    return std::clamp(GetVisibleCameoCount(), 0, 60);
#endif
}
void SidebarClass::UpdateLayout() noexcept {
    const auto& view = DSurface::ViewBounds;
    DSurface::SidebarBounds = {view.X + view.Width, 158, 168, view.Y + view.Height + 32 - 158};
    const auto& bounds = DSurface::SidebarBounds;
    const bool other_side = ScenarioClass::Instance && ScenarioClass::Instance->PlayerSideIndex;
    RepairPosition = {bounds.X + (other_side ? 33 : 20), bounds.Y + (other_side ? 7 : 8)};
    RepairPitch = other_side ? 52 : 64;
    TabPosition = {bounds.X + (other_side ? 20 : 26), bounds.Y + 39};
    TabPitch = other_side ? 32 : 29;
    CameoPosition = {bounds.X + 22, bounds.Y + 69};
    CameoPitch = {other_side ? 64 : 63, 50};
    const int previousHeight=CameoHeight;
    CameoHeight = 50 * (GetUsableCameoCount() / 2);
    // Host resizing changes the original bar's scale even if House power is
    // unchanged. Resume its existing pip transition state, not a second bar.
    if(CameoHeight!=previousHeight)unknown_bool_1538=true;
    ScrollPosition = {bounds.X + 39, CameoPosition.Y + CameoHeight + 7};
    ScrollPitch = {other_side ? 45 : 46, 50};
    for (int index = 0; index < 4; ++index) {
        auto& strip = Tabs[index];
        strip.Location = CameoPosition;
        strip.Bounds = {CameoPosition.X, CameoPosition.Y, 2 * CameoPitch.X, CameoHeight};
        strip.Initialize(index);
        strip.NeedsRedraw = true;
        strip.TopRowIndex = std::clamp(strip.TopRowIndex, 0,
            std::max(0, (strip.CameoCount + 1) / 2 - GetUsableCameoCount() / 2));
    }
    SidebarNeedsRedraw = SidebarBackgroundNeedsRedraw = true;
}
void StripClass::Initialize(int index) {
    if (index < 0 || index >= 4) return;
    Index = index;
    auto* buttons = SelectClass::Array() + index * 60;
    const int count = SidebarClass::Instance.GetUsableCameoCount();
    for (int i = 0; i < 60; ++i) {
        auto& button = buttons[i];
        button.ID = SelectClass::ButtonID;
        button.X = Location.X + (i % 2) * SelectClass::CameoPitchX();
        button.Y = Location.Y + (i / 2) * SelectClass::CameoPitchY() + 1;
        button.Width = i < count ? SelectClass::CameoWidth : 0;
        button.Height = i < count ? SelectClass::CameoHeight : 0;
        button.Strip = this;
        button.Index = i;
    }
}
