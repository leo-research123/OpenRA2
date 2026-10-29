// Original sidebar queue 0x005BF390 and original movie-handle device adapter.
#include "sidebar_movie.hpp"
#include "ui_render_state.hpp"
#include "movie_player.hpp"
#include "movie_subtitles.hpp"
#include "api/sidebar_movie_backend.hpp"
#include "game_ui_runtime.hpp"
#include "images/drawing_resources.hpp"
#include "yrpp/SessionClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/Surface.h"
#include <cstdio>
#include <cstring>
#include <memory>

#if !defined(RA2_YRPP_GAME)
namespace {
DynamicVectorClass<WWMovieHandle*> movies;
}
DynamicVectorClass<WWMovieHandle*>& WWMovieHandle::Array = movies;
#endif
namespace game {
namespace {
SidebarMovieBackend backend;
SidebarMoviePicture* capture_picture = nullptr;
struct MoviePlayer {
    const std::uint64_t content_identity = allocate_raster_content_identity();
    std::uint64_t content_version = 1;
    SidebarMovieBackend device;
    SidebarMovieHandle* handle = nullptr;
    SidebarMovieFrame frame;
    SidebarMovieStatus status = SidebarMovieStatus::waiting;
    MovieSubtitles subtitles;
    MovieSubtitleCallback callback = nullptr;
    Point2D position{};
    ~MoviePlayer() {
        if (handle) device.destroy(device.context, handle);
    }
};
MoviePlayer& player(WWMovieHandle* handle) { return *static_cast<MoviePlayer*>(handle->Player); }
void YRPP_THISCALL destroy(WWMovieHandle* handle, int deleting) {
    delete static_cast<MoviePlayer*>(handle->Player);
    handle->Player = nullptr;
    if (deleting) delete handle;
}
bool YRPP_THISCALL advance(WWMovieHandle* handle) {
    auto& p = player(handle);
    p.status = p.device.update(p.device.context, p.handle, p.frame);
    if (p.status == SidebarMovieStatus::frame) {
        ++p.content_version;
        p.subtitles.advance(p.frame.subtitle_time, p.callback);
    }
    // AdvanceFrame also blits in the original Bink handle.
    draw_movie_handle(handle);
    return p.status == SidebarMovieStatus::frame;
}
bool YRPP_THISCALL waiting(WWMovieHandle* handle) { return player(handle).status == SidebarMovieStatus::waiting; }
void YRPP_THISCALL pause(WWMovieHandle* handle, int value) {
    auto& p = player(handle);
    p.device.pause(p.device.context, p.handle, value != 0);
}
void YRPP_THISCALL stop(WWMovieHandle* handle) {
    auto& p = player(handle);
    p.status = SidebarMovieStatus::finished;
    p.device.pause(p.device.context, p.handle, true);
}
bool YRPP_THISCALL finished(WWMovieHandle* handle) {
    const auto s = player(handle).status;
    return s == SidebarMovieStatus::finished || s == SidebarMovieStatus::failed || player(handle).frame.finished;
}
void YRPP_THISCALL position(WWMovieHandle* handle, int x, int y) { player(handle).position = {x, y}; }
void YRPP_THISCALL seek(WWMovieHandle* handle, int frame) {
    auto& p = player(handle);
    if (p.device.seek(p.device.context, p.handle, frame)) {
        p.subtitles.rewind();
        p.status = SidebarMovieStatus::waiting;
    }
}
void YRPP_THISCALL subtitles(WWMovieHandle* handle, MovieSubtitleCallback callback) {
    player(handle).callback = callback;
}
int YRPP_THISCALL timing(WWMovieHandle* handle) { return int(player(handle).frame.frame_period_ms); }
void YRPP_THISCALL blit(WWMovieHandle* handle) { draw_movie_handle(handle); }
WWMovieHandleVTable table{destroy, advance, waiting, pause, stop, finished, position, seek, subtitles, timing, blit};
bool resolve(const char* name, char* output, unsigned size) {
    if (!name || std::strlen(name) > 199) return false;
    char stem[200];
    std::snprintf(stem, sizeof(stem), "%s", name);
    if (auto* dot = std::strchr(stem, '.')) *dot = 0;
    for (const char* suffix : {"BIK", "VQA"}) {
        std::snprintf(output, size, "%s.%s", stem, suffix);
        CCFileClass file(output);
        if (file.Exists()) return true;
    }
    return false;
}
}
bool set_sidebar_movie_backend(const SidebarMovieBackend& value) noexcept {
    if (value.version != 2 || value.struct_size != sizeof(value) || !value.create || !value.describe || !value.update ||
        !value.pause || !value.seek || !value.destroy)
        return false;
    clear_sidebar_movies();
    backend = value;
    return true;
}
void reset_sidebar_movie_backend() noexcept {
    clear_sidebar_movies();
    backend = {};
}
WWMovieHandle* create_movie_handle(const char* name, DSurface* surface, const RectangleStruct*) noexcept {
    if (!surface) return nullptr;
    return create_sidebar_movie_handle(name);
}
WWMovieHandle* create_sidebar_movie_handle(const char* name) noexcept {
    if (!backend.create) return nullptr;
    try {
        char filename[208];
        if (!resolve(name, filename, sizeof(filename))) return nullptr;
        auto p = std::make_unique<MoviePlayer>();
        p->device = backend;
        p->handle = backend.create(backend.context, filename);
        if (!p->handle) return nullptr;
        if (!backend.describe(backend.context, p->handle, p->frame) || p->frame.width <= 0 || p->frame.height <= 0)
            return nullptr;
        p->subtitles.load(filename);
        auto result = std::make_unique<WWMovieHandle>();
        result->VTable = &table;
        result->State = false;
        result->Width = p->frame.width;
        result->Height = p->frame.height;
        result->Player = p.release();
        return result.release();
    } catch (...) {
        return nullptr;
    }
}
void queue_sidebar_movie(int index, bool pause_logic) noexcept {
    if (index < 0 || index >= MovieInfo::Array.Count || SessionClass::Instance.GameMode != GameMode::Campaign) return;
#if defined(RA2_YRPP_GAME)
    auto* handle = OwnerDraw::InitMovieHandle(MovieInfo::Array[index], DSurface::Sidebar, nullptr);
#else
    auto* handle = OwnerDraw::InitSidebarMovieHandle(MovieInfo::Array[index]);
#endif
    if (!handle) return;
    auto& radar = RadarClass::Instance;
    handle->VTable->SetPosition(handle, int(radar.unknown_11F0) - 3, int(radar.unknown_11F4) - 1);
    handle->VTable->InitSubtitles(handle, sidebar_subtitle_message);
    handle->State = pause_logic;
    // Native allocation failure must not cross the host boundary. The original
    // leaves the unqueued handle allocated; the native device releases it.
    bool queued = false;
    try {
        queued = WWMovieHandle::Array.AddItem(handle);
    } catch (...) {
    }
    if (!queued) handle->VTable->Destructor(handle, 1);
}
void draw_movie_handle(WWMovieHandle* handle) noexcept {
    if (!handle || !handle->Player) return;
    const auto& p = player(handle);
    const auto& current = p.frame;
    if (!current.pixels) return;
    if (capture_picture) {
        try {
            auto& picture = *capture_picture;
            picture.pixels.assign(current.pixels, current.pixels + current.pixel_count);
            picture.position = p.position;
            picture.width = current.width;
            picture.height = current.height;
            picture.pitch = current.pitch;
            picture.identity = p.content_identity;
            picture.version = p.content_version;
        } catch (...) { capture_picture->pixels.clear(); }
        return;
    }
    auto* ui = game_ui_frame();
    if (!ui) return;
    const auto& sidebar = DSurface::SidebarBounds;
    RasterDrawingRequest r;
    r.target = ui->drawing.types.target;
    r.position = {sidebar.X + p.position.X, p.position.Y};
    r.clip = {sidebar.X, 0, sidebar.Width, DSurface::WindowBounds.Height};
    r.width = current.width;
    r.height = current.height;
    r.pitch = current.pitch;
    r.pixels = current.pixels;
    r.pixel_count = current.pixel_count;
    r.content_identity = p.content_identity;
    r.content_version = p.content_version;
    record_ui_drawing(submit_type_raster(ui->drawing.types, r));
}
void visit_sidebar_movie_state(void* context, void (*visitor)(void*, const void*, unsigned) noexcept) noexcept {
    if (!visitor) return;
    for (int i = 0; i < WWMovieHandle::Array.Count; ++i) {
        auto* handle = WWMovieHandle::Array[i];
        if (handle && handle->VTable == &table && handle->Player) visitor(context, handle->Player, sizeof(MoviePlayer));
    }
}
void capture_sidebar_movie(SidebarMoviePicture& picture) noexcept {
    picture.pixels.clear();
    auto* previous = capture_picture;
    capture_picture = &picture;
    RadarClass::Instance.PlayMovie();
    capture_picture = previous;
}
void draw_sidebar_movie(const SidebarMoviePicture& picture) noexcept {
    auto* ui = game_ui_frame();
    if (!ui || picture.pixels.empty()) return;
    const auto& sidebar = DSurface::SidebarBounds;
    RasterDrawingRequest r;
    r.target = ui->drawing.types.target;
    r.position = {sidebar.X + picture.position.X, picture.position.Y};
    r.clip = {sidebar.X, 0, sidebar.Width, DSurface::WindowBounds.Height};
    r.width = picture.width;
    r.height = picture.height;
    r.pitch = picture.pitch;
    r.pixels = picture.pixels.data();
    r.pixel_count = unsigned(picture.pixels.size());
    r.content_identity = picture.identity;
    r.content_version = picture.version;
    record_ui_drawing(submit_type_raster(ui->drawing.types, r));
}
void clear_sidebar_movies() noexcept {
    auto& queue = WWMovieHandle::Array;
    for (int i = queue.Count - 1; i >= 0; --i)
        if (auto* handle = queue[i]) handle->VTable->Destructor(handle, 1);
    queue.Clear();
    auto& radar = RadarClass::Instance;
    if (radar.unknown_bool_14BC && ScenarioClass::Instance) ScenarioClass::ResumeLogic();
    radar.unknown_bool_14BC = radar.unknown_bool_14BD = false;
    leave_movie_audio();
}
}
