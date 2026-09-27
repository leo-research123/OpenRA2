#pragma once

namespace game {

// Host execution of the existing Game::PlayMovie operation. The backend owns
// decoding, presentation, audio and event pumping while the movie is active.
class VideoBackend {
public:
    virtual ~VideoBackend() = default;

    // Filename is borrowed until return. Return only after playback finishes,
    // is cancelled, or cannot start; all playback resources must be detached.
    // Preserve original values, including -1 defaults: the four char options
    // have distinct zero/nonzero and ==1 checks, and are not bools.
    // clear_after_playback == 1 clears/presents black after playback;
    // clear_before_playback != 0 does so before playback. Both use the same
    // original Hidden surface; hosts implement the corresponding screen transition.
    // queue_theme is passed through. Game::PlayMovie has no success result;
    // backend availability can be queried before calling.
    virtual void play_movie(const char* movie_name, int queue_theme,
        char clear_after_playback, char stretch_movie,
        char clear_before_playback, char set_state_1) noexcept = 0;
};

// Borrow the backend; core never copies or deletes it. Register/reset on the
// game thread with no movie call in flight, then destroy the previous backend.
void set_video_backend(VideoBackend& backend) noexcept;
// Restore the link-selected default: unavailable in standalone core, YR in compat.
void reset_video_backend() noexcept;
// On a miss return false and preserve output.
bool get_video_backend(VideoBackend*& output) noexcept;

}
