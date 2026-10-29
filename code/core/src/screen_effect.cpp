#include "screen_effect.hpp"
#include "presentation/recorder.hpp"
#include "game_ui_runtime.hpp"
#include "yrpp/Surface.h"
#include "yrpp/VoxClass.h"
#include "audio_session.hpp"

#include <algorithm>
namespace game {
namespace {
ScreenEffectDrawingRequest frame;
bool capture_pending = false;
DrawingStatus submit(ScreenEffectDrawingRequest request) noexcept {
    auto* ui = game_ui_frame();
    if (!ui) return DrawingStatus::unavailable;
    request.target = ui->drawing.types.target;
    // The Composite is the Tactical view plus the command bar below it; the
    // sidebar is a separate surface the transition never writes.
    const auto& view = DSurface::ViewBounds;
    request.width = view.X + view.Width;
    request.height = DSurface::WindowBounds.Height - 32;
    if (supports_drawing(ui->drawing.types, DrawingKind::screen_effect)) return submit_screen_effect(ui->drawing.types, request);
    return request.stage == ScreenEffectStage::inactive && request.operation == ScreenEffectOperation::apply
               ? DrawingStatus::skipped
               : DrawingStatus::unavailable;
}
}
void chrono_device(ChronoDeviceOperation operation, int step, int duration) noexcept {
    switch (operation) {
        case ChronoDeviceOperation::capture_old:
            ++frame.capture_id;
            if (!frame.capture_id) ++frame.capture_id;
            frame.stage = ScreenEffectStage::old_world;
            capture_pending = false;
            frame.warp_step = 0;
            frame.initial_duration = 1;
            break;
        case ChronoDeviceOperation::warp_old:
        case ChronoDeviceOperation::warp_new:
            frame.stage = operation == ChronoDeviceOperation::warp_old ? ScreenEffectStage::old_world
                                                                       : ScreenEffectStage::new_world;
            frame.warp_step = step;
            frame.initial_duration = duration;
            frame.brightness = int(255ll * step / duration);
            break;
        case ChronoDeviceOperation::white:
            frame.stage = ScreenEffectStage::white;
            break;
        case ChronoDeviceOperation::capture_new:
            capture_pending = true;
            break;
        case ChronoDeviceOperation::restore:
            frame.stage = ScreenEffectStage::restore;
            break;
        case ChronoDeviceOperation::discard:
            frame.stage = ScreenEffectStage::inactive;
            capture_pending = false;
            break;
    }
}
void chrono_audio_begin() noexcept {
    stop_native_sound_effects();
    VoxClass::Stop(true);
}
void chrono_audio_reserve(bool reserve) noexcept {
    if (reserve)
        VoxClass::Suppress();
    else
        VoxClass::Unsuppress();
}
void draw_ion_blast(Point2D center, int lifetime, int depthBase) noexcept {
    ScreenEffectDrawingRequest request;
    request.operation = ScreenEffectOperation::ion_blast;
    request.center_x = center.X;
    request.center_y = center.Y;
    request.depth_base = depthBase;
    request.warp_step = lifetime;
    request.initial_duration = 79;
    record_ui_drawing(submit(request));
}
void draw_chrono_screen() noexcept {
    auto* ui = game_ui_frame();
    if (!ui) return;
    // Pixel-exact 0x53BBA0 / 0x53C0A0 warp sampling is intentionally skipped.
    // The device uses an approximate swirl; gameplay timing remains original.
    record_ui_drawing(submit(frame));
    if (ui->advance_presentation && frame.stage == ScreenEffectStage::restore)
        frame.stage = ScreenEffectStage::inactive;
}
bool chrono_capture_pending() noexcept { return capture_pending; }
void submit_chrono_capture(bool begin) noexcept {
    auto request = frame;
    request.operation = begin ? ScreenEffectOperation::capture_begin : ScreenEffectOperation::capture_end;
    record_ui_drawing(submit(request));
    if (!begin) capture_pending = false;
}
}
