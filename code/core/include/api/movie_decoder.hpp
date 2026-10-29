#pragma once
#include <cstdint>
namespace game {
struct MovieSession;
struct MovieStatistics {
    std::uint64_t created = 0, destroyed = 0, video_frames = 0, audio_frames = 0, mixed_frames = 0;
    std::uint32_t failures = 0;
    bool active = false;
};
// Optional core decoder; no libav, STL or host types cross this ABI.
// One session owns the installed backend. Detach the audio consumer before
// destruction; all movie handles are retired before the session is freed.
// output_rate=0 selects video-only playback when there is no audio device.
bool create_movie_session(unsigned output_rate, MovieSession*& output) noexcept;
void destroy_movie_session(MovieSession*& session) noexcept;
void mix_movie_session(MovieSession&, float* stereo, unsigned frames) noexcept;
bool get_movie_statistics(const MovieSession&, MovieStatistics&) noexcept;
const char* movie_session_error(const MovieSession&) noexcept;
void set_movie_volume(MovieSession&, float volume) noexcept;
// Explicit media pause, independently of Scenario logic paused by a movie.
// Map hosts use set_game_view_activity to synchronize activity; focus alone
// never pauses media. This low-level entry remains for standalone decoders.
void set_movie_activity(MovieSession&, bool playing) noexcept;
}
