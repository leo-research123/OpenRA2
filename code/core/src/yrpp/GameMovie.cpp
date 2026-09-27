#include "yrpp/Unsorted.h"
#include "api/video_backend.hpp"

void YRPP_FASTCALL Game::PlayMovie(const char* movie_name, int queue_theme,
    char clear_after_playback, char stretch_movie,
    char clear_before_playback, char set_state_1) {
    game::VideoBackend* backend = nullptr;
    if (game::get_video_backend(backend)) {
        backend->play_movie(movie_name, queue_theme, clear_after_playback,
            stretch_movie, clear_before_playback, set_state_1);
    }
}
