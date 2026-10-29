// Complete original 0x006578F0 / 0x006579E0, using the original handle queue.
#include "yrpp/RadarClass.h"
#include "yrpp/OwnerDraw.h"
#include "yrpp/VoxClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/FPSCounter.h"
#include "yrpp/ScenarioClass.h"
#include "movie_player.hpp"

#if !defined(RA2_YRPP_GAME)
void RadarClass::PrepareMovie() noexcept {
    enum MoviePreludeState : int { Announce = 0, WaitAnnouncement = 1, WaitStreams = 2 };
    auto& queue = WWMovieHandle::Array;
    switch (unknown_14B8) {
        case MoviePreludeState::Announce:
            VoxClass::ClearQueue();
            VoxClass::PlayIndex(unknown_int_14D4, 0, 3);
            VoxClass::Suppress();
            unknown_14B8 = MoviePreludeState::WaitAnnouncement;
            break;
        case MoviePreludeState::WaitAnnouncement:
            if (VoxClass::IsFinished(unknown_int_14D4)) {
                if (queue.Count) Audio::FadeSounds();
                unknown_14B8 = MoviePreludeState::WaitStreams;
            }
            break;
        case MoviePreludeState::WaitStreams:
            if (queue.Count) {
                if (!Audio::SoundsFaded()) return;
                if (queue.Count) {
                    VoxClass::Unsuppress();
                    game::enter_movie_audio();
                }
            }
            SetRadarMode(3, false);
            RadarAudio.EndLooping();
            if (auto* rules = RulesClass::Instance) {
                VocClass::PlayGlobal(rules->MovieOn, 0x2000, 1.0f, &RadarAudio);
                Detail::SetMinFrameRate(unsigned(rules->DetailMinFrameRateMovie));
            }
            break;
        default:
            break;
    }
}
void RadarClass::PlayMovie() noexcept {
    auto& queue = WWMovieHandle::Array;
    bool remains = queue.Count > 0, complete = false, pause = false;
    if (!unknown_bool_14BD && remains) (void)VoxClass::IsSpeaking();
    const auto invalidate = [&] { unknown_rect_120C = {int(padding_11E4), int(unknown_11EC), 168, 110}; };
    if (unknown_bool_14DA && !unknown_bool_14BD) invalidate();
    if (remains) {
        auto* handle = queue.Count > 0 ? queue[0] : nullptr;
        if (handle) {
            if (handle->VTable->AdvanceFrame(handle)) invalidate();
            complete = handle->VTable->Finished(handle);
            pause = handle->State;
        } else
            complete = true;
        if (complete) {
            if (handle) {
                queue.RemoveItem(queue.FindItemIndex(handle));
                handle->VTable->Destructor(handle, 1);
            }
            remains = queue.Count != 0;
            if (remains) {
                RadarAudio.EndLooping();
                if (RulesClass::Instance)
                    VocClass::PlayGlobal(RulesClass::Instance->MovieOn, 0x2000, 1.0f, &RadarAudio);
                QueueNextMovie();
            }
        }
    }
    unknown_bool_14D9 = false;
    if (!remains) {
        unknown_14AC = 5;
        SetRadarMode(int(unknown_14B4), false);
        game::leave_movie_audio();
        pause = false;
        RadarAudio.EndLooping();
        if (auto* rules = RulesClass::Instance) {
            VocClass::PlayGlobal(rules->MovieOff, 0x2000, 1.0f, &RadarAudio);
            Detail::SetMinFrameRate(unsigned(rules->DetailMinFrameRateNormal));
        }
        unknown_bool_14D9 = true;
    }
    unknown_bool_14BD = !complete;
    if (unknown_bool_14BC != pause) {
        if (pause)
            ScenarioClass::PauseLogic(0);
        else
            ScenarioClass::ResumeLogic();
        unknown_bool_14BC = pause;
    }
}

#endif
