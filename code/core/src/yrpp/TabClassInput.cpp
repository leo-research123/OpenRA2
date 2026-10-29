// YR 6D03A0 / 6D0FD0 / 6D0680. Original command slots, toggles and ThumbActive.
#include "yrpp/MouseClass.h"
#include "game_ui_runtime.hpp"
#include "yrpp/Unsorted.h"
#include "yrpp/PlanningTokenClass.h"
#include <algorithm>
void TabClass::InitializeCommandButtons() noexcept {
    auto* frame = game::game_ui_frame();
    if (!frame) return;
    const auto& assets = frame->resources;
    const auto bar = GetCommandBarBounds();
    const int end_width = assets.image(game::UiImage::rightcap)->Width;
    const int cap_width = assets.image(game::UiImage::leftcap)->Width;
    const int pitch = assets.image(game::UiImage::button_background)->Width;
    const int end_x = bar.Width - end_width;
    const int count = std::max(0, (bar.Width - cap_width - end_width) / pitch);
    const int begin = end_x - count * pitch;
    for (int slot = 0; slot < 25; ++slot) {
        auto& button = CommandButtons[slot];
        int command = -1;
        for (int i = 0; i < 25; ++i)
            if (CommandPositions[i] == slot) command = i;
        auto* shape =
            command < 0 || slot >= count
                ? nullptr
                : assets.image(static_cast<game::UiImage>(static_cast<unsigned>(game::UiImage::button0) + command));
        button.ID = 214 + command;
        button.SetShape(shape, 0, 0);
        button.SetPosition(begin + slot * pitch, bar.Y);
        button.ToggleType = command == 9 ? 1 : 0;
        button.UseFlash = command == 9;
        button.Flags = static_cast<GadgetFlag>(command >= 0 && command <= 2 ? 0x55 : 0x05);
        if (command >= 0 && command <= 10 && shape)
            button.Enable();
        else
            button.Disable();
        if (ThumbActive && shape)
            AddButton(&button);
        else
            RemoveButton(&button);
    }
    auto* shape = assets.image(game::UiImage::leftcap);
    CollapseButton.ID = 240;
    CollapseButton.SetPosition(begin - cap_width, bar.Y);
    CollapseButton.SetShape(shape, 0, 0);
    CollapseButton.Enable();
    ExpandButton.ID = 241;
    ExpandButton.SetPosition(end_x - cap_width, bar.Y);
    ExpandButton.SetShape(shape, 0, 0);
    ExpandButton.Enable();
    if (ThumbActive) {
        RemoveButton(&ExpandButton);
        AddButton(&CollapseButton);
    } else {
        RemoveButton(&CollapseButton);
        AddButton(&ExpandButton);
    }
}
void TabClass::ProcessButtonKey(DWORD key) noexcept {
    if (key >= 0xC000 + 214 && key <= 0xC000 + 216)
        Game::ClearControlGroup(int(key) - 0xC000 - 214 + 1);
    else if (key >= 0x8000 + 214 && key <= 0x8000 + 216) {
        const int group = int(key) - 0x8000 - 214 + 1;
        if (!Game::ControlGroupCount(group))
            Game::AssignControlGroup(group);
        else if (Game::ControlGroupIsSelected(group))
            Game::CenterControlGroup(group);
        else
            Game::SelectControlGroup(group);
    } else if (key == 0x8000 + 214 + 3)
        Game::TypeSelect();
    else if (key == 0x8000 + 214 + 5)
        Game::BeginAttackMove();
    else if (key == 0x8000 + 214 + 7)
        Game::ToggleBeacon();
    else if (key == 0x8000 + 214 + 10)
        Game::CheerSelectedHouse();
    else if (key == 0x8000 + 214 + 4)
        Game::DeploySelected();
    else if (key == 0x8000 + 214 + 6)
        Game::GuardSelected();
    else if (key == 0x8000 + 214 + 8)
        Game::StopSelected();
    else if (key == 0x8000 + 214 + 9) {
        if (PlanningNodeClass::PlanningModeActive)
            Game::EndPlanning(true);
        else
            Game::BeginPlanning(true);
    } else if (key == 0x8000 + 240 && ThumbActive) {
        ThumbActive = false;
        unknown_byte_5546 = true;
    } else if (key == 0x8000 + 241 && !ThumbActive) {
        ThumbActive = true;
        unknown_byte_5546 = true;
    } else
        SidebarClass::ProcessButtonKey(key);
}
