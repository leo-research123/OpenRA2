#include "yrpp/Unsorted.h"
#include "api/video_backend.hpp"

void YRPP_FASTCALL Game::PlayMovie(const char* movie_name, int queue_theme, char clear_after_playback,
                                   char stretch_movie, char clear_before_playback, char set_state_1) {
    game::VideoBackend* backend = nullptr;
    if (game::get_video_backend(backend)) {
        backend->play_movie(movie_name, queue_theme, clear_after_playback, stretch_movie, clear_before_playback,
                            set_state_1);
    }
}

void YRPP_FASTCALL Game::PlayMovie(int index, int, char clearAfter, char stretch, char clearBefore) {
    if (index >= 0 && index < MovieInfo::Array.Count) {
        // 0x005BF260 suppresses tactical presentation only during indexed playback.
        const bool active = Unsorted::TacticalActive;
        Unsorted::TacticalActive = false;
        PlayMovie(MovieInfo::Array[index], -1, clearAfter, stretch, clearBefore, 0);
        Unsorted::TacticalActive = active;
    }
}
