// Command phases of Game::KeyboardProcess, 0x55DEF4..0x55E025.
// Device buffering, focus and physical-key ownership stay with the caller.
#include "yrpp/CommandClass.h"

bool CommandClass::FindHotkey(WWKey input, CommandClass*& command) noexcept {
    command = nullptr;
    try {
        const int key = int(input) & ~int(WWKey::Release);
        // 0x55DF03 also strips the virtual-key marker (0x1000) from the plain lookup.
        const int plain = key & ~int(WWKey::Shift | WWKey::Ctrl | WWKey::Alt | WWKey::VirtualKey);
        Hotkeys.TryGet(plain, command);
        if (!command || !command->PreventCombinationOverride(WWKey(key))) {
            command = nullptr;
            Hotkeys.TryGet(key, command);
        }
        return command != nullptr;
    } catch (...) {
        command = nullptr;
        return false;
    }
}

bool CommandClass::ProcessInput(WWKey input, bool& drain) const noexcept {
    drain = false;
    try {
        if (ExtraTriggerCondition(input)) Execute(input);
        drain = CheckLoop55E020(input);
        return true;
    } catch (...) {
        return false;
    }
}
