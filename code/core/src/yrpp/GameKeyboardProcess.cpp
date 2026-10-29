// YR 0x0055DEE0 Game::KeyboardProcess and its message-line step 0x0055E420.
// The campaign/skirmish path: chat, beacon and page commands exist only in the
// network session modes and are not part of this host.
#include "yrpp/Unsorted.h"
#include "yrpp/CommandClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/SessionClass.h"
#include "platform_input_codes.hpp"
#include <cwctype>

namespace {
// "Debug" at 0x008278B4: while input is locked only this command category stays usable.
constexpr wchar_t debug_category[] = L"Debug";

bool equals_ignore_case(const wchar_t* a, const wchar_t* b) noexcept {
    if (!a || !b) return a == b;
    for (; *a && *b; ++a, ++b)
        if (std::towlower(*a) != std::towlower(*b)) return false;
    return *a == *b;
}
}  // namespace

#if !defined(RA2_YRPP_GAME)
void YRPP_FASTCALL Game::ProcessMessageInput(DWORD& input) noexcept {
    // 0x55E43F flushes IME characters (word 0xB730EC): a Windows text-service
    // buffer that has no counterpart here, so it is always empty.
    // 0x55E48D: the Enter / backslash / Backspace chat openers and the whole
    // network-mode block run only for session modes other than Campaign and
    // Skirmish. 0x55E61F then asks the message list to consume the key; with no
    // edit line open (the only state reachable without chat) that returns 0 and
    // leaves the key unchanged (0x5D4510 returns at 0x5D4990). A 0 result skips
    // the chat-line updates of 0x55E640..0x55F1C2, so nothing remains to run.
    (void)input;
}

void YRPP_FASTCALL Game::KeyboardProcess(DWORD& input) noexcept {
    ProcessMessageInput(input);
    if (!input) return;
    const WWKey pressed = WWKey(int(input) & ~int(WWKey::Release));
    // 0x55DF05 clears the modifier and virtual-key bits for the default switch.
    const int plain = int(pressed) & ~int(WWKey::Shift | WWKey::Ctrl | WWKey::Alt | WWKey::VirtualKey);
    CommandClass* command = nullptr;
    CommandClass::FindHotkey(WWKey(input), command);
    // 0x55DFE0: a locked-input session runs only commands of the Debug category.
    const bool blocked =
        command && Unsorted::UserInputLocked && !equals_ignore_case(command->GetUICategory(), debug_category);
    if (command && !blocked) {
        bool drain = false;
        if (!command->ProcessInput(WWKey(input), drain) || !drain) return;
        // 0x55E031: the command asked for duplicate buffered keys to be dropped.
        auto* keyboard = InputManagerClass::Instance;
        if (!keyboard) return;
        int removed = 0;
        WORD next = keyboard->Check();
        if (next == input) {
            do {
                keyboard->Get();
                next = keyboard->Check();
                ++removed;
            } while (removed < 10 && next == input);
        }
        return;
    }
    // No command (or a blocked one): the fixed keys.
    const bool release = (input & unsigned(WWKey::Release)) != 0;
    auto set_arrow = [&](DWORD bit) {
        Game::KeyboardScrollFlags = release ? (Game::KeyboardScrollFlags & ~bit) : (Game::KeyboardScrollFlags | bit);
    };
    switch (plain) {
        case game::Win32KeyEscape:
            Game::QueueOptions();
            break;
        case game::Win32KeyTab:  // 0x6ABC40 opens the session dialog outside Campaign.
            if (SessionClass::Instance.GameMode != GameMode::Campaign) Game::SpecialDialog = 8;
            break;
        case game::Win32KeyLeft:
            set_arrow(0x100);
            break;
        case game::Win32KeyRight:
            set_arrow(0x1000);
            break;
        case game::Win32KeyUp:
            set_arrow(0x1);
            break;
        case game::Win32KeyDown:
            set_arrow(0x10);
            break;
        default:
            break;
    }
}
#endif
