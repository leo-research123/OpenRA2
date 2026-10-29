#pragma once
namespace game {
struct SidebarMoviePicture;
void visit_sidebar_movie_state(void*, void (*visitor)(void*, const void*, unsigned) noexcept) noexcept;
void capture_sidebar_movie(SidebarMoviePicture&) noexcept;
void draw_sidebar_movie(const SidebarMoviePicture&) noexcept;
void queue_sidebar_movie(int index, bool pause) noexcept;
void clear_sidebar_movies() noexcept;
// The map activity entry owns host pause policy; no decoder is also valid.
void set_native_movie_activity(bool playing) noexcept;
}
