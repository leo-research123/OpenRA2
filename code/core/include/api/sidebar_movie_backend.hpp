#pragma once
#include <cstdint>
namespace game {
struct SidebarMovieHandle;
enum class SidebarMovieStatus : std::uint32_t { waiting, frame, finished, failed };
struct SidebarMovieFrame {
    const std::uint16_t* pixels = nullptr;
    std::uint32_t pixel_count = 0;
    int width = 0, height = 0, pitch = 0;  // pitch is in RGB565 pixels, not bytes.
    // Original subtitle clock: integer 60-unit frame period * Bink FrameNum.
    std::uint32_t subtitle_time = 0, frame_period_ms = 0;
    bool finished = false;
};
// Device state and borrowed RGB565 storage remain backend-owned. The clock
// starts on the first update, never while the handle waits in the radar queue.
// update returns frame whenever pixel contents change, including after seeking;
// waiting preserves the previously published contents.
// All callbacks contain exceptions; creation failure acquires no game pause.
struct SidebarMovieBackend {
    std::uint32_t struct_size = sizeof(SidebarMovieBackend), version = 2;
    void* context = nullptr;
    SidebarMovieHandle* (*create)(void*, const char*) noexcept = nullptr;
    bool (*describe)(void*, SidebarMovieHandle*, SidebarMovieFrame&) noexcept = nullptr;
    SidebarMovieStatus (*update)(void*, SidebarMovieHandle*, SidebarMovieFrame&) noexcept = nullptr;
    void (*pause)(void*, SidebarMovieHandle*, bool) noexcept = nullptr;
    bool (*seek)(void*, SidebarMovieHandle*, int) noexcept = nullptr;
    void (*destroy)(void*, SidebarMovieHandle*) noexcept = nullptr;
};
bool set_sidebar_movie_backend(const SidebarMovieBackend&) noexcept;
void reset_sidebar_movie_backend() noexcept;
}
