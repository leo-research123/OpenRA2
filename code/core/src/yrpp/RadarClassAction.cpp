#include "debug/debug_state.hpp"
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 radar.cpp RTactical::Action.
// Copyright Electronic Arts Inc. / OpenTS; EA Section 7 terms:
// third_party/opents/LICENSE.md. YR 0x006539D0 full caller and native drag.
#include "yrpp/RadarClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/Unsorted.h"
#include "game_ui_runtime.hpp"
#if !defined(RA2_YRPP_GAME)
bool RadarClass::RTacticalClass::Action(GadgetFlag flags, DWORD* key, KeyModifier modifier) {
    const auto* input = game::game_ui_input();
    if (!input) return false;
    auto& radar = RadarClass::Instance;
    const unsigned bits = static_cast<unsigned>(flags);
    // Original active-mode gate. The host event has already captured MouseQ
    // coordinates for presses/releases, or the live device point for hovering.
    if (radar.unknown_14B0 != 1 || radar.unknown_14AC != 1) {
        // Original input never captures the radar gadget. Our retained drag
        // extension must retire its capture when the map stops being usable.
        if (StuckOn == this) StuckOn = nullptr;
        return false;
    }
    const auto panel = radar.GetPanelBounds();
    const Point2D point{input->point.X - panel.X + int(radar.unknown_11F0),
                        input->point.Y - panel.Y + int(radar.unknown_11F4)};
    // Retained extension: only a navigation press captures the gadget. Held
    // events and the terminating release never turn a drag into a command.
    if (this == StuckOn) {
        if (bits & 0x44) {
            StickyProcess(flags);
            return true;
        }
        if (bits & 0x22) {
            radar.Navigate(point);
            return true;
        }
    }
    if (bits & 0x22) return false;
    const auto& rect = radar.unknown_rect_149C;
    if (point.X < rect.X || point.Y < rect.Y || point.X >= rect.X + rect.Width || point.Y >= rect.Y + rect.Height) {
        MouseClass::Instance.MouseClass::UpdateCursor(MouseCursorType::Default, false);
        return false;
    }
    CellStruct cell{0, 0};
    TechnoClass* target = nullptr;
    radar.RadarToCell(point, cell, target);
    if (cell != CellStruct{-1, -1}) {
        CoordStruct world{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0};
        world.Z = radar.GetCellFloorHeight(world);
        const bool shadow = game::debug_player_location_shrouded(world) && input->has_window;
        bool navigate = true;
        // Right press/release bypass action and planning checks altogether.
        if (!(bits & 0x50)) {
            ::Action action = ::Action::None;
            bool evaluate = true;
            switch (radar.CurrentSWTypeIndex) {
                case 0:
                    action = ::Action::Nuke;
                    break;
                case 1:
                    action = ::Action::IronCurtain;
                    break;
                case 2:
                    action = ::Action::LightningStorm;
                    break;
                case 5:
                    action = ::Action::ParaDrop;
                    break;
                case 6:
                    action = ::Action::AmerParaDrop;
                    break;
                case 7:
                    action = ::Action::PsychicDominator;
                    break;
                case 8:
                    action = ::Action::SpyPlane;
                    break;
                case 9:
                    action = ::Action::GeneticConverter;
                    break;
                case 11:
                    action = ::Action::PsychicReveal;
                    break;
                default:
                    if (!ObjectClass::CurrentObjects.Count)
                        evaluate = false;
                    else if (target)
                        action = Unsorted::BestSelectedObject(nullptr, target)->MouseOverObject(target, false);
                    else
                        action = Unsorted::BestSelectedObject(&cell, nullptr)->MouseOverCell(&cell, false, false);
                    break;
            }
            if (evaluate) {
                // YR's exact whitelist. In particular SW types 1 and 9 are
                // mapped above but then rejected; preserve that distinction.
                switch (action) {
                    case ::Action::Move:
                    case ::Action::NoMove:
                    case ::Action::Enter:
                    case ::Action::Attack:
                    case ::Action::Harvest:
                    case ::Action::Capture:
                    case ::Action::Sabotage:
                    case ::Action::Nuke:
                    case ::Action::GuardArea:
                    case ::Action::LightningStorm:
                    case ::Action::ParaDrop:
                    case ::Action::AmerParaDrop:
                    case ::Action::PsychicDominator:
                    case ::Action::SpyPlane:
                    case ::Action::PsychicReveal:
                        break;
                    default:
                        action = ::Action::None;
                        target = nullptr;
                        break;
                }
                // Original evaluates the planner gate even for Action::None.
                const bool supported =
                    !PlanningNodeClass::PlanningModeActive || Game::PlanningManager_UnsupportedType() == -1;
                if (supported && action != ::Action::None) {
                    if (bits & 0x08) radar.DisplayClass::ConvertAction(cell, shadow, target, action, true);
                    if ((bits & 0x04) && !Unsorted::DragSelectAborted) {
                        const CoordStruct release{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0};
                        radar.DisplayClass::LeftMouseButtonUp(release, cell, target, action, 1);
                    }
                    navigate = false;
                }
            }
            if (navigate) MouseClass::Instance.MouseClass::SetCursor(MouseCursorType::Default, true);
        }
        if (navigate && (bits & 0x11) && radar.NavigateCell(cell)) StickyProcess(flags);
    }
    // Original passes zero flags/modifier and preserves the caller's key.
    GadgetClass::Action(static_cast<GadgetFlag>(0), key, KeyModifier::None);
    return true;
}
#endif
