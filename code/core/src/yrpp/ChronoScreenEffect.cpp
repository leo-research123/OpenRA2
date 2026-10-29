// Original screen transition, 0x53B460 / 0x53B560. Device storage is host-owned.
#include "yrpp/SuperClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/MapClass.h"
#include "screen_effect.hpp"
#include <bit>
#include <new>
namespace {
enum ScreenTransitionState : int { Inactive = 0, WarpOld = 1, WhiteHold = 2, WarpNew = 3 };
int initial_duration{};
bool prior_input_lock{}, pause_acquired{}, controller_live{};
alignas(AudioController) unsigned char controller_storage[sizeof(AudioController)];
AudioController& controller() { return *reinterpret_cast<AudioController*>(controller_storage); }
void decrement() { ChronoScreenEffect::Duration = std::bit_cast<int>(unsigned(ChronoScreenEffect::Duration) - 1u); }
}
void YRPP_FASTCALL ChronoScreenEffect::Start(int duration) noexcept {
    if (duration <= 0) return;
    game::chrono_device(game::ChronoDeviceOperation::capture_old);
    Duration = initial_duration = duration;
    Status = ScreenTransitionState::WarpOld;
    game::chrono_audio_begin();
    ::new (controller_storage) AudioController;
    controller_live = true;
    VocClass::PlayGlobal(RulesClass::Instance->LetsDoTheTimeWarpOutAgain, 0x2000, 1.0f, &controller());
    VocClass::VoicesEnabled = false;
    prior_input_lock = ScenarioClass::Instance->UserInputLocked;
    ScenarioClass::LockInput();
    game::chrono_audio_reserve(true);
}
void ChronoScreenEffect::Update() noexcept {
    switch (Status) {
        case ScreenTransitionState::WarpOld:
            game::chrono_device(game::ChronoDeviceOperation::warp_old, initial_duration - Duration, initial_duration);
            decrement();
            if (!Duration) {
                Status = ScreenTransitionState::WhiteHold;
                Duration = 45;
            }
            break;
        case ScreenTransitionState::WhiteHold:
            game::chrono_device(game::ChronoDeviceOperation::white);
            decrement();
            if (Duration == 1) {
                ScenarioClass::PauseLogic(0);
                pause_acquired = true;
            }
            if (!Duration) {
                MapClass::Instance.MarkNeedsRedraw(2);
                // The original temporarily changes Composite to its snapshot and
                // renders the new world with Status=0. The host captures that draw
                // before compositing the still-white display; no recursive GPU draw.
                Status = ScreenTransitionState::Inactive;
                game::chrono_device(game::ChronoDeviceOperation::capture_new);
                Duration = initial_duration;
                Status = ScreenTransitionState::WarpNew;
                VocClass::VoicesEnabled = true;
                controller().EndLooping();
                VocClass::PlayGlobal(RulesClass::Instance->LetsDoTheTimeWarpInAgain, 0x2000, 1.0f, &controller());
                VocClass::VoicesEnabled = false;
            }
            break;
        case ScreenTransitionState::WarpNew:
            game::chrono_device(game::ChronoDeviceOperation::warp_new, Duration, initial_duration);
            decrement();
            if (!Duration) {
                ScenarioClass::ResumeLogic();
                pause_acquired = false;
                controller().EndLooping();
                controller().SetEvent(nullptr, nullptr);
                controller().~AudioController();
                controller_live = false;
                VocClass::VoicesEnabled = true;
                game::chrono_audio_reserve(false);
                if (!prior_input_lock) ScenarioClass::UnlockInput();
                Status = ScreenTransitionState::Inactive;
                game::chrono_device(game::ChronoDeviceOperation::restore);
                MapClass::Instance.MarkNeedsRedraw(2);
            }
            break;
        default:
            break;
    }
}
namespace game {
void reset_chrono_screen_effect() noexcept {
    // A map may close while the original effect owns a nested pause/lock.
    if (pause_acquired && ScenarioClass::Instance) ScenarioClass::ResumeLogic();
    pause_acquired = false;
    if (controller_live) {
        controller().EndLooping();
        controller().SetEvent(nullptr, nullptr);
        controller().~AudioController();
    }
    controller_live = false;
    if (ChronoScreenEffect::Status) {
        VocClass::VoicesEnabled = true;
        chrono_audio_reserve(false);
        if (!prior_input_lock && ScenarioClass::Instance) ScenarioClass::UnlockInput();
    }
    ChronoScreenEffect::Duration = initial_duration = 0;
    ChronoScreenEffect::Status = ScreenTransitionState::Inactive;
    chrono_device(ChronoDeviceOperation::discard);
}
}
