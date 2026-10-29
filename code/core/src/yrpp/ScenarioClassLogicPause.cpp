// Original logic-pause nesting and input ownership, 0x684060..0x6842E0.
#include "yrpp/ScenarioClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/WWMouseClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/Audio.h"
#include "yrpp/Unsorted.h"
#include "yrpp/SessionClass.h"
#include <bit>
#include <cstdlib>

namespace {
bool previous_input_lock{};
int input_unlock_frame{};
}
bool& ScenarioClass::LogicPauseWasInputLocked = previous_input_lock;
int& ScenarioClass::InputUnlockFrame = input_unlock_frame;

void YRPP_FASTCALL ScenarioClass::LockInput() noexcept {
    try {
        MouseClass::Instance.ClearDragBand();
        if (WWMouseClass::Instance && !WWMouseClass::Instance->GetRefCount()) WWMouseClass::Instance->HideCursor();
        Instance->UserInputLocked = true;
        Unsorted::UserInputLocked = true;
        if (InputManagerClass::Instance) InputManagerClass::Instance->Reset();
        InputUnlockFrame = 0;
    } catch (...) {
        std::abort();
    }
}
void YRPP_FASTCALL ScenarioClass::UnlockInput() noexcept {
    if (Instance->UserInputLocked && WWMouseClass::Instance)
        while (WWMouseClass::Instance->GetRefCount())
            WWMouseClass::Instance->ShowCursor();
    Instance->UserInputLocked = false;
    Unsorted::UserInputLocked = false;
    if (InputManagerClass::Instance) InputManagerClass::Instance->Reset();
    InputUnlockFrame = 0;
}
void YRPP_FASTCALL ScenarioClass::PauseLogic(int duration) noexcept {
    try {
        auto& scenario = *Instance;
        if (duration) {
            scenario.PauseTimer.Start(std::bit_cast<int>(unsigned(duration) * 60u));
            if (scenario.IsGamePaused) return;
            scenario.IsGamePaused = true;
        }
        if (!scenario.unknown_62C) {
            Audio::SuspendAll();
            const int mode = static_cast<int>(SessionClass::Instance.GameMode);
            if (mode == 0 || mode == 5) scenario.ElapsedTimer.Pause();
            LogicPauseWasInputLocked = scenario.UserInputLocked;
            if (!LogicPauseWasInputLocked) LockInput();
        }
        scenario.unknown_62C = std::bit_cast<int>(unsigned(scenario.unknown_62C) + 1u);
    } catch (...) {
        std::abort();
    }
}
void YRPP_FASTCALL ScenarioClass::ResumeLogic() noexcept {
    try {
        auto& scenario = *Instance;
        if (!scenario.unknown_62C) return;
        scenario.unknown_62C = std::bit_cast<int>(unsigned(scenario.unknown_62C) - 1u);
        if (scenario.unknown_62C) return;
        if (!LogicPauseWasInputLocked) UnlockInput();
        scenario.ElapsedTimer.Resume();
        Audio::ResumeAll();
    } catch (...) {
        std::abort();
    }
}
