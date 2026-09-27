#pragma once
#include "yrpp/GeneralDefinitions.h"
class CCINIClass;
namespace game {
// Native bootstrap and device adapter for the existing CommandClass registry.
// Exceptions are contained here; no command/registry is duplicated in Godot.
bool initialize_player_commands() noexcept;
bool load_player_hotkeys(CCINIClass&) noexcept;
bool dispatch_player_hotkey(WWKey) noexcept;
void deploy_selected_objects() noexcept;
void stop_selected_objects() noexcept;
}
