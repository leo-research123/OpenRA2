// Original Select/Strip ownership and Sidebar tab switch (6A5310 / 6A7590).
#include "yrpp/SidebarClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Unsorted.h"
#include "game_ui_runtime.hpp"
#include <bit>

void SidebarClass::UpdateScrollButtons() {
    const int count =
        HouseClass::CurrentPlayer == HouseClass::Observer
            ? std::bit_cast<int>(4u * unsigned(static_cast<SidebarClass&>(GScreenClass::Instance).DiplomacyNumHouses))
            : Tabs[ActiveTabIndex].CameoCount;
    const bool enabled = count > GetVisibleCameoCount();
    for (auto* button : {&ScrollUpButton, &ScrollDownButton}) {
        if (enabled && button->Disabled)
            button->GadgetClass::Enable();
        else if (!enabled && !button->Disabled)
            button->GadgetClass::Disable();
    }
}
void SidebarClass::RepaintSidebar(int tab) {
    if (tab == ActiveTabIndex) {
        SidebarNeedsRedraw = true;
        Tabs[tab].NeedsRedraw = true;
        GScreenClass::Instance.MarkNeedsRedraw(0);
    }
}
int YRPP_FASTCALL SidebarClass::GetObjectTabIdx(AbstractType kind, int index, int) {
    switch (kind) {
        case AbstractType::Infantry:
        case AbstractType::InfantryType:
            return 2;
        case AbstractType::Unit:
        case AbstractType::UnitType:
        case AbstractType::Aircraft:
        case AbstractType::AircraftType:
            return 3;
        case AbstractType::Building:
        case AbstractType::BuildingType:
            return BuildingTypeClass::Array[index]->BuildCat == BuildCat::Combat;
        case AbstractType::Special:
        case AbstractType::Super:
        case AbstractType::SuperWeaponType:
            return 1;
        default:
            return -1;
    }
}
void SidebarClass::OnTechnoDestroyed(TechnoClass* object) {
    if (!IsSidebarActive || !object) return;
    auto* type = object->GetTechnoType();
    if (!HouseClass::CurrentPlayer) return;
    const auto kind = object->WhatAmI();
    const int tab = GetObjectTabIdx(kind, object->GetArrayIndex(), 0);
    if (tab == ActiveTabIndex) {
        if (!HouseClass::CurrentPlayer->HasReachedBuildLimit(type)) return;
    } else if (kind != AbstractType::Building || !static_cast<BuildingClass*>(object)->Type->Helipad ||
               ActiveTabIndex != 3)
        return;
    SidebarNeedsRedraw = true;
    Tabs[ActiveTabIndex].NeedsRedraw = true;
    // Both original entries resolve to 0x004F42F0. Use the native screen
    // boundary already used by RepaintSidebar, not an EXE-address trampoline.
    GScreenClass::Instance.MarkNeedsRedraw(0);
    if (int(unknown_53A0) <= Unsorted::CurrentFrame) unknown_53A0 = DWORD(Unsorted::CurrentFrame) + 2u;
}
SelectClass::SelectClass() noexcept
    : ControlClass(ButtonID, 0, 0, 0, 0, static_cast<GadgetFlag>(0x55), true), Strip(nullptr), Index(0), unknown_34(0) {
}
void SidebarClass::InitializeButtons() noexcept {
    auto* frame = game::game_ui_frame();
    if (!frame) return;
    ShapeButtonClass* commands[]{&ToggleRepairButton, &ToggleSellButton};
    for (int i = 0; i < 2; ++i) {
        auto& button = *commands[i];
        button.ID = 101 + i;
        button.ToggleType = 1;
        button.UseFlash = true;
        button.SetPosition(RepairPosition.X + i * RepairPitch, RepairPosition.Y);
        button.SetShape(frame->resources.image(i ? game::UiImage::sell : game::UiImage::repair), 0, 0);
        GScreenClass::Instance.AddButton(&button);
    }
    UpdateCommandButtons();
    auto* cameos = SelectClass::Array() + ActiveTabIndex * 60;
    for (int i = 0; i < GetUsableCameoCount(); ++i)
        GScreenClass::Instance.AddButton(cameos + i);
    for (int i = 0; i < 4; ++i) {
        auto& button = TabButtons[i];
        button.ID = 203 + i;
        button.ToggleType = 2;
        button.UseFlash = true;
        button.SetPosition(TabPosition.X + i * TabPitch, TabPosition.Y);
        button.SetShape(
            frame->resources.image(static_cast<game::UiImage>(static_cast<unsigned>(game::UiImage::tab0) + i)), 0, 0);
        button.IsOn = i == ActiveTabIndex;
        // Empty production tabs are disabled in the original startup path.
        if (Tabs[i].CameoCount)
            button.Enable();
        else
            button.Disable();
        GScreenClass::Instance.AddButton(&button);
    }
}
int SidebarClass::SetTab(int tab) {
    if (tab < 0 || tab >= 4 || tab == ActiveTabIndex) return ActiveTabIndex;
    const int old = ActiveTabIndex;
    TabButtons[old].TurnOff();
    Tabs[old].AllowedToDraw = false;
    auto* old_buttons = SelectClass::Array() + old * 60;
    for (int i = 0; i < 60; ++i)
        GScreenClass::Instance.RemoveButton(old_buttons + i);
    ActiveTabIndex = tab;
    TabButtons[tab].TurnOn();
    Tabs[tab].AllowedToDraw = true;
    auto* buttons = SelectClass::Array() + tab * 60;
    for (int i = 0; i < GetUsableCameoCount(); ++i) {
        buttons[i].LinkClass::Zap();
        GScreenClass::Instance.AddButton(buttons + i);
    }
    Game::SidebarChanged = true;
    UpdateScrollButtons();
    SidebarBackgroundNeedsRedraw = true;
    return old;
}
void SidebarClass::ProcessButtonKey(DWORD key) noexcept {
    if (key == 32869)
        SetRepairMode(-1);
    else if (key == 32870)
        SetSellMode(-1);
    else if (key >= 32971 && key < 32975)
        SetTab(static_cast<int>(key - 32971));
    else
        RadarClass::ProcessButtonKey(key);
    UpdateCommandButtons();
}
void SidebarClass::UpdateCommandButtons() noexcept {
    const bool enabled = HouseClass::CurrentPlayer && HouseClass::CurrentPlayer->OwnedBuildings > 0;
    if (!enabled) {
        SetRepairMode(0);
        SetSellMode(0);
    }
    ShapeButtonClass* buttons[]{&ToggleRepairButton, &ToggleSellButton};
    for (auto* button : buttons) {
        if (enabled)
            button->Enable();
        else
            button->Disable();
    }
    ToggleRepairButton.IsOn = RepairMode;
    ToggleSellButton.IsOn = SellMode;
}
// Cursor/mode and button-animation branches of 0x6A7780. Native production
// completion remains owned by LogicClass's existing UpdateProduction pass.
#if !defined(RA2_YRPP_GAME)
void SidebarClass::Update(const int& key, const Point2D& point) {
    if (IsSidebarActive) {
        const bool previous = unknown_bool_5514;
        unknown_bool_5514 = HouseClass::CurrentPlayer->OwnedBuildings > 0;
        if (previous != unknown_bool_5514) {
            MarkNeedsRedraw(0);
            SidebarNeedsRedraw = true;
            if (!unknown_bool_5514) SetCursor(MouseCursorType::Default, false);
        }
        if (key == 0x8000 + 101 || key == 0x8000 + 102 || (key >= 0x8000 + 203 && key < 0x8000 + 207))
            ProcessButtonKey(static_cast<DWORD>(key));
        bool changed = false;
        for (auto& button : TabButtons)
            if (button.AI()) changed = true;
        if (changed) {
            SidebarNeedsRedraw = SidebarBackgroundNeedsRedraw = true;
            MarkNeedsRedraw(0);
        }
    }
    if (!SellMode && ToggleSellButton.IsOn) ToggleSellButton.TurnOff();
    if (!RepairMode && ToggleRepairButton.IsOn) ToggleRepairButton.TurnOff();
    PowerClass::Update(key, point);
}
#endif
